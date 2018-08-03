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

void TestAuxTofCleanup(void) {
  Opto::stop();
  DUT_I2C::deinit();
}

void TOF_init(void) {
  ConsolePrintf("initializing opto driver...\n");
  DUT_I2C::init();
  Opto::start();
  ConsolePrintf("done!\n");
}

void TOF_sensorCheck(void)
{
  ConsolePrintf("sampling TOF sensor:\n");
  int reading_avg=0; const int num_readings=25;
  for(int i=-10; i<num_readings; i++)
  {
    tof_dat_t tof = *Opto::read();
    uint16_t value = __REV16(tof.reading);
    if( i>=0 ) reading_avg += value; //don't include initial readings; "warm-up" period
    if( i==0 ) ConsolePrintf("\n");
    ConsolePrintf("%i,", value );
  }
  ConsolePrintf("\navg reading: %imm\n", reading_avg /= num_readings);
  
  //nominal reading expected: 80mm
  if( reading_avg < 65 || reading_avg > 125 )
    throw ERROR_SENSOR_TOF;
}

void TOF_debugInspectRaw(void)
{
  //spew raw sensor data. break on console input
  while( ConsoleReadChar() > -1 );
  while(1)
  {
    tof_dat_t tof = *Opto::read();
    ConsolePrintf("TOF: status=%03i reading=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
      tof.status, __REV16(tof.reading), __REV16(tof.signal_rate), __REV16(tof.ambient_rate), __REV16(tof.spad_count) );
    
    if( ConsoleReadChar() > -1 ) break;
    Timer::delayMs(250);
  }
}

TestFunction* TestAuxTofGetTests(void)
{
  static TestFunction m_tests[] = {
    TOF_init,
    TOF_sensorCheck,
    NULL,
  };
  static TestFunction m_tests_debug[] = {
    TOF_init,
    TOF_debugInspectRaw,
    NULL,
  };
  return g_fixmode==FIXMODE_TOF_DEBUG ? m_tests_debug : m_tests;
}

