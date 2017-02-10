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

void DebugStart(void)
{
  start_time = getMicroCounter();
  ConsolePrintf("Debug Test %d:\r\n", ++test_cnt );
}

extern void TestLED(int i); //motorTest.c
static void BackpackLeds(void)
{
  int me = ERROR_OK;
  for( int i=0; i < LEDCnt(); i++ )
  {
    int print_len = ConsolePrintf("led %02d...", i);
    LEDOn(i);
    MicroWait(1000*333);
    LEDOn(255); //all off
    
    try {
      TestLED(i);
      ConsolePrintf("ok\r\n");
    } catch(int e) {
      me = e;
      ConsolePrintf("FAIL\r\n");
    }
  }
  if( me != ERROR_OK )
    throw me;
}

static void BackpackButton(void)
{
  ConsolePrintf("Backback Button Voltage = ");
  start_time = getMicroCounter();
  u32 tick_time = start_time;
  int print_len = 0;
  
  while(1) //( getMicroCounter() - start_time < (1000000 * 10) )
  {
    if( getMicroCounter() - tick_time > 250*1000 )
    {
      tick_time = getMicroCounter();
      
      const int avg_num_samples = 4;
      const int bnt_press_threshold_mv = 500;
      
      //single measurement
      int btn_mv_single = BPBtnGetMv();
      
      MicroWait(10000);
      
      //Fast average
      int btn_mv_avg_fast = 0;
      for(int i=0; i<avg_num_samples; i++) {
        btn_mv_avg_fast += BPBtnGetMv();
        //MicroWait(10000);
      }
      btn_mv_avg_fast /= avg_num_samples;
      
      MicroWait(10000);
      
      //Slow average
      int btn_mv_avg_slow = 0;
      for(int i=0; i<avg_num_samples; i++) {
        btn_mv_avg_slow += BPBtnGetMv();
        MicroWait(10000);
      }
      btn_mv_avg_slow /= avg_num_samples;
      
      //digital
      bool btn_pressed = btn_mv_avg_fast < bnt_press_threshold_mv;
      
      //display
      for(int x=0; x < print_len; x++ )
        ConsolePutChar(0x08); //backspace
      print_len = ConsolePrintf("single/avg-fast/avg-slow: %d.%03dV %d.%03dV %d.%03dV %s", 
        btn_mv_single/1000, btn_mv_single%1000, 
        btn_mv_avg_fast/1000, btn_mv_avg_fast%1000,
        btn_mv_avg_slow/1000, btn_mv_avg_slow%1000,
        btn_pressed ? "on" : "off" );
    }
  }
  //ConsolePrintf("\r\n");
}

void DebugEnd(void)
{
  u32 time = getMicroCounter() - start_time;
  ConsolePrintf("Debug Test Time %d.%03dms\r\n", time/1000, time%1000);
  start_time = getMicroCounter(); //reset start time for detect delay
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
    BackpackLeds,
    BackpackButton,
    DebugEnd,
    NULL
  };
  return m_debugFunctions;
}

