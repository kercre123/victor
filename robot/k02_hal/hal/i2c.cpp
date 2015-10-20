// Placeholder bit-banging I2C implementation
// Vandiver:  Replace me with a nice DMA version that runs 4 transactions at a time off HALExec
// For HAL use only - see i2c.h for instructions, imu.cpp and camera.cpp for examples 
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "MK02F12810.h"

// Internal Settings
typedef struct {
  int flags;
  i2c_callback cb;
  
  void* data;
  void* buffer;
  int count;
  int size;
} I2C_Queue;

static int _fifo_start;
static int _fifo_end;
static int _fifo_count;

static I2C_Queue i2c_queue[MAX_QUEUE];

static bool _active;

static inline void start_transaction();

// HAL
namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // I2C calls
      bool I2CCmd(int mode, uint8_t *bytes, int len, i2c_callback cb) {
        // Queue is full.  Sorry 'bout it.
        if (_fifo_count >= MAX_QUEUE) {
          return false;
        }
        
        I2CDisable();

        I2C_Queue *active = &i2c_queue[_fifo_start++];
        if (_fifo_start >= MAX_QUEUE) { _fifo_start = 0; }
        _fifo_count++;
        
        active->cb = cb;
        active->size = active->count = len;
        active->buffer = active->data = bytes;
        active->flags = mode;

        if (!_active) {
          _active = true;
          start_transaction();
        }

        I2CEnable();
        
        return true;
      }

      void I2CInit()
      {
        // Clear our FIFO
        _fifo_start = 0;
        _fifo_end = 0;
        _fifo_count = 0;

        _active = false;
       
        // Enable clocking on I2C, PortB, PortE, and DMA
        SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;
        SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

        // Configure port mux for i2c
        PORTB_PCR1 = PORT_PCR_MUX(2);   //I2C0_SDA
        PORTE_PCR24 = PORT_PCR_MUX(5);  //I2C0_SCL

        // Configure i2c
        I2C0_F  = I2C_F_ICR(0x0E) | I2C_F_MULT(1);
        I2C0_C1 = I2C_C1_IICEN_MASK | I2C_C1_IICIE_MASK;
        
        // Enable IRQs
        NVIC_SetPriority(I2C0_IRQn, 0);
        I2CEnable();
      }
    }
  }
}

static inline uint8_t* next_byte() {
  I2C_Queue *active = &i2c_queue[_fifo_end];
  uint8_t *data = (uint8_t*)active->data;
  active->data = data + 1;
  active->count--;
  return data;
}

static inline void start_transaction() {
  using namespace Anki::Cozmo::HAL;
  
  I2C_Queue *active = &i2c_queue[_fifo_end];
  bool write = active->flags & I2C_DIR_WRITE;

  if (write) {
    I2C0_C1 = I2C0_C1 | I2C_C1_TX_MASK | I2C_C1_MST_MASK;
    I2C0_D = *next_byte();
  } else {
    I2C0_C1 = (I2C0_C1 & ~I2C_C1_TX_MASK) | I2C_C1_MST_MASK;
    MicroWait(1);
    int throwaway = I2C0_D;
  } 
}

extern "C"
void I2C0_IRQHandler(void) {
  using namespace Anki::Cozmo::HAL;
  
  // Clear interrupt
  I2C0_S |= I2C_S_IICIF_MASK;  
  I2C_Queue *active = &i2c_queue[_fifo_end];

  if (!_active) {
    return ;
  }
  
  // Continue down current chain
  if (active->count > 0) {
    bool write = active->flags & I2C_DIR_WRITE;

    if (write) {
      I2C0_D = *next_byte();
      return ;
    } else {
      // Prevent extra transactions
      if (active->count == 1) {
        I2C0_C1 |= I2C_C1_TX_MASK;
        MicroWait(1);
      }

      *(next_byte()) = I2C0_D;

      if (active->count) {
        return ;
      }
    } 
  }

  // Dequeue FIFO
  if (++_fifo_end >= MAX_QUEUE) { _fifo_end = 0; }

  // Tell HAL we completed
  if (active->cb) {
    active->cb(active->buffer, active->size);
  }

  // Send stop
  if (active->flags & I2C_SEND_STOP) {
    I2C0_C1 &= ~(I2C_C1_TX_MASK | I2C_C1_MST_MASK);
    MicroWait(1);
  }

  // Dequeue transaction when available
  if (--_fifo_count <= 0) {
    _active = false;
  } else {
    start_transaction();
  }
}
