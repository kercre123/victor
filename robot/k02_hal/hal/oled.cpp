#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "MK02F12810.h"

#include "hal/i2c.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

#include "hardware.h"

#include "font.h"

const uint8_t SLAVE_ADDRESS     = 0x78;

const uint8_t I2C_READ          = 0x01;
const uint8_t I2C_WRITE         = 0x00;

const uint8_t I2C_COMMAND       = 0x00;
const uint8_t I2C_DATA          = 0x40;
const uint8_t I2C_CONTINUATION  = 0x00;
const uint8_t I2C_SINGLE        = 0x80;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;

// Display constants
static const uint8_t InitDisplay[] = {
  SLAVE_ADDRESS | I2C_WRITE,
  I2C_COMMAND | I2C_SINGLE, DISPLAYOFF,
  I2C_COMMAND | I2C_SINGLE, SETDISPLAYCLOCKDIV, 
  I2C_COMMAND | I2C_SINGLE, 0xF0, 
  I2C_COMMAND | I2C_SINGLE, SETMULTIPLEX, 
  I2C_COMMAND | I2C_SINGLE, 0x3F, 
  I2C_COMMAND | I2C_SINGLE, SETDISPLAYOFFSET, 
  I2C_COMMAND | I2C_SINGLE, 0x0, 
  I2C_COMMAND | I2C_SINGLE, SETSTARTLINE | 0x0, 
  I2C_COMMAND | I2C_SINGLE, CHARGEPUMP,
  I2C_COMMAND | I2C_SINGLE, 0x14, 
  I2C_COMMAND | I2C_SINGLE, MEMORYMODE,
  I2C_COMMAND | I2C_SINGLE, 0x01, 
  I2C_COMMAND | I2C_SINGLE, SEGREMAP | 0x1,
  I2C_COMMAND | I2C_SINGLE, COMSCANDEC,
  I2C_COMMAND | I2C_SINGLE, SETCOMPINS,
  I2C_COMMAND | I2C_SINGLE, 0x12, 
  I2C_COMMAND | I2C_SINGLE, SETCONTRAST,
  I2C_COMMAND | I2C_SINGLE, 0xCF,
  I2C_COMMAND | I2C_SINGLE, SETPRECHARGE,
  I2C_COMMAND | I2C_SINGLE, 0xF1,
  I2C_COMMAND | I2C_SINGLE, SETVCOMDETECT,
  I2C_COMMAND | I2C_SINGLE, 0x40,
  I2C_COMMAND | I2C_SINGLE, DISPLAYALLON_RESUME,
  I2C_COMMAND | I2C_SINGLE, NORMALDISPLAY,
  I2C_COMMAND | I2C_SINGLE, DISPLAYON
};

static const int FramePrefixLength = 14;
static const int FrameBufferLength = 128*64/8;

static uint8_t ResetCursor[FramePrefixLength+FrameBufferLength] = {
  SLAVE_ADDRESS | I2C_WRITE,
  I2C_COMMAND | I2C_SINGLE, COLUMNADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 127,
  I2C_COMMAND | I2C_SINGLE, PAGEADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 7,
  I2C_DATA | I2C_CONTINUATION
};

static uint8_t *FrameBuffer = &ResetCursor[FramePrefixLength];

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void OLEDFlip(void) {
        I2CCmd(I2C_DIR_WRITE | I2C_SEND_START, ResetCursor, sizeof(ResetCursor), NULL);
      }

      void FacePrintf(const char *format, ...) {
        const int MAX_CHARS = SCREEN_WIDTH / (CHAR_WIDTH + 1);
        char buffer[MAX_CHARS];
        
        va_list aptr;
        int chars;

        va_start(aptr, format);
        vsnprintf(buffer, sizeof(buffer), format, aptr);
        va_end(aptr);

        char *write = buffer;
        int px_ptr = 0;
        
        memset(FrameBuffer, 0, FrameBufferLength);
        
        while (*write) {
          int idx = *(write++) - CHAR_START;
          if (idx < 0 || idx >= CHAR_END) {
            idx = 0;
          }

          const uint8_t* pixels = (const uint8_t*)&FONT[idx];
          
          for (int i = 0; i < CHAR_WIDTH; i++) {
            FrameBuffer[px_ptr] = pixels[i];
            px_ptr += 8;
          }
          FrameBuffer[px_ptr] = 0;
          px_ptr += 8;
        }
        
        OLEDFlip();
      }
      
      void OLEDInit(void) {
        using namespace Anki::Cozmo::HAL;
        
        GPIO_OUT(GPIO_OLED_RST, PIN_OLED_RST);
        PORTA_PCR19  = PORT_PCR_MUX(1);

        MicroWait(80);
        GPIO_RESET(GPIO_OLED_RST, PIN_OLED_RST);
        MicroWait(80);
        GPIO_SET(GPIO_OLED_RST, PIN_OLED_RST);

        I2CCmd(I2C_DIR_WRITE | I2C_SEND_START, (uint8_t*)InitDisplay, sizeof(InitDisplay), NULL);
      }
    }
  }
}
