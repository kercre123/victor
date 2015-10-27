// i2c.h is meant to be used INSIDE the HAL only - not by external uses
// This gives multiplexed access to the I2C bus for HAL classes
#ifndef I2C_H
#define I2C_H

#include "anki/types.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Initialize I2C system
      void I2CInit();

      // Read a register - wait for completion
      // @param addr 7-bit I2C address
      // @param reg 8-bit register
      int I2CRead(u8 addr, u8 reg);

      // Write a register - wait for completion
      // @param addr 7-bit I2C address
      // @param reg 8-bit register
      // @param val 8-bit value
      void I2CWrite(u8 addr, u8 reg, u8 val);

      // Asynchronously run a list of I2C operations
      // You must call I2CWait before reading back the results
      // @param list Pointer to an array of I2COps (see below)
      void I2CRun(s16* list);

      // Wait for any outstanding I2CRun to complete
      void I2CWait();

      enum I2COps {
        EndOfList = -1,   // Marks the end of the list
        StartBit = -2,    // Causes a start bit to be sent
        StopBit = -3,     // Causes a stop bit to be sent
        ReadByteEnd = -4, // Causes a byte to be read into this array element with ACK = 1
        ReadByte = -5,    // Causes a byte to be read into this array element with ACK = 0
        // Any byte value from 0 to 255 is written as-is
      };
    }
  }
}

#endif
