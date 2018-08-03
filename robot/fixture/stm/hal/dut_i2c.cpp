#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "dut_i2c.h"
#include "timer.h"

#define DUT_I2C_DEBUG  0

#if DUT_I2C_DEBUG > 0
#warning DUT_I2C debug mode
#include "console.h"
#endif

//-----------------------------------------------------------------------------
//                  DUT I2C
//-----------------------------------------------------------------------------

namespace DUT_I2C
{
  //map to fixture dut signals
  #define DUT_SCL   DUT_SWC
  #define DUT_SDA   DUT_SWD

  static const int READ_BIT = 1; //RnW
  static const int CLOCK_WAIT = 9; // ~500kHz/(1+CLOCK_WAIT). 9=50kHz. 1=250KHz (2uS per edge), 0=500KHz
  
  //----------------------------- primitives -----------------------------
  
  static void pulse_(void)
  {
    DUT_SCL::set();
    Timer::wait(CLOCK_WAIT);
    DUT_SCL::reset();
    Timer::wait(CLOCK_WAIT);
  }

  static void start_(void)
  {
    DUT_SDA::mode(MODE_OUTPUT);
    
    DUT_SDA::set();
    DUT_SCL::set();
    Timer::wait(CLOCK_WAIT);
    DUT_SDA::reset();
    Timer::wait(CLOCK_WAIT);
    DUT_SCL::reset();
    Timer::wait(CLOCK_WAIT);
  }

  static void restart_(void)
  {
    DUT_SDA::mode(MODE_OUTPUT);
    
    DUT_SDA::set();
    //DUT_SCL::reset(); //precondition
    Timer::wait( MAX(10,CLOCK_WAIT*2) );
    start_();
  }
  
  static void stop_(void)
  {
    DUT_SDA::mode(MODE_OUTPUT);
    
    DUT_SDA::reset();
    Timer::wait(CLOCK_WAIT);
    DUT_SCL::set();
    Timer::wait(CLOCK_WAIT);
    DUT_SDA::set();
    Timer::wait(CLOCK_WAIT);
  }

  static void ack_(void)
  {
    DUT_SDA::mode(MODE_OUTPUT);
    
    DUT_SDA::reset();
    pulse_();
  }

  static void nack_(void)
  {
    DUT_SDA::mode(MODE_OUTPUT);
    
    DUT_SDA::set();
    pulse_();
  }

  static void put8_(u8 data)
  {
    for (int i = 7; i >= 0; i--)
    {
      if (data & (1 << i))
        nack_();
      else
        ack_();
    }
  }

  static u8 get8_(void)
  {
    u8 value = 0;
    
    DUT_SDA::mode(MODE_INPUT);
    
    for (int i = 0; i < 8; i++)
    {
      DUT_SCL::set();
      Timer::wait(CLOCK_WAIT);
      
      value <<= 1;
      value |= DUT_SDA::read();
      
      DUT_SCL::reset();
      Timer::wait(CLOCK_WAIT);
    }
    
    return value;
  }

  //----------------------------- API -----------------------------
  
  static bool initialized = 0;
  void init(void)
  {
    if( !initialized )
    {
      //Init bus pins
      DUT_SCL::init(MODE_OUTPUT, PULL_UP, TYPE_OPENDRAIN, SPEED_LOW);
      DUT_SDA::init(MODE_OUTPUT, PULL_UP, TYPE_OPENDRAIN, SPEED_LOW);
      
      // Let the lines float high
      DUT_SDA::set();
      DUT_SCL::set();
    }
    initialized = 1;
  }
  
  void deinit(void)
  {
    if( initialized ) {
      DUT_SCL::init(MODE_INPUT, PULL_NONE);
      DUT_SDA::init(MODE_INPUT, PULL_NONE);
    }
    initialized = 0;
  }
  
  int transfer(uint8_t address, int wlen, uint8_t *wdat, int rlen, uint8_t *out_rdat)
  {
    if( !initialized )
      return -1; //not init
    if( wlen<1 && rlen<1 )
      return -2; //no-op
    if( wlen>0 && (!wdat || wlen>255) )
      return -3; //bad write args
    if( rlen>0 && (!out_rdat || rlen>255) )
      return -4; //bad read args
    
    start_();
    
    //write phase
    if( wlen>0 )
    {
      put8_(address);
      DUT_SDA::set(); Timer::wait(CLOCK_WAIT); //let slave drive
      pulse_();  // Skip device ACK
      
      for(int w=0; w<wlen; w++) {
        put8_( wdat[w] );
        DUT_SDA::set(); Timer::wait(CLOCK_WAIT); //let slave drive
        pulse_();  // Skip device ACK
      }
    }
    
    //repeat start
    if( wlen>0 && rlen>0 )
      restart_();
    
    //read phase
    if( rlen>0 )
    {
      put8_(address | READ_BIT);
      DUT_SDA::set(); Timer::wait(CLOCK_WAIT); //let slave drive
      pulse_();  // Skip device ACK
      
      for(int r=0; r<rlen; ) 
      {
        out_rdat[r] = get8_();
        if( ++r < rlen )
          ack_();
        else
          nack_();
      }
    }
    
    stop_();
    return 0;
  }
  
  //----------------------------- syscon wrappers -----------------------------
  
  bool multiOp(I2C_Op func, uint8_t channel, uint8_t slave, uint8_t reg, int size, void* data)
  {
    const int max_write_size = 40;
    if( size<1 || !data )
      return 1;
    
    if( func == I2C_REG_READ )
      return transfer(slave, 1,&reg, size, (uint8_t*)data) != 0;
    
    if( func == I2C_REG_WRITE && size <= max_write_size) {
      uint8_t buf[1+max_write_size];
      buf[0] = reg;
      memcpy( buf+1, data, size );
      return transfer(slave, 1+size,buf, 0,NULL) != 0;
    }
    
    return 1; //unsupported
  }
  
  bool writeReg(uint8_t channel, uint8_t slave, uint8_t reg, uint8_t data) {
    uint8_t buf[2] = { reg, data };
    return transfer(slave, sizeof(buf),buf, 0,NULL) != 0;
  }
  
  bool writeReg16(uint8_t channel, uint8_t slave, uint8_t reg, uint16_t data) {    
    uint8_t buf[3] = { reg, data, data>>8 };
    return transfer(slave, sizeof(buf),buf, 0,NULL) != 0;
  }
  
  bool writeReg32(uint8_t channel, uint8_t slave, uint8_t reg, uint32_t data) {
    uint8_t buf[5] = { reg, data, data>>8, data>>16, data>>24 };
    return transfer(slave, sizeof(buf),buf, 0,NULL) != 0;
  }
  
  int readReg(uint8_t channel, uint8_t slave, uint8_t reg) {
    uint8_t buf[1];
    if( transfer(slave, 1,&reg, sizeof(buf),buf) != 0 ) return -1;
    return buf[0];
  }
  
  int readReg16(uint8_t channel, uint8_t slave, uint8_t reg) {
    uint8_t buf[2];
    if( transfer(slave, 1,&reg, sizeof(buf),buf) != 0 ) return -1;
    return buf[1]<<8 | buf[0];
  }
  
} //namespace DUT_I2C

