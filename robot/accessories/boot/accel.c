#include "hal.h"
#include "bma222.h"

// Edge-to-edge I2C delay is 1.25uS (20 cycles) total - this adds 10 for call/return
static void Delay()
{
}

// Issue a Start condition for I2C address with Read/Write bit
static void Start()
{
  Delay();
  DriveSDA(0);
  Delay();
  DriveSCL(0);
  Delay();
}

// Issue a Stop condition
static void Stop()
{
  DriveSDA(0);
  Delay();
  DriveSCL(1);
  Delay();
  DriveSDA(1);
  Delay();
}

// Benchmarked at 375KHz
static u8 Read(u8 ack)
{
  u8 b = 0, i;
  DriveSDA(1);
  for (i = 0; i < 8; i++)
  {
    b <<= 1;
    DriveSCL(1);
    Delay();
    b |= ReadSDA();
    DriveSCL(0);
    Delay();
  }
  // send Ack or Nak
  DriveSDA(ack);
  DriveSCL(1);
  Delay();
  DriveSCL(0);
  Delay();
  
  return b;
}

// Write byte blindly, ignoring ACK/NAK
// Benchmarked at 333KHz
static void Write(u8 b)
{
  u8 m;
  for (m = 0x80; m != 0; m >>= 1)
  {
    DriveSDA(!!(m & b));
    DriveSCL(1);
    Delay();
    DriveSCL(0);
    // No delay needed since the loop is slow enough
  }
  
  DriveSDA(1);
  DriveSCL(1); 
  // ReadSDA(); // If we cared about ack/nak
  Delay();
  DriveSCL(0);
  Delay();
}

static u8 DataRead(u8 addr)
{
  u8 readData = 0x00;
  Start(); //start condition
  Write((I2C_ADDR << 1) | I2C_WRITE_BIT); //slave address send
  Write(addr); //word address send
  Stop();
  Start();
  Write((I2C_ADDR << 1) | I2C_READ_BIT); //slave address send
  readData = Read(I2C_NACK); //nack for last read
  Stop();
  
  return readData;
}

static void DataWrite(u8 ctrlByte, u8 dataByte)
{
  Start(); //start condition     
  Write(I2C_ADDR << 1 | I2C_WRITE_BIT); //slave address send      
  Write(ctrlByte); //control byte send
  Write(dataByte); //write data send
  Stop();
}

// Initialize accelerometer, test it (return 1 if okay), then leave it asleep for now
bit AccelInit()
{
  if (IsCharger())
    return 1;   // Auto-pass self-test, DO NOT init P1.x (they're grounded on EP3)
  
  // Reset I2C, check chip ID
  I2CInit();
  DataRead(BGW_CHIPID);
  if (CHIPID != DataRead(BGW_CHIPID))
    return 0;   // Fail self-test
    
  // Put the chip to sleep - must perform a soft reset to use it again
  DataWrite(PMU_LPW, LPU_DEEP_SUSPEND);
  return 1;     // Pass self-test
}
