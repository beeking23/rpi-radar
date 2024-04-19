/*
Copyright 2024 Carri King

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include "ppi-datafile.h"
#include "ppi-display.h"
#include "geoutils.h"
#include "dump1090.h"

#include "ws2812-rpi.h"

#include "i2clcd.h"

/// The led ring is one long strip, this hold the indices of the start of each ring.
static std::vector<int> g_LEDRingStartPos;

/// The space in miles between each ring on the map.
static const double LEDRingSpacing = 1.0;

/// The numbers of LEDs in each nested ring.
static const std::vector<int> LEDRingConfig = {
  60,
  48,
  40,
  32,
  24,
  16,
  12,
  8,
  1
};

/// How many Leds there are in total in the strip.
static int g_totalLeds = 0;

/// The map coordinate for each LED in the strip.
static std::vector<std::vector<XY>> g_LEDRingMapCoords;

static constexpr double PI2 = M_PI * 2.0;

/// I use a back buffer scheme so that aircraft positions that are being updated by dump1090s worker
/// thread does not interfere with the worker thread that draws the leds.
static std::vector<Color_t> g_ledBuffer[2];

/// To make the radar animation, we simulate a phospor screen by having each green led have a value
/// and lifespan so that the brightness reduces over time rather than instantly.
static std::vector<float> g_sweepBuffer;

/// holds which of the LED buffers is the front (being drawn to the display) and the back (being drawn by dump1090).
static int g_ledBackbuffer = 0;

/// interface to the LED renderer.
static NeoPixel *g_neoPixel = nullptr;

/// worker thread for drawing the neopixels.
static pthread_t g_renderThread;

/// mutex so that the buffers can't swap during an update.
static pthread_mutex_t g_swapMutex;

/// forward declare of the thread method for the led drawing thread.
static void *RenderThreadEntryPoint(void *);

/// interface to the LCD display which shows the aircraft information.
static LCD *g_lcd = nullptr;
  
const std::vector<int>& GetLEDRingConfig()
{
  return LEDRingConfig;
}

/// Converts a position on the LED to a map position and then a screen position
/// Used when drawing to a monitor and the ring to verify that the ring
/// is showing the map data correctly.
bool LEDRingToScreen(const int ringNum, const int ledID, XY& screenPos)
{
  // pre-calculate all the points.
  if(!g_LEDRingMapCoords.size()) {
    for(int r = 0; r<int(LEDRingConfig.size()); r++) {    
      int count = LEDRingConfig[r];
      std::vector<XY> points;
      const double radius = (LEDRingSpacing * r) / MetersToMiles;
      for(int c = 0; c<count; c++) {
	double angle = (c * PI2) / count;
	double mapX = g_homeEasting + (sin(angle) * radius);
	double mapY = g_homeNorthing + (cos(angle) * radius);
	XY tmp;
	MapToScreen(mapX, mapY, tmp.x, tmp.y);
	points.push_back(tmp);
      }
      g_LEDRingMapCoords.push_back(points);
    }
  }

  if(ringNum >= int(g_LEDRingMapCoords.size()))
    return false;
  if(ledID >= int(g_LEDRingMapCoords[ringNum].size()))
    return false;
  
  screenPos = g_LEDRingMapCoords[ringNum][ledID];
  return true;
}

/// Find the nearest LED to the specified map coordinate.
bool MapCoordToLEDRings(const double range, const double mapX, const double mapY, int& ringNum, int& ledID, double& bearing)
{
  ringNum = 8 - std::min(std::max(0, int(range / LEDRingSpacing) - 1), int(LEDRingConfig.size())-1);
 
  // this gives the angle with respect to the X axis, we want the vertical Y which
  // precedes it by PI/2
  bearing = atan2(mapY - g_homeNorthing, mapX - g_homeEasting);
  bearing = (PI2/4.0) - bearing;  
  if(bearing < 0.0)
    bearing += (PI2);
  
  const int NumLEDs = LEDRingConfig[ringNum];
  const double angle = PI2 / NumLEDs;

  bearing += (angle/2.0);
  if(NumLEDs > 1)  
    ledID = (bearing / angle);
  else
    ledID = 0;

  return true;
}

/// Given a ring number and a led on that ring, return the index of the led on the entire strip.
int GetRingLEDIndex(int ringNum, int ledID)
{  
  if(g_LEDRingStartPos.size() == 0) {
    for(int n = 0; n<int(LEDRingConfig.size()); n++) {
      g_totalLeds += LEDRingConfig[n];
    }

    int startPos = 0;
    for(int n = 0; n<int(LEDRingConfig.size()); n++) {
      g_LEDRingStartPos.push_back(startPos);
      startPos += LEDRingConfig[n];

    }
  }

  int ledIndex = g_LEDRingStartPos[ringNum] + ledID;
  return (ledIndex);
}

/// Sets the color of a specific led on a specific ring.
void SetRingLED(int ringNum, int ledId, Color_t color)
{
  int idx = GetRingLEDIndex(ringNum, ledId);
  g_ledBuffer[g_ledBackbuffer][idx] = color;
}

/// Sets all the leds off.
void ClearRingLED()
{
  static const Color_t blank(0, 0, 0);
  for(auto& i : g_ledBuffer[g_ledBackbuffer])
    i = blank;
}

/// Swaps the LED front and back buffers so that now the backbuffer is the front
// and will actually be drawn on the ring by the rendering thread.
void PresentRingLED()
{
  pthread_mutex_unlock(&g_swapMutex);
  g_ledBackbuffer = !g_ledBackbuffer;
  pthread_mutex_unlock(&g_swapMutex);
}

/// Initialises the displays, starts threads, loads data files, gets everything ready so that the
/// update method from dump1090 can be called.
bool InitDisplayNeoRing()
{
  g_lcd = new LCD(0x27, 0);
  g_lcd->lcd_display_string("ADS-B 1090", 1);
  for(int n = 2; n<=4; n++) {
    g_lcd->lcd_display_string("                ", n);
  }
  
  pthread_mutex_init(&g_swapMutex, NULL);
  
  GetRingLEDIndex(0, 0);
  printf("%i leds in total\n", g_totalLeds);

  g_ledBuffer[0].resize(g_totalLeds);
  g_ledBuffer[1].resize(g_totalLeds);
  g_sweepBuffer.resize(g_totalLeds);
  ClearRingLED();
  
  pthread_create(&g_renderThread, NULL, RenderThreadEntryPoint, NULL);

  
  if(!LoadPPIDataFiles())
    return false;
  return true;
}

/// Returns a random float point between 0 and 'r'
float RandFloat(float r)
{
  return r * float(rand()) / float(RAND_MAX);
}

/// Render the radar like sweep.
void RenderSweep(float angle, Color_t c)
{
  // Fade everything a small amount.
  for(int n = 0; n<g_totalLeds; n++) {
    g_sweepBuffer[n] = pow(g_sweepBuffer[n] * 0.98, 2.0);
    // add a little 'static' to the display to make it seem more analog.
    if(RandFloat(1.0) < (1/100.0)) {
      g_sweepBuffer[n] += RandFloat(0.075f);
      if(g_sweepBuffer[n] > 1.0)
        g_sweepBuffer[n] = 1.0;
    }
  }

  auto& pixels = g_neoPixel->getPixels();

  // if the angle is less than zero then we don't want to draw a line, only
  // update the simulated phosphor.
  if(angle >= 0.0) {
    if(angle > 360.0f)
      angle -= 360.0f;

    // excite the LEDs along the sweep line.
    for(int r = 0; r<int(LEDRingConfig.size()); r++) {
      float seg = 360.0 / LEDRingConfig[r];
      float pos = angle / seg;
      float intPart = floor(pos);
      //float fracPart = pos - intPart;

      int idx = GetRingLEDIndex(r, int(intPart));
      // use a little noise to keep it more analog looking.
      g_sweepBuffer[idx] = 0.9f + RandFloat(0.1f);
    }
  }

  // update the backbuffer with the green.
  // Would be nice to use more dynamic range but it makes the ring too bright!
  // Worse that can cause the hardware to brown out, which usually first shows
  // as errors in the SDR stream.
  for(int n = 0; n<g_totalLeds; n++) {
    if(g_ledBuffer[!g_ledBackbuffer][n] == Color_t(0, 0, 0))
      pixels[n] = Color_t(0, std::max(1, int(24 * g_sweepBuffer[n])), 0);
  }
}

/// The rendering thread for the ring, this runs continually in the background (at about 40Hz) rendering the
/// radar animation and aircraft positions.
void *RenderThreadEntryPoint(void *)
{
  g_neoPixel = new NeoPixel(g_totalLeds);
  g_neoPixel->setBrightness(1.0);
  float angle = 0.0;
  while(1) {
    pthread_mutex_lock(&g_swapMutex);    
    for(int n = 0; n<g_totalLeds; n++)
      g_neoPixel->setPixelColor(n, g_ledBuffer[!g_ledBackbuffer][n]);

    RenderSweep(angle, Color_t(1, 16, 1));    

    pthread_mutex_unlock(&g_swapMutex);
    g_neoPixel->show();
    usleep(25 * 1000);

    angle += 6.0f;
    if(angle > 360.0)
      angle -= (360.0 * 1.2);
  }

  return nullptr;
}

/// Structure for one line of the 4 line LCD that the hardware has.
struct LCDLine {
  double m_range = -1.0;
  const char *m_displayName = nullptr;
  const char *m_airlineName = nullptr;
  struct aircraft *m_aircraft = nullptr;
};

/// Vector lines of information we want to draw
static std::vector<LCDLine> g_lcdLines;
  
/// Entry point called from the regular dump1090 interactive show data method.
void RenderPPIDisplayNeoRing(struct aircraft *a)
{
  static bool init = false;
  
  // is this the first time the method has been called, if so load the data.
  if(!init) {
    init = true;
    if(!InitDisplayNeoRing()) {
      printf("Display open failed\n");
      exit(0);
    }
    printf("Display open OK\n");
  }


  // save the current time so we can give an indication of how old the data is
  // for each aircraft.
  time_t now = time(NULL);

  //printf("\x1b[H\x1b[2J");    /* Clear the screen */

  ClearRingLED();
  g_lcdLines.clear();

  // draw the aircraft.
  int count = 0;
  while(a && count < 30) {
    LCDLine lcdLine;
    int altitude = a->altitude;
    int speed = a->speed;

    // determine the range from the station to the aircraft.
    double range = -1.0;
    double rangeMeters = 0.0;
    double easting, northing, altMerc = 0.0f;    
    if(a->lat != 0.0 || a->lon != 0.0) {
      ConvertCoordinate(a->lat, a->lon, a->altitude / 3.2823, &easting, &northing, &altMerc);
      double deltaY = northing - g_homeNorthing;
      double deltaX = easting - g_homeEasting;
      rangeMeters = sqrt((deltaY * deltaY) + (deltaX * deltaX));
      // convert back to miles.
      range = rangeMeters * MetersToMiles;
      lcdLine.m_range = range;
    }

    // work out which code to show for the aircraft, either the raw hex code from the data packets
    // or preferrables the flight code when that has been recieved.
    // Match the airline code in the database if possible.
    char code[64];
    const char *displayName;
    const char *airlineName = "";
    if(a->flight[0]) {
      displayName = a->flight;
      int n;
      for(n = 0; n<64 && a->flight[n] && !isdigit(a->flight[n]); n++)
	code[n] = a->flight[n];
      code[n] = 0;
      airlineName = GetAirlineFromCode(code);
      lcdLine.m_airlineName = airlineName;
    } else {
      displayName = a->hexaddr;      
    }
    lcdLine.m_displayName = displayName;

    // format the text for the list at the top of the display
    /// try to keep this tidy and nicely lined up.
    int age = (int)(now - a->seen);
    /*
      if(range > 0.0) {
      printf("%-8s %-20.20s %-6d %-6d %5.02f %-4ld %d\n",
      displayName, airlineName, altitude, speed, range, a->messages, age);
      } else {
      printf("%-8s %-20.20s %-6d %-6d    ?? %-4ld %d\n",
      displayName, airlineName, altitude, speed, a->messages, age);
      }
    */
    // if we have a range then we have geographic coords for the aircraft.
    if(range > 0.0) {
      lcdLine.m_aircraft = a;
      g_lcdLines.push_back(lcdLine);
      int ringNum, ledID;
      double bearing;
      if(MapCoordToLEDRings(range, easting, northing, ringNum, ledID, bearing)) {
	XY ringScreenPos;
	if(LEDRingToScreen(ringNum, ledID, ringScreenPos)) {
          Color_t color = (age > 5) ?  Color_t(0, 0, 32) : Color_t(64, 0, 0);
	  SetRingLED(ringNum, ledID, color);
          a->blink++;
	}
      }
    }
    
    a = a->next;
    count++;
  }

  PresentRingLED();

  std::sort(g_lcdLines.begin(), g_lcdLines.end(), [](const auto& lineA, const auto& lineB) -> bool {
    return lineA.m_range < lineB.m_range;
  });

  static int showAirline = 0;
  char lcdbuf[32];
  for(size_t n=0; n<4;n++) {
    if(n < g_lcdLines.size()) {
      const auto& line = g_lcdLines[n];
      int tlen = 0;
      if(showAirline > 2 && line.m_airlineName && line.m_airlineName[0])
	tlen = snprintf(lcdbuf, 21, "%04.01f %s", line.m_range, line.m_airlineName);
      else
	tlen = snprintf(lcdbuf, 21, "%04.01f %-8s %-5d", line.m_range, line.m_displayName, line.m_aircraft->altitude);
      if(tlen < 20) {
	memset(lcdbuf+tlen, ' ', 20 - tlen);
	lcdbuf[20] = 0;
      }
    } else {
      static const char *spin = ".-#";
      if(n != 3)
	snprintf(lcdbuf, 21, "%c                   ", spin[(showAirline) % 3]);
      else
	snprintf(lcdbuf, 21, "%i/%i Unknown           ", count - g_lcdLines.size(), count);
    }
    g_lcd->lcd_display_string(lcdbuf, n+1);
  }

  showAirline++;
  if(showAirline > 4)
    showAirline = 0;
}
