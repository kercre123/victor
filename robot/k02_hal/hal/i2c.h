// i2c.h is meant to be used INSIDE the HAL only - not by external uses
// This gives multiplexed access to the I2C bus for HAL classes
#ifndef I2C_H
#define I2C_H

#include "anki/types.h"

static const int MAX_QUEUE = 16;

#define SLAVE_WRITE(x)  (x << 1)
#define SLAVE_READ(x)   ((x << 1) | 1)


typedef void (*i2c_callback)(const void *data, int count);

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace I2C
      {
        void Init(void);
        void Restart(void);
        void Enable(void);
        void Disable(void);
        void ForceStop(void);
        
        bool Write(uint8_t slave, const uint8_t *bytes, int len, i2c_callback cb) ;
        bool Read (uint8_t slave, uint8_t *bytes, int len, i2c_callback cb);

        void WriteReg(uint8_t slave, uint8_t addr, uint8_t data);
        uint8_t ReadReg(uint8_t slave, uint8_t addr);
        void WriteAndVerify(uint8_t slave, uint8_t addr, uint8_t data);
      }
    }
  }
}

#endif
