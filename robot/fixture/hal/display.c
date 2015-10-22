// Originally from Drive Testfix, using a serial-driven color OLED
// Needs to be updated to DMA OLED for Cozmo EP1 Testfix

#include "hal/display.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "../app/fixture.h"

static BOOL m_alreadyTryingToInitialize = FALSE;
static BOOL m_displayMissing = FALSE;

extern FixtureType g_fixtureType;
static BOOL IsDisplayAvailable(void)
{
  return !m_displayMissing; // && (g_fixtureType & FIXTURE_MASK) != FIXTURE_PCB_TEST;
}

// Write a command to the display
static void DisplayWrite(u8* p, int count)
{
}

// Wait for the display to Ack the command
static void DisplayWait(void)
{
  if (!IsDisplayAvailable())
    return;
}

// Display a bitmap
void DisplayBitmap(u8 x, u8 y, u8 width, u8 height, u8* pic)
{
  if (!IsDisplayAvailable())
    return;
  
  u8 bytes[] = { 0,0xA, 0,x, 0,y, 0,width, 0,height };
  DisplayWrite(bytes, sizeof(bytes));
  MicroWait(5000);  // Avoid stupid Goldelox bug - it is not ready for the data for a bit
  DisplayWrite(pic, width*height*2);
  DisplayWait();
}

// Clears the screen to the specified 16-bit color
void DisplayClear(u16 color)
{
  if (!IsDisplayAvailable())
    return;
  
  u8 bytesColor[] = {0xFF, 0x6E, color, color >>  8};
  DisplayWrite(bytesColor, sizeof(bytesColor));
  DisplayWait();
  
  u8 bytesClear[] = {0xFF, 0xD7};
  DisplayWrite(bytesClear, sizeof(bytesClear));
  DisplayWait();
}

// Display a string
void DisplayPutString(const char* string)
{
  if (!IsDisplayAvailable())
    return;
  
  const char* p = string;
  int length = 0;
  while (*p++)
    length++;
  
  u8 bytes[] = {0x00, 0x06};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWrite((u8*)string, length + 1);
  DisplayWait();
}

// Display a character
void DisplayPutChar(u16 character)
{
  if (!IsDisplayAvailable())
    return;
  
  u8 bytes[] = {0xFF, 0xFE, 0x00, character};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWait();
}

// Display a hexadecimal number
void DisplayPutHex8(u8 data)
{
  DisplayPutChar(g_hex[(data >> 4) & 0x0F]);
  DisplayPutChar(g_hex[data & 0x0F]);
}

void DisplayPutHex16(u16 data)
{
  DisplayPutHex8(data >> 8);
  DisplayPutHex8(data & 0xFF);
}

void DisplayPutHex32(u32 data)
{
  DisplayPutHex16(data >> 16);
  DisplayPutHex16(data & 0xFFFF);
}

// Move the current cursor location
void DisplayMoveCursor(u16 line, u16 column)
{
  if (!IsDisplayAvailable())
    return;
  
  u8 bytes[] = {0xFF, 0xE4, line >> 8, line & 0xFF, column >> 8, column & 0xFF};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWait();
}

// Set the text foreground color
void DisplayTextForegroundColor(Color_t color)
{
  if (!IsDisplayAvailable())
    return;
  
  u8 bytes[] = {0xFF, 0x7F, color, color >> 8};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWait();
}

// Set the text background color
void DisplayTextBackgroundColor(Color_t color)
{
  if (!IsDisplayAvailable())
    return;
  
  u8 bytes[] = {0xFF, 0x7E, color, color >> 8};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWait();
}

// Set the text width multiplier
void DisplayTextWidthMultiplier(u16 multiplier)
{
  if (!IsDisplayAvailable())
    return;
  
  // Enforce the bounds
  if (multiplier < 1)
    multiplier = 1;
  if (multiplier > 16)
    multiplier = 16;
  
  u8 bytes[] = {0xFF, 0x7C, multiplier >> 8, multiplier & 0xFF};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWait();
}

// Set the text height multiplier
void DisplayTextHeightMultiplier(u16 multiplier)
{
  if (!IsDisplayAvailable())
    return;
  
  // Enforce the bounds
  if (multiplier < 1)
    multiplier = 1;
  if (multiplier > 16)
    multiplier = 16;
  
  u8 bytes[] = {0xFF, 0x7B, multiplier >> 8, multiplier & 0xFF};
  DisplayWrite(bytes, sizeof(bytes));
  DisplayWait();
}

// Initialize the display on power up
void InitDisplay(void)
{
  if (!IsDisplayAvailable())
    return;
}

// Get the native display color from RGB8
Color_t DisplayGetRGB565(u8 red, u8 green, u8 blue)
{
  u16 r, g, b;
  r = red >> 3;
  g = green >> 2;
  b = blue >> 3;
  
  // Get rid of weird keil warning of possible usage before 
  u8 data[2] = {0};
  data[0] = ((r << 3) | (g >> 3));
  data[1] = ((g << 5) | b);
  
  // Byte swap
  return (data[1] << 8) | data[0];
}
