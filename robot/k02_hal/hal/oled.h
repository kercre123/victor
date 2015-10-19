#include <stdint.h>

#ifndef OLED_H
#define OLED_H

// These are the command bytes
#define SETCONTRAST 0x81
#define DISPLAYALLON_RESUME 0xA4
#define DISPLAYALLON 0xA5
#define NORMALDISPLAY 0xA6
#define INVERTDISPLAY 0xA7
#define DISPLAYOFF 0xAE
#define DISPLAYON 0xAF

#define SETDISPLAYOFFSET 0xD3
#define SETCOMPINS 0xDA

#define SETVCOMDETECT 0xDB

#define SETDISPLAYCLOCKDIV 0xD5
#define SETPRECHARGE 0xD9

#define SETMULTIPLEX 0xA8

#define SETLOWCOLUMN 0x00
#define SETHIGHCOLUMN 0x10

#define SETSTARTLINE 0x40

#define MEMORYMODE 0x20
#define COLUMNADDR 0x21
#define PAGEADDR   0x22

#define COMSCANINC 0xC0
#define COMSCANDEC 0xC8

#define SEGREMAP 0xA0

#define CHARGEPUMP 0x8D

#define EXTERNALVCC 0x1
#define SWITCHCAPVCC 0x2

#define ACTIVATE_SCROLL 0x2F
#define DEACTIVATE_SCROLL 0x2E
#define SET_VERTICAL_SCROLL_AREA 0xA3
#define RIGHT_HORIZONTAL_SCROLL 0x26
#define LEFT_HORIZONTAL_SCROLL 0x27
#define VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

static const int MAX_QUEUE = 16;

typedef void (*i2c_callback)(const void *data, int count);

enum I2C_Mode {
  I2C_DIR_READ  = 0,
  I2C_DIR_WRITE = 1,
  I2C_SEND_STOP = 2
};

void i2c_init(void);
bool i2c_cmd(int mode, uint8_t *bytes, int len, i2c_callback cb);

#endif
