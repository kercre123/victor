// Originally from Drive Testfix, using a serial-driven color OLED
// Needs to be updated to DMA OLED for Cozmo EP1 Testfix

#include <stdio.h>
#include <stdarg.h>

#include "hal/board.h"
#include "hal/display.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "../app/fixture.h"
#include "hal/oled.h"

static const u8 InitDisplayCmd[] = {
  DISPLAYOFF,
  SETDISPLAYCLOCKDIV, 0xF0,
  SETMULTIPLEX, 0x3F,
  SETDISPLAYOFFSET, 0x0,
  SETSTARTLINE | 0x0,
  CHARGEPUMP, 0x14,
  MEMORYMODE, 0x01,
  SEGREMAP | 0x1,
  COMSCANDEC,
  SETCOMPINS, 0x12,
  SETCONTRAST, 0xCF,
  SETPRECHARGE, 0xF1,
  SETVCOMDETECT, 0x40,
  DISPLAYALLON_RESUME,
  NORMALDISPLAY,
  DISPLAYON
};
static const u8 ResetCursorCmd[] = {
  COLUMNADDR, 0, DISPLAY_WIDTH-1,
  PAGEADDR, 0, (DISPLAY_HEIGHT / 8) - 1
};

static GPIO_TypeDef* MOSI_PORT = GPIOA;
static GPIO_TypeDef* MISO_PORT = GPIOA;
static GPIO_TypeDef* SCK_PORT = GPIOA;

static const uint32_t MOSI_PIN = GPIO_Pin_7;
static const uint32_t MISO_PIN = GPIO_Pin_6;
static const uint32_t SCK_PIN = GPIO_Pin_5;

static const uint32_t MOSI_SOURCE = GPIO_PinSource7;
static const uint32_t MISO_SOURCE = GPIO_PinSource6;
static const uint32_t SCK_SOURCE = GPIO_PinSource5;

static GPIO_TypeDef* CS_PORT = GPIOB;
static GPIO_TypeDef* CMD_PORT = GPIOB;
static GPIO_TypeDef* RES_PORT = GPIOB;

static const uint32_t CS_SOURCE = 11;
static const uint32_t CMD_SOURCE = 10;
static const uint32_t RES_SOURCE = GPIO_PinSource15;

// Write a command to the display
static void DisplayWrite(bool cmd, const u8* p, int count)
{
  if (cmd) {
    GPIO_RESET(CMD_PORT, CMD_SOURCE);
  } else {
    GPIO_SET(CMD_PORT, CMD_SOURCE);
  }
  
  GPIO_RESET(CS_PORT, CS_SOURCE);

  while (count-- > 0) {
    while (SPI1->SR & SPI_SR_BSY) ;
    SPI1->DR = *(p++);
  }

  GPIO_SET(CS_PORT, CS_SOURCE);
}

// Initialize the display on power up
void InitDisplay(void)
{
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;

  // Configure the pins for SPI in AF mode
  GPIO_PinAFConfig(MOSI_PORT, MOSI_SOURCE, GPIO_AF_SPI1);
  GPIO_PinAFConfig(MISO_PORT, MISO_SOURCE, GPIO_AF_SPI1);
  GPIO_PinAFConfig(SCK_PORT, SCK_SOURCE, GPIO_AF_SPI1);

  // Configure the SPI pins
  GPIO_InitStructure.GPIO_Pin = MOSI_PIN | MISO_PIN | SCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(SCK_PORT, &GPIO_InitStructure);
  
  PIN_OUT(CMD_PORT, CMD_SOURCE);  
  
  PIN_OUT(CS_PORT, CS_SOURCE);
  GPIO_SET(CS_PORT, CS_SOURCE);  // Force CS high

  PIN_OUT(RES_PORT, RES_SOURCE);
  GPIO_RESET(RES_PORT, RES_SOURCE);  // Force RESET low

  // Initialize SPI in master mode
  SPI_I2S_DeInit(SPI1);
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);
  SPI_Cmd(SPI1, ENABLE);

  MicroWait(10000);
  GPIO_SET(RES_PORT, RES_SOURCE);

  SPI1->SR = 0;

  DisplayWrite(true, InitDisplayCmd, sizeof(InitDisplayCmd));
  DisplayInvert(true);
}

// Display a bitmap
void DisplayBitmap(u8 x, u8 y, u8 width, u8 height, u8* pic)
{
}

// Clears the screen
void DisplayClear()
{
}

void DisplayInvert(bool invert)
{
  u8 command[] = { invert ? 0xA7 : 0xA6 };
  DisplayWrite(true, command, sizeof(command));
}

// Display a character
void DisplayPutChar(char character)
{
}

void DisplayPutString(const char* string)
{
  while(*string) {
    DisplayPutChar(*(string++));
  }
}

void DisplayPrintf(const char* format, ...)
{
  char dest[64];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(dest, 64, format, argptr);
  va_end(argptr);  
  DisplayPutString(dest);
}

void DisplaySetScroll(u8 line)
{
  u8 command[] = { 0x40 | (line & 0x3F) };
  DisplayWrite(true, command, sizeof(command));
}

// Move the current cursor location
void DisplayMoveCursor(u16 line, u16 column)
{
  // TODO
}

// Set the text width multiplier
void DisplayTextWidthMultiplier(u16 multiplier)
{
}

// Set the text height multiplier
void DisplayTextHeightMultiplier(u16 multiplier)
{
}
