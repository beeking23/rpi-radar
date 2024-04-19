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
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <cmath>

#include "strokefont.hpp"
#include "fb-display.h"

void FBDisplay::PutPixel(int x, int y, int color)
{
  if(x < 0 || x >= m_screenWidth || y<0 || y>=m_screenHeight)
    return;
  
  *((int *)(m_fbp) + x + (y * m_screenWidth)) = color;
}

void FBDisplay::PlotLine(int x0, int y0, int x1, int y1, int color)
{
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while(true) {
    PutPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * error;
    if(e2 >= dy) {
      if(x0 == x1)
	break;
      error = error + dy;
      x0 = x0 + sx;
    }
    if(e2 <= dx) {
      if(y0 == y1)
	break;
      error = error + dx;
      y0 = y0 + sy;
    }
  }
}


void FBDisplay::DrawCircle(int x, int y, int radius, int color)
{
  constexpr float sin0 = sin(0.0f);
  constexpr float cos0 = cos(0.0f);
  
  int lastPx = x + sin0 * radius;
  int lastPy = y + cos0 * radius;
  
  for(int n = 1; n<=100; n++) {
    float a = (2.0 * M_PI * n) / 100.0f;
    int px = x + sin(a) * radius;
    int py = y + cos(a) * radius;
    PlotLine(lastPx, lastPy, px, py, color);
    lastPx = px;
    lastPy = py;
  }
}

void FBDisplay::DrawEllipse(int x, int y, int radiusX, int radiusY, int color)
{
  constexpr float sin0 = sin(0.0f);
  constexpr float cos0 = cos(0.0f);
  
  int lastPx = x + sin0 * radiusX;
  int lastPy = y + cos0 * radiusY;
  
  for(int n = 1; n<=100; n++) {
    float a = (2.0 * M_PI * n) / 100.0f;
    int px = x + sin(a) * radiusX;
    int py = y + cos(a) * radiusY;
    PlotLine(lastPx, lastPy, px, py, color);
    lastPx = px;
    lastPy = py;
  }
}

void FBDisplay::StrokeCharacterLine(float x1, float y1, float x2, float y2, int xoff, int yoff)
{
  FBDisplay::PlotLine(x1 + xoff, (y1 + yoff), x2 + xoff, (y2 + yoff), m_textColor);
}

int FBDisplay::StrokeCharacter(const SFG_StrokeFont* font, int character, int xoff, int yoff, float scaleX, float scaleY)
{
  const SFG_StrokeChar *schar = font->Characters[ character ];
  const SFG_StrokeStrip *strip = schar->Strips;

  for(int i = 0; i < schar->Number; i++, strip++ ) {
    float lastX = strip->Vertices[0].X * scaleX;
    float lastY = strip->Vertices[0].Y * scaleY;
    for(int j = 1; j < strip->Number; j++ ) {
      float X = strip->Vertices[ j ].X * scaleX;
      float Y = strip->Vertices[ j ].Y * scaleY;
      StrokeCharacterLine(lastX, lastY, X, Y, xoff, yoff);
      lastX = X;
      lastY = Y;
    }
  }

  xoff += (schar->Right * scaleX);
  return xoff;
}

void FBDisplay::StrokeString(const SFG_StrokeFont* font, const char *str,int xoff, int yoff, float scaleX, float scaleY)
{
  while(*str) {
    int c = (int)*str;
    if(c > 0 && c< font->Quantity) {
      xoff = StrokeCharacter(font, c, xoff, yoff, scaleX, scaleY);
    }
    ++str;
  }
}

void FBDisplay::Clear()
{
  constexpr int clearcolor = 0xff000000;
  int *fptr = (int *)m_fbp;
  int *endPtr = fptr + (m_screenHeight * m_screenWidth);
  while(fptr < endPtr)
    *fptr++ = clearcolor;
}

bool FBDisplay::Open()
{
  // Open the framebuffer device file for reading and writing
  m_fbfd = open("/dev/fb0", O_RDWR);
  if (m_fbfd == -1) {
    printf("Error: cannot open framebuffer device.\n");
    return false;
  }
  printf("The framebuffer device opened.\n");

  // Get fixed screen information
  fb_fix_screeninfo finfo;  
  if (ioctl(m_fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Error reading fixed information.\n");
    return false;
  }

  // Get variable screen information
  fb_var_screeninfo vinfo;  
  if (ioctl(m_fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Error reading variable screen info.\n");
    return false;
  }
  
  m_screenHeight = vinfo.yres;
  m_screenWidth = vinfo.xres;
  m_bpp = vinfo.bits_per_pixel; 

  if(vinfo.bits_per_pixel != 32) {
    printf("Only supporting 32bpp\n");
    return false;
  }
  
  // map framebuffer to user memory 
  m_screensize = finfo.smem_len;

  m_fbp = (char*)mmap(0, 
			m_screensize, 
		      PROT_READ | PROT_WRITE, 
		      MAP_SHARED, 
		      m_fbfd, 0);
  
  if (m_fbp == (char *)-1) {
    printf("Failed to mmap.\n");
    return false;
  }

  Clear();
  usleep(MICROS / 10);
  Clear();

  return true;
}

void FBDisplay::Close()
{
  if(m_fbp)
    munmap(m_fbp, m_screensize);
  if(m_fbfd)
    close(m_fbfd);
}

/* static */ const SFG_StrokeFont *FBDisplay::Font()
{
  return &fgStrokeMonoRoman;
}

