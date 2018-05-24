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

#include "../syscon/hal/tests.h"

#define BTN_DEBUG 0

static struct {
  u32           power_time;
  u32           led_time;
  u8            led_state;
  u16           btn_press_lpf_cnt;
  u16           btn_release_lpf_cnt;
  u16           btn_threshold_violation;
} m;

//true if robot test
static __inline bool _isRobot(void) {
  return  (g_fixtureType == FIXTURE_ROBOT1_TEST) ||
          (g_fixtureType == FIXTURE_ROBOT2_TEST) ||
          (g_fixtureType == FIXTURE_ROBOT3_TEST) ||
          (g_fixtureType == FIXTURE_ROBOT3_CE_TEST) ||
          (g_fixtureType == FIXTURE_ROBOT3_LE_TEST) ||
          (g_fixtureType == FIXTURE_PACKOUT_TEST) ||
          (g_fixtureType == FIXTURE_PACKOUT_CE_TEST) ||
          (g_fixtureType == FIXTURE_PACKOUT_LE_TEST) ||
          (g_fixtureType == FIXTURE_COZ187_TEST);
}

//true for backpack test (motor2h or 3h)
static __inline bool _isBackpack(void) {
  return  (g_fixtureType == FIXTURE_MOTOR2H_TEST) ||
          (g_fixtureType == FIXTURE_MOTOR3H_TEST);
}

//LED Bitfield Defines. bit<n> is arg map -> LEDOn(n).
//matches the internal BPLED[] array in motorled.c - could break if that changes.
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

/*/Debug-Test pattern: led identification/test
static const u32 _led_bf_patterns[] = {
  LED_BF_D1_BLU,
  LED_BF_D2_BLU,
  LED_BF_D3_BLU,
  LED_BF_D1_GRN,
  LED_BF_D2_GRN,
  LED_BF_D3_GRN,
  LED_BF_D1_RED,
  LED_BF_D2_RED,
  LED_BF_D3_RED,
  LED_BF_D4_RED,
  LED_BF_D5_RED,
};//-*/

/*/Debug-Test pattern: multicolor out->in blink
static const u32 _led_bf_patterns[] = {
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
};//-*/

static void _leds_release(void) {
  if( _isRobot() )
    SendCommand(TEST_LIGHT, 0, 0, NULL); //index 0 release LED control back to robot
}

static void _throw_gracefully(int e) {
  _leds_release(); //first release led control back to robot
  throw e;
}

//convert from bitfield index (LEDOn) to robot index (Sendcommand(TEST_LIGHT...)
static u8 _led_idx_bf2robot(u8 idx)
{
  u8 ri = 0xF; //clear value
  switch(idx) {
    //For Robot index mapping, see 'BackpackLight setting[]' array in backpack.cpp
    //    BP.idx  Robot.idx
    case  0:      ri = 11; break; //D1.BLUE
    case  1:      ri = 10; break; //D1.GRN
    case  2:      ri =  9; break; //D1.RED
    case  3:      ri =  5; break; //D2.BLUE
    case  4:      ri =  4; break; //D2.GRN
    case  5:      ri =  3; break; //D2.RED
    case  6:      ri =  8; break; //D3.BLUE
    case  7:      ri =  7; break; //D3.GRN
    case  8:      ri =  6; break; //D3.RED
    case  9:      ri =  2; break; //D4.RED
    case 10:      ri =  1; break; //D5.RED
  }
  return ri;
}

static void _led_manage(void)
{
  //Cycle through LED states every Xms
  if( !m.led_time || getMicroCounter() - m.led_time > 250*1000 )
  {
    m.led_time = getMicroCounter();
    if( ++m.led_state >= LED_BF_PATTERN_COUNT )
      m.led_state = 0;
    
    //update LEDs on the robot
    if( _isRobot() )
    {
      SendCommand(TEST_LIGHT, 0xF, 0, NULL); //v88+ fw clears all BP leds if index > last led
      u32 led_bf = _led_bf_patterns[m.led_state];
      for (int i = 0; i < LEDCnt(); i++) {
        if( led_bf & 1 ) {
          SendCommand(TEST_LIGHT, _led_idx_bf2robot(i), 0, NULL);
        }
        led_bf >>= 1;
      }
    }
  }
  
  //Motor2H assembly (backpack). Direct drive the LEDs every update
  if( _isBackpack() )
  {
    u32 led_bf = _led_bf_patterns[m.led_state];
    for (int i = 0; i < LEDCnt(); i++) {
      LEDOn( led_bf & 1 ? i : 255 ); //display each enabled led
      led_bf >>= 1;
      MicroWait(150);
    }
    LEDOn(255); //all off
  }
}

//for assembled robots, keep hitting the watchdog
static void _power_manage(void)
{
  const int watchdog_update_time_s = 10;
  
  if( _isRobot() ) {
    if( !m.power_time || getMicroCounter() - m.power_time > (watchdog_update_time_s*1000*1000) ) {
      m.power_time = getMicroCounter();
      #if BTN_DEBUG > 0
      ConsolePrintf("%u TEST_POWERON %ds\r\n", getMicroCounter(), watchdog_update_time_s*2);
      #endif
      SendCommand(TEST_POWERON, watchdog_update_time_s*2, 0, NULL);
    }
  }
}

static void _btn_execute(void)
{
  _power_manage(); //keep power flowing (robot)
  _led_manage(); //juice the LEDs
  
  //read the button and update (LPF) counter
  const int idle_threshold_mv = 2240; //0.8*Vdd, Vdd=2.8V
  int btn_mv = idle_threshold_mv;
  u8 pressed = 0;
  if( _isRobot() ) {
    pressed = 123; //nonsensical value to detect old robot FW
    SendCommand(TEST_BACKBUTTON, 0, sizeof(u8), &pressed);
    if( pressed == 123 ) //no data received from body (pre-v88 body FW ignores button cmd)
      _throw_gracefully( ERROR_BODY_OUTOFDATE );
  } else { //_isBackpack()
    pressed = BPBtnGet(&btn_mv);
  }
  m.btn_press_lpf_cnt = !pressed ? 0 : (m.btn_press_lpf_cnt >= 65535 ? 65535 : m.btn_press_lpf_cnt+1);
  m.btn_release_lpf_cnt = pressed ? 0 : (m.btn_release_lpf_cnt >= 65535 ? 65535 : m.btn_release_lpf_cnt+1);
  
  //button signal voltage outside normal cmos input voltages
  if( !pressed && btn_mv < idle_threshold_mv ) {
    m.btn_threshold_violation++;
    #if BTN_DEBUG > 0
    ConsolePrintf("threshold violation %dmV\r\n", btn_mv);
    #endif
    if( m.btn_threshold_violation >= 3 )
      _throw_gracefully( ERROR_BACKPACK_BTN_THRESH );
  }
}

void ButtonTest(void)
{
  memset( &m, 0, sizeof(m) ); //reset all counters/state
  u32 btn_start, cnt_compare = _isRobot() ? 5 : 50; //adjust LPF count for different sample timing
  
  if( !_isRobot() && !_isBackpack() )
    return;
  
  ConsolePrintf("Waiting for button...\r\n");
  
  //wait for button press
  m.btn_press_lpf_cnt = 0;  //reset LPF count
  m.btn_threshold_violation = 0;
  btn_start = getMicroCounter();
  while( m.btn_press_lpf_cnt < cnt_compare ) {
    _btn_execute();
    if( getMicroCounter() - btn_start > 5*1000*1000 )
      _throw_gracefully( ERROR_BACKPACK_BTN_PRESS_TIMEOUT );
  }
  
  ConsolePrintf("btn pressed\r\n");
  
  //wait for button release
  m.btn_release_lpf_cnt = 0; //reset LPF count
  m.btn_threshold_violation = 0;
  btn_start = getMicroCounter();
  while( m.btn_release_lpf_cnt < cnt_compare ) {
    _btn_execute();
    if( getMicroCounter() - btn_start > 3500*1000 )
      _throw_gracefully( ERROR_BACKPACK_BTN_RELEASE_TIMEOUT );
  }
  
  ConsolePrintf("btn released\r\n");
  _leds_release();
}

