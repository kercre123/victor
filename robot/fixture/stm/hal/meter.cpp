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
  const uint8_t INA220_VEXT_BUS_ADDR = 0x80; //A<1:0>=00 b1000000
  const uint8_t INA220_VBAT_BUS_ADDR = 0x88; //A<1:0>=10 b1000100
  const uint8_t INA220_CBAT_BUS_ADDR = 0x82; //A<1:0>=01 b1000001 [CUBEBAT]
  const uint8_t INA220_UAMP_BUS_ADDR = 0x8A; //A<1:0>=11 b1000101
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
  
  //static board_rev_t m_hw_rev = BOARD_REV_INVALID;
  void init()
  {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    //m_hw_rev = Board::revision();
    
    //Init bus pins
    SCL::init(MODE_OUTPUT, PULL_UP, TYPE_OPENDRAIN, SPEED_LOW);
    SDA::init(MODE_OUTPUT, PULL_UP, TYPE_OPENDRAIN, SPEED_LOW);
    
    // Let the lines float high
    SDA::set();
    SCL::set();
    
    /*/INA220 Configuration Register
    uint16_t cfg_reg = 0
        | (0x7 << 0)   //Mode: shunt and bus continuous measurement
        | (0x1 << 3)   //Shunt ADC resolution 10-bit / 148us sample time
        | (0x0 << 7)   //Bus ADC resolution 9-bit / 84us sample time
        | (0x0 << 11)  //Programmable Gain /1
        | (0x0 << 13)  //Bus Range 16V
        ;
    //-*/
    
    //init VEXT monitor IC
    I2C_Send16(INA220_VEXT_BUS_ADDR, 0, 7 + (1<<3)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,+/-40mV -> 2A @ 0.02R full-scale)
    I2C_Send16(INA220_VEXT_BUS_ADDR, 5, 0);          // No calibration - we can do our own math
    
    //init VBAT monitor IC
    I2C_Send16(INA220_VBAT_BUS_ADDR, 0, 7 + (1<<3)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,+/-40mV -> 2A @ 0.02R full-scale)
    I2C_Send16(INA220_VBAT_BUS_ADDR, 5, 0);          // No calibration - we can do our own math
    
    //init CUBEBAT monitor IC
    //I2C_Send16(INA220_CBAT_BUS_ADDR, 0, 7 + (1<<3) + (3<<11)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,GAIN/8,+/-320mV -> 106.666mA @ 3R full-scale)
    I2C_Send16(INA220_CBAT_BUS_ADDR, 0, 7 + (1<<3) + (1<<11)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,GAIN/2,+/-80mV -> 156.863mA @ 0.51R full-scale)
    I2C_Send16(INA220_CBAT_BUS_ADDR, 5, 0);          // No calibration - we can do our own math
    
    //init UAMP monitor IC
    I2C_Send16(INA220_UAMP_BUS_ADDR, 0, 7 + (1<<3)); // Measure voltage(9-bit,84us) + current(10-bit,148uS,+/-40mV -> 4mA @ 10R full-scale)
    I2C_Send16(INA220_UAMP_BUS_ADDR, 5, 0);          // No calibration - we can do our own math
  }
  
  //ADC sample timing, based on IC config
  static const int sample_time_voltage_us = 84;  //@ 9-bit resolution
  static const int sample_time_current_us = 148; //@ 10-bit resolution
  static const int sample_time_us = sample_time_voltage_us + sample_time_current_us; //V+I sampling active (Mode 7)
  
  //scale raw shunt voltage register value to mA (varies per IC configuration)
  static inline int32_t iscale_ma_(int32_t shunt_reg, uint8_t address)
  {
    switch( address ) {
      case INA220_VEXT_BUS_ADDR: return shunt_reg >> 1;   //R=0.02: FS 40mV  = reg val 4000 -> 2000mA
      case INA220_VBAT_BUS_ADDR: return shunt_reg >> 1;   //R=0.02: FS 40mV  = reg val 4000 -> 2000mA
      //case INA220_CBAT_BUS_ADDR: return shunt_reg / 300;  //R=3.0 : FS 320mV = reg val 32000 -> 106.666mA  
      case INA220_CBAT_BUS_ADDR: return shunt_reg / 51;   //R=0.51 : FS 80mV  = reg val 8000 -> 156.863mA
      //case INA220_UAMP_BUS_ADDR: return shunt_reg / 1000; //R=10  : FS 40mV  = reg val 4000 -> 4mA
      case INA220_UAMP_BUS_ADDR: return shunt_reg;        //R=10  : FS 40mV  = reg val 4000 -> 4000uA -- PSYCH! CONVERT THIS ONE TO UA
    }
    return 0;
  }
  
  static int32_t getVoltageMv_(int oversample, uint8_t address)
  {
    int32_t v = 0;
    for(int x=0; x < (1 << oversample); x++)
    {
      Timer::wait( sample_time_us + 10 ); //wait for next valid sample
      
      I2C_Send8(address, 2); //read bus voltage register
      int16_t value = I2C_Receive16(address);
      value >>= 1; //scale to mV
      
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
      value = iscale_ma_(value, address); //convert to mA
      
      v += value;
    }
    return v >> oversample; //average
  }
  
  int32_t getVoltageMv(pwr_e net, int oversample)
  {
    switch( net )
    {
      case PWR_VEXT:    return getVoltageMv_(oversample, INA220_VEXT_BUS_ADDR);
      case PWR_VBAT:    return getVoltageMv_(oversample, INA220_VBAT_BUS_ADDR);
      case PWR_CUBEBAT: return getVoltageMv_(oversample, INA220_CBAT_BUS_ADDR);
      case PWR_UAMP:    return getVoltageMv_(oversample, INA220_UAMP_BUS_ADDR);
      case PWR_DUTVDD:  return Board::getAdcMv(ADC_DUT_VDD_IN, oversample) << 1; //this input has a resistor divider
      case PWR_DUTPROG:
        break;
    }
    return 0;
  }
  
  int32_t getCurrentMa(pwr_e net, int oversample)
  {
    switch( net )
    {
      case PWR_VEXT:    return getCurrentMa_(oversample, INA220_VEXT_BUS_ADDR);
      case PWR_VBAT:    return getCurrentMa_(oversample, INA220_VBAT_BUS_ADDR);
      case PWR_CUBEBAT: return getCurrentMa_(oversample, INA220_CBAT_BUS_ADDR);
      case PWR_UAMP:    return getCurrentMa_(oversample, INA220_UAMP_BUS_ADDR) / 1000; //UAMP scales to uA
      case PWR_DUTPROG:
      case PWR_DUTVDD:
        break;
    }
    return 0;
  }
  
  int32_t getCurrentUa(pwr_e net, int oversample)
  {
    if( net == PWR_UAMP )
      return getCurrentMa_(oversample, INA220_UAMP_BUS_ADDR); //UAMP scales to uA
    return 0;
  }
  
  void setDoubleSpeed(bool enable) {
    CLOCK_WAIT = enable ? 0 : 1;
  }
  
}

