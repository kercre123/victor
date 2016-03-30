#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "hal/i2c.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "hal/portable.h"

#include "hardware.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

#define ONLY_DIGITS
#include "font.h"

const uint8_t SLAVE_ADDRESS     = 0x78 >> 1;

const uint8_t I2C_COMMAND       = 0x00;
const uint8_t I2C_DATA          = 0x40;
const uint8_t I2C_CONTINUATION  = 0x00;
const uint8_t I2C_SINGLE        = 0x80;

const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int FRAME_BUFFER_SIZE = SCREEN_WIDTH * SCREEN_HEIGHT / 8;
const int MAX_FACE_POSITIONS = FRAME_BUFFER_SIZE / MAX_SCREEN_BYTES_PER_DROP;

// Display constants
static const uint8_t InitDisplay[] = {
  I2C_COMMAND | I2C_CONTINUATION, 
  DISPLAYOFF,
  SETDISPLAYCLOCKDIV, 0xF0, 
  SETMULTIPLEX, 0x3F,
  SETDISPLAYOFFSET, 0x0, 
  SETSTARTLINE | 0x0, 
  CHARGEPUMP, 0x14, 
  MEMORYMODE, 0x00,
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

static const uint8_t ResetCursor[] = {
  I2C_COMMAND | I2C_CONTINUATION,
  COLUMNADDR, 0, 127,
  PAGEADDR, 0, 7
};

struct ScreenRect {
  uint8_t left;
  uint8_t right;
  uint8_t top;
  uint8_t bottom;
};

static const uint8_t StartWrite = I2C_DATA | I2C_CONTINUATION;

static const uint8_t PinFont[][16] = {
  {0,248,252,204,12,252,248,0,0,31,63,48,51,63,31,0},
  {0,0,24,252,252,0,0,0,0,0,0,63,63,0,0,0},
  {0,56,60,140,140,252,248,0,0,62,63,51,49,49,48,0},
  {0,56,60,140,140,252,120,0,0,28,60,49,49,63,31,0},
  {0,240,240,128,128,252,252,0,0,1,1,1,1,63,63,0},
  {0,124,252,204,204,204,140,0,0,28,60,48,48,63,31,0},
  {0,248,252,204,204,220,152,0,0,31,63,48,48,63,31,0},
  {0,28,28,12,12,252,252,0,0,0,0,0,0,63,63,0},
  {0,120,252,204,204,252,120,0,0,31,63,48,48,63,31,0},
  {0,120,252,204,204,252,248,0,0,24,56,48,48,63,31,0},
  {0,248,252,204,204,252,248,0,0,63,63,0,0,63,63,0},
  {0,252,252,204,204,252,184,0,0,63,63,48,48,63,31,0},
  {0,248,252,12,12,60,56,0,0,31,63,48,48,60,28,0},
  {0,252,252,12,12,252,248,0,0,63,63,48,48,63,31,0},
  {0,252,252,204,204,12,12,0,0,63,63,48,48,48,48,0},
  {0,252,252,204,204,12,12,0,0,63,63,0,0,0,0,0}
};

static bool FaceLock = false;
static int FaceRemaining = 0;

void Anki::Cozmo::HAL::OLED::Init(void) {
  using namespace Anki::Cozmo::HAL;
  
  GPIO_OUT(GPIO_OLED_RST, PIN_OLED_RST);
  PORTA_PCR19  = PORT_PCR_MUX(1);

  MicroWait(80);
  GPIO_RESET(GPIO_OLED_RST, PIN_OLED_RST);
  MicroWait(80);
  GPIO_SET(GPIO_OLED_RST, PIN_OLED_RST);

  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), InitDisplay, sizeof(InitDisplay), I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), ResetCursor, sizeof(ResetCursor), I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)StartWrite, sizeof(StartWrite), I2C_FORCE_START);
}

void Anki::Cozmo::HAL::OLED::FeedFace(bool rect, uint8_t *face_bytes) {
  if (FaceLock) { return ; }

  static bool was_rect = false;
  
  if (rect) {
    ScreenRect* bounds = (ScreenRect*) face_bytes;

    uint8_t command[] = {
      I2C_COMMAND | I2C_CONTINUATION,
      COLUMNADDR, bounds->left, bounds->right,
      PAGEADDR, bounds->top, bounds->bottom
    };

    I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), 
        command,
        sizeof(command),
        I2C_FORCE_START);

    FaceRemaining = (bounds->bottom - bounds->top + 1) * (bounds->right - bounds->left + 1);
    was_rect = true;
  } else {
    int tx_size = MIN(FaceRemaining, MAX_SCREEN_BYTES_PER_DROP);
    if (tx_size <= 0) return ;

    // If we sent a rectangle, force a reset
    I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), 
      &StartWrite, 
      sizeof(StartWrite), 
      was_rect ? I2C_FORCE_START : I2C_OPTIONAL);

    I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), face_bytes, tx_size);
    FaceRemaining -= MAX_SCREEN_BYTES_PER_DROP;
  }

  was_rect = rect;
}

void Anki::Cozmo::HAL::OLED::ReleaseFace() {
  FaceLock = false;
}

void Anki::Cozmo::HAL::OLED::DisplayNumber(int code, int x, int y) {
  static const int TOTAL_DIGITS = 5;	
  static const int SYMBOL_BITS = 4;

  static const int FONT_WIDTH = 8;
  static const int FONT_HEIGHT = 2;

  // Bind to proper rectangle
  const uint8_t Rectangle[] = {
    I2C_COMMAND | I2C_CONTINUATION,
    COLUMNADDR, x, x + (TOTAL_DIGITS * FONT_WIDTH) - 1,
    PAGEADDR, y, y + FONT_HEIGHT - 1
  };

  FaceLock = true;

  // Start writting the font
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)Rectangle, sizeof(Rectangle), I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)StartWrite, sizeof(StartWrite), I2C_FORCE_START);
    
  for (int o = 0; o < FONT_HEIGHT * FONT_WIDTH; o += FONT_WIDTH) {
    for (int i = (TOTAL_DIGITS - 1) * SYMBOL_BITS; i >= 0; i -= SYMBOL_BITS) {
      int character = (code >> i) & 0xF;
      I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), &PinFont[character][o], FONT_WIDTH);
    }
  }
}

using namespace Anki::Cozmo::RobotInterface;

namespace Anki {
  namespace Cozmo {
    namespace Messages {
      void Process_oledRelease(const DisplayRelease& msg) {
        Anki::Cozmo::HAL::OLED::ReleaseFace();
      }

      void Process_oledDisplayNumber(const DisplayNumber& msg) {
        Anki::Cozmo::HAL::OLED::DisplayNumber(msg.value, msg.x, msg.y);
      }
    }
  }
}
