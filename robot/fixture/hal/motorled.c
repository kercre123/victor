#include <string.h>
#include "hal/board.h"
#include "hal/portable.h"
#include "hal/uart.h"
#include "hal/testport.h"
#include "hal/display.h"
#include "hal/timers.h"
#include "../app/fixture.h"
#include "hal/cube.h"
#include "hal/flash.h"

#include "hal/motorled.h"

#define MOTOR_PWM_MAXVAL 3000

// Set motor speed between 0 and 5000 millivolts
void MotorMV(int millivolts)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = MOTOR_PWM_MAXVAL-1;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  
  TIM_OCInitStructure.TIM_Pulse = 0;
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
  TIM_OC1Init(TIM3, &TIM_OCInitStructure);  

  int pwm = millivolts * MOTOR_PWM_MAXVAL / 5000;
  TIM_SetCompare1(TIM3, pwm);
  TIM_Cmd(TIM3, ENABLE);
  TIM_CtrlPWMOutputs(TIM3, ENABLE);

  GPIO_PinAFConfig(GPIOC, PINC_CHGTX, GPIO_AF_TIM3);
  PIN_AF(GPIOC, PINC_CHGTX);
}

// Init the ADC (if not already initialized)
void InitADC()
{
  static bool inited = false;
  if (inited)
    return;
  
  ADC_InitTypeDef ADC_InitStructure;
  
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
  ADC_DeInit();
  /*
  ADC_CommonInitStructure.ADC_Mode = ADC_DualMode_RegSimult;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1; // 2 half-words one by one, 1 then 2
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
  */
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 1 Channel
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // Conversions Triggered
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
  ADC_Cmd(ADC1, ENABLE);
  
  inited = true;
}

static int VCC = 2800, OVERSAMPLE = 4;  // 2^n samples

// Grab an ADC channel quickly/noisily - for scanning encoders
int QuickADC(int channel)
{
  InitADC();
  ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_15Cycles);
  
  // Turn off prefetch, then oversample to get a stable reading
  FLASH->ACR &= ~FLASH_ACR_PRFTEN;
  FLASH->ACR |= (FLASH_ACR_ICEN | FLASH_ACR_DCEN);
  ADC_SoftwareStartConv(ADC1);
  while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == RESET)
    ;
  int result = ADC_GetConversionValue(ADC1);
  FLASH->ACR |= FLASH_ACR_PRFTEN;
  return (result * VCC) >> 12;
}

// Sample one ADC channel slowly (about 500uS) - workaround STM32F215 ADC erratum (see AN4703)
int GrabADC(int channel)
{
  InitADC();
  ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_15Cycles);
  
  // Turn off prefetch, then oversample to get a stable reading
  FLASH->ACR &= ~FLASH_ACR_PRFTEN;
  FLASH->ACR |= (FLASH_ACR_ICEN | FLASH_ACR_DCEN);
  int result = 0;
  for (int i = 0; i < (1 << OVERSAMPLE); i++)
  {
    PIN_IN(GPIOA, channel);
    MicroWait(20);
    PIN_ANA(GPIOA, channel);
    ADC_SoftwareStartConv(ADC1);
    while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == RESET)
      ;
    result += ADC_GetConversionValue(ADC1);
    PIN_IN(GPIOA, channel);
  }
  FLASH->ACR |= FLASH_ACR_PRFTEN;
  
  return (result*VCC) >> (12+OVERSAMPLE);
}

// Grab quadrature encoder data
void ReadEncoder(bool light, int usDelay, int& a, int& b, bool skipa)
{
  // Turn on encoder LED and read both channels
  PIN_PULL_NONE(GPIOA, PINA_ENCLED);
  PIN_RESET(GPIOA, PINA_ENCLED);
  if (light)
    PIN_OUT(GPIOA, PINA_ENCLED);
  else
    PIN_IN(GPIOA, PINA_ENCLED);    
  
  PIN_ANA(GPIOC, PINC_ENCA);
  PIN_ANA(GPIOC, PINC_ENCB);

  if (usDelay)
    MicroWait(usDelay);  // Let LED turn on or off and ADC charge up

  if (!skipa)
    a = QuickADC(ADC_ENCA);
  b = QuickADC(ADC_ENCB);
}

static const int LED_COUNT = 4, MAP_COUNT = 12;
static const int LEDA[LED_COUNT] = { PINA_BPLED0, PINA_BPLED1, PINA_BPLED2, PINA_BPLED3 };
static const int LEDMAP[MAP_COUNT] = { 0+1, 0+2, 0+3,  4+0, 4+2, 4+3,  8+0, 8+1, 8+3, 12+0, 12+1, 12+2 };

// Turn on one of 12 LEDs
void LEDOn(u8 led)
{
  led = LEDMAP[led];
  int pinlow = led & 3;
  int pinhigh = (led >> 2) & 3;

  // Float everything
  for (int i = 0; i < LED_COUNT; i++)
  {
    PIN_PULL_NONE(GPIOA, LEDA[i]);
    PIN_IN(GPIOA, LEDA[i]);
  }
  
  PIN_RESET(GPIOA, LEDA[pinlow]);
  PIN_OUT(GPIOA, LEDA[pinlow]);
  PIN_SET(GPIOA, LEDA[pinhigh]);
  PIN_OUT(GPIOA, LEDA[pinhigh]);
}
  
// Drive one of 12 LEDs on the backpack, then measure voltage on ADC
int LEDTest(u8 led)
{
  if (led >= MAP_COUNT)
    throw ERROR_OUT_OF_RANGE;
  
  led = LEDMAP[led];
  int pinlow = led & 3;
  int pinhigh = (led >> 2) & 3;
  
  // Float everything
  for (int i = 0; i < LED_COUNT; i++)
  {
    PIN_PULL_NONE(GPIOA, LEDA[i]);
    PIN_IN(GPIOA, LEDA[i]);
  }
  
  // Now set the requested LED and wait to settle down
  PIN_RESET(GPIOA, LEDA[pinlow]);
  PIN_OUT(GPIOA, LEDA[pinlow]);
  PIN_PULL_UP(GPIOA, LEDA[pinhigh]);
 
  // Now grab the voltage
  return GrabADC(LEDA[pinhigh]);
}

