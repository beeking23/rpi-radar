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

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
#include <linux/types.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
#include <sys/ioctl.h>
}

#include "i2clcd.h"

// commands
const i2c_cmd LCD_CLEARDISPLAY = 0x01;
const i2c_cmd LCD_RETURNHOME = 0x02;
const i2c_cmd LCD_ENTRYMODESET = 0x04;
const i2c_cmd LCD_DISPLAYCONTROL = 0x08;
const i2c_cmd LCD_CURSORSHIFT = 0x10;
const i2c_cmd LCD_FUNCTIONSET = 0x20;
const i2c_cmd LCD_SETCGRAMADDR = 0x40;
const i2c_cmd LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
const i2c_flag LCD_ENTRYRIGHT = 0x00;
const i2c_flag LCD_ENTRYLEFT = 0x02;
const i2c_flag LCD_ENTRYSHIFTINCREMENT = 0x01;
const i2c_flag LCD_ENTRYSHIFTDECREMENT = 0x00;

// flags for display on/off control
const i2c_flag LCD_DISPLAYON = 0x04;
const i2c_flag LCD_DISPLAYOFF = 0x00;
const i2c_flag LCD_CURSORON = 0x02;
const i2c_flag LCD_CURSOROFF = 0x00;
const i2c_flag LCD_BLINKON = 0x01;
const i2c_flag LCD_BLINKOFF = 0x00;

// flags for display/cursor shift
const i2c_flag LCD_DISPLAYMOVE = 0x08;
const i2c_flag LCD_CURSORMOVE = 0x00;
const i2c_flag LCD_MOVERIGHT = 0x04;
const i2c_flag LCD_MOVELEFT = 0x00;

// flags for function set
const i2c_flag LCD_8BITMODE = 0x10;
const i2c_flag LCD_4BITMODE = 0x00;
const i2c_flag LCD_2LINE = 0x08;
const i2c_flag LCD_1LINE = 0x00;
const i2c_flag LCD_5x10DOTS = 0x04;
const i2c_flag LCD_5x8DOTS = 0x00;

// flags for backlight control
const i2c_flag LCD_BACKLIGHT = 0x08;
const i2c_flag LCD_NOBACKLIGHT = 0x00;

const i2c_bitmask En = 0b00000100; // Enable bit
const i2c_bitmask Rw = 0b00000010; // Read/Write bit
const i2c_bitmask Rs = 0b00000001; // Register select bit

/*
 * 
 * name: rpiI2cSetup
 * @param __u8 addr (Address of I2C Device), __u8 busnum
 * @return a file descriptor that is open
 * 
 * Sets up an I2C Device
 * 
 */
int rpiI2cSetup(__u8 addr, __u8 busnum)
{
  int fd;
  
  char fileName[64];
  snprintf(fileName, 64, "/dev/i2c-%i", busnum);
  
  // Open File for READ/WRITE
  if((fd = open(fileName, O_RDWR)) < 0) {
    printf("Failed to open %s\n", fileName);
    exit(1);
  }
  
  // Sets Device as I2C Slave
  if(ioctl(fd, I2C_SLAVE, addr) < 0) {
    printf("Failed to set i2c device as secondary\n");
    close(fd);
    exit(1);
  }

  return fd;
}

/*
 * 
 * name: rpiI2cWrite
 * @param __u8 reg (Register), __u8 value (1 Byte Value)
 * @return void
 * 
 * Writes Value to I2C Device on Register reg
 * 
 */
void rpiI2cWrite(int fd, __u8 reg, __u8 value)
{
  if (i2c_smbus_write_byte_data(fd, reg, value) < 0) {
    printf("Failed to to write i2c byte\n");
    close(fd);
    exit(1);
  }
}

/*
 * 
 * name: rpiI2cWriteByte
 * @param __u8 value (1 Byte Value)
 * @return void
 * 
 * Writes Value to I2C Device
 * 
 */
void rpiI2cWriteByte(int fd, __u8 value)
{
  if (i2c_smbus_write_byte(fd, value) < 0) {
    printf("Failed to to write i2c byte\n");
    close(fd);
    exit(1);
  }
}



/*
 * 
 * name: rpiI2cRead8
 * @param __u8 reg (Register)
 * @return __u8 (1 Byte Data)
 * 
 * Reads 1 Byte Data from I2C Device on Register reg
 * 
 */
__u8 rpiI2cRead8(int fd, __u8 reg)
{
  __u8 res = i2c_smbus_read_byte_data(fd, reg);
  return res;
}

/*
 * 
 * name: rpiI2cRead16
 * @param __u8 reg (Register)
 * @return __u16 (2 Byte Data)
 * 
 */
__u16 rpiI2cRead16(int fd, __u8 reg)
{
	
  __u16 hi = i2c_smbus_read_byte_data(fd, reg);
  __u16 res = (hi << 8) + i2c_smbus_read_byte_data(fd, reg+1);
	
  return res;
}

/*
 * 
 * name: rpiI2cClose
 * @param void
 * @return void
 * 
 */
void rpiI2cClose(int fd)
{
  close(fd);
}


LCD::LCD(const i2c_addr addr, int port)
{
  m_fd = rpiI2cSetup(addr, port);
    
  // TODO: Why write all of these? Assuming they're necessary to init.
  lcd_write(0x03);
  lcd_write(0x03);
  lcd_write(0x03);
  lcd_write(0x02);

  lcd_write(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE);
  lcd_write(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
  lcd_write(LCD_CLEARDISPLAY);
  lcd_write(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
  sleepms(200);
}

LCD::~LCD()
{
  if(m_fd > 0)
    rpiI2cClose(m_fd);
}

void LCD::sleepms(unsigned int ms)
{
  usleep(ms * 1000);
}

void LCD::lcd_device_write_cmd(uint8_t data)
{
  rpiI2cWriteByte(m_fd, data);
}

void LCD::lcd_strobe(const uint8_t data)
{
  lcd_device_write_cmd(data | En | LCD_BACKLIGHT);
  sleepms(5);
  lcd_device_write_cmd(((data & ~En) | LCD_BACKLIGHT));
  sleepms(1);
}
  
void LCD::lcd_write_four_bits(const uint8_t data)
{
  lcd_device_write_cmd(data | LCD_BACKLIGHT);
  lcd_strobe(data);
}
  
void LCD::lcd_write(const i2c_cmd cmd, const i2c_bitmask mode)
{
  lcd_write_four_bits(mode | (cmd & 0xF0));
  lcd_write_four_bits(mode | ((cmd << 4) & 0xF0));
}
  
void LCD::lcd_write_char(const char charvalue, const i2c_bitmask mode)
{
  /*
    Args:
    charvalue (int): ordinal value of character to write.
    mode (int): Mode? Defaults to 1.
  */
  lcd_write_four_bits(mode | (charvalue & 0xF0));
  lcd_write_four_bits(mode | ((charvalue << 4) & 0xF0));
  // Couldn't we just do this?
  // lcd_write(charvalue, mode=1)
}
  
void LCD::lcd_display_string(const char *str, const uint8_t line)
{
  /*
    Args:
    string (str): String to be displayed.
    line (int): Line for string to be displayed on. Must be 1-4.
  */
  switch(line) {
  case 1:
    lcd_write(0x80);
    break;
  case 2:
    lcd_write(0xC0);
    break;
  case 3:
    lcd_write(0x94);
    break;
  case 4:
    lcd_write(0xD4);
    break;
  default:
    printf("Line parameter must be 1, 2, 3, or 4.");
    return;
  }
  while(*str) {
    lcd_write((uint8_t)*str, Rs);
    // Could we use lcd_write_char?
    // lcd_write_char(char, Rs)
    ++str;
  }
}
  
void LCD::lcd_clear()
{
  lcd_write(LCD_CLEARDISPLAY);
  lcd_write(LCD_RETURNHOME);
}

void LCD::backlight_on(bool set_to_on)
{
  if(set_to_on)
    lcd_device_write_cmd(LCD_BACKLIGHT);
  else
    lcd_device_write_cmd(LCD_NOBACKLIGHT);
}
  
void LCD::lcd_display_string_pos(const char *str, uint8_t line, uint8_t pos)
{
  /*
    Args:
    string (str): String to be displayed.
    line (int): Line for string to be displayed on. Must be 1-4.
    pos (int): Offset for displaying string
  */
    
  uint8_t pos_new = 0;
  switch(line) {
  case 1:
    pos_new = pos;
    break;
  case 2:
    pos_new = 0x40 + pos;
    break;
  case 3:
    pos_new = 0x14 + pos;
    break;
  case 4:
    pos_new = 0x54 + pos;
    break;
  default:
    printf("Line parameter must be 1, 2, 3, or 4.");
    return;
  }
    
  lcd_write(0x80 + pos_new);
    
  while(*str) {
    lcd_write(*str, Rs);
    ++str;
    // Could we use lcd_write_char?
    // lcd_write_char(char, Rs)
  }
}

