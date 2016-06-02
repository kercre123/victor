#include "hal.h"
#include "bma222.h"

// Bit-bang SPI read
static u8 Read()
{ 
  u8 b, i;
  for (i = 0; i < 8; i++)
  {
    SCK = 0;            // Change on falling edge
    b <<= 1;
    SCK = 1;            // Sample on rising edge
    b |= SDI;
  }
  return b;
}

// Bit-bang SPI write
static void Write(u8 b)
{
  u8 m;
  for (m = 0x80; m != 0; m >>= 1)
  {
    SCK = 0;            // Change on falling edge
    SDI = !!(m & b);
    SCK = 1;            // Sample on rising edge
  }
}

// Read one byte reg
static u8 DataRead(u8 addr)
{
  u8 val;
  
  CSB = 0;
  Write(SPI_READ | addr);  
  val = Read();
  CSB = 1;
  
  return val;
}

// Write one byte reg
static void DataWrite(u8 addr, u8 dataByte)
{
  CSB = 0;
  Write(addr);  // Write is default
  Write(dataByte);
  CSB = 1;
}

// Initialize accelerometer, test it (return 1 if okay), then leave it asleep for now
bit AccelInit()
{
  if (IsCharger())
    return 1;
  
  // Set up SPI, check chip ID
  SPIInit();
  DataWrite(BGW_SPI3_WDT, 1);   // 3 wire mode
  if (CHIPID != DataRead(BGW_CHIPID))
    return 0;   // Fail self-test
    
  // Put the chip to sleep - must perform a soft reset to use it again
  DataWrite(PMU_LPW, LPU_DEEP_SUSPEND);
  return 1;     // Pass self-test
}
