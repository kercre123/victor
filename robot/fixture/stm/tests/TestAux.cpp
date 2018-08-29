#include "app.h"
#include "board.h"
#include "console.h"
#include "dut_i2c.h"
#include "fixture.h"
#include "flexflow.h"
#include "meter.h"
#include "opto.h"
#include "portable.h"
#include "robotcom.h"
#include "tests.h"
#include "timer.h"

//force start (via console or button) skips detect
#define TOF_REQUIRE_FORCE_START 1

static uint16_t m_last_read_mm = 0;
uint16_t TestAuxTofLastRead(void) {
  return m_last_read_mm;
}

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
  m_last_read_mm = 0;
  ConsolePrintf("initializing opto driver...\n");
  DUT_I2C::init();
  Opto::start();
  ConsolePrintf("done!\n");
}

//Playpen: 80mm+/-20mm after -30 window adjustment
const int tof_window_adjust = 30;
const int tof_read_min = 80-15; //test to tighter tolerance than playpen
const int tof_read_max = 80+15;

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
  ConsolePrintf("\n");
  
  reading_avg /= num_readings;
  uint16_t reading_adj = reading_avg > tof_window_adjust ? reading_avg - tof_window_adjust : 0;
  
  //save result for display
  m_last_read_mm = reading_adj;
  
  //FlexFlow report (print before throwing err to allow data collection on failed sensors)
  FLEXFLOW::printf("<flex> TOF reading %imm </flex>\n", reading_adj );
  
  //nominal reading expected by playpen 90-130mm raw (80mm +/-20mm after -30 window adjustment)
  if( reading_adj < tof_read_min || reading_adj > tof_read_max )
    throw ERROR_SENSOR_TOF;
}

void TOF_calibrate(void)
{
  ConsolePrintf("calibrating...");
  
}

void TOF_debugInspectRaw(void)
{
  uint32_t Tsample = 0, displayErr=0;
  while( ConsoleReadChar() > -1 );
  
  //ignore spurious initial readings
  for(int i=0; i<5; i++)
    Opto::read();
  
  //spew raw sensor data
  while(1)
  {
    if( Timer::elapsedUs(Tsample) > 250*1000 )
    {
      Tsample = Timer::get();
      
      tof_dat_t tof = *Opto::read();
      uint16_t reading_raw = __REV16(tof.reading);
      uint16_t reading_adj = reading_raw > tof_window_adjust ? reading_raw - tof_window_adjust : 0;
      ConsolePrintf("TOF: status=%03i mm=%05i mm.adj=%05i sigrate=%05i ambRate=%05i spad=%05i\n",
        tof.status, reading_raw, reading_adj, __REV16(tof.signal_rate), __REV16(tof.ambient_rate), __REV16(tof.spad_count) );
      
      m_last_read_mm = reading_adj; //save for display
      
      //real-time measurements on helper display
      if( displayErr < 3 ) { //give up if no helper detected
        char b[30], color = reading_adj < tof_read_min || reading_adj > tof_read_max ? 'r' : 'g';
        //int status = helperLcdShow(1,0,color, snformat(b,sizeof(b),"%imm [%i]   ", reading_adj, reading_raw) );
        int status = helperLcdShow(1,0,color, snformat(b,sizeof(b),"%imm", reading_adj) );
        displayErr = status==0 ? 0 : displayErr+1;
      }
    }
    
    //break on console input
    if( ConsoleReadChar() > -1 ) break;
    
    //break on btn4 press
    if( Board::btnEdgeDetect(Board::BTN_4, 1000, 50) > 0 )
      break;
    
    //break on device disconnect
    uint8_t status = DUT_I2C::readReg(0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS);
    if( status == 0xff ) {
      ConsolePrintf("disconnected\n");
      break;
    }
  }
  
  if( m_last_read_mm < tof_read_min || m_last_read_mm > tof_read_max )
    throw ERROR_SENSOR_TOF;
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
  static TestFunction m_tests_calibrate[] = {
    TOF_init,
    TOF_calibrate,
    TOF_sensorCheck,
    NULL,
  };
  
  if( g_fixmode==FIXMODE_TOF_DEBUG ) return m_tests_debug;
  if( g_fixmode==FIXMODE_TOF_CALIB ) return m_tests_calibrate;
  return m_tests;
}

