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

extern void TestLED(int i); //motorTest.c
void DebugBackpackLeds(void)
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
#define LED_BF_PATTERN_COUNT  sizeof(_led_bf_patterns)/sizeof(u32)
static const u32 _led_bf_patterns[] = {
  LED_BF_D1_RED   | LED_BF_D2_RED   | LED_BF_D3_RED,
  0,
  LED_BF_D1_BLU   | LED_BF_D2_BLU   | LED_BF_D3_BLU,
  0,
  LED_BF_D1_GRN   | LED_BF_D2_GRN   | LED_BF_D3_GRN,
  0,
};

static void _led_manage(void)
{
  static u32 led_time = 0;
  static u8 led_state = 0;
  
  //Cycle through LED states every Xms
  if( !led_time || getMicroCounter() - led_time > 250*1000 )
  {
    led_time = getMicroCounter();
    if( ++led_state >= LED_BF_PATTERN_COUNT )
      led_state = 0;
  }
  
  u32 led_bf = _led_bf_patterns[led_state];
  for (int i = 0; i < LEDCnt(); i++) {
    LEDOn( led_bf & 1 ? i : 255 ); //display each enabled led
    led_bf >>= 1;
    MicroWait(150);
  }
  LEDOn(255); //all off
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

void DebugBackpackButton(void)
{
  u32 tick_time = getMicroCounter();
  int print_len = 0, btn_press_cnt = 0, btn_invalid_cnt = 0;
  
  //print header
  ConsolePrintf("Backpack Btn/D4: ");
  
  while(1) //( getMicroCounter() - start_time < (1000000 * 10) )
  {
    _led_manage();  //juice the LEDs
    
    //periodically sample the button and update console info
    if( getMicroCounter() - tick_time > 100*1000 )
    {
      tick_time = getMicroCounter();
      
      const int btn_idle_threshold_mv = 2240; //0.8*Vdd, Vdd=2.8V
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
      print_len+= ConsolePrintf(" %s", (btn_state ? "on " : (btn_mv < btn_idle_threshold_mv ? "---" : "off")) );
      print_len+= ConsolePrintf(" cnt:%d inval:%d", btn_press_cnt, btn_invalid_cnt);
      
      //if( btn_press_cnt >= 5 )
      //  break;
    }
  }
  //ConsolePrintf("\r\n");
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
    DebugBackpackLeds,
    DebugBackpackButton,
    DebugEnd,
    NULL
  };
  return m_debugFunctions;
}

