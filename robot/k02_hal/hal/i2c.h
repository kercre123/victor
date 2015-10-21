// i2c.h is meant to be used INSIDE the HAL only - not by external uses
// This gives multiplexed access to the I2C bus for HAL classes
#ifndef I2C_H
#define I2C_H

#include "anki/common/types.h"

static const int MAX_QUEUE = 16;

typedef void (*i2c_callback)(void *data, int count);

enum I2C_Mode {
  I2C_DIR_READ  = 1,
  I2C_DIR_WRITE = 2,
  I2C_SEND_STOP = 4,
  I2C_SEND_NACK = 8
};

#define I2CEnable() NVIC_EnableIRQ(I2C0_IRQn)
#define I2CDisable() NVIC_DisableIRQ(I2C0_IRQn);

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void I2CInit(void);
      bool I2CCmd(int mode, uint8_t *bytes, int len, i2c_callback cb);
      void I2CRestart(void);

      void I2CWriteReg(uint8_t slave, uint8_t addr, uint8_t data);
      uint8_t I2CReadReg(uint8_t slave, uint8_t addr);
      void I2CWriteAndVerify(uint8_t slave, uint8_t addr, uint8_t data);
    }
  }
}

#endif
