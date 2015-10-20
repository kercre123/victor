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
  int count;
} I2C_Queue;

static int _fifo_start;
static int _fifo_end;
static int _fifo_count;

static I2C_Queue i2c_queue[MAX_QUEUE];

static bool _active;

extern "C"
void I2C0_IRQHandler(void);

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

        NVIC_DisableIRQ(I2C0_IRQn);
        I2C_Queue *active = &i2c_queue[_fifo_start++];
        if (_fifo_start >= MAX_QUEUE) { _fifo_start = 0; }
        _fifo_count++;
        
        active->cb = cb;
        active->count = len;
        active->data = bytes;
        active->flags = mode;

        if (!_active) {
          _active = true;
          I2C0_IRQHandler();
        }

        NVIC_EnableIRQ(I2C0_IRQn);
        
        return true;
      }

      void I2CRestart(void) {
        // THIS WILL FLAG LOOP FOR SAFE TO RUN
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
        I2C0_F  = I2C_F_ICR(0x0D) | I2C_F_MULT(1);
        I2C0_C1 = I2C_C1_IICEN_MASK | I2C_C1_IICIE_MASK;
        
        // Enable IRQs
        NVIC_SetPriority(I2C0_IRQn, 3);
        NVIC_EnableIRQ(I2C0_IRQn);
      }
    }
  }
}

static inline void set_read(bool read) {
  using namespace Anki::Cozmo::HAL;

  if (read) {
    I2C0_C1 = (I2C0_C1 & ~I2C_C1_TX_MASK) | I2C_C1_MST_MASK;
  }
  
  else  {
    I2C0_C1 = I2C0_C1 | I2C_C1_TX_MASK | I2C_C1_MST_MASK;
    MicroWait(1);
  } 
}

static inline void do_transaction(bool dequeue) {
  using namespace Anki::Cozmo::HAL;

  I2C0_C1 |= I2C_C1_MST_MASK;

  I2C_Queue *active = &i2c_queue[_fifo_end];
  uint8_t *data = (uint8_t*)active->data;

  active->data = data+1;
  if (dequeue) {
    active->data
  }
  if (dequeue) active->count--;

  if (active->flags & I2C_DIR_WRITE) {
    I2C0_C1 |= I2C_C1_TX_MASK | I2C_C1_MST_MASK ;
    MicroWait(1);
    I2C0_D = *data;
  } else {
    
    *next_byte(dequeue) = I2C0_D;
  }
  _active = true;
}

extern "C"
void I2C0_IRQHandler(void) {
  using namespace Anki::Cozmo::HAL;
  
  // Clear interrupt
  I2C0_S |= I2C_S_IICIF_MASK;  
  I2C_Queue *active = &i2c_queue[_fifo_end];

  if (!_active) { return ; }
  
  // Continue down current chain
  if (active->count > 0) {
    do_transaction(true);
    return ;
  }

  // Dequeue FIFO
  if (++_fifo_end >= MAX_QUEUE) { _fifo_end = 0; }

  // Tell HAL we completed
  if (active->cb) {
    active->cb(active->data);
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
    do_transaction(false);
  }
}
