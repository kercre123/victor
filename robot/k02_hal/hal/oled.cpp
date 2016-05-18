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

  ClearFace();
}

void Anki::Cozmo::HAL::OLED::ClearFace() {
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), ResetCursor, sizeof(ResetCursor), I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), &StartWrite, sizeof(StartWrite), I2C_FORCE_START);
  
  static const uint32_t clear = 0;
  for (int i = 0; i < 1024; i += sizeof(clear)) {
    I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (const uint8_t*)&clear, sizeof(clear));
    I2C::Flush();
  }
}

void Anki::Cozmo::HAL::OLED::FeedFace(bool rect, const uint8_t *face_bytes) {
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

void Anki::Cozmo::HAL::OLED::DisplayDigit(int x, int y, int digit) {
  static const uint8_t DIGITS[10][5] = {
   { 0x3E, 0x51, 0x49, 0x45, 0x3E },
   { 0x00, 0x42, 0x7F, 0x40, 0x00 },
   { 0x72, 0x49, 0x49, 0x49, 0x46 },
   { 0x21, 0x41, 0x49, 0x4D, 0x33 },
   { 0x18, 0x14, 0x12, 0x7F, 0x10 },
   { 0x27, 0x45, 0x45, 0x45, 0x39 },
   { 0x3C, 0x4A, 0x49, 0x49, 0x31 },
   { 0x41, 0x21, 0x11, 0x09, 0x07 },
   { 0x36, 0x49, 0x49, 0x49, 0x36 },
   { 0x46, 0x49, 0x49, 0x29, 0x1E }
  };

  uint8_t command[] = {
    I2C_COMMAND | I2C_CONTINUATION,
    COLUMNADDR, x, x + sizeof(DIGITS[digit]) - 1,
    PAGEADDR, y, y
  };

  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), 
      command,
      sizeof(command),
      I2C_FORCE_START);

  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), 
    &StartWrite, 
    sizeof(StartWrite), 
    I2C_FORCE_START);

  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), 
    DIGITS[digit], 
    sizeof(DIGITS[digit]));
}
