#include "hal.h"
#include "hal_delay.h"

//GPIO_PIN_SOURCE(SDA, P0, 0)
//GPIO_PIN_SOURCE(SCL, P0, 1)

#define SOURCE_SDA      0
#define PIN_SDA         (1 << SOURCE_SDA)
#define GPIO_SDA        P0

#define SOURCE_SCL      1
#define PIN_SCL         (1 << SOURCE_SCL)
#define GPIO_SCL        P0

// I2C addresses
#define I2C_ADDR      0x18 // 7-bit slave address
#define I2C_ADDR_ALT  0x19 // 7-bit slave address

// IMU Chip IDs
#define CHIPID          0xF8

// Accelerometer Register Map
#define BGW_CHIPID      0x00
#define ACCD_X_LSB      0x02
#define ACCD_X_MSB      0x03
#define ACCD_Y_LSB      0x04
#define ACCD_Y_MSB      0x05
#define ACCD_Z_LSB      0x06
#define ACCD_Z_MSB      0x07
#define ACCD_TEMP       0x08
#define INT_STATUS_0    0x09
#define INT_STATUS_1    0x0A
#define INT_STATUS_2    0x0B
#define INT_STATUS_3    0x0C
//#define FIFO_STATUS     0x0E
#define PMU_RANGE       0x0F
#define PMU_BW          0x10
#define PMU_LPW         0x11
#define PMU_LOW_POWER   0x12
#define ACCD_HBW        0x13
#define BGW_SOFTRESET   0x14
#define INT_EN_0        0x16
#define INT_EN_1        0x17
#define INT_EN_2        0x18
#define INT_MAP_0       0x19
#define INT_MAP_1       0x1A
#define INT_MAP_2       0x1B
#define INT_SRC         0x1E
#define INT_OUT_CTRL    0x20
#define INT_RST_LATCH   0x21
#define INT_0           0x22
#define INT_1           0x23
#define INT_2           0x24
#define INT_3           0x25
#define INT_4           0x26
#define INT_5           0x27
#define INT_6           0x28
#define INT_7           0x29
#define INT_8           0x2A
#define INT_9           0x2B
#define INT_A           0x2C
#define INT_B           0x2D
#define INT_C           0x2E
#define INT_D           0x2F
#define FIFO_CONFIG_0   0x30
#define PMU_SELF_TEST   0x32
#define TRIM_NVM_CTRL   0x33
#define BGW_SPI3_WDT    0x34
#define OFC_CTRL        0x36
#define OFC_SETTING     0x37
#define OFC_OFFSET_X    0x38
#define OFC_OFFSET_Y    0x39
#define OFC_OFFSET_Z    0x3A
#define TRIM_GP0        0x3B
#define TRIM_GP1        0x3C
#define FIFO_CONFIG_1   0x3E
#define FIFO_DATA       0x3F


// Accelerometer Register values  // XXX check these values
#define RANGE_2G            0x03
#define RANGE_4G            0x05
#define RANGE_8G            0x08
#define RANGE_16G           0x0B
#define BW_250              0x0D

#define ACC_INT_OPEN_DRAIN  0x0F  // Active high, open drain

const u8 I2C_ACK  = 0;
const u8 I2C_NACK  = 1;
const u8 I2C_READ_BIT = 1;    
const u8 I2C_WRITE_BIT = 0;

const u8 I2C_WAIT = 1;    


static void DriveSCL(u8 b)
{
  if(b)
    GPIO_SET(GPIO_SCL, PIN_SCL);
  else
    GPIO_RESET(GPIO_SCL, PIN_SCL);
  delay_us(I2C_WAIT);
}


static void DriveSDA(u8 b)
{
  if(b)
    GPIO_SET(GPIO_SDA, PIN_SDA);
  else
    GPIO_RESET(GPIO_SDA, PIN_SDA);
  delay_us(I2C_WAIT);
}

// Read SDA bit
static u8 ReadSDA()
{ 
  return !!(GPIO_READ(GPIO_SDA) & PIN_SDA);
}


// Issue a Start condition for I2C address with Read/Write bit
static void Start()
{
  DriveSDA(0);
  DriveSCL(0);
}

// Issue a Stop condition
static void Stop()
{
  DriveSDA(0);
  DriveSCL(1);
  DriveSDA(1);
}

static u8 Read(u8 ack)
{
  u8 b = 0, i;
  PIN_IN(P0DIR, PIN_SDA);
  for (i = 0; i < 8; i++)
  {
    b <<= 1;
    DriveSCL(1);
    b |= ReadSDA();
    DriveSCL(0);
  }
  PIN_OUT(P0DIR, PIN_SDA);
  // send Ack or Nak
  DriveSDA(ack);
  DriveSCL(1);
  DriveSCL(0);

  return b;
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
    DriveSCL(0);
  }
  
  DriveSDA(0);
  PIN_IN(P0DIR, PIN_SDA);
  DriveSCL(1); 
  b = ReadSDA();
  PIN_OUT(P0DIR, PIN_SDA);
  DriveSCL(0);
  
  return b;
}



static void VerifyAck(ack)
{
  if (ack == I2C_NACK)
  {
    LightOn(0); // ERROR (RED)
    while(1);
  }
  return;
}

static u8 DataRead(u8 addr)
{
  u8 readData = 0x00;
  Start(); //start condition
  VerifyAck(Write(I2C_ADDR << 1 | I2C_WRITE_BIT)); //slave address send
  VerifyAck(Write(addr)); //word address send
  Stop();
  Start();
  VerifyAck(Write(I2C_ADDR << 1 | I2C_READ_BIT)); //slave address send
  readData = Read(I2C_NACK); //nack for last read
  Stop();
  
  return readData;
 } //End of DataRead function

 
static void DataReadMultiple(u8 addr, u8 numBytes, u8* buffer)
{
  u8 i=0;
  Start(); //start condition
  VerifyAck(Write(I2C_ADDR << 1 | I2C_WRITE_BIT)); //slave address send
  VerifyAck(Write(addr)); //word address send
  Stop();
  Start();
  VerifyAck(Write(I2C_ADDR << 1 | I2C_READ_BIT)); //slave address send
  while(i < numBytes)
  {
    if (i == (numBytes-1))
    {
      buffer[i] = Read(I2C_NACK); //nack for last read
    }
    else
    {
      buffer[i] = Read(I2C_ACK); 
    }
    i++;
  }
  Stop();

} //End of DataRead function
 

static void DataWrite(u8 ctrlByte, u8 dataByte)
{
  Start(); //start condition     
  VerifyAck(Write(I2C_ADDR << 1 | I2C_WRITE_BIT)); //slave address send      
  VerifyAck(Write(ctrlByte)); //control byte send
  VerifyAck(Write(dataByte)); //write data send
  Stop();
  return;
}


// Initialize I2C
static void InitI2C()
{ 
  // SDA output, normal drive strength
  P0CON = (0x000 << 5) | (0 << 4) | (0 << 3) | (SOURCE_SDA << 0);
  // SDA input, pull up
//  P0CON = (0x010 << 5) | (1 << 4) | (0 << 3) | (SOURCE_SDA << 0);
  // SCL output, normal drive strength
  P0CON = (0x000 << 5) | (0 << 4) | (0 << 3) | (SOURCE_SCL << 0);

  GPIO_SET(GPIO_SCL, PIN_SCL);
  GPIO_SET(GPIO_SDA, PIN_SDA);
  
  PIN_OUT(P0DIR, PIN_SCL);
  PIN_OUT(P0DIR, PIN_SDA);
 
}

static void WriteVerify(u8 ctrlByte, u8 dataByte)
{
  DataWrite(ctrlByte, dataByte);
  if(DataRead(ctrlByte) != dataByte)
  {
    LightOn(0); // ERROR (RED)
    while(1);
  }
  return;
}

// Initialize accelerometer
void InitAcc()
{
  delay_ms(5);
  InitI2C();
  delay_ms(5);

  if(DataRead(BGW_CHIPID) != CHIPID)
  {
    LightOn(0); // ERROR (RED)
  }
  delay_ms(1);

  WriteVerify(PMU_RANGE, RANGE_2G);
  delay_ms(1);
  
  WriteVerify(PMU_BW, BW_250);
  delay_ms(1);
}

void ReadAcc(s8 *accData)
{
  static u8 buffer[6];
  DataReadMultiple(ACCD_X_LSB, 6, buffer);
  accData[0] = (s8)buffer[1];
  accData[1] = (s8)buffer[3];
  accData[2] = (s8)buffer[5];
}