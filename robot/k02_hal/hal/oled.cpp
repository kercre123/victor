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
  I2C_COMMAND | I2C_SINGLE, DISPLAYON,
};

static const uint8_t ResetCursor[] = {
  I2C_COMMAND | I2C_SINGLE, COLUMNADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 127,
  I2C_COMMAND | I2C_SINGLE, PAGEADDR,
  I2C_COMMAND | I2C_SINGLE, 0,
  I2C_COMMAND | I2C_SINGLE, 7
};

static const uint8_t StartWrite = I2C_DATA | I2C_CONTINUATION;

static bool FaceLock = false;

void Anki::Cozmo::HAL::OLED::Init(void) {
  using namespace Anki::Cozmo::HAL;
  
  GPIO_OUT(GPIO_OLED_RST, PIN_OLED_RST);
  PORTA_PCR19  = PORT_PCR_MUX(1);

  MicroWait(80);
  GPIO_RESET(GPIO_OLED_RST, PIN_OLED_RST);
  MicroWait(80);
  GPIO_SET(GPIO_OLED_RST, PIN_OLED_RST);

  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), InitDisplay, sizeof(InitDisplay), NULL, I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), ResetCursor, sizeof(ResetCursor), NULL);
}

void Anki::Cozmo::HAL::OLED::SendFrame(uint8_t *frame, i2c_callback cb) {
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)ResetCursor, sizeof(ResetCursor), NULL, I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), (uint8_t*)StartWrite, sizeof(StartWrite), NULL);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), frame, FRAME_BUFFER_SIZE, cb);
}

void Anki::Cozmo::HAL::OLED::FeedFace(uint8_t address, uint8_t *face_bytes) {
  static uint8_t FaceCopyLocation = 0;

  if (address != FaceCopyLocation || FaceLock) return ;

  // NOTE: The length of the ring buffer is currently arbitrary
  static uint8_t screen_bytes[8 * MAX_SCREEN_BYTES_PER_DROP];
  static int ring_pos = 0;
  
  // Use a ring buffer to pad out screen data
  uint8_t *bytes = &screen_bytes[ring_pos];
  ring_pos += MAX_SCREEN_BYTES_PER_DROP;
  if (ring_pos >= sizeof(screen_bytes)) {
    ring_pos = 0;
  }
  
  // Load face data on the I2C bus
  memcpy(bytes, face_bytes, MAX_SCREEN_BYTES_PER_DROP);  
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), &StartWrite, sizeof(StartWrite), NULL, I2C_OPTIONAL | I2C_FORCE_START);
  I2C::Write(SLAVE_WRITE(SLAVE_ADDRESS), bytes, MAX_SCREEN_BYTES_PER_DROP, NULL);
  FaceCopyLocation = (FaceCopyLocation + 1) % MAX_FACE_POSITIONS;
}

static volatile bool pending = false;
static void StopHalt(const void* data, int count) {
  pending = false;
}

// Display a number
void Anki::Cozmo::HAL::OLED::ErrorCode(uint16_t code) {
  using namespace Anki::Cozmo::HAL;
  
  uint8_t FrameBuffer[SCREEN_HEIGHT*SCREEN_WIDTH/8];
  uint8_t *write_ptr = &FrameBuffer[sizeof(FrameBuffer) - SCREEN_WIDTH/8];
  
  memset(FrameBuffer, 0, sizeof(FrameBuffer));
  
  FaceLock = true;
  do {
    const uint8_t *px = &FONT[code % 10][CHAR_WIDTH-1];
    code /= 10;

    for (int i = 0; i < CHAR_WIDTH; i++) {
      *write_ptr = *(px--);
      write_ptr -= SCREEN_HEIGHT/8;
    }
  } while (code > 0);

  pending = true;
  OLED::SendFrame(FrameBuffer, &StopHalt);
  while (pending) ;
}
