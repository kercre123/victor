#include <string.h>

#include "common.h"
#include "hardware.h"
#include "power.h"

#include "i2c.h"
#include "opto.h"
#include "messages.h"
#include "lights.h"
#include "flash.h"
#include "comms.h"

struct I2C_BulkWrite {
  uint8_t reg;
  uint8_t data;
};

static const I2C_Operation* I2C_BLOCKED = (I2C_Operation*) 0xDEADFACE;
extern const I2C_Operation I2C_LOOP;

// Current I2C state
static const I2C_Operation* volatile i2c_op = I2C_BLOCKED;
static uint8_t* i2c_target;
static volatile uint8_t total_bytes;
static bool address_sent;
typedef void (*callback)(void);

static void setupChannel(uint8_t* target) {
  // Move our pins around
  if (i2c_op->channel & 1) {
    SDA1::mode(MODE_INPUT);
    SDA2::mode(MODE_ALTERNATE);
  } else {
    SDA2::mode(MODE_INPUT);
    SDA1::mode(MODE_ALTERNATE);
  }

  if (i2c_op->channel & 2) {
    SCL1::mode(MODE_INPUT);
    SCL2::mode(MODE_ALTERNATE);
  } else {
    SCL2::mode(MODE_INPUT);
    SCL1::mode(MODE_ALTERNATE);
  }

  i2c_target = target;
  address_sent = false;
  total_bytes = 0;  // used to see if we aborted early on sync functions
}

static void kickOff() {
  // We have not selected our save address
  volatile uint8_t flush = I2C2->RXDR;
  I2C2->ICR = ~0; // Clear all flags
  
  switch (i2c_op->op) {
    case I2C_HOLD:
      i2c_op = I2C_BLOCKED;
      return ;
    case I2C_DONE:
      i2c_op = NULL;
      return ;
    case I2C_REG_READ:
      setupChannel((uint8_t*) i2c_op->data);

      // Enable NBYTE interrupt (Read specific)
      I2C2->CR1 |= I2C_CR1_TCIE;
      I2C2->CR2   = (1 << 16)
                  | I2C_CR2_START
                  | i2c_op->slave
                  ;
      break ;
    case I2C_REG_WRITE_VALUE:
      setupChannel((uint8_t*) &i2c_op->data);

      I2C2->CR1 &= ~I2C_CR1_TCIE;
      I2C2->CR2  = ((i2c_op->length + 1) << 16)
                 | I2C_CR2_START
                 | I2C_CR2_AUTOEND
                 | i2c_op->slave
                 ;
      break ;
    case I2C_REG_WRITE:
      setupChannel((uint8_t*) i2c_op->data);

      I2C2->CR1 &= ~I2C_CR1_TCIE;
      I2C2->CR2  = ((i2c_op->length + 1) << 16)
                 | I2C_CR2_START
                 | I2C_CR2_AUTOEND
                 | i2c_op->slave
                 ;
      break ;
  }
}

extern "C" void I2C2_IRQHandler(void) {
  // Operation finished (possibly aborted early)
  if (I2C2->ISR & I2C_ISR_STOPF) {
    i2c_op++;
    kickOff();
    return ;
  }

  // Register selection complete (read only)
  if (I2C2->ISR & I2C_ISR_TC) {
    I2C2->CR2   = (i2c_op->length << 16)
                | I2C_CR2_START
                | I2C_CR2_RD_WRN
                | I2C_CR2_AUTOEND
                | i2c_op->slave
                ;
    return ;
  }

  // Read register not empty
  if (I2C2->ISR & I2C_ISR_RXNE) {
    *(i2c_target++) = I2C2->RXDR;
    total_bytes++;
  }

  // Transmission register empty
  else if (I2C2->ISR & I2C_ISR_TXE) {
    if (address_sent) {
      total_bytes++;
      I2C2->TXDR = *(i2c_target++);
    } else {
      address_sent = true;
      I2C2->TXDR = i2c_op->reg;
    }
  }
}

bool I2C::multiOp(I2C_Op func, uint8_t channel, uint8_t slave, uint8_t reg, int size, void* data) {
  int max_retries = 15;
  do {
    // Welp, something went wrong, we should just give up
    if (max_retries-- == 0) {
      Opto::failure = (channel << 8) || slave;
      return true;
    }

    I2C_Operation opTable[] = {
      { func, channel, slave, reg, size, data },
      { I2C_HOLD }
    };
    i2c_op = opTable;
    kickOff();
    while (i2c_op != I2C_BLOCKED) __wfi();
  } while (total_bytes < size);

  return false;
}

bool I2C::writeReg(uint8_t channel, uint8_t slave, uint8_t reg, uint8_t data) {
  return multiOp(I2C_REG_WRITE, channel, slave, reg, sizeof(data), &data);
}

bool I2C::writeReg16(uint8_t channel, uint8_t slave, uint8_t reg, uint16_t data) {
  return multiOp(I2C_REG_WRITE, channel, slave, reg, sizeof(data), &data);
}

bool I2C::writeReg32(uint8_t channel, uint8_t slave, uint8_t reg, uint32_t data) {
  return multiOp(I2C_REG_WRITE, channel, slave, reg, sizeof(data), &data);
}

int I2C::readReg(uint8_t channel, uint8_t slave, uint8_t reg) {
  uint8_t data;
  if (multiOp(I2C_REG_READ, channel, slave, reg, sizeof(data), &data)) return -1;
  return data;
}

int I2C::readReg16(uint8_t channel, uint8_t slave, uint8_t reg) {
  uint16_t data;
  if (multiOp(I2C_REG_READ, channel, slave, reg, sizeof(data), &data)) return -1;
  return data;
}


void I2C::init(void) {
  // Disable the perf at boot
  I2C2->CR1 = 0;
  I2C2->TIMINGR = 0
                | (5 << 20)   // Data setup time
                | (15 << 16)  // Data hold time
                | (30 << 8)   // SCL high
                | (63)        // SCL low
                ;

  I2C2->CR1 = 0
            | I2C_CR1_ANFOFF
            | I2C_CR1_RXIE
            | I2C_CR1_STOPIE
            | I2C_CR1_TXIE
            ;

  // Configure I/O
  SCL1::alternate(1);
  SCL1::speed(SPEED_HIGH);
  SCL1::pull(PULL_NONE);

  SDA1::alternate(1);
  SDA1::speed(SPEED_HIGH);
  SDA1::pull(PULL_NONE);

  SCL2::alternate(1);
  SCL2::speed(SPEED_HIGH);
  SCL2::pull(PULL_NONE);

  SDA2::alternate(1);
  SDA2::speed(SPEED_HIGH);
  SDA2::pull(PULL_NONE);

  // Enable the I2C perfs
  I2C2->CR1 |= I2C_CR1_PE;

  NVIC_SetPriority(I2C2_IRQn, PRIORITY_I2C_TRANSMIT);
  NVIC_EnableIRQ(I2C2_IRQn);
}

void I2C::execute(const I2C_Operation* cmd) {
  // The I2C perf is busy
  if (i2c_op != NULL) {
    return ;
  }

  i2c_op = cmd;
  kickOff();
}

// These can only be called in the main thread (anti-watchdog)
void I2C::capture() {
  while (i2c_op != I2C_BLOCKED) {
    __disable_irq();
    if (i2c_op == NULL) {
      i2c_op = I2C_BLOCKED;
    }
    __enable_irq();
    __wfi();
  }
}

void I2C::release() {
  __disable_irq();
  i2c_op = NULL;
  __enable_irq();
}
