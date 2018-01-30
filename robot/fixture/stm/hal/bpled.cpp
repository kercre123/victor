#include <string.h>
#include <stdio.h>
#include "board.h"
#include "bpled.h"
#include "console.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "tests.h"
#include "testcommon.h"
#include "timer.h"

//signal map (aliases)
#define BP_LED_DAT    DUT_SWD
#define BP_LED_CLK    DUT_SWC

//shift register bits<->signals
#define SR_BIT_BPA3   0x80
#define SR_BIT_BPA2   0x40
#define SR_BIT_BPA1   0x20
#define SR_BIT_BPA0   0    /*dummy*/
#define SR_BIT_BLUE   0x10
#define SR_BIT_RED    0x08
#define SR_BIT_BPBL   0x04
#define SR_BIT_BPGN   0x02
#define SR_BIT_BPRD   0x01

namespace BPLED
{
  typedef struct {
    uint8_t   dx;     //specify D1-4, which physical rgb component it's on
    color_e   color;  //specify color
    uint8_t   srval;  //shift register value to turn on the led
  } bpled_t;
  
  //map logical led # <n> to debug info and shift register bit pattern
  static const bpled_t bpled[BPLED::num] = {
    {1, BPLED::COLOR_RED, (SR_BIT_BPA0 | 0           | 0           | 0           | SR_BIT_BLUE | 0          ) },
    {1, BPLED::COLOR_GRN, (SR_BIT_BPA0 | 0           | 0           | 0           | SR_BIT_BLUE | SR_BIT_RED ) },
    {1, BPLED::COLOR_BLU, (SR_BIT_BPA0 | 0           | 0           | 0           | 0           | SR_BIT_RED ) },
    {2, BPLED::COLOR_RED, (SR_BIT_BPA1 | 0           | SR_BIT_BPGN | SR_BIT_BPBL | SR_BIT_BLUE | SR_BIT_RED ) },
    {2, BPLED::COLOR_GRN, (SR_BIT_BPA1 | SR_BIT_BPRD | 0           | SR_BIT_BPBL | SR_BIT_BLUE | SR_BIT_RED ) },
    {2, BPLED::COLOR_BLU, (SR_BIT_BPA1 | SR_BIT_BPRD | SR_BIT_BPGN | 0           | SR_BIT_BLUE | SR_BIT_RED ) },
    {3, BPLED::COLOR_RED, (SR_BIT_BPA2 | 0           | SR_BIT_BPGN | SR_BIT_BPBL | SR_BIT_BLUE | SR_BIT_RED ) },
    {3, BPLED::COLOR_GRN, (SR_BIT_BPA2 | SR_BIT_BPRD | 0           | SR_BIT_BPBL | SR_BIT_BLUE | SR_BIT_RED ) },
    {3, BPLED::COLOR_BLU, (SR_BIT_BPA2 | SR_BIT_BPRD | SR_BIT_BPGN | 0           | SR_BIT_BLUE | SR_BIT_RED ) },
    {4, BPLED::COLOR_RED, (SR_BIT_BPA3 | 0           | SR_BIT_BPGN | SR_BIT_BPBL | SR_BIT_BLUE | SR_BIT_RED ) },
    {4, BPLED::COLOR_GRN, (SR_BIT_BPA3 | SR_BIT_BPRD | 0           | SR_BIT_BPBL | SR_BIT_BLUE | SR_BIT_RED ) },
    {4, BPLED::COLOR_BLU, (SR_BIT_BPA3 | SR_BIT_BPRD | SR_BIT_BPGN | 0           | SR_BIT_BLUE | SR_BIT_RED ) }
  };
  
  //write 8 bits to the shift register
  static void writeShiftReg(uint8_t val)
  {
    const int SHIFT_DELAY = 1; //<1MHz should be ok
    for(int x=0; x<8; x++) {
      BP_LED_DAT::write( val & 0x80 ); //74AHC164 input is MSB 1st
      Timer::wait(SHIFT_DELAY);
      BP_LED_CLK::set();
      Timer::wait(SHIFT_DELAY);
      BP_LED_CLK::reset();
      val <<= 1;
    }
    Timer::wait(SHIFT_DELAY);
    BP_LED_DAT::reset();
  }
  
  static bool bpled_enabled = 0;
  void enable()
  {
    if( !bpled_enabled )
    {
      //init pins
      BP_LED_DAT::reset();
      BP_LED_CLK::reset();
      BP_LED_DAT::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
      BP_LED_CLK::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
      
      Board::powerOff(PWR_VBAT);
      TestCommon::powerOnProtected(PWR_VBAT, 100/*ms*/, 50/*mA*/, 2); //power on + short circuit check
      writeShiftReg(0);
    }
    bpled_enabled = 1;
  }

  void disable()
  {
    Board::powerOff(PWR_VBAT);
    BP_LED_DAT::mode(MODE_INPUT);
    BP_LED_CLK::mode(MODE_INPUT);
    bpled_enabled = 0;
  }
  
  void on(uint8_t n) {
    writeShiftReg( n < BPLED::num ? bpled[n].srval : 0 );
  }
  
  void off() {
    writeShiftReg(0);
  }
  
  void putFrame(uint32_t frame, uint16_t display_time_us)
  {
    for (int n = 0; n < BPLED::num; n++)
    {
      if( frame & (1<<n) )
        BPLED::on(n);
      else
        BPLED::off();
      
      Timer::wait( display_time_us );
    }
    BPLED::off();
  }
  
  //----------------------------------
  //  Debug
  //----------------------------------
  
  int getDx(uint8_t n) {
    return n < BPLED::num ? bpled[n].dx : 0;
  }
  
  color_e getColor(uint8_t n) {
    return n < BPLED::num ? bpled[n].color : BPLED::COLOR_INVALID;
  }
  
  static inline char* colorString_(color_e c) {
    switch(c) {
      case COLOR_RED: return (char*)"RED"; //break;
      case COLOR_GRN: return (char*)"GRN"; //break;
      case COLOR_BLU: return (char*)"BLU"; //break;
      default:        return (char*)"???"; //break; //COLOR_INVALID
    }
  }
  
  char *description(uint8_t n) {
    static char s[10];
    int len = sprintf(s, "D%u.%s", (n < BPLED::num ? bpled[n].dx : 255), colorString_(getColor(n)) );
    s[len] = '\0';
    return s;
  }
  
}



