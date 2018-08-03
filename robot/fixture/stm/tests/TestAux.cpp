#include "app.h"
#include "board.h"
#include "console.h"
#include "dut_i2c.h"
#include "fixture.h"
#include "meter.h"
#include "opto.h"
#include "portable.h"
#include "tests.h"
#include "timer.h"

bool TestAuxTofDetect(void)
{
  // Make sure power is not applied, as it messes up the detection code below
  TestAuxTofCleanup();
  
  /*
  //weakly pulled-up - it will detect as grounded when the board is attached
  DUT_SWC::init(MODE_INPUT, PULL_UP);
  Timer::wait(100);
  bool detected = !DUT_SWC::read(); //true if pin is pulled down by the board
  DUT_SWC::init(MODE_INPUT, PULL_NONE); //prevent pu from doing strange things to mcu (phantom power?)
  
  return detected;
  */
  
  return false;
}

void TestAuxTofCleanup(void)
{
  Opto::stop();
  DUT_I2C::deinit();
}

/*
void TestAuxI2CDebug(void)
{
  ConsolePrintf("initializing i2c driver...\n");
  DUT_I2C::init();
  ConsolePrintf("done!\n");
  Timer::delayMs(1000);
  
  DUT_I2C::writeReg(0, TOF_SENSOR_ADDRESS, 0xAD, 0xA5);
  Timer::wait(250);
  DUT_I2C::writeReg16(0, TOF_SENSOR_ADDRESS, 0xAD, 0x11A5);
  Timer::wait(250);
  DUT_I2C::writeReg32(0, TOF_SENSOR_ADDRESS, 0xAD, 0x332211A5);
  Timer::wait(250);
  
  uint8_t buf[9] = {1,2,3,4,5,6,7,8,9};
  DUT_I2C::multiOp(I2C_REG_WRITE, 0, TOF_SENSOR_ADDRESS, 0xAD, sizeof(buf), buf);
  Timer::wait(250);
  
  Timer::delayMs(5000);
  
  DUT_I2C::deinit();
}
*/

void TestAuxTOF1(void)
{
  ConsolePrintf("initializing opto driver...\n");
  Opto::start(); //DUT_I2C::init();
  ConsolePrintf("done!\n");
  Timer::delayMs(250);
  
  while( ConsoleReadChar() > -1 );
  while(1) //for( int x=0; x<10; x++ )
  {
    Timer::delayMs(250);
    
    tof_dat_t tof = *Opto::read();
    ConsolePrintf("TOF: status=%03i reading=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
      tof.status, tof.reading, tof.signal_rate, tof.ambient_rate, tof.spad_count );
    
    if( ConsoleReadChar() > -1 )
      break;
  }
  
  Opto::stop(); //DUT_I2C::deinit();
}

TestFunction* TestAuxTofGetTests(void)
{
  static TestFunction m_tests[] = {
    //TestAuxI2CDebug,
    TestAuxTOF1,
    NULL,
  };
  return m_tests;
}

