#include <string.h>
#include "app.h"
#include "board.h"
#include "contacts.h"
#include "fixture.h"
#include "flash.h"
#include "motorled.h"
#include "portable.h"
#include "timer.h"

#define MOTOR_PWM_MAXVAL 3000

//1/2 HBridge config.
typedef enum { HBC_OFF /*high Z*/, HBC_GND, HBC_VCC } HBridgeHalfCfg;

//Configure the motor H-Bridge circuit (control lines)
static void m_motor_set_cfg( HBridgeHalfCfg MOTA, HBridgeHalfCfg MOTB )
{
  //config motor controller pins on first use
  //static int init_motordrv_pins = 0;
  //if( !init_motordrv_pins ) {
  //  init_motordrv_pins = 1;
    MOTDRV_IN1::reset();
    MOTDRV_IN1::mode(MODE_OUTPUT);
    MOTDRV_EN1::reset();
    MOTDRV_EN1::mode(MODE_OUTPUT);
    MOTDRV_IN2::reset();
    MOTDRV_IN2::mode(MODE_OUTPUT);
    MOTDRV_EN2::reset();
    MOTDRV_EN2::mode(MODE_OUTPUT);
  //}
  
  //disable both channels
  //MOTDRV_EN1::reset();
  //MOTDRV_EN2::reset();
  
  switch( MOTA ) {
    case HBC_GND:
      //MOTDRV_IN1::reset();
      MOTDRV_EN1::set();
      break;
    case HBC_VCC:
      MOTDRV_IN1::set();
      MOTDRV_EN1::set();
      break;
    default: //HBC_OFF:
      //MOTDRV_EN1::reset();
      //MOTDRV_IN1::reset();
      break;
  }
  
  switch( MOTB ) {
    case HBC_GND:
      //MOTDRV_IN2::reset();
      MOTDRV_EN2::set();
      break;
    case HBC_VCC:
      MOTDRV_IN2::set();
      MOTDRV_EN2::set();
      break;
    default: //HBC_OFF:
      //MOTDRV_EN2::reset();
      //MOTDRV_IN2::reset();
      break;
  }
}

// Set motor speed between 0 and 5000 millivolts
void MotorMV(int millivolts, bool reverse_nForward )
{
  //if( Board::revision() >= BOARD_REV_1_0 ) //obsoleted hw check
  {
    if( millivolts < 0 )
      m_motor_set_cfg(HBC_GND, HBC_GND); //electrical motor brake
    else if( !millivolts )
      m_motor_set_cfg(HBC_OFF, HBC_OFF);
    else if( !reverse_nForward )
      m_motor_set_cfg(HBC_VCC, HBC_GND); //MOTA=Vcc, MOTB=Gnd
    else
      m_motor_set_cfg(HBC_GND, HBC_VCC); //MOTA=Gnd, MOTB=Vcc
  }
  
  if( millivolts < 0 )
    millivolts = 0;
  
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
  
  Contacts::deinit(); //allow CHGTX to override CHGPWR
  CHGTX::alternate(GPIO_AF_TIM3); //GPIO_PinAFConfig(GPIOC, PINC_CHGTX, GPIO_AF_TIM3);
  CHGTX::mode(MODE_ALTERNATE); //PIN_AF(GPIOC, PINC_CHGTX);
}

//static int VCC = 2800, OVERSAMPLE = 4;  // 2^n samples

// Grab an ADC channel quickly/noisily - for scanning encoders
int QuickADC(int channel)
{
  /*
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
  */
  
  return Board::getAdcMv((adc_chan_e)channel, 0, -1); //disable oversample and pin cfg
}

// Sample one ADC channel slowly (about 500uS) - workaround STM32F215 ADC erratum (see AN4703)
int GrabADC(int channel)
{
  /*
  InitADC();
  ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_15Cycles);
  
  // Turn off prefetch, then oversample to get a stable reading
  FLASH->ACR &= ~FLASH_ACR_PRFTEN;
  FLASH->ACR |= (FLASH_ACR_ICEN | FLASH_ACR_DCEN);
  int result = 0;
  for (int i = 0; i < (1 << OVERSAMPLE); i++)
  {
    PIN_IN(GPIOA, channel);
    Timer::wait(20);
    PIN_ANA(GPIOA, channel);
    ADC_SoftwareStartConv(ADC1);
    while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == RESET)
      ;
    result += ADC_GetConversionValue(ADC1);
    PIN_IN(GPIOA, channel);
  }
  FLASH->ACR |= FLASH_ACR_PRFTEN;
  
  return (result*VCC) >> (12+OVERSAMPLE);
  */
  
  return Board::getAdcMv((adc_chan_e)channel, 4, 20);
}

//[LEGACY] pin defines -- DEPRECATED
// Quadrature encoder IOs/ADC channels
#define PINA_ENCLED 4
#define PINC_ENCA   3
#define ADC_ENCA    13
#define PINC_ENCB   4
#define ADC_ENCB    14

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
    Timer::wait(usDelay);  // Let LED turn on or off and ADC charge up

  if (!skipa)
    a = QuickADC(ADC_ENCA);
  b = QuickADC(ADC_ENCB);
}

//====================================================================================
//    Backpack: LEDs + Button
//====================================================================================

//[LEGACY] pin defines -- DEPRECATED
// Backpack LEDs/ADC channels
#define PINA_BPLED0 2
#define PINA_BPLED1 3
#define PINA_BPLED2 6
#define PINA_BPLED3 7

//map pin defines from board.h to schematic signal names for clarity
#define D1  PINA_BPLED0   /*A.2*/
#define D2  PINA_BPLED2   /*A.6*/
#define D3  PINA_BPLED1   /*A.3*/
#define D4  PINA_BPLED3   /*A.7*/
/*
typedef struct {
  u8  pinhigh;    //LED +Anode, pull-up side of Btns
  u8  pinlow;     //LED -Cathode, gnd side of Btns
  u16 getMv;      //LEDGetExpectedMv()
} bp_led_cfg_t; //backpack led(+btn) config

//backpack expected voltage levels
#define LED_SHORT_MV  0
#define LED_RED_MV    1500
#define LED_GRN_MV    2000
#define LED_BLU_MV    2400
#define LED_OPEN_MV   2800

static const bp_led_cfg_t BPLED[] = {
  {D1,D2,LED_BLU_MV},  {D1,D3,LED_GRN_MV},  {D1,D4,LED_RED_MV},  //D1.B,G,R
  {D2,D1,LED_BLU_MV},  {D2,D3,LED_GRN_MV},  {D2,D4,LED_RED_MV},  //D2.B,G,R  
  {D3,D1,LED_BLU_MV},  {D3,D2,LED_GRN_MV},  {D3,D4,LED_RED_MV},  //D3.B,G,R
  {D4,D1,LED_RED_MV},  {D4,D2,LED_RED_MV},  {D4,D3,LED_OPEN_MV}  //D4,D5,{BPv1.0=NC, BPv1.5=Btn}
};
#define BP_LED_COUNT  (sizeof(BPLED)/sizeof(bp_led_cfg_t))
*/
#define BPLED_BTN_IDX (BP_LED_COUNT-1) /*index of the button cfg*/
/*
static void _bp_signal_float(void)
{
  PIN_PULL_NONE(GPIOA,D1);  PIN_IN(GPIOA,D1);
  PIN_PULL_NONE(GPIOA,D2);  PIN_IN(GPIOA,D2);
  PIN_PULL_NONE(GPIOA,D3);  PIN_IN(GPIOA,D3);
  PIN_PULL_NONE(GPIOA,D4);  PIN_IN(GPIOA,D4);
}

int LEDCnt(void) {
  return BP_LED_COUNT;
}

void LEDOn(u8 led)
{
  _bp_signal_float(); //all backpack signals -> input Z
  if( led >= BP_LED_COUNT )
    return;
  
  PIN_RESET(GPIOA, BPLED[led].pinlow);
  PIN_OUT(GPIOA, BPLED[led].pinlow);
  PIN_SET(GPIOA, BPLED[led].pinhigh);
  PIN_OUT(GPIOA, BPLED[led].pinhigh);
}

//PU LED high side and measure led anode voltage [mV]
int LEDGetHighSideMv(u8 led)
{
  _bp_signal_float(); //all backpack signals -> input Z
  if( led >= BP_LED_COUNT )
    throw ERROR_OUT_OF_RANGE;
  
  // Now set the requested LED and wait to settle down
  PIN_RESET(GPIOA, BPLED[led].pinlow);
  PIN_OUT(GPIOA, BPLED[led].pinlow);
  PIN_PULL_UP(GPIOA, BPLED[led].pinhigh);
 
  // Now grab the voltage
  return GrabADC( BPLED[led].pinhigh );
}

//expected value from LEDGetHighSideMv(), if circuit is working correctly.
int LEDGetExpectedMv(u8 led)
{
  if( led >= BP_LED_COUNT )
    throw ERROR_OUT_OF_RANGE;
  return BPLED[led].getMv;
}

//return button state (true=pressed)
bool BPBtnGet(int *out_mv) {
  //backpack btn has 10k series + 100k pull-up.
  //Worst case (highest) pressed threshold is 11/(90+11)*Vcc -- assuming 10% tolerance resistors
  //-> ~305mV @ Vcc=2.8V.
  int btn_mv = BPBtnGetMv();
  if( out_mv != NULL )
    *out_mv = btn_mv;
  return btn_mv < 310;
}

//return button input voltage [mV]
int BPBtnGetMv(void)
{
  _bp_signal_float(); //all backpack signals -> input Z
  
  //Drive Gnd-side low
  PIN_RESET(GPIOA, BPLED[BPLED_BTN_IDX].pinlow);
  PIN_OUT(GPIOA, BPLED[BPLED_BTN_IDX].pinlow);

  //if( Board::revision() <= BOARD_REV_1_0_REV3 ) //v1.0 doesn't support backpack button. pull high to always read as open
  //  PIN_PULL_UP(GPIOA, BPLED[BPLED_BTN_IDX].pinhigh);
  //else //v1.5+
    PIN_PULL_NONE(GPIOA, BPLED[BPLED_BTN_IDX].pinhigh);
  
  // Now grab the voltage
  Timer::wait(100); //wait for voltages to stabilize
  return GrabADC( BPLED[BPLED_BTN_IDX].pinhigh );
}
*/
