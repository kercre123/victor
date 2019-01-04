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

extern "C" void hal_delay_us(uint32_t us);

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
  
  //analog pin cfg
  VEXT_SENSE::init(MODE_ANALOG, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
  VIN_SENSE::init(MODE_ANALOG, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
  NTC_ADC::init(MODE_ANALOG, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
  
  //ADC
  NVIC_DisableIRQ(ADC1_IRQn);
  if ((ADC1->CR & ADC_CR_ADEN) != 0)
    ADC1->CR |= ADC_CR_ADDIS;
  while ((ADC1->CR & ADC_CR_ADEN) != 0)
    ;
  hal_delay_us(20); //wait vreg to stabilize (NA on ths mcu?)
  ADC1->CFGR1 = 0 //12bit, no-DMA
    //| ADC_CFGR1_ALIGN //L-align (default R)
    | ADC_CFGR1_OVRMOD | ADC_CFGR1_DISCEN //overrun-mode, discontinuous (one-shot), 1-channel
    ;
  ADC1->CFGR2 = 0; //use ADCCLK
  ADC1->SMPR = 6; //sample time 71.5 adc clock cycles
  ADC->CCR = ADC_CCR_TSEN | ADC_CCR_VREFEN; //en internal channels: Temp,Vref
  ADC1->CR |= ADC_CR_ADCAL; //start calibration
  while( ADC1->CR & ADC_CR_ADCAL )
    ;
  ADC1->IER = 0;
  ADC1->ISR = 0x7ff; //clear flags
  ADC1->CR |= ADC_CR_ADEN; //enable ADC
  while( !(ADC1->ISR & ADC_ISR_ADRDY) )
    ;
  
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
    nVDDs_EN::set();
    for(volatile int x=0; x<100; x++);
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

//-----------------------------------------------------------------------------
//                  ADC
//----------------------------------------------------------------------------

//VREFINT bandgap voltage, calibrated value @30C+-5C VDDA = 3.3V+-10mV
#define VREFINT_CAL_ADDR  ((uint16_t*)0x1FFFF7BA)
#define VREFINT_CAL_VAL   (*VREFINT_CAL_ADDR)

//VDDA = 3.3V x VREFINT_CAL / VREFINT_DATA
//VCHANNELx = (VDDA * ADC_DATAx) / FULL_SCALE
//VCHANNELx = (3.3V * VREFINT_CAL * ADC_DATAx) / (VREFINT_DATA * FULL_SCALE)
//where:  VREFINT_CAL is the VREFINT calibration value
//        VREFINT_DATA is the actual VREFINT output value converted by the ADC
//        ADC_DATAx is the value measured by the ADC on channel x (right-aligned)
//        FULL_SCALE is the maximum digital value of the ADC output. (e.g. 12-bit = 4095)

static uint32_t adcReadChannel_(int chan) //single channel sample
{
  if( chan < 0 || chan > 17 )
    return 0;
  
  ADC1->CHSELR = (1 << chan);
  ADC1->ISR = ADC_ISR_EOS; //clear end-of-sequence flag
  ADC1->CR |= ADC_CR_ADSTART; //start conversion
  while( !(ADC1->ISR & ADC_ISR_EOS) )
    ;
  //ADC1->ISR = ADC_ISR_EOC; //w1 to clear
  return ADC1->DR; //reading DR clears EOC
}

uint32_t Board::adcRead(adc_chan_e chan, int oversample)
{
  uint32_t adc_raw=0;
  for(int x=0; x < (1 << oversample); x++) {
    adc_raw += adcReadChannel_((int)chan);
  }
  return adc_raw >> oversample;
}

uint32_t Board::adcReadMv(adc_chan_e chan, int oversample)
{
  uint32_t adc_raw=0, vrefint=0;
  
  for(int x=0; x < (1 << oversample); x++) {
    vrefint += adcReadChannel_((int)ADC_VREFINT);
    adc_raw += adcReadChannel_((int)chan);
  }
  vrefint >>= oversample;
  adc_raw >>= oversample;
  
  //Vref[mV] = 3300 * VrefCal / adcMax <-- cal @ 12-bit resolution?
  //Vref[mV] = Vdda * vrefint / adcMax  <-- as measured
  const int VDDA = 3300 * VREFINT_CAL_VAL / vrefint;
  return ((adc_raw * VDDA) >> 12);
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
