#include <string.h>
#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/cube.h"
#include "hal/monitor.h"
#include "hal/radio.h"
#include "app/app.h"
#include "app/fixture.h"
#include "binaries.h"

#define CUBE_TEST_SKIP_BURN   0   /*don't try to pgm the cube*/
#define CUBE_TEST_DEBUG       0   /*turns on debug logging*/
#define CUBE_TEST_CSV_OUT     0   /*formats output as CSV. Slow printing for sluggish console loggers.*/
#define CUBE_TEST_SCAN_EXTEND 0   /*performs extended cubescan*/

static bool radio_initialized = false;
static cubeid_t cube;

// Return true if device is detected on contacts
bool CubeDetect(void)
{
  DisableBAT();
  DisableVEXT();
  
  // Set VDD high (probably was already) 
  PIN_SET(GPIOB, PINB_VDD);
  PIN_OUT(GPIOB, PINB_VDD);
  
  // Pull down RESET - max 30K fights a 10K yielding 0.25 - or just barely low
  PIN_IN(GPIOC, PINC_RESET);
  PIN_PULL_DOWN(GPIOC, PINC_RESET);
  
  // Wait for pull-ups to fight it out
  MicroWait(10);
  
  // Return true if reset is pulled up by board
  bool detect = !!(GPIO_READ(GPIOC) & (1 << PINC_RESET));
  
  // Put everything back to normal
  PIN_PULL_NONE(GPIOC, PINC_RESET);
  
  // Wait 1ms in detect
  MicroWait(1000);
  
  return detect;
}

void CubeInit(void)
{
  memset(&cube,0,sizeof(cube));
  
  if( !radio_initialized )
  {
    //Put radio into cube scan mode
    char mode;
    switch( g_fixtureType )
    {
      //case FIXTURE_CHARGER_TEST:  //4
      case FIXTURE_CUBE1_TEST:      //5
      case FIXTURE_CUBE2_TEST:      //6
      case FIXTURE_CUBE3_TEST:      //7
        mode = 'S' + (g_fixtureType-FIXTURE_CHARGER_TEST); //filter for cube 1,2,3 only
        break;
      //case FIXTURE_CUBEX_TEST:    //21
      default:
        mode = 'S'; //allow all cube types
        break;
    }
    ConsolePrintf("Set Radio Mode '%c'\r\n", mode);
    SetRadioMode(mode);
    radio_initialized = true;
  }
}

// Connect to and burn the program into the cube or charger
void CubeBurn(void)
{
#if CUBE_TEST_SKIP_BURN > 0
  #warning "DEBUG: SKIP CUBE BURN"
  ConsolePrintf("DEBUG: SKIP CUBE BURN\r\n");
  cube = ProgramCubeWithSerial( true ); //read s/n only (skips pgm)
#else
  cube = ProgramCubeWithSerial();    // Normal bootloader (or cert firmware in FCC build)
#endif
  ConsolePrintf("cubeid,%u,%08x\r\n", cube.type, cube.serial );
  if( !cube.serial || cube.serial == 0xFFFFffff )
    throw ERROR_CUBE_CANNOT_READ;
}

static int g_deltaMA = 0;
void SetCubeCurrent(int deltaMA)
{
  g_deltaMA = deltaMA;
}

// Watch the device boot up, checking LED currents in the process
const int DELTA_CHARGER = 15, DELTA_CUBE = 15;// Delta current with LEDs on and off
const int CUBE_TEST_TIME = 2500 * 1000;  // 2.5 seconds is long enough, right?
const int MAX_MA = 100, MIN_MA = 30;     // Shouldn't be drawing more or less than this during self-test
const int STANDBY_UA = 200;              // Don't burn more than this number of microamps in standby
void CubePOST(void)
{
  // Let every GPIO into the cube float, drive reset down
  PIN_IN(GPIOA, PINA_DUTCS);
  PIN_IN(GPIOA, PINA_SCK);
  PIN_IN(GPIOA, PINA_MISO);
  PIN_IN(GPIOA, PINA_MOSI);
  PIN_IN(GPIOA, PINA_PROGHV);
  
  // First, turn everything off
  PIN_RESET(GPIOC, PINC_RESET);
  PIN_OUT(GPIOC, PINC_RESET);
  DisableVEXT();
  PIN_RESET(GPIOB, PINB_VDD);   // Forcibly discharge VDD caps
  PIN_OUT(GPIOB, PINB_VDD);
  DisableBAT();
  MicroWait(250000);
  
  // Now, bring up external power
  PIN_IN(GPIOB, PINB_VDD);
  EnableBAT();
  EnableVEXT();
  MicroWait(25000); // Let power stabilize

  // Set up fast measurement thresholds (for charger or cube)
  int deltaMA = (g_fixtureType == FIXTURE_CHARGER_TEST) ? DELTA_CHARGER : DELTA_CUBE;
  if (g_deltaMA)  deltaMA = g_deltaMA;    // Override
  MonitorSetDoubleSpeed();
  
  // Monitor self-test sequence for LED indicators
  // It takes us 110uS to read one sample, and LEDs are on for 770uS, off for 770uS
  // So, a 7 sample sliding window is sufficient to detect the rising edge of a blink
  // Cubes blink 16 LEDs + 1 LED per type (1, 2, or 3) - chargers blink 11 LEDs
#if CUBE_TEST_DEBUG > 0
  #define HISTORY_LEN 30000
  static s8 sample_history[HISTORY_LEN];
  //memset( &sample_history, 0, sizeof(sample_history) );
#endif
  int on = 0, blinks = 0, sample = 0, peak = 0, current = 0, avgpeak = 0;
  const int MASK = 15, WINDOW = 7;
  int buf[MASK+1];
  u32 start = getMicroCounter();
  
  //release reset; run cpu
#if CUBE_TEST_DEBUG > 0
  ConsolePrintf("cube power up\r\n");
#endif
  PIN_IN(GPIOC, PINC_RESET);
  
  while (getMicroCounter() - start < CUBE_TEST_TIME)
  {
    if (g_fixtureType == FIXTURE_CHARGER_TEST)
      current = ChargerGetCurrentMa() * 5;   // Because charger runs at 5x the voltage
    else
      current = BatGetCurrent();
    buf[sample&MASK] = current;
    if (current > peak)  //save peak for this blink cycle
      peak = current;
    
    int diff = 0;
    if (sample > MASK)   // Look back WINDOW samples, average 3 adjacent samples
      diff = ((buf[(sample-1)&MASK] + buf[(sample)&MASK] + buf[(sample+1)&MASK])
             -(buf[(sample-WINDOW-1)&MASK] + buf[(sample-WINDOW)&MASK] + buf[(sample-WINDOW+1)&MASK])) / 3;
    
#if CUBE_TEST_DEBUG > 0
    #define ABS(x)  ( (x)>=0 ? (x) : -(x) )
    if( sample < HISTORY_LEN )
      sample_history[sample] = current > 99 ? 111 : current < -99 ? -111 : current;
#endif
    
    if (diff > deltaMA && !on)
    {
      blinks++;
      on = 1;
    }
    if (diff < -deltaMA && on)
    {
      avgpeak += peak;
      on = 0;
      peak = 0;
    }
    sample++;
  };
  // Measure standby current
  int microamps = 0;
  for (int i = 0; i < 1000; i++)
    microamps += BatGetCurrent();
  
#if CUBE_TEST_DEBUG > 0
  ConsolePrintf("window sampling complete\r\n");
#endif
  
  // Calculate how many LEDs we saw light
  int leds = (blinks + 32) >> 6;  // Each LED blinks 64 times
  int expected = g_fixtureType == FIXTURE_CUBEX_TEST ? cube.type : (g_fixtureType - FIXTURE_CHARGER_TEST);
  if (0 == expected)
    expected = 11;    // Charger has 11 LEDs
  else
    expected += 16;   // Other cubes light 16 LEDs + their ID code
  
  // Shut down and print results
  avgpeak = blinks > 0 ? avgpeak/blinks : 0;
/*
#if CUBE_TEST_DEBUG > 0
  ConsolePrintf("disabling power\r\n");
#endif
  DisableVEXT();
  DisableBAT();
*/

#if CUBE_TEST_DEBUG > 0
  {
    int num_samples = sample > HISTORY_LEN ? HISTORY_LEN : sample;
    ConsolePrintf("logged %d of %d samples\r\n", num_samples, sample);
    
    if( CUBE_TEST_CSV_OUT > 0 )
    {
      //output samples and derived metrics in CSV rows for easy import to excel/graphing
      ConsolePrintf("sample,i[mA],i-lowpass,i-lookback-lp,i-diff,on\r\n");
      int i_on = 0; //track filtered LED on/off state
      for( int x=0; x < num_samples; x++ )
      {
        int i_lowpass_x3 = 0, i_lookback_lowpass_x3 = 0, i_diff = 0;
        if( x > 0 && x < num_samples-1 )
          i_lowpass_x3 = sample_history[x-1] + sample_history[x] + sample_history[x+1];
        if( x > WINDOW )
          i_lookback_lowpass_x3 = sample_history[x-WINDOW-1] + sample_history[x-WINDOW] + sample_history[x-WINDOW+1];
        if( x > MASK )
          i_diff = (i_lowpass_x3 - i_lookback_lowpass_x3) / 3;
        if (i_diff > deltaMA && !i_on)
          i_on = 1;
        if (i_diff < -deltaMA && i_on)
          i_on = 0;
        ConsolePrintf("%d,%i,%i,%i,%i,%i\r\n", x, sample_history[x], i_lowpass_x3/3, i_lookback_lowpass_x3/3, i_diff, i_on*100 );
        MicroWait(1000); //console logging to file is slower than a turtle in peanut butter
      }
    }
    else
    {
      //for simple pattern inspection in console, just dump the whole thing
      for( int x=0; x < num_samples; x++ )
        ConsolePrintf("%i,", sample_history[x]);
      ConsolePrintf("\r\n");
    }
  }
#endif
  
  ConsolePrintf("cube-test,%d,%d,%d,%d,%d,%d,%d\r\n", leds, expected, blinks, microamps, avgpeak, sample, deltaMA);
  
  // Check all the results and throw exceptions if faults are found
  if (avgpeak < MIN_MA)
    throw ERROR_CUBE_UNDERPOWER;
  if (leds == 0)
    throw ERROR_CUBE_NO_BOOT;
  if (leds != expected)
    throw ERROR_CUBE_MISSING_LED;
  if (avgpeak > MAX_MA)
    throw ERROR_CUBE_OVERPOWER;
  if (microamps > STANDBY_UA)
    throw ERROR_CUBE_STANDBY;
}

void CubeScan(void)
{
#if CUBE_TEST_SCAN_EXTEND > 0
  #warning "DEBUG: Extended cube scanning"
  ConsolePrintf("Debug: extended cubescan\r\n");
  const int cubescan_time_s = 15;
#else
  const int cubescan_time_s = 8;
#endif
  
  int cube_detected = 0;
  RadioPurgeBuffer(); //clear radio rx; likely overflowed by now and out of sync
  
  u32 start = getMicroCounter();
  while( getMicroCounter() - start < cubescan_time_s*1000*1000 )
  {
    RadioProcess();
    
    u32 id; u8 type;
    if( RadioGetCubeScan(&id,&type) )
    {
      #if CUBE_TEST_SCAN_EXTEND > 0
        ConsolePrintf("cubescan,%u,%08x%s\r\n", type, id, id == cube.serial && type == cube.type ? " <---" : "" ); //identify our cube
      #else
        ConsolePrintf("cubescan,%u,%08x\r\n", type, id );
      #endif
      
      if( id == cube.serial && type == cube.type ) { //found our cube!
        cube_detected = 1;
        #if !(CUBE_TEST_SCAN_EXTEND > 0)
          break; //exit immediately on regular scan
        #endif
      }
    }
  }
  
  //turn off the cube
  DisableVEXT();
  DisableBAT();
  
  if( !cube_detected )
    throw ERROR_CUBE_SCAN_FAILED;
}

// List of all functions invoked by the test, in order
TestFunction* GetCubeTestFunctions(void)
{
  static TestFunction functions[] =
  {
    CubeInit,
    CubeBurn,
    CubePOST,
    CubeScan,
    NULL
  };

  return functions;
}
