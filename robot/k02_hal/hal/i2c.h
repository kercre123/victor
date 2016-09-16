// i2c.h is meant to be used INSIDE the HAL only - not by external uses
// This gives multiplexed access to the I2C bus for HAL classes
#ifndef I2C_H
#define I2C_H

#include "anki/types.h"

#define SLAVE_WRITE(x)  (x << 1)
#define SLAVE_READ(x)   ((x << 1) | 1)


typedef void (*i2c_callback)(const void*);

enum I2C_FLAGS {
  I2C_NONE = 0,
  I2C_OPTIONAL = 1,
  I2C_FORCE_START = 2,
};

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace I2C
      {
        void Init(void);
        void Enable(void);
        void Disable(void);
        void ForceStop(void);
        void FullStop(void);
        void Flush(void);
          
        void SetupRead(void* target, int size);
        void Write(uint8_t slave, const uint8_t *bytes, int len, uint8_t flags = I2C_NONE) ;
        void Read (uint8_t slave, uint8_t flags = I2C_NONE) ;

        void WriteReg(uint8_t slave, uint8_t addr, uint8_t data);
        uint8_t ReadReg(uint8_t slave, uint8_t addr);
      }
    }
  }
}

#endif
