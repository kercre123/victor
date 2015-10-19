#include <string.h>
#include <stdint.h>

#include "MK02F12810.h"

#include "oled.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"

const uint8_t SLAVE_ADDRESS     = 0x78; // Or 0x78 if SA is 0.

const uint8_t I2C_READ          = 0x01;
const uint8_t I2C_WRITE         = 0x00;

const uint8_t I2C_COMMAND       = 0x00;
const uint8_t I2C_DATA          = 0x40;
const uint8_t I2C_CONTINUATION  = 0x00;
const uint8_t I2C_SINGLE        = 0x80;

typedef struct {
  int flags;
  i2c_callback cb;
  
  void* data;
  int count;
} I2C_Queue;

static int _fifo_start;
static int _fifo_end;
static int _fifo_count;

static I2C_Queue i2c_queue[MAX_QUEUE];

static bool _active;

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

static const int FramePrefixLength = 13;
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

void FlipOLED(void);
void invert(const void *data, int length) {
  uint8_t *px = (uint8_t*) FrameBuffer;
  length = FrameBufferLength;
  while(length--) *(px++) ^= 0xFF;
  FlipOLED();
}

void FlipOLED(void) {
  i2c_cmd(I2C_DIR_WRITE | I2C_SEND_STOP, ResetCursor, sizeof(ResetCursor), invert);
}

static void oled_reset(void) {
  GPIO_PIN_SOURCE(OLED_RST, PTA,  19);
  GPIO_OUT(GPIO_OLED_RST, PIN_OLED_RST);
  PORTA_PCR19  = PORT_PCR_MUX(1);

  Anki::Cozmo::HAL::MicroWait(80);
  GPIO_RESET(GPIO_OLED_RST, PIN_OLED_RST);
  Anki::Cozmo::HAL::MicroWait(80);
  GPIO_SET(GPIO_OLED_RST, PIN_OLED_RST);

  i2c_cmd(I2C_DIR_WRITE | I2C_SEND_STOP, (uint8_t*)InitDisplay, sizeof(InitDisplay), NULL);
  FlipOLED();
}

// Start I2C bus stuff
static inline uint8_t *next_byte() {
  I2C_Queue *active = &i2c_queue[_fifo_end];

  uint8_t *data = (uint8_t*)active->data;
  active->data = data++;
  active->count--;
  return data;
}

static inline void do_transaction(bool dequeue) {
  // Are we transmitting ?
  if (_active) {
    return ;
  }
  _active = true;

  I2C_Queue *active = &i2c_queue[_fifo_end];

  if (active->flags & I2C_DIR_WRITE) {
    I2C0_C1 |= I2C_C1_TX_MASK | I2C_C1_MST_MASK ;
    I2C0_D = *next_byte();
  } else {
    I2C0_C1 = (I2C0_C1 & ~I2C_C1_TX_MASK) | I2C_C1_MST_MASK;

    if (dequeue) {
      *next_byte() = I2C0_D;
    }
    I2C0_D = 0;
  }
}

// I2C calls
bool i2c_cmd(int mode, uint8_t *bytes, int len, i2c_callback cb) {
  // Queue is full.  Sorry 'bout it.
  if (_fifo_count >= MAX_QUEUE) {
    return false;
  }

  NVIC_DisableIRQ(I2C0_IRQn);
  I2C_Queue *active = &i2c_queue[_fifo_start++];
  if (_fifo_start >= MAX_QUEUE) { _fifo_start = 0; }
  _fifo_count++;
  
  active->cb = cb;
  active->count = len;
  active->data = bytes;
  active->flags = mode;

  do_transaction(false);
  NVIC_EnableIRQ(I2C0_IRQn);
  
  return true;
}

void i2c_init()
{
   // Enable clocking on I2C, PortB, PortE, and DMA
  SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

  // Configure port mux for i2c
  PORTB_PCR1 = PORT_PCR_MUX(2);   //I2C0_SDA
  PORTE_PCR24 = PORT_PCR_MUX(5);  //I2C0_SCL

  // Configure i2c
  I2C0_F  = I2C_F_ICR(0x0D) | I2C_F_MULT(1);
  I2C0_C1 = I2C_C1_IICEN_MASK | I2C_C1_IICIE_MASK;
  
  // Enable IRQs
  NVIC_SetPriority(I2C0_IRQn, 3);
  NVIC_EnableIRQ(I2C0_IRQn);
  
  NVIC_SetPriority(DMA1_IRQn, 3);
  NVIC_EnableIRQ(DMA1_IRQn);

  // Clear our FIFO
  _fifo_start = 0;
  _fifo_end = 0;
  _fifo_count = 0;

  _active = false;
 
  // Init Display the proper way
  oled_reset();
}

void I2C0_IRQHandler(void) {
  // Clear interrupt
  I2C0_S |= I2C_S_IICIF_MASK;
  
  I2C_Queue *active = &i2c_queue[_fifo_end];
  
  if (active->count <= 0) {
    if (++_fifo_end >= MAX_QUEUE) { _fifo_end = 0; }
    
    if (active->flags & I2C_SEND_STOP) {
      I2C0_C1 = I2C0_C1 & ~(I2C_C1_TX_MASK | I2C_C1_MST_MASK);
    } else {
      do_transaction(true);
    }

    if (active->cb) {
      active->cb(active->data, active->count);
    }
    if (--_fifo_count <= 0) {
      _active = false;
    }
  } else {
    do_transaction(true);
  }
}
