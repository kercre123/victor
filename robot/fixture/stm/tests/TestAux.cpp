#include "app.h"
#include "board.h"
#include "console.h"
#include "dut_i2c.h"
#include "fixture.h"
#include "flexflow.h"
#include "meter.h"
#include "opto.h"
#include "portable.h"
#include "tests.h"
#include "timer.h"

//force start (via console or button) skips detect
#define TOF_REQUIRE_FORCE_START 1

static struct { uint16_t reading_mm; uint16_t reading_adj; } flexnfo;

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
  memset( &flexnfo, 0, sizeof(flexnfo) );
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
  
  //FlexFlow report (print here to allow data collection on failed sensors)
  flexnfo.reading_mm = reading_avg;
  flexnfo.reading_adj = reading_avg > 30 ? reading_avg - 30 : 0;
  FLEXFLOW::printf("<flex> TOF reading %imm adj %imm </flex>\n", flexnfo.reading_mm, flexnfo.reading_adj );
  
  //nominal reading expected by playpen 90-130mm raw (80mm +/-20mm after -30 window adjustment)
  if( reading_avg < 95 || reading_avg > 125 )
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
      uint16_t reading_adj = __REV16(tof.reading)>30 ? __REV16(tof.reading)-30 : 0; //adjustment for window interference
      ConsolePrintf("TOF: status=%03i mm=%05i mm.adj=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
        tof.status, __REV16(tof.reading), reading_adj, __REV16(tof.signal_rate), __REV16(tof.ambient_rate), __REV16(tof.spad_count) );      
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

