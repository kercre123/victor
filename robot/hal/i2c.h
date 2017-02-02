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

// Rectangle data for fifo
struct ScreenRect {
  // This is the ESP data
  uint8_t left;
  uint8_t right;
  uint8_t top;
  uint8_t bottom;

  // Internal state
  unsigned int sent;         // To OLED
  unsigned int received;     // From espressif
  unsigned int total;        // Rectangle size
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
        void Start(void);
        void Enable(void);
        bool GetWatermark(void);
        
        // This triggers an IMU read
        void ReadIMU(void);
        void FeedFace(bool rect, const uint8_t *face_bytes);
        void SetCameraExposure(uint16_t lines);
        void SetCameraGain(uint8_t gain);

        // These are used only during boot
        void WriteSync(const uint8_t *bytes, int len) ;
        void WriteReg(uint8_t slave, uint8_t addr, uint8_t data);
        uint8_t ReadReg(uint8_t slave, uint8_t addr);
      }
    }
  }
}

#endif
