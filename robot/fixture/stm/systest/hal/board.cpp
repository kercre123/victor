#include <assert.h>
#include "board.h"
#include "portable.h"
#include "timer.h"

//------------------------------------------------  
//    Cherry-Pick from syscon
//------------------------------------------------

void Power_enableClocking(void) {
  RCC->AHBENR |= 0
              | RCC_AHBENR_CRCEN
              | RCC_AHBENR_DMAEN
              | RCC_AHBENR_GPIOAEN
              | RCC_AHBENR_GPIOBEN
              | RCC_AHBENR_GPIOCEN
              | RCC_AHBENR_GPIOFEN
              ;

  RCC->APB1ENR |= 0
               | RCC_APB1ENR_TIM14EN
               ;
  RCC->APB2ENR |= 0
               | RCC_APB2ENR_USART1EN
               | RCC_APB2ENR_SYSCFGEN
               | RCC_APB2ENR_ADC1EN
               ;
}

//------------------------------------------------  
//    Local
//------------------------------------------------

static void NopLoopDelayMs_(unsigned int ms) {
	for(; ms>0; ms--) {
    //MicroWait(1000) -- Not Optimized. Runs about 0.6 us long.
    unsigned int x = 1000;
    unsigned int i;
    for(; x>0; x--) {
      for(i=0; i<5; i++)
        __nop();
      // or 41x __nop();
    }
  }
}

void Board::init()
{
  static bool inited = false;
  if (inited)
    return;
  
  Power_enableClocking(); //bus clocks
  NopLoopDelayMs_(10);//100); ??
  
  // Set N pins on motors low for power up
  LN1::reset();
  LN2::reset();
  HN1::reset();
  HN2::reset();
  RTN1::reset();
  RTN2::reset();
  LTN1::reset();
  LTN2::reset();
  LN1::mode(MODE_OUTPUT);
  LN2::mode(MODE_OUTPUT);
  HN1::mode(MODE_OUTPUT);
  HN2::mode(MODE_OUTPUT);
  RTN1::mode(MODE_OUTPUT);
  RTN2::mode(MODE_OUTPUT);
  LTN1::mode(MODE_OUTPUT);
  LTN2::mode(MODE_OUTPUT);

  //ah, ah, ah, ah, stayin alive, stayin alive
  Board::vdd(1);  //Enable MCU power
  Board::charger(CHRG_OFF);
  
  //The debug pins are in AF pull-up/pull-down after reset:
  //PA14: SWCLK in pull-down [muxed: LED_DAT]
  //PA13: SWDIO in pull-up   [muxed: LED_CLK]
  //LED_DAT::mode(MODE_INPUT); LED_DAT::alternate(0);
  //LED_CLK::mode(MODE_INPUT); LED_CLK::alternate(0);
  
  inited = true;
}

//------------------------------------------------  
//    Power Control
//------------------------------------------------

void Board::vdd(bool en)
{
  if( en ) {
    POWER_EN::set();
    POWER_EN::mode(MODE_OUTPUT); //MODE_INPUT);
    //POWER_EN::pull(PULL_UP);
  } else {
    //POWER_EN::reset();
    //POWER_EN::mode(MODE_OUTPUT);
    POWER_EN::mode(MODE_INPUT);
  }
}

void Board::vdds(bool en)
{
  if( en ) {
    nVDDs_EN::reset();
    nVDDs_EN::mode(MODE_OUTPUT);
  } else {
    //nVDDs_EN::set();
    nVDDs_EN::mode(MODE_INPUT); //MODE_OUTPUT);
    //nVDDs_EN::pull(PULL_UP);
  }
}

static inline bool is_bat_en_(void) { return BAT_EN::getMode() == MODE_OUTPUT && BAT_EN::read() > 0; }
static inline void bat_en_(bool on) {
  if( on ) {
    BAT_EN::set();
    BAT_EN::mode(MODE_OUTPUT);
  } else {
    BAT_EN::reset();
    BAT_EN::mode(MODE_OUTPUT);
  }
}

static inline bool is_vext_en_(void) { return nVEXT_EN::getMode() == MODE_OUTPUT; }
static inline void vext_en_(bool on) {
  if( on ) {
    nVEXT_EN::reset();
    nVEXT_EN::mode(MODE_OUTPUT);
  } else {
    nVEXT_EN::mode(MODE_INPUT);
  }
}

void Board::vbats(vbats_src_e src)
{
  //make sure only 1 power source is switched into VBATs, or could short circuit
  //turn-off delays for power fet gates to fully turn off
  switch(src)
  {
    case VBATS_SRC_OFF:
      if( is_vext_en_() )
        vext_en_(0), NopLoopDelayMs_(10);
      if( is_bat_en_() )
        bat_en_(0), NopLoopDelayMs_(10);
      break;
      
    case VBATS_SRC_VBAT:
      if( is_vext_en_() )
        vext_en_(0), NopLoopDelayMs_(10);
      bat_en_(1);
      break;
      
    case VBATS_SRC_VEXT:
      if( is_bat_en_() )
        bat_en_(0), NopLoopDelayMs_(10);
      vext_en_(1);
      break;
  }
}

#if 0 /*HWVERP3*/
static inline bool is_chg_en_(void) { return nCHG_EN::getMode() == MODE_OUTPUT && !nCHG_EN::read(); }
static inline void chg_en_(bool on) {
  (void)is_chg_en_();
  if( on ) {
    nCHG_EN::reset();
    nCHG_EN::mode(MODE_OUTPUT);
  } else {
    nCHG_EN::mode(MODE_INPUT);
  }
}
#else /*HWVER DVT1-A????? still in design*/
static inline bool is_chg_en_(void) { return CHG_EN::read() > 0; }
static inline void chg_en_(bool on) {
  (void)is_chg_en_();
  if( on ) {
    CHG_EN::set();
    CHG_EN::mode(MODE_OUTPUT);
  } else {
    CHG_EN::reset();
    CHG_EN::mode(MODE_OUTPUT);
  }
}
#endif

//static inline bool is_chghc_en_(void) { return nCHG_HC::getMode() == MODE_OUTPUT && !nCHG_HC::read(); }
static inline void chghc_en_(bool on) {
  if( on ) {
    nCHG_HC::reset();
    nCHG_HC::mode(MODE_OUTPUT);
  } else {
    nCHG_HC::mode(MODE_INPUT);
  }
}

void Board::charger(chrg_en_e state)
{
  switch(state)
  {
    case CHRG_OFF:
      chg_en_(0);
      chghc_en_(0);
      break;
    case CHRG_LOW:
      chghc_en_(0);
      chg_en_(1);
      break;
    case CHRG_HIGH:
      chghc_en_(1);
      chg_en_(1);
      break;
  }
}

//------------------------------------------------  
//    Global
//------------------------------------------------
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char* snformat(char *s, size_t n, const char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  int len = vsnprintf(s, n, format, argptr);
  va_end(argptr);
  return s;
}
