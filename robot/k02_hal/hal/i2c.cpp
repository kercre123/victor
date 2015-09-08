// Placeholder bit-banging I2C implementation
// Vandiver:  Replace me with a nice DMA version that runs 4 transactions at a time off HALExec
// For HAL use only - see i2c.h for instructions, imu.cpp and camera.cpp for examples 
#include "board.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // I2C pins
      GPIO_PIN_SOURCE(SDA, PTB, 1);
      GPIO_PIN_SOURCE(SCL, PTE, 24);

      // Set up I2C hardware and stack
      void I2CInit(void)
      {
        // Set up pins
        GPIO_SET(GPIO_SCL, PIN_SCL);        
        GPIO_OUT(GPIO_SCL, PIN_SCL);
        SOURCE_SETUP(GPIO_SCL, SOURCE_SCL, SourceGPIO | SourceOpenDrain | SourcePullUp);

        GPIO_SET(GPIO_SDA, PIN_SDA);
        GPIO_OUT(GPIO_SDA, PIN_SDA);
        SOURCE_SETUP(GPIO_SDA, SOURCE_SDA, SourceGPIO | SourceOpenDrain | SourcePullUp);
      }

      // Soft I2C stack, borrowed from Arduino (BSD license)
      static void DriveSCL(u8 bit)
      {
        if (bit)
          GPIO_SET(GPIO_SCL, PIN_SCL);
        else
          GPIO_RESET(GPIO_SCL, PIN_SCL);

        MicroWait(2);
      }

      static void DriveSDA(u8 bit)
      {
        if (bit)
          GPIO_SET(GPIO_SDA, PIN_SDA);
        else
          GPIO_RESET(GPIO_SDA, PIN_SDA);

        MicroWait(2);
      }

      // Read SDA bit by allowing it to float for a while
      // Make sure to start reading the bit before the clock edge that needs it
      static u8 ReadSDA(void)
      {
        GPIO_SET(GPIO_SDA, PIN_SDA);
        MicroWait(2);
        return !!(GPIO_READ(GPIO_SDA) & PIN_SDA);
      }

      static u8 Read(u8 last)
      {
        u8 b = 0, i;

        for (i = 0; i < 8; i++)
        {
          b <<= 1;
          ReadSDA();
          DriveSCL(1);
          b |= ReadSDA();
          DriveSCL(0);
        }

        // send Ack or Nak
        if (last)
          DriveSDA(1);
        else
          DriveSDA(0);

        DriveSCL(1);
        DriveSCL(0);
        DriveSDA(1);

        return b;
      }

      // Issue a Stop condition
      static void Stop(void)
      {
        DriveSDA(0);
        DriveSCL(1);
        DriveSDA(1);
      }

      // Write byte and return true for Ack or false for Nak
      static u8 Write(u8 b)
      {
        u8 m;
        // Write byte
        for (m = 0x80; m != 0; m >>= 1)
        {
          DriveSDA(m & b);

          DriveSCL(1);
          //if (m == 1)
          //  ReadSDA();  // Let SDA fall prior to last bit
          DriveSCL(0);
        }

        DriveSCL(1);
        b = ReadSDA();
        DriveSCL(0);

        return b;
      }

      // Issue a Start condition for I2C address with Read/Write bit
      static u8 Start(u8 addressRW)
      {
        DriveSDA(0);
        DriveSCL(0);

        return Write(addressRW);
      }

      // Read a register - wait for completion
      // @param addr 7-bit I2C address (not shifted left)
      // @param reg 8-bit register
      int I2CRead(u8 addr, u8 reg)
      {
        int val;
        Start(addr << 1);       // Base address is Write (for writing address)
        Write(reg);
        Stop();
        Start((addr << 1) + 1); // Base address + 1 is Read (for Reading address)
        val = Read(1);          // 1 for 'last Read'
        Stop();
        return val;
      }

      // Write a register - wait for completion
      // @param addr 7-bit I2C address (not shifted left)
      // @param reg 8-bit register
      void I2CWrite(u8 addr, u8 reg, u8 val)
      {
        Start(addr << 1);    // Base address is Write
        Write(reg);
        Write(val);
        Stop();
      }
    }
  }
}
