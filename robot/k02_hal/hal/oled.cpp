#include <string.h>
#include <stdint.h>

#include "board.h"
#include "oled.h"
#include "anki/cozmo/robot/hal.h"

#define SOFT_RESET

const uint8_t SLAVE_ADDRESS     = 0x7A; // Or 0x78 if SA is 0.

const uint8_t I2S_READ          = 0x01;
const uint8_t I2S_WRITE         = 0x00;

const uint8_t I2S_COMMAND       = 0x00;
const uint8_t I2S_DATA          = 0x40;
const uint8_t I2S_CONTINUATION  = 0x00;
const uint8_t I2S_SINGLE        = 0x80;


// FIFO junk
const int MAX_QUEUE = 16;

typedef void (*i2c_callback)(const void *data, int count);

enum I2C_Direction {
  I2C_READ,
  I2C_WRITE
};

typedef struct {
  int direction;
  i2c_callback cb;
  
  const void* data;
  int count;
} I2C_Queue;

static int _fifo_start;
static int _fifo_end;
static int _fifo_count;

static I2C_Queue i2c_queue[MAX_QUEUE];

static bool _active;

// Display constants
static const uint8_t InitDisplay[] = {
  I2S_COMMAND | I2S_SINGLE, DISPLAYOFF,
  I2S_COMMAND | I2S_SINGLE, SETDISPLAYCLOCKDIV, 
  I2S_COMMAND | I2S_SINGLE, 0xF0, 
  I2S_COMMAND | I2S_SINGLE, SETMULTIPLEX, 
  I2S_COMMAND | I2S_SINGLE, 0x3F, 
  I2S_COMMAND | I2S_SINGLE, SETDISPLAYOFFSET, 
  I2S_COMMAND | I2S_SINGLE, 0x0, 
  I2S_COMMAND | I2S_SINGLE, SETSTARTLINE | 0x0, 
  I2S_COMMAND | I2S_SINGLE, CHARGEPUMP,
  I2S_COMMAND | I2S_SINGLE, 0x14, 
  I2S_COMMAND | I2S_SINGLE, MEMORYMODE,
  I2S_COMMAND | I2S_SINGLE, 0x01, 
  I2S_COMMAND | I2S_SINGLE, SEGREMAP | 0x1,
  I2S_COMMAND | I2S_SINGLE, COMSCANDEC,
  I2S_COMMAND | I2S_SINGLE, SETCOMPINS,
  I2S_COMMAND | I2S_SINGLE, 0x12, 
  I2S_COMMAND | I2S_SINGLE, SETCONTRAST,
  I2S_COMMAND | I2S_SINGLE, 0xCF,
  I2S_COMMAND | I2S_SINGLE, SETPRECHARGE,
  I2S_COMMAND | I2S_SINGLE, 0xF1,
  I2S_COMMAND | I2S_SINGLE, SETVCOMDETECT,
  I2S_COMMAND | I2S_SINGLE, 0x40,
  I2S_COMMAND | I2S_SINGLE, DISPLAYALLON_RESUME,
  I2S_COMMAND | I2S_SINGLE, NORMALDISPLAY,
  I2S_COMMAND | I2S_SINGLE, DISPLAYON
};

static const int FramePrefixLength = 13;
static const int FrameBufferLength = 128*64/8;

static uint8_t ResetCursor[FramePrefixLength+FrameBufferLength] = {
  I2S_COMMAND | I2S_SINGLE, COLUMNADDR,
  I2S_COMMAND | I2S_SINGLE, 0,
  I2S_COMMAND | I2S_SINGLE, 127,
  I2S_COMMAND | I2S_SINGLE, PAGEADDR,
  I2S_COMMAND | I2S_SINGLE, 0,
  I2S_COMMAND | I2S_SINGLE, 7,
  I2S_DATA | I2S_CONTINUATION
};

static uint8_t *FrameBuffer = &ResetCursor[FramePrefixLength];

const uint32_t OLED_RST  = GPIO_MAKE_PIN(HW_PORTC, 10);

void DMA_TX_Init(const void* source_addr, void* dest_addr, uint32_t num_bytes) {
  // Disable DMA
  DMA_ERQ &= ~DMA_ERQ_ERQ1_MASK;

  // DMA source DMA Mux (i2c0)
  DMAMUX_CHCFG1 = (DMAMUX_CHCFG_ENBL_MASK | DMAMUX_CHCFG_SOURCE(18)); 
  
  // Configure source address
  DMA_TCD1_SADDR          = (uint32_t)source_addr;
  DMA_TCD1_SOFF           = 1;
  DMA_TCD1_SLAST          = -num_bytes;

  // Configure source address
  DMA_TCD1_DADDR          = (uint32_t)dest_addr;
  DMA_TCD1_DOFF           = 0;
  DMA_TCD1_DLASTSGA       = 0;
  
  DMA_TCD1_NBYTES_MLNO    = 1;                                       // The minor loop moves 32 bytes per transfer
  DMA_TCD1_BITER_ELINKNO  = num_bytes;                               // Major loop iterations
  DMA_TCD1_CITER_ELINKNO  = num_bytes;                               // Set current interation count  
  DMA_TCD1_ATTR           = (DMA_ATTR_SSIZE(0) | DMA_ATTR_DSIZE(0)); // Source/destination size (8bit)
 
  DMA_TCD1_CSR            = DMA_CSR_INTMAJOR_MASK | DMA_CSR_DREQ_MASK; // Enable end of loop DMA interrupt; clear ERQ @ end of major iteration               

  // Enable DMA
  DMA_ERQ |= DMA_ERQ_ERQ1_MASK;
}

void dequeue() {
  I2C_Queue *active = &i2c_queue[_fifo_end];

  switch (active->direction) {
    case I2C_WRITE:
      // Enable i2C circuit as TX master, which will send a start condition
      I2C0_C1 |= I2C_C1_TX_MASK | I2C_C1_MST_MASK | I2C_C1_DMAEN_MASK;
      I2C0_D =  SLAVE_ADDRESS | I2S_WRITE;

      // Setup DMA for the future *ghost noises here*
      DMA_TX_Init(active->data, (void*)&(I2C0_D), active->count);  
      break ;
    case I2C_READ:
      // TODO
      break ;
  }

  _active = true;
}

// I2C calls
bool i2s_write(const uint8_t *bytes, int len, i2c_callback cb) {
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
  active->direction = I2C_WRITE;

  if (!_active) {
    dequeue();
  }
  NVIC_EnableIRQ(I2C0_IRQn);
  
  return true;
}

bool i2s_read(const uint8_t *bytes, int len, i2c_callback cb) {
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
  active->direction = I2C_READ;

  if (!_active) {
    dequeue();
  }
  NVIC_EnableIRQ(I2C0_IRQn);
  
  return true;
}

void FlipOLED(void);
void invert(const void *data, int length) {
  uint8_t *px = (uint8_t*) FrameBuffer;
  length = FrameBufferLength;
  while(length--) *(px++) ^= 0xFF;
  FlipOLED();
}

void FlipOLED(void) {
  i2s_write(ResetCursor, sizeof(ResetCursor), invert);
}

void i2c_init()
{
  #ifdef SOFT_RESET
  // Toggle the reset bit
  PORT_HAL_SetMuxMode(PORTC_BASE, 10, kPortMuxAsGpio);
  GPIO_DRV_WritePinOutput(OLED_RST, 0);
  GPIO_DRV_SetPinDir(OLED_RST, kGpioDigitalOutput); 
  Anki::Cozmo::HAL::MicroWait(80);
  GPIO_DRV_WritePinOutput(OLED_RST, 1);
  #endif

  // Enable clocking on I2C, PortB, and DMA
  SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;
  SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK;
  SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;

  // Configure port mux for i2c
  PORTB_PCR2 = PORT_PCR_MUX(2); //I2C0_SDA
  PORTB_PCR3 = PORT_PCR_MUX(2); //I2C0_SCL
  
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
  i2s_write(InitDisplay, sizeof(InitDisplay), NULL);
  FlipOLED();
}

void DMA1_IRQHandler(void) {
  // Let the I2C controller know it should start interrupting
  DMA_CINT = 1;
  I2C0_C1 &= ~I2C_C1_DMAEN_MASK; 
}

void I2C0_IRQHandler(void) {
  // Clear interrupt
  I2C0_S |= I2C_S_IICIF_MASK;
  I2C0_C1 &= ~(I2C_C1_TX_MASK | I2C_C1_MST_MASK);

  // Dequeue last transmitted I2C packet
  I2C_Queue *active = &i2c_queue[_fifo_end++];
  if (_fifo_end >= MAX_QUEUE) { _fifo_end = 0; }

  if (active->cb) {
    active->cb(active->data, active->count);
  }
  
  if (--_fifo_count > 0) {
    dequeue();
  } else {
    _active = false;
  }
}
