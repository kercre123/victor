// Driver for INA220AIDGST
// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#include "hal/monitor.h"
#include "hal/timers.h"
#include "lib/stm32f2xx.h"
#include "lib/stm32f2xx_rcc.h"
#include "hal/console.h"

// These addresses are shifted left by 1 for the R/nW bit in the LSB
#define CHARGE_CONTACT_ADDRESS  (0x80)  // 8'b10000000
#define BATTERY_ADDRESS         (0x88)  // 8'b10001000
#define SET_VBAT_ADDRESS        (0x5E)  // 7'b0101111. (see MCP4018T datasheeet)

#define READ                    1

static int CLOCK_WAIT = 1;  // 1=250KHz (2uS per edge), 0=500KHz

#define GPIOB_SCL               8
#define GPIOB_SDA               9

static void I2C_Pulse(void)
{  
  PIN_SET(GPIOB, GPIOB_SCL);
  MicroWait(CLOCK_WAIT);
  PIN_RESET(GPIOB, GPIOB_SCL);
  MicroWait(CLOCK_WAIT);
}

static void I2C_Start(void)
{
  PIN_OUT(GPIOB, GPIOB_SDA);
  
  PIN_SET(GPIOB, GPIOB_SDA);
  PIN_SET(GPIOB, GPIOB_SCL);
  MicroWait(CLOCK_WAIT);
  PIN_RESET(GPIOB, GPIOB_SDA);
  MicroWait(CLOCK_WAIT);
  PIN_RESET(GPIOB, GPIOB_SCL);
  MicroWait(CLOCK_WAIT);
}

static void I2C_Stop(void)
{
  PIN_OUT(GPIOB, GPIOB_SDA);
  
  PIN_RESET(GPIOB, GPIOB_SDA);
  MicroWait(CLOCK_WAIT);
  PIN_SET(GPIOB, GPIOB_SCL);
  MicroWait(CLOCK_WAIT);
  PIN_SET(GPIOB, GPIOB_SDA);
  MicroWait(CLOCK_WAIT);
}

static void I2C_ACK(void)
{
  PIN_OUT(GPIOB, GPIOB_SDA);
  
  PIN_RESET(GPIOB, GPIOB_SDA);
  I2C_Pulse();
}

static void I2C_NACK(void)
{
  PIN_OUT(GPIOB, GPIOB_SDA);
  
  PIN_SET(GPIOB, GPIOB_SDA);
  I2C_Pulse();
}

static void I2C_Put8(u8 data)
{
  for (int i = 7; i >= 0; i--)
  {
    if (data & (1 << i))
      I2C_NACK();
    else
      I2C_ACK();
  }
}

static u8 I2C_Get8(void)
{
  u8 value = 0;
  
  PIN_IN(GPIOB, GPIOB_SDA);
  
  for (int i = 0; i < 8; i++)
  {
    PIN_SET(GPIOB, GPIOB_SCL);
    MicroWait(CLOCK_WAIT);
    
    value <<= 1;
    value |= (GPIO_READ(GPIOB) >> GPIOB_SDA) & 1;
    
    PIN_RESET(GPIOB, GPIOB_SCL);
    MicroWait(CLOCK_WAIT);
  }
  
  return value;
}

static void I2C_Send8(u8 address, u8 data)
{
  I2C_Start();
  I2C_Put8(address);
  I2C_Pulse();  // Skip device ACK
  I2C_Put8(data);
  I2C_Pulse();  // Skip device ACK
  I2C_Stop();
}

static void I2C_Send16(u8 address, u8 reg, u16 data)
{
  I2C_Start();
  I2C_Put8(address);
  I2C_Pulse();  // Skip device ACK
  I2C_Put8(reg);
  I2C_Pulse();  // Skip device ACK
  I2C_Put8(data >> 8);
  I2C_Pulse();  // Skip device ACK
  I2C_Put8(data & 0xff);
  I2C_Pulse();  // Skip device ACK
  I2C_Stop();
}

static u16 I2C_Receive16(u8 address)
{
  u16 value = 0;
  
  I2C_Start();
  I2C_Put8(address | READ);
  I2C_Pulse();  // Skip device ACK
  value = I2C_Get8() << 8;
  I2C_ACK();
  value |= I2C_Get8();
  I2C_NACK();
  I2C_Stop();
  
  return value;
}

void InitMonitor(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
  
  // Setup PB8 and PB9 for I2C1
  // SCL
  GPIO_InitTypeDef   GPIO_InitStructure;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  // SDA
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_9;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  // Let the lines float high
  PIN_SET(GPIOB, GPIOB_SDA);
  PIN_SET(GPIOB, GPIOB_SCL);
  
  // Setup the charge contact calibration register
  // XXX: This is likely wrong for Cozmo testfix, but everything else is using it right now
  I2C_Send16(CHARGE_CONTACT_ADDRESS, 5, 0x75A5);  // Setup by TI's app... LSB = 20u
  
  // Setup the battery calibration registers
  I2C_Send16(BATTERY_ADDRESS, 0, 5 + (1<<3)); // Measure current only, 10-bit (150uS), +/- 40mV (2A @ 0.02 ohm) full-scale
  I2C_Send16(BATTERY_ADDRESS, 5, 0);          // No calibration - we can do our own math
}

s32 ChargerGetCurrent(void)
{
  static bool init = false;
  if (!init)
  {
    I2C_Send16(CHARGE_CONTACT_ADDRESS, 0, 5 + (1<<3)); // Measure current only, 10-bit (150uS), +/- 40mV (2A @ 0.02 ohm) full-scale
    I2C_Send16(CHARGE_CONTACT_ADDRESS, 5, 0);          // No calibration - we can do our own math
    init = true;
  }
  I2C_Send8(CHARGE_CONTACT_ADDRESS, 1);  // Read shunt register directly - 4000 = 40mV or 2A
  s16 value = I2C_Receive16(CHARGE_CONTACT_ADDRESS);
  return (s32)value/2;  // 4000 = 2000mA, so easy to convert to mA 
}

s32 BatGetCurrent(void)
{
  I2C_Send8(BATTERY_ADDRESS, 1);  // Read shunt register directly - 4000 = 40mV or 2A
  s16 value = I2C_Receive16(BATTERY_ADDRESS);
  return (s32)value/2;  // 4000 = 2000mA, so easy to convert to mA 
}

s32 MonitorGetCurrent(void)
{
  I2C_Send8(CHARGE_CONTACT_ADDRESS, 4);
  s16 value = I2C_Receive16(CHARGE_CONTACT_ADDRESS);
  return (s32)value * 20;
}

s32 MonitorGetVoltage(void)
{
  I2C_Send8(CHARGE_CONTACT_ADDRESS, 2);
  s16 value = I2C_Receive16(CHARGE_CONTACT_ADDRESS);
  return (s32)value;
}

void MonitorSetDoubleSpeed(void)
{
  CLOCK_WAIT = 0;
}
