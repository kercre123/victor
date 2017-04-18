#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hal/board.h"
#include "hal/display.h"
#include "hal/flash.h"
#include "hal/monitor.h"
#include "hal/motorled.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "hal/testport.h"
#include "hal/uart.h"
#include "hal/console.h"
#include "hal/cube.h"
#include "app/fixture.h"
#include "hal/espressif.h"
#include "hal/random.h"

#include "app/app.h"
#include "app/tests.h"
#include "nvReset.h"

static u32 start_time = 0;
static int test_cnt = 0;

void DebugTestBuzzer(void)
{
  ConsolePrintf("Debug Test Buzzer: ");
  for(int x=1; x<=20; x++) {
    ConsolePrintf("%dkHz,",x);
    Buzzer(x,100);
    MicroWait(100*1000);
  }
  ConsolePrintf("\r\n");
}

void DebugStart(void)
{
  start_time = getMicroCounter();
  ConsolePrintf("Debug Test %d:\r\n", ++test_cnt );
}

void DebugEnd(void)
{
  u32 time = getMicroCounter() - start_time;
  ConsolePrintf("Debug Test Time %d.%03dms\r\n", time/1000, time%1000);
  start_time = getMicroCounter(); //reset start time for detect delay
  
  g_fixtureType = FIXTURE_NONE; //back to the stone ages...
}

bool DebugTestDetectDevice(void)
{
  //periodic detect
  return getMicroCounter() - start_time > (1000*1000*10);
}

TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    DebugStart,
    DebugTestBuzzer,
    DebugEnd,
    NULL
  };
  return m_debugFunctions;
}

