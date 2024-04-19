#pragma once

/*
  Translated from Python by Carri King 2024

Compiled, mashed and generally mutilated 2014-2015 by Denis Pleic
Made available under GNU GENERAL PUBLIC LICENSE

# Modified Python I2C library for Raspberry Pi
# as found on http://www.recantha.co.uk/blog/?p=4849
# Joined existing 'i2c_lib.py' and 'lcddriver.py' into a single library
# added bits and pieces from various sources
# By DenisFromHR (Denis Pleic)
# 2015-02-10, ver 0.1

*/

/*
 * rpii2c.c
 * 
 * Created by Tobias Gall <toga@tu-chemnitz.eu>
 * Based on Adafruit's python code for CharLCDPlate
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

typedef uint8_t i2c_addr;
typedef uint8_t i2c_cmd;
typedef uint8_t i2c_flag;
typedef uint8_t i2c_bitmask;

// default address - mine is at 0x3f though :)
const i2c_addr ADDRESS = 0x27;

class LCD {
public:
  LCD(const i2c_addr addr=ADDRESS, int port=1);
  ~LCD();

  void sleepms(unsigned int ms);
  void lcd_device_write_cmd(uint8_t data);
  void lcd_strobe(const uint8_t data);
  void lcd_write_four_bits(const uint8_t data);
  
  // write a command to lcd
  void lcd_write(const i2c_cmd cmd, const i2c_bitmask mode=0);
  void lcd_write_char(const char charvalue, const i2c_bitmask mode=1);
  void lcd_display_string(const char *str, const uint8_t line);
  void lcd_clear();
  void backlight_on(bool set_to_on);
  void lcd_display_string_pos(const char *str, uint8_t line, uint8_t pos);

protected:
  int m_fd = -1;
};
