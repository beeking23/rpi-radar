/*
Copyright 2024 Carri King

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

  void RenderPPIDisplay(struct aircraft *firstAircraft, int withNeoring);

#ifdef __cplusplus
}
#endif

// the C code doesn't need to see this
#ifdef __cplusplus

// Fixed scalar to convert meters to (standard international) miles.
constexpr double MetersToMiles = 0.0006213712;

// The size of the display region in miles.
constexpr double RegionSizeMiles = 10 * 2.0;//25 * 2;

extern double g_regionMinX;
extern double g_regionMinY;
extern double g_regionWidthY;
extern double g_regionWidthX;
void MapToScreen(double mapX, double mapY, int& x, int& y);

#endif

