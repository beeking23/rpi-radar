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

#include <algorithm>
#include <cmath>
#include <vector>
#include <string>
#include <memory>

#include "fb-display.h"
#include "ppi-datafile.h"
#include "ppi-display.h"
#include "ppi-neoring.h"
#include "geoutils.h"
#include "dump1090.h"

static FBDisplay g_fbDisplay;

double g_regionMinX = 0.0;
double g_regionMinY = 0.0;
double g_regionWidthY = 0.0;
double g_regionWidthX = 0.0;

// Size of the text to draw in the map
static constexpr float FontSize = 0.08f;

// The distances in miles for each range ring.
//static constexpr  double RangeRings[] = {60, 45, 30, 15, 2};
static constexpr  double RangeRings[] = {5, 10};

// The length as a fraction of the screen of the track indicator line.
static constexpr float TrackLineLen = 0.1f;


//////////////////////////////////

/// Converts a map position to a screen position.
void MapToScreen(double mapX, double mapY, int& x, int& y)
{
  mapX -= g_regionMinX;
  mapX /= g_regionWidthX;
  mapX *= g_fbDisplay.GetScreenWidth();

  mapY -= g_regionMinY;
  mapY /= g_regionWidthY;
  mapY *= g_fbDisplay.GetScreenHeight();

  x = (int)mapX;
  y = g_fbDisplay.GetScreenHeight() - (int)mapY;
}

/// Draws the ring LED locations (see ppi-neoring) on the map display
void DrawLEDRings()
{
  const auto LEDRingConfig = GetLEDRingConfig();
  for(int n = 0; n<int(LEDRingConfig.size()); n++) {
    for(int i = 0; i<LEDRingConfig[n]; i++) {
	XY ringScreenPos;
	if(LEDRingToScreen(n, i, ringScreenPos)) {
	  g_fbDisplay.PutPixel(ringScreenPos.x, ringScreenPos.y, 0x0000ffff); 	  
	}
    }      
  }
}

/// Draws the polyline for the coastline or similar.
void DrawRegionMap()
{
  auto regionMap = GetRegionOutline();
  XY lastXY = regionMap[0];
  for(size_t n = 1; n<regionMap.size(); n++) {
    XY xy = regionMap[n];
    g_fbDisplay.PlotLine(lastXY.x, lastXY.y, xy.x, xy.y, 0xff009900);
    lastXY = xy;
  }
}

/// Draws a point on the map taking the position in screen coordinates.
void DrawMapPoint(const char *title, int color, const XY& screenPos)
{
  g_fbDisplay.DrawCircle(screenPos.x, screenPos.y, 3, color);      
  g_fbDisplay.StrokeString(FBDisplay::Font(), title, screenPos.x, screenPos.y, FontSize, -FontSize);
}

/// Draw a point on the map taking the position in map coordinates.
void DrawMapPoint(const char *title, int color, double easting, double northing)
{
  XY pos;
  MapToScreen(easting, northing, pos.x, pos.y);
  DrawMapPoint(title, color, pos);
}

// ======================================
// ======================================

/// Initialise the display and loads the required data.
bool InitDisplayFB()
{
  if(!g_fbDisplay.Open())
    return false;

  if(!LoadPPIDataFiles())
    return false;

  return true;
}

/// Entry point called from the regular dump1090 interactive show data method.
void RenderPPIDisplayFB(struct aircraft *a)
{
  static bool init = false;
  
  // is this the first time the method has been called, if so load the data.
  if(!init) {
    init = true;
    if(!InitDisplayFB()) {
      printf("Display open failed\n");
      exit(0);
    }
    printf("Display open OK\n");
  }

  // erase the old framebuffer - would be nice to speed this up.
  g_fbDisplay.Clear();
  
  // save the current time so we can give an indication of how old the data is
  // for each aircraft.
  time_t now = time(NULL);

  // draw the coastline or similar.
  DrawRegionMap();

  // draw the ranging rings.
  
  for(auto r : RangeRings) {
    int extentX, extentY;
    double lenMeters = r / MetersToMiles;
    MapToScreen(g_homeEasting + lenMeters, g_homeNorthing + lenMeters, extentX, extentY);
    auto homePOI = GetPOI()[0];
    g_fbDisplay.DrawEllipse(homePOI.m_pos.x, homePOI.m_pos.y, abs(extentX - homePOI.m_pos.x), abs(extentY - homePOI.m_pos.y), 0xff006600);
  }

  g_fbDisplay.SetTextColor(0xfffffff);
  
  // draw the points of interest.
  for(auto& poi : GetPOI())
    DrawMapPoint(poi.m_code.c_str(), 0xffff0000, poi.m_pos);

  // draw the aircraft.
  int count = 0;
  while(a && count < 30) {
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
      // convert back miles.
      range = rangeMeters * MetersToMiles;
    }

    // color the aircraft by range.
    int color = 0;
    if(range < 0.0)
      color = 0xffaaaaaa;
    else if(range < 2.0)
      color = 0xffff3333;
    else if(range < 15.0)
      color = 0xff8888ff;
    else if(range < 30.0)
      color = 0xff33ff33;
    else
      color = 0xffffffff;

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
    } else {
      displayName = a->hexaddr;      
    }

    // format the text for the list at the top of the display
    /// try to keep this tidy and nicely lined up.
    char buf[256];
    if(range > 0.0) {
      sprintf(buf, "%-8s %-20.20s %-6d %-6d %5.02f %-4ld %d ",
	      displayName, airlineName, altitude, speed, range, a->messages, (int)(now - a->seen));
    } else {
      sprintf(buf, "%-8s %-20.20s %-6d %-6d    ?? %-4ld %d ",
	      displayName, airlineName, altitude, speed, a->messages, (int)(now - a->seen));
    }
    g_fbDisplay.SetTextColor(color);
    g_fbDisplay.StrokeString(FBDisplay::Font(), buf, 10, (count+1) * 20, FontSize, -FontSize);

    // if we have a range then we have geographic coords for the aircraft.
    if(range > 0.0) {
      g_fbDisplay.SetTextColor(0xfffffff);
      
      /*
      // draw the aircraft on the map
      DrawMapPoint(displayName, 0xff0000ff, easting, northing);
      // draw the track indicator line (note the reverse of handedness here.)
      float angle = (-a->track * M_PI * 2.0) / 360.0;
      int tx = sin(angle) * TrackLineLen * g_fbDisplay.GetScreenWidth();
      int ty = cos(angle) * TrackLineLen * g_fbDisplay.GetScreenHeight();
      int x, y;
      MapToScreen(easting, northing, x, y);
      g_fbDisplay.PlotLine(x, y, x - tx, y - ty, 0xff0000ff);
      */
      int ringNum, ledID;
      double bearing;
      if(MapCoordToLEDRings(range, easting, northing, ringNum, ledID, bearing)) {
	XY ringScreenPos;
	if(LEDRingToScreen(ringNum, ledID, ringScreenPos)) {
	  g_fbDisplay.DrawCircle(ringScreenPos.x, ringScreenPos.y, 6, 0xff00ffff);
	  g_fbDisplay.DrawCircle(ringScreenPos.x, ringScreenPos.y, 3, 0xff00ffff);
	  //DrawMapPoint(displayName, 0xff00ffff, ringScreenPos);
	}
      }

    }

    a = a->next;
    count++;
  }
}


/// Entry point called from the regular dump1090 interactive show data method.
extern "C" void RenderPPIDisplay(struct aircraft *a, int withNeoring)
{
  if(withNeoring)
    return RenderPPIDisplayNeoRing(a);
  else
    return RenderPPIDisplayFB(a);
}
  
