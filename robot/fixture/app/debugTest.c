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
    //int print_len = ConsolePrintf("led %02d...", i);
    //LEDOn(i);
    //MicroWait(1000*100);
    //LEDOn(255); //all off
    
    try {
      TestLED(i);
      MicroWait(1000);
    } catch(int e) {
      me = e;
      ConsolePrintf("...LED %02d FAIL\r\n", i);
    }
  }
  if( me != ERROR_OK ) {
    //throw me;
  }
}

//LED Bitfield Defines.
//Q: Where did these come from!??
//A: Manually mapped to BPLED[] array in motorled.c - could break if that changes.
#define LED_BF_D1_BLU     (1 << 0)
#define LED_BF_D1_GRN     (1 << 1)
#define LED_BF_D1_RED     (1 << 2)
#define LED_BF_D1_MAGENTA (LED_BF_D1_BLU | LED_BF_D1_RED)
#define LED_BF_D1_CYAN    (LED_BF_D1_BLU | LED_BF_D1_GRN)
#define LED_BF_D1_YELLOW  (LED_BF_D1_RED | LED_BF_D1_GRN)
#define LED_BF_D1_WHITE   (LED_BF_D1_BLU | LED_BF_D1_GRN | LED_BF_D1_RED)
#define LED_BF_D2_BLU     (1 << 3)
#define LED_BF_D2_GRN     (1 << 4)
#define LED_BF_D2_RED     (1 << 5)
#define LED_BF_D2_MAGENTA (LED_BF_D2_BLU | LED_BF_D2_RED)
#define LED_BF_D2_CYAN    (LED_BF_D2_BLU | LED_BF_D2_GRN)
#define LED_BF_D2_YELLOW  (LED_BF_D2_RED | LED_BF_D2_GRN)
#define LED_BF_D2_WHITE   (LED_BF_D2_BLU | LED_BF_D2_GRN | LED_BF_D2_RED)
#define LED_BF_D3_BLU     (1 << 6)
#define LED_BF_D3_GRN     (1 << 7)
#define LED_BF_D3_RED     (1 << 8)
#define LED_BF_D3_MAGENTA (LED_BF_D3_BLU | LED_BF_D3_RED)
#define LED_BF_D3_CYAN    (LED_BF_D3_BLU | LED_BF_D3_GRN)
#define LED_BF_D3_YELLOW  (LED_BF_D3_RED | LED_BF_D3_GRN)
#define LED_BF_D3_WHITE   (LED_BF_D3_BLU | LED_BF_D3_GRN | LED_BF_D3_RED)
#define LED_BF_D4_RED     (1 << 9)
#define LED_BF_D5_RED     (1 << 10)

//our state machine will cycle through this display pattern
#define LED_BITFIELD_PATTERN_COUNT  sizeof(_led_bitfield_patterns)/sizeof(u32)
static const u32 _led_bitfield_patterns[] = {
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | 0               | LED_BF_D3_BLU,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  
  LED_BF_D1_GRN   | LED_BF_D2_BLU   | LED_BF_D3_GRN,
  
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | 0               | LED_BF_D3_GRN,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  
  LED_BF_D1_RED   | LED_BF_D2_GRN   | LED_BF_D3_RED,
  
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  LED_BF_D1_RED   | 0               | LED_BF_D3_RED,
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  
  LED_BF_D1_BLU   | LED_BF_D2_RED   | LED_BF_D3_BLU,
};

static void _LEDDisplay(u32 led_bitfield, u32 led_time_us) {
  for (int i = 0; i < LEDCnt(); i++) {
    LEDOn( led_bitfield & 1 ? i : 255 ); //display each enabled led
    led_bitfield >>= 1;
    MicroWait(led_time_us);
  }
  LEDOn(255); //all off
}

static void _LEDSequence(void)
{
  static int pattern = -1;
  static u32 pattern_time = 0;
  
  //update state every Xms
  if( pattern < 0 || getMicroCounter() - pattern_time > 100*1000 ) {
    pattern_time = getMicroCounter();
    if( ++pattern >= LED_BITFIELD_PATTERN_COUNT )
      pattern = 0;
  }
  
  _LEDDisplay( _led_bitfield_patterns[pattern], 150 );
}

static bool _BtnDetectRising(int *out_sample_mv, int *out_state)
{
  static bool pressed_prev = 0; //state memory
  bool pressed = BPBtnGet( out_sample_mv );
  if( out_state != NULL )
    *out_state = pressed;
  bool rising_edge = pressed && !pressed_prev ? 1 : 0;
  pressed_prev = pressed;
  return rising_edge;
}

static void BackpackButton(void)
{
  u32 tick_time = getMicroCounter();
  int print_len = 0, btn_press_cnt = 0, btn_invalid_cnt = 0;
  
  //print header
  ConsolePrintf("Backpack Btn/D4: ");
  
  while(1) //( getMicroCounter() - start_time < (1000000 * 10) )
  {
    //give some juice to LED sequencer
    _LEDSequence();
    
    //periodically sample the button and update console info
    if( getMicroCounter() - tick_time > 100*1000 )
    {
      tick_time = getMicroCounter();
      
      const int btn_idle_threshold_mv = 2600;
      int btn_mv, btn_state;
      btn_press_cnt += _BtnDetectRising(&btn_mv,&btn_state);
      
      //detect samples in invalid region
      if( !btn_state && btn_mv < btn_idle_threshold_mv )
        btn_invalid_cnt++;
      
      for(int x=0; x < print_len; x++ ) {
        ConsolePutChar(0x08); //backspace
        ConsolePutChar(0x20); //space
        ConsolePutChar(0x08); //backspace
      }
      
      print_len = ConsolePrintf("%d.%03dV", btn_mv/1000, btn_mv%1000);
      print_len+= ConsolePrintf(" %s", (btn_state ? "on " : (btn_mv < btn_idle_threshold_mv ? "---INVALID---" : "off")) );
      print_len+= ConsolePrintf(" cnt:%d inval:%d", btn_press_cnt, btn_invalid_cnt);
      
      if( btn_press_cnt >= 5 )
        break;
    }
  }
  ConsolePrintf("\r\n");
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
    BackpackLeds,
    BackpackButton,
    DebugEnd,
    NULL
  };
  return m_debugFunctions;
}

