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
#include "app/fixture.h"
#include "binaries.h"

#define CUBE_TEST_DEBUG 0

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

// Connect to and burn the program into the cube or charger
void CubeBurn(void)
{  
  ProgramCubeWithSerial();    // Normal bootloader (or cert firmware in FCC build)
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

  // Let power stabilize, then free from reset
  MicroWait(25000);
  #if CUBE_TEST_DEBUG > 0
  ConsolePrintf("cube power up\r\n");
  #endif
  PIN_IN(GPIOC, PINC_RESET);

  // Set up fast measurement thresholds (for charger or cube)
  int deltaMA = (g_fixtureType == FIXTURE_CHARGER_TEST) ? DELTA_CHARGER : DELTA_CUBE;
  if (g_deltaMA)  deltaMA = g_deltaMA;    // Override
  MonitorSetDoubleSpeed();
  
  #if CUBE_TEST_DEBUG > 0
  deltaMA = 10;
  ConsolePrintf("======== SET DELTA %dma ==========\r\n", deltaMA);
  #endif
  
  // Monitor self-test sequence for LED indicators
  // It takes us 110uS to read one sample, and LEDs are on for 770uS, off for 770uS
  // So, a 7 sample sliding window is sufficient to detect the rising edge of a blink
  // Cubes blink 16 LEDs + 1 LED per type (1, 2, or 3) - chargers blink 11 LEDs
  int on = 0, blinks = 0, sample = 0, peak = 0, current = 0, avgpeak = 0;
  #if CUBE_TEST_DEBUG > 0
  #define HISTORY_LEN 30000
  static s8 sample_history[HISTORY_LEN];
  memset( &sample_history, 0, sizeof(sample_history) );
  int imax=0, imin=9999, absdiffmax=0;
  #endif
  const int MASK = 15, WINDOW = 7;
  int buf[MASK+1];
  u32 start = getMicroCounter();
  while (getMicroCounter() - start < CUBE_TEST_TIME)
  {
    if (g_fixtureType == FIXTURE_CHARGER_TEST)
      current = ChargerGetCurrent() * 5;   // Because charger runs at 5x the voltage
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
    if( current > imax ) //save current maximum for entire sampling procedure
      imax = current;
    if( current < imin ) //save current minimum
      imin = current;
    if( ABS(diff) > absdiffmax ) //find maximum window difference
      absdiffmax = ABS(diff);
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
  ConsolePrintf("finished window sampling\r\n");
  #endif
  
  // Calculate how many LEDs we saw light
  int leds = (blinks + 32) >> 6;  // Each LED blinks 64 times
  int expected = (g_fixtureType - FIXTURE_CHARGER_TEST);
  if (0 == expected)
    expected = 11;    // Charger has 11 LEDs
  else
    expected += 16;   // Other cubes light 16 LEDs + their ID code
  
  // Shut down and print results
  avgpeak = blinks > 0 ? avgpeak/blinks : 0;
  #if CUBE_TEST_DEBUG > 0
  ConsolePrintf("disabling power\r\n");
  #endif
  DisableVEXT();
  DisableBAT();
  
  ConsolePrintf("cube-test,%d,%d,%d,%d,%d,%d,%d\r\n", leds, expected, blinks, microamps, avgpeak, sample, deltaMA);
  
  #if CUBE_TEST_DEBUG > 0
  ConsolePrintf("debug: imax=%d,imin=%d,absdiffmax=%d\r\n", imax, imin, absdiffmax);
  ConsolePrintf("sample_history(%d):\r\n", sample);
  for( int x=0; x < (sample > HISTORY_LEN ? HISTORY_LEN : sample) ; x++ )
      ConsolePrintf("%i,", sample_history[x]);
  ConsolePrintf("\r\n");
  #endif
  
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

// List of all functions invoked by the test, in order
TestFunction* GetCubeTestFunctions(void)
{
  static TestFunction functions[] =
  {
    #if CUBE_TEST_DEBUG > 0
    #warning "skip cube burn"
    #else
    CubeBurn,
    #endif
    CubePOST,
    NULL
  };

  return functions;
}
