/*
Copyright 2024 Carri King

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#pragma once

struct SFG_StrokeFont;

/// Framebuffer display class - handy for drawing graphics from the Pi in console mode.
/// Which for the older Pi is better than trying to run X and all that. The accelerated
/// DRM mode is probably even better but this is good enough.
class FBDisplay {
public:
  FBDisplay() { }
  ~FBDisplay() { Close(); }

  bool IsOpen() const { return m_fbp != NULL; }
  
  bool Open();
  void Close();
  void Clear();

  void PutPixel(int x, int y, int color);
  void PlotLine(int x0, int y0, int x1, int y1, int color);
  void DrawCircle(int x, int y, int radius, int color);
  void DrawEllipse(int x, int y, int radiusX, int radiusY, int color);
  int StrokeCharacter(const SFG_StrokeFont* font, int character, int xoff, int yoff, float scaleX, float scaleY);
  void StrokeString(const SFG_StrokeFont* font, const char *str,int xoff, int yoff, float scaleX, float scaleY);

  static const SFG_StrokeFont *Font();

  int GetScreenWidth() const { return m_screenWidth; }
  int GetScreenHeight() const { return m_screenHeight; }

  void SetTextColor(const int c) { m_textColor = c; }
  
private:
  void StrokeCharacterLine(float x1, float y1, float x2, float y2, int xoff, int yoff);
  
  int m_screenWidth = 0;
  int m_screenHeight = 0;
  int m_bpp = 0;
  char *m_fbp = nullptr;
  long int m_screensize = 0;
  int m_fbfd = -1;
  int m_textColor = 0xffffffff;
  static constexpr useconds_t MICROS = 1000000;
};




