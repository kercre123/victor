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
#include "hal/radio.h"
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

enum {
  CUBETYPE_ALL  = 0,
  CUBETYPE_1    = 1,
  CUBETYPE_2    = 2,
  CUBETYPE_3    = 3,
};

static int _whichType = 0;
bool DebugTestDetectDevice(void)
{
  RadioPurgeBuffer();
  return _whichType != g_fixtureType;
}

void DebugRadioInit(void)
{
  SetRadioMode('S' + CUBETYPE_ALL);
  //SetRadioMode('S' + CUBETYPE_1);
  //SetRadioMode('S' + CUBETYPE_2);
  //SetRadioMode('S' + CUBETYPE_3);
  _whichType = g_fixtureType;
}

void DebugRadioProcess(void)
{
  u32 id,start; u8 type;
  
  while(1)
  {
    //test scan/print of cube-id data
    start = getMicroCounter();
    while( getMicroCounter() - start < 10*1000*1000 ) {
      RadioProcess();
      if( RadioGetCubeScan(&id,&type) )
        ConsolePrintf("cubescan,%u,%08x\r\n", type, id );
    }
    
    //test overflowing the rx buffer and purging/resync
    const int delay_s = 5;
    start = getMicroCounter();
    ConsolePrintf("delay %ds\r\n", delay_s);
    while( getMicroCounter() - start < delay_s*1000*1000 )
    {
    }
    RadioPurgeBuffer();
  }
}

TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    DebugRadioInit,
    DebugRadioProcess,
    NULL
  };
  return m_debugFunctions;
}

