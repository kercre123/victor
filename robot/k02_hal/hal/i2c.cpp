// Placeholder bit-banging I2C implementation
// Vandiver:  Replace me with a nice DMA version that runs 4 transactions at a time off HALExec
// For HAL use only - see i2c.h for instructions, imu.cpp and camera.cpp for examples 
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "MK02F12810.h"
#include "hal/hardware.h"

// Internal Settings
typedef struct {  
  uint8_t slave_address;

  uint8_t flags;
  
  const void* buffer;
  uint8_t* data;
  int count;
  int size;
  
  i2c_callback callback;
} I2C_Queue;

static const uint8_t UNUSED_SLAVE = 0xFF;

static I2C_Queue i2c_queue[MAX_QUEUE];
static int _fifo_start;
static int _fifo_end;
static int _fifo_count;
static bool enabled = false;

static bool _active = false;
static bool _reselected = false;

static inline void start_transaction();

extern "C" void I2C0_IRQHandler(void);

// Send a stop condition first thing to make sure perfs are not holding the bus
static inline void SendEmergencyStop(void) {
  using namespace Anki::Cozmo::HAL;

  GPIO_SET(GPIO_I2C_SCL, PIN_I2C_SCL);

  // Drive PWDN and RESET to safe defaults
  GPIO_OUT(GPIO_I2C_SCL, PIN_I2C_SCL);
  SOURCE_SETUP(GPIO_I2C_SCL, SOURCE_I2C_SCL, SourceGPIO);

  // GPIO, High drive, open drain
  GPIO_IN(GPIO_I2C_SDA, PIN_I2C_SDA);
  SOURCE_SETUP(GPIO_I2C_SDA, PIN_I2C_SDA, SourceGPIO);
  
  // Clock the output until the data line goes high
  for (int i = 0; i < 100 || !GPIO_READ(GPIO_I2C_SDA); i++) {
    GPIO_RESET(GPIO_I2C_SCL, PIN_I2C_SCL);
    MicroWait(1);
    GPIO_SET(GPIO_I2C_SCL, PIN_I2C_SCL);
    MicroWait(1);
  }
}

void Anki::Cozmo::HAL::I2C::Init()
{
  SendEmergencyStop();
  
  // Clear our FIFO
  _fifo_start = 0;
  _fifo_end = 0;
  _fifo_count = 0;

  _active = false;
 
  // Enable clocking on I2C, PortB, PortE, and DMA
  SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

  // Configure port mux for i2c
  PORTB_PCR1 = PORT_PCR_MUX(2) |
               PORT_PCR_ODE_MASK |
               PORT_PCR_DSE_MASK |
               PORT_PCR_PE_MASK |
               PORT_PCR_PS_MASK;   //I2C0_SDA
  PORTE_PCR24 = PORT_PCR_MUX(5) |
                PORT_PCR_DSE_MASK;  //I2C0_SCL

  // Configure i2c
  I2C0_F  = I2C_F_ICR(0x1A);
  I2C0_C1 = I2C_C1_IICEN_MASK | I2C_C1_IICIE_MASK | I2C_C1_MST_MASK;
  
  // Enable IRQs
  NVIC_SetPriority(I2C0_IRQn, 0);
  Enable();
}

void Anki::Cozmo::HAL::I2C::Enable(void) {
  enabled = true;
  NVIC_EnableIRQ(I2C0_IRQn);
}

void Anki::Cozmo::HAL::I2C::Disable(void) {
  enabled = false;
  NVIC_DisableIRQ(I2C0_IRQn);
}

// Register calls
static volatile bool i2c_data_active;
static void i2cRegCallback(const void* data, int size) {
  i2c_data_active = false;
}

void Anki::Cozmo::HAL::I2C::WriteReg(uint8_t slave, uint8_t addr, uint8_t data) {
  uint8_t cmd[2] = { addr, data };

  i2c_data_active = true;

  Write(SLAVE_WRITE(slave), cmd, sizeof(cmd), i2cRegCallback, I2C_FORCE_START);

  while(i2c_data_active) __asm { WFI } ;
}

uint8_t Anki::Cozmo::HAL::I2C::ReadReg(uint8_t slave, uint8_t addr) {
  uint8_t resp;

  i2c_data_active = true;
  
  Write(SLAVE_WRITE(slave), &addr, sizeof(addr), NULL, I2C_FORCE_START);
  Read(SLAVE_READ(slave), &resp, sizeof(resp), i2cRegCallback);
  
  while(i2c_data_active) ;
  
  return resp;
}

void Anki::Cozmo::HAL::I2C::WriteAndVerify(uint8_t slave, uint8_t addr, uint8_t data) {
  uint8_t resp;
  WriteReg(slave, addr, data);
  resp = ReadReg(slave, addr);
  
  while (resp != data) ;
}

// This should only be used during 
void Anki::Cozmo::HAL::I2C::ForceStop(void) {
  I2C0_C1 &= ~I2C_C1_MST_MASK;
  MicroWait(1);
}

static bool Enqueue(uint8_t slave, const uint8_t *bytes, int len, i2c_callback cb, uint8_t flags) {
  using namespace Anki::Cozmo::HAL::I2C;
  
  if (_fifo_count >= MAX_QUEUE) {
    return false;
  }

  bool unsafe = enabled;
  if (unsafe) Disable();
  I2C_Queue *active = &i2c_queue[_fifo_start];
  if (++_fifo_start >= MAX_QUEUE) { _fifo_start = 0; }
  
  active->callback = cb;
  active->size = active->count = len;
  active->buffer = active->data = (uint8_t*) bytes;
  active->slave_address = slave;
  active->flags = flags;

  _fifo_count++;

  if (!_active) {
    start_transaction();
  }
  if (unsafe) Enable();
  
  return true;
}

bool Anki::Cozmo::HAL::I2C::Write(uint8_t slave, const uint8_t *bytes, int len, i2c_callback cb, uint8_t flags) {
  return Enqueue(slave, bytes, len, cb, flags);
}

bool Anki::Cozmo::HAL::I2C::Read(uint8_t slave, uint8_t *bytes, int len, i2c_callback cb, uint8_t flags) {
  return Enqueue(slave, bytes, len, cb, flags | I2C_READ);
}


static inline uint8_t* next_byte() {
  I2C_Queue *active = &i2c_queue[_fifo_end];
  uint8_t *data = (uint8_t*)(active->data++);
  active->count--;
  return data;
}

static inline I2C_Queue *getActive() {
  return  &i2c_queue[_fifo_end];
}

static inline void send_nack(bool nack) {
  if (nack) {
    I2C0_C1 |= I2C_C1_TXAK_MASK;
  } else {
    I2C0_C1 &= ~I2C_C1_TXAK_MASK;
  }
}

static void next_transaction() {
  if (++_fifo_end >= MAX_QUEUE) { _fifo_end = 0; }
  _fifo_count--;
}

static void start_transaction() {
  using namespace Anki::Cozmo::HAL;
  static uint8_t _active_slave = UNUSED_SLAVE;

  // Find a non-optional transaction
  I2C_Queue *active;
  
  for(;;) {
    if (_fifo_count <= 0) {
      _active = false;
      return ;
    }
    
    active = getActive();

    if (~active->flags & I2C_OPTIONAL || active->slave_address != _active_slave) {
      active->flags &= ~I2C_OPTIONAL;
      break ;
    }
    
    next_transaction();
  }

  _active = true;

  // Send a stop and transmit the slave address
  if (active->slave_address != _active_slave || active->flags & I2C_FORCE_START || ~I2C0_C1 & I2C_C1_MST_MASK) {
    if (I2C0_C1 & I2C_C1_MST_MASK) {
      I2C0_C1 |= I2C_C1_RSTA_MASK | I2C_C1_TX_MASK;
    } else {
      I2C0_C1 |= I2C_C1_MST_MASK | I2C_C1_TX_MASK;
      MicroWait(1);
    }
    
    I2C0_D = active->slave_address;

    active->flags &= ~I2C_FORCE_START;
    _active_slave = active->slave_address;
    _reselected = true;
    return ;
  }

  _reselected = false;

  
  if (active->flags & I2C_READ) {
    I2C0_C1 &= ~I2C_C1_TX_MASK;
    send_nack(active->count == 1);
    uint8_t throwaway = I2C0_D;
  } else {
    I2C0_C1 |= I2C_C1_TX_MASK;
    I2C0_D = *next_byte();
  }
}

static inline bool send_byte(void) {
  using namespace Anki::Cozmo::HAL;
  
  I2C_Queue *active = getActive();

  if (active->count == 2) {
    send_nack(true);
  } else if (active->count == 1) {
    I2C0_C1 |= I2C_C1_TX_MASK;
    *next_byte() = I2C0_D;
    return false;
  }
  
  *next_byte() = I2C0_D;
  return true;
}

extern "C"
void I2C0_IRQHandler(void) {
  using namespace Anki::Cozmo::HAL;
  
  // Clear interrupt
  I2C0_S |= I2C_S_IICIF_MASK;

  I2C_Queue *active = getActive();
  
  // We need to start the transaction again because the slave address did not match
  if (_reselected) {
    start_transaction();
    return ;
  }
  
  // Handle next queued byte
  if (active->flags & I2C_READ) {
    if (send_byte()) return ;
  } else if (active->count) {
    I2C0_D = *next_byte();
    return ;
  }
  
  if (active->callback) {
    active->callback(active->buffer, active->size);
  }

  // Packet completed, continue on our merry way.
  next_transaction();
  start_transaction();
}
