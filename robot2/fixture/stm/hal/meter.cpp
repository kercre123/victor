#include "board.h"
#include "meter.h"
#include "timer.h"

#define METER_DEBUG 0
#if METER_DEBUG > 0
#include "console.h"
#endif

namespace Meter
{
  //I2C bus addresses for the current/voltage monitor ICs
  //(left shifted by 1 for the R/nW bit in the LSB)
  const uint8_t INA220_VEXT_BUS_ADDR = 0x80; //b10000000
  const uint8_t INA220_VBAT_BUS_ADDR = 0x88; //b10001000
  const int I2C_READ = 1; //RnW
  
  static int CLOCK_WAIT = 1;  // 1=250KHz (2uS per edge), 0=500KHz
  
  static void I2C_Pulse(void)
  {
    SCL::set();
    Timer::wait(CLOCK_WAIT);
    SCL::reset();
    Timer::wait(CLOCK_WAIT);
  }

  static void I2C_Start(void)
  {
    SDA::mode(MODE_OUTPUT);
    
    SDA::set();
    SCL::set();
    Timer::wait(CLOCK_WAIT);
    SDA::reset();
    Timer::wait(CLOCK_WAIT);
    SCL::reset();
    Timer::wait(CLOCK_WAIT);
  }

  static void I2C_Stop(void)
  {
    SDA::mode(MODE_OUTPUT);
    
    SDA::reset();
    Timer::wait(CLOCK_WAIT);
    SCL::set();
    Timer::wait(CLOCK_WAIT);
    SDA::set();
    Timer::wait(CLOCK_WAIT);
  }

  static void I2C_ACK(void)
  {
    SDA::mode(MODE_OUTPUT);
    
    SDA::reset();
    I2C_Pulse();
  }

  static void I2C_NACK(void)
  {
    SDA::mode(MODE_OUTPUT);
    
    SDA::set();
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
    
    SDA::mode(MODE_INPUT);
    
    for (int i = 0; i < 8; i++)
    {
      SCL::set();
      Timer::wait(CLOCK_WAIT);
      
      value <<= 1;
      value |= SDA::read();
      
      SCL::reset();
      Timer::wait(CLOCK_WAIT);
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
    I2C_Put8(address | I2C_READ);
    I2C_Pulse();  // Skip device ACK
    value = I2C_Get8() << 8;
    I2C_ACK();
    value |= I2C_Get8();
    I2C_NACK();
    I2C_Stop();
    
    return value;
  }
  
  void init()
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    //Init bus pins
    SCL::init(MODE_OUTPUT, PULL_UP, TYPE_OPENDRAIN, SPEED_LOW);
    SDA::init(MODE_OUTPUT, PULL_UP, TYPE_OPENDRAIN, SPEED_LOW);
    
    // Let the lines float high
    SDA::set();
    SCL::set();
    
    //init VEXT monitor IC
    I2C_Send16(INA220_VEXT_BUS_ADDR, 0, 7 + (1<<3)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,+/-40mV -> 2A @ 0.02R full-scale)
    I2C_Send16(INA220_VEXT_BUS_ADDR, 5, 0);          // No calibration - we can do our own math
    
    //init VBAT monitor IC
    I2C_Send16(INA220_VBAT_BUS_ADDR, 0, 7 + (1<<3)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,+/-40mV -> 2A @ 0.02R full-scale)
    I2C_Send16(INA220_VBAT_BUS_ADDR, 5, 0);          // No calibration - we can do our own math
  }
  
  //ADC sample timing, based on IC config
  static const int sample_time_voltage_us = 84;  //@ 9-bit resolution
  static const int sample_time_current_us = 148; //@ 10-bit resolution
  static const int sample_time_us = sample_time_voltage_us + sample_time_current_us; //V+I sampling active (Mode 7)
  
  static int32_t getVoltageMv_(int oversample, uint8_t address)
  {
    int32_t v = 0;
    for(int x=0; x < (1 << oversample); x++)
    {
      Timer::wait( sample_time_us + 10 ); //wait for next valid sample
      
      I2C_Send8(address, 2); //read bus voltage register
      int16_t value = I2C_Receive16(address);
      value /= 2; //convert to mV
      
      v += value;
    }
    return v >> oversample; //average
  }
  
  static int32_t getCurrentMa_(int oversample, uint8_t address)
  {
    int32_t v = 0;
    for(int x=0; x < (1 << oversample); x++)
    {
      Timer::wait( sample_time_us + 10 ); //wait for next valid sample
      
      I2C_Send8(address, 1); //read current shunt register
      int16_t value = I2C_Receive16(address);
      value /= 2; //4000 = 40mV->2000mA, so easy to convert to mA 
      
      v += value;
    }
    return v >> oversample; //average
  }
  
  int32_t getVextVoltageMv(int oversample) {
    return getVoltageMv_(oversample, INA220_VEXT_BUS_ADDR);
  }
  
  int32_t getVextCurrentMa(int oversample) {
    return getCurrentMa_(oversample, INA220_VEXT_BUS_ADDR);
  }
  
  int32_t getVbatVoltageMv(int oversample) {
    return getVoltageMv_(oversample, INA220_VBAT_BUS_ADDR);
  }
  
  #if METER_DEBUG > 0
  int32_t getVbatCurrentMa(int oversample) {
    uint32_t start = Timer::get();
    int i = getCurrentMa_(oversample, INA220_VBAT_BUS_ADDR);
    uint32_t time = Timer::get() - start;
    ConsolePrintf("getVbatCurrentMa(%u) in %uus\n", oversample, time);
    return i;
  }
  #else
  int32_t getVbatCurrentMa(int oversample) {
    return getCurrentMa_(oversample, INA220_VBAT_BUS_ADDR);
  }
  #endif
  
  void setDoubleSpeed(bool enable) {
    CLOCK_WAIT = enable ? 0 : 1;
  }
  
}

