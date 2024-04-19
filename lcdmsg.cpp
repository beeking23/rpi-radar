/*
Copyright 2024 Carri King

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// This is a little console app that can be added to the Raspberry Pi boot sequence so that when running headless
// there is some more indication that stuff is going on. Better would be to hook the boot log messages and show
// them but this served my purpose of simply knowing that it was alive and not hung, for example, because the
// the NeoRing was drawing too much power!

#include <cstdint>
#include "i2clcd.h"

LCD *g_lcd = nullptr;

int
main(int argc, char **argv)
{
  g_lcd = new LCD(0x27, 0);
  int a = 1;
  for(a=1; a<argc && a<=4; ++a)
    g_lcd->lcd_display_string(argv[a], a);

  for(; a<=4; a++) {
    g_lcd->lcd_display_string("                ", a);
  }
  return 0;
}
