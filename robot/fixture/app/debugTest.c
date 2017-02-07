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
static int test_cnt;
static int expired = 0;

void DebugStart(void)
{
  start_time = getMicroCounter();
  test_cnt = 0;
}

void DebugTestX(void)
{
  ++test_cnt;
  ConsolePrintf("Debug Test %d...", test_cnt );
  for(int x=0; x<test_cnt; x++)
    MicroWait(1000*1000);
  ConsolePrintf("Done\r\n");
}

void DebugEnd(void)
{
  u32 time = getMicroCounter() - start_time;
  ConsolePrintf("Debug Test Time %d.%03dms\r\n", time/1000, time%1000);
  expired = 1; //inhibit further device detects
}

bool DebugTestDetectDevice(void)
{
  //test debug flow - detect phantom device once after boot delay.
  if( !expired && getMicroCounter() > (1000*1000*10) )
    return true;
  return false;
}

TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    DebugStart,
    DebugTestX,
    DebugTestX,
    DebugTestX,
    DebugEnd,
    NULL
  };
  return m_debugFunctions;
}

