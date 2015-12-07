#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "MK02F12810.h"

#include "hal/i2c.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
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
const int MAX_FACE_POSITIONS = SCREEN_WIDTH * SCREEN_HEIGHT / 8 / MAX_SCREEN_BYTES_PER_DROP;

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
  I2C_COMMAND | I2C_SINGLE, 0x00, 
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
  I2C_COMMAND | I2C_SINGLE, DISPLAYON,
  I2C_COMMAND | I2C_SINGLE, COLUMNADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 127,
  I2C_COMMAND | I2C_SINGLE, PAGEADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 7
};

static const uint8_t ResetCursor[] = {
  SLAVE_ADDRESS | I2C_WRITE,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 127,
  I2C_COMMAND | I2C_SINGLE, PAGEADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 7
};

static const uint8_t StartWrite[] = {
  SLAVE_ADDRESS | I2C_WRITE,
  I2C_DATA | I2C_CONTINUATION
};

static uint8_t FaceCopyLocation = 0;
static bool PrintFaceLock = false;

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
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
      
      void OLEDFeedFace(uint8_t address, uint8_t *face_bytes) {
        if (address != FaceCopyLocation) return ;

        static uint8_t bytes[MAX_SCREEN_BYTES_PER_DROP];

        memcpy(bytes, face_bytes, sizeof(bytes));
        I2CCmd(I2C_DIR_WRITE | I2C_SEND_START, (uint8_t*)StartWrite, sizeof(StartWrite), NULL);
        I2CCmd(I2C_DIR_WRITE | I2C_SEND_STOP, bytes, sizeof(bytes), NULL);
        address = (address + 1) % MAX_FACE_POSITIONS;
      }
    }
  }
}

// Detach from the face and resume streaming of face data
extern "C" void LeaveFacePrintf(void) {
  using namespace Anki::Cozmo::HAL;

  if (!PrintFaceLock) return ;
    
  I2CCmd(I2C_DIR_WRITE | I2C_SEND_START | I2C_SEND_STOP, (uint8_t*)ResetCursor, sizeof(ResetCursor), NULL);
  PrintFaceLock = true;
  FaceCopyLocation = 0;
}

// Flag when the frame has copied and the stack is safe to return
static volatile bool PrintComplete = false;
static void FinishFace(void *data, int count) {
  PrintComplete = true;
}

// Print to the face
extern "C" void FacePrintf(const char *format, ...) {
  using namespace Anki::Cozmo::HAL;

  const int CHARS_PER_LINE = SCREEN_WIDTH / (CHAR_WIDTH + 1);
  const int LINES = SCREEN_HEIGHT / CHAR_HEIGHT;
  const int OVERFLOW = SCREEN_WIDTH - CHARS_PER_LINE * (CHAR_WIDTH + 1);
  const int MAX_CHARS = CHARS_PER_LINE * LINES;
  
  uint8_t FrameBuffer[SCREEN_HEIGHT*SCREEN_WIDTH/8];
  char buffer[MAX_CHARS];
  
  va_list aptr;
  int chars;
  
  va_start(aptr, format);
  vsnprintf(buffer, sizeof(buffer), format, aptr);
  va_end(aptr);

  char *write = buffer;
  int px_ptr = 0;
  
  memset(FrameBuffer, 0, sizeof(FrameBuffer));
  
  int x_rem = CHARS_PER_LINE;
  int y_rem = LINES;

  PrintComplete = false ;
  while (*write && y_rem > 0) {
    if (*write == '\n' || x_rem-- <= 0) {
      px_ptr += OVERFLOW + x_rem * (CHAR_WIDTH + 1);
      x_rem = CHARS_PER_LINE;
      y_rem --;
    } 

    uint8_t ch = *(write++);

    if (ch == '\n') {
      continue ;
    } else if (ch < CHAR_START || ch > CHAR_END) {
      ch = CHAR_START;
    }

    memcpy(&FrameBuffer[px_ptr], FONT[ch - CHAR_START], CHAR_WIDTH);
    px_ptr += CHAR_WIDTH;
    FrameBuffer[px_ptr++] = 0;
  }

  PrintFaceLock = true;

  I2CCmd(I2C_DIR_WRITE | I2C_SEND_START | I2C_SEND_STOP, (uint8_t*)ResetCursor, sizeof(ResetCursor), NULL);
  I2CCmd(I2C_DIR_WRITE | I2C_SEND_START, (uint8_t*)StartWrite, sizeof(StartWrite), NULL);
  I2CCmd(I2C_DIR_WRITE | I2C_SEND_STOP, FrameBuffer, px_ptr, &FinishFace);
  
  while (!PrintComplete)  __asm { WFI } ;
}
