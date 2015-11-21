// Originally from Drive Testfix, using a serial-driven color OLED
// Updated to DMA OLED for Cozmo EP1 Testfix

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "hal/board.h"
#include "hal/display.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "../app/fixture.h"
#include "hal/oled.h"

#include "hal/font.h"


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

static const uint32_t CS_SOURCE = GPIO_PinSource11;
static const uint32_t CMD_SOURCE = GPIO_PinSource10;
static const uint32_t RES_SOURCE = GPIO_PinSource15;

static uint64_t frame[DISPLAY_WIDTH];  // 1 bit per pixel
static int display_x = 0;
static int display_y = 0;
static int scale_x = 1;
static int scale_y = 1;

// Write a command to the display
static void DisplayWrite(bool cmd, const u8* p, int count)
{
  if (cmd) {
    GPIO_RESET(CMD_PORT, CMD_SOURCE);
  } else {
    GPIO_SET(CMD_PORT, CMD_SOURCE);
  }
  MicroWait(1);
  
  GPIO_RESET(CS_PORT, CS_SOURCE);
  MicroWait(1);

  while (count-- > 0) {
    while (!(SPI1->SR & SPI_FLAG_TXE)) ;
    SPI1->DR = *(p++);
  }
  
  // Make sure SPI is totally drained
  while (!(SPI1->SR & SPI_FLAG_TXE)) ;
  while (SPI1->SR & SPI_FLAG_BSY) ;

  GPIO_SET(CS_PORT, CS_SOURCE);
  MicroWait(1);
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
  
  GPIO_SET(CS_PORT, CS_SOURCE);  // Force CS high
  PIN_OUT(CS_PORT, CS_SOURCE);

  GPIO_RESET(RES_PORT, RES_SOURCE);  // Force RESET low
  PIN_OUT(RES_PORT, RES_SOURCE);

  // Initialize SPI in master mode
  SPI_I2S_DeInit(SPI1);
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);
  SPI_Cmd(SPI1, ENABLE);

  MicroWait(10000);
  GPIO_SET(RES_PORT, RES_SOURCE);

  SPI1->SR = 0;


  DisplayWrite(true, InitDisplayCmd, sizeof(InitDisplayCmd));
  DisplayClear();
  DisplayPrintf("Starting up...");
}

void DisplaySetScroll(bool scroll) {
  if (scroll) {
    static const uint8_t enable[] = { 0x29, 0, 0, 0, 0x00, 0x01, 0x2F };
    DisplayWrite(true, enable, sizeof(enable));
  } else {
    static const uint8_t disable[] = { 0x2E };
    DisplayWrite(true, disable, sizeof(disable));
  }
}

void DisplayInvert(bool invert)
{
  u8 command[] = { invert ? 0xA7 : 0xA6 };
  DisplayWrite(true, command, sizeof(command));
}

static unsigned int lastUpdateTime_ = 0;

void DisplayFlip(void) {
  // When the display changes, stop scrolling for a moment
  DisplaySetScroll(false);
  lastUpdateTime_ = getMicroCounter();
  
  DisplayWrite(true, ResetCursorCmd, sizeof(ResetCursorCmd));
  DisplayWrite(false, (u8*) &frame, sizeof(frame));
}

// Call this periodically to enable scrolling
void DisplayUpdate(void) {
  if (lastUpdateTime_ && getMicroCounter() - lastUpdateTime_ > 5000000)
  {
    DisplaySetScroll(true);
    lastUpdateTime_ = 0;
  }
}

// Clears the screen
void DisplayClear()
{
  memset(frame, 0, sizeof(frame));
  DisplayFlip();
}

// Display a character
void DisplayPutChar(char character)
{
  if (display_x + CHAR_WIDTH * scale_x > DISPLAY_WIDTH || character == '\n') {
    display_x = 0;
    display_y += CHAR_HEIGHT * scale_y;
  }

  if (character < CHAR_START || character > CHAR_END) {
    display_x += (CHAR_WIDTH + 1) * scale_x;
    return ;
  }

  const uint8_t *disp = FONT[character-CHAR_START];

  for (int x = 0; x < CHAR_WIDTH && display_x < DISPLAY_WIDTH; x++, disp++) {
    uint64_t mask = (1LL << (CHAR_HEIGHT * scale_y)) - 1;
    uint64_t setbits = ((1LL << scale_y) - 1) << display_y;
    uint64_t setables = 0LL;
    uint8_t bits = *disp;

    for (int y = 0; y < CHAR_HEIGHT; y++, setbits <<= scale_y) {
      // We are not drawing this pixel
      if ((bits >> y) & 1) {
        setables |= setbits;
      }
    }

    for (int xo = 0; xo < scale_x && display_x < DISPLAY_WIDTH; xo++, display_x++) {
      frame[display_x] = (frame[display_x] & ~(uint64_t)(mask << display_y)) | setables;
    }
  }
  
  display_x += scale_x;
}

void DisplayPutString(const char* string)
{
  while(*string) {
    DisplayPutChar(*(string++));
  }
}

// Move the current cursor location
void DisplayMoveCursor(u16 line, u16 column)
{
  display_x = column;
  display_y = line;
}

// Set the text width multiplier
void DisplayTextWidthMultiplier(u16 multiplier)
{
  scale_x = multiplier;
}

// Set the text height multiplier
void DisplayTextHeightMultiplier(u16 multiplier)
{
  scale_y = multiplier;
}

// Print a message to the face - this will permanently replace the face
void DisplayPrintf(const char *format, ...)
{
  char buffer[169];

  va_list argptr;
  va_start(argptr, format);
  vsnprintf(buffer, sizeof(buffer), format, argptr);
  va_end(argptr);
  
  DisplayPutString(buffer);
  DisplayFlip();
}

// This is used to print codes and fixture names
void DisplayBigCenteredText(char* text)
{
  int len = strlen(text);
  const int scale = 3;
  
  DisplayTextHeightMultiplier(scale);
  DisplayTextWidthMultiplier(scale);
  
  DisplayMoveCursor(32-scale*4, 64-len*scale*3);
  
  DisplayPutString(text);
}
