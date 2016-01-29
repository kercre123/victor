#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "hal/i2c.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "hal/portable.h"

#include "hardware.h"

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

// Display a number
void Anki::Cozmo::HAL::OLED::ErrorCode(uint16_t code) {
  using namespace Anki::Cozmo::HAL;
  
  uint8_t FrameBuffer[(CHAR_WIDTH + 1) * 5];
  uint8_t *write_ptr = &FrameBuffer[sizeof(FrameBuffer)];
  
  FaceLock = true;
  int bytes = 0;
  do {
    write_ptr -= CHAR_WIDTH;
    memcpy(write_ptr, FONT[code % 10], CHAR_WIDTH);
    *(--write_ptr) = 0;
    bytes += CHAR_WIDTH + 1;
    code /= 10;
  } while (code > 0);


  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)ResetCursor, sizeof(ResetCursor), I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)StartWrite, sizeof(StartWrite), I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), write_ptr, bytes);
}
