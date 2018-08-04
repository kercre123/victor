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

//force start (via console or button) skips detect
#define TOF_REQUIRE_FORCE_START 1

int detectPrescale=0; bool detectSticky=0;
bool TestAuxTofDetect(void)
{
  if( TOF_REQUIRE_FORCE_START && !g_isDevicePresent ) {
    TestAuxTofCleanup();
    return false;
  }
  
  //throttle detection so we don't choke out the console
  if( ++detectPrescale >= 50 ) {
    detectPrescale = 0;
    DUT_I2C::init();
    uint8_t status = DUT_I2C::readReg(0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS);
    detectSticky = status != 0xff;
  }
  
  return detectSticky;
}

void TestAuxTofCleanup(void) {
  Opto::stop();
  DUT_I2C::deinit();
  detectPrescale = 0;
  detectSticky = g_isDevicePresent;
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
  if( reading_avg < 75 || reading_avg > 125 )
    throw ERROR_SENSOR_TOF;
}

void TOF_debugInspectRaw(void)
{
  uint32_t Tsample = 0;
  while( ConsoleReadChar() > -1 );
  
  //spew raw sensor data
  while(1)
  {
    if( Timer::elapsedUs(Tsample) > 250*1000 ) {
      Tsample = Timer::get();
      tof_dat_t tof = *Opto::read();
      ConsolePrintf("TOF: status=%03i reading=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
        tof.status, __REV16(tof.reading), __REV16(tof.signal_rate), __REV16(tof.ambient_rate), __REV16(tof.spad_count) );      
    }
    
    //break on console input
    if( ConsoleReadChar() > -1 ) break;
    
    //break on device disconnect
    uint8_t status = DUT_I2C::readReg(0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS);
    if( status == 0xff ) {
      ConsolePrintf("disconnected\n");
      break;
    }
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

