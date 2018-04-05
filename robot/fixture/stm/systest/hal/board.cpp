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
  Board::pwr_vdd(1);  //Enable MCU power (+VBATs)
  Board::pwr_vdds(0);
  Board::pwr_vmain(0);
  Board::pwr_charge(0);
  
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

void Board::pwr_vdd(bool en)
{
  POWER_EN::write( en );
  POWER_EN::mode(MODE_OUTPUT);
}

void Board::pwr_vdds(bool en)
{
  if( en ) {
    nVDDs_EN::reset();
    nVDDs_EN::mode(MODE_OUTPUT);
  } else {
    nVDDs_EN::mode(MODE_INPUT);
  }
}

void Board::pwr_vmain(bool en)
{
  MAIN_EN::write( en );
  MAIN_EN::mode(MODE_OUTPUT);
}

void Board::pwr_charge(bool en)
{
  CHG_PWR::write( !en );
  CHG_PWR::mode(MODE_OUTPUT);
  /*
  if( en ) {
    CHG_PWR::mode(MODE_INPUT);
    //CHG_EN::mode(MODE_INPUT);
  } else {
    CHG_PWR::reset();
    CHG_PWR::mode(MODE_OUTPUT);
    //CHG_EN::reset();
    //CHG_EN::mode(MODE_OUTPUT);
  }
  */
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
