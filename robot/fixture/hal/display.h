#ifndef DISPLAY_H
#define DISPLAY_H

#include "lib/stm32f2xx.h"

// Initialize the display on power up
void InitDisplay(void);

// Display a bitmap
//void DisplayBitmap(u8 x, u8 y, u8 width, u8 height, u8* pic);

// Clears the screen to the specified 16-bit color
void DisplayClear();

void DisplayInvert(bool invert);

void DisplayPutChar(char character);

void DisplayPutString(const char* string);

void DisplaySetScroll(u8 line);
  
// Display a string
void DisplayPrintf(const char* format, ...);

// Move the current cursor location
void DisplayMoveCursor(u16 line, u16 column);

// Set the text width multiplier
void DisplayTextWidthMultiplier(u16 multiplier);

// Set the text height multiplier
void DisplayTextHeightMultiplier(u16 multiplier);

#endif
