#ifndef DISPLAY_H
#define DISPLAY_H

#include "lib/stm32f2xx.h"

#define DISPLAY_WIDTH   (96)
#define DISPLAY_HEIGHT  (64)

typedef u16 Color_t;

// Initialize the display on power up
void InitDisplay(void);

// Display a bitmap
void DisplayBitmap(u8 x, u8 y, u8 width, u8 height, u8* pic);

// Clears the screen to the specified 16-bit color
void DisplayClear(u16 color);

// Display a string
void DisplayPutString(const char* string);

// Display a character at the current cursor location
void DisplayPutChar(u16 character);

// Display a hexadecimal number
void DisplayPutHex8(u8 data);
void DisplayPutHex16(u16 data);
void DisplayPutHex32(u32 data);

// Move the current cursor location
void DisplayMoveCursor(u16 line, u16 column);

// Set the text foreground color
void DisplayTextForegroundColor(Color_t color);

// Set the text background color
void DisplayTextBackgroundColor(Color_t color);

// Set the text width multiplier
void DisplayTextWidthMultiplier(u16 multiplier);

// Set the text height multiplier
void DisplayTextHeightMultiplier(u16 multiplier);

// Get the native display color from RGB8
Color_t DisplayGetRGB565(u8 red, u8 green, u8 blue);

// Creates a display color code from an 8-bit RGB color (range 0..255)
#define RGB2CODE(r, g, b) (   (( (((r)>>3)<<3) | (((g)>>2)>>3) )&0xff)   |   ( (( (((g)>>2)<<5) | ((b)>>3) )&0xff) << 8)    )

#endif
