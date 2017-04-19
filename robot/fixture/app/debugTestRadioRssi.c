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

bool DebugTestDetectDevice(void)
{
  RadioProcess(); //keep uart clear
  return true; //simulate PCB detect (body, head, cube...)
}

static int8_t m_rssidat[9];
void DebugRadioModeR(void)
{
  ConsolePrintf("rf.ch   0   1   2  18  19  20  37  38  39\r\n");
  ConsolePrintf("rssi ");
  int print_len = 0;
  u32 updateTime = 0, totalTime = getMicroCounter();
  while( getMicroCounter()-totalTime < 1*60*1000*1000 )
  {
    if( getMicroCounter() - updateTime > 100*1000 )
    {
      updateTime = getMicroCounter();
      RadioGetRssi( m_rssidat );
      
      //erase old data
      for(int x=0; x<print_len; x++) {
        ConsolePutChar(0x08); //backspace
        //ConsolePutChar(0x20); //space
        //ConsolePutChar(0x08); //backspace
      }
      print_len = 0;
      
      //write new data
      for(int i=0; i<9; i++)
        print_len += ConsolePrintf(" %03i", (m_rssidat[i] < -99 ? -99 : m_rssidat[i]) );
    }
  }
  ConsolePrintf("\r\n");
  g_fixtureType = FIXTURE_NONE;
}

TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    DebugRadioModeR,
    NULL
  };
  return m_debugFunctions;
}

