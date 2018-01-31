#include <assert.h>
#include "board.h"
#include "portable.h"
#include "timer.h"

namespace Board
{
  static board_rev_t m_board_revision = BOARD_REV_INVALID;
  
  enum pinstate_e { HIGH, LOW, Z };
  static pinstate_e test_pin_state_(GPIO_TypeDef* GPIOx, uint32_t pin)
  {
    uint16_t GPIO_Pin = (1 << pin);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit( &GPIO_InitStructure ); //set struct defaults
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
    
    //test value with internal pull-up
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    Timer::wait(100);
    u8 pu_val = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
    
    //test value with internal pull-down
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    Timer::wait(100);
    u8 pd_val = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
    
    pinstate_e state;
    if( !pu_val && !pd_val )    //pulled low externally
      state = LOW;
    else if( pu_val && pd_val ) //pulled high exeternally
      state = HIGH;
    else if( pu_val && !pd_val ) //floating
      state = Z;
    else //if( !pu_val && pd_val ) //bizarro world?
      assert( false );
    
    //if we're not floating, remove pin pull to save power
    if( state != Z ) {
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_Init(GPIOB, &GPIO_InitStructure);
    }
    
    return state;
  }

  static board_rev_t revision_detect_()
  {
    pinstate_e board_id0 = test_pin_state_( PC13_BRD_REV0::bank, PC13_BRD_REV0::pin );
    pinstate_e board_id1 = test_pin_state_( PC14_BRD_REV1::bank, PC14_BRD_REV1::pin );
    
      //reserved ID pins are NC/float. return unknown if firmware is older than fixture hardware (or hw error).
    if( board_id1 != Z )
      return BOARD_REV_UNKNOWN;
    
    //fixture 1.0 rev1,2,3 did not have a board revision check. pins are NC/float.
    //fixture 1.5+ implemented this check
    if( board_id0 == Z )
      return BOARD_REV_1_0_REV3; //or rev1,2. Can't tell.
    else if( board_id0 == LOW )
      return BOARD_REV_1_5_1;
    else //no revision currently pulls this pin high.
      return BOARD_REV_UNKNOWN;
  }
  
  void init()
  {
    static bool inited = false;
    if (inited)
      return;
    
    //enable all gpio blocks
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    
    //LEDs
    LEDPIN_RED::set(); //off
    LEDPIN_RED::init(MODE_OUTPUT, PULL_UP, TYPE_PUSHPULL, SPEED_LOW);
    LEDPIN_GRN::set(); //off
    LEDPIN_GRN::init(MODE_OUTPUT, PULL_UP, TYPE_PUSHPULL, SPEED_LOW);
    
    //Piezo Buzzer (BZZ): no-pull (input state), out 0 (output state)
    BZZ::init(MODE_INPUT, PULL_NONE);
    BZZ::reset();
    
    //XXX: Always enable charger/ENCHG - this is the only way to turn off cube power
    ENCHG::set();
    ENCHG::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_LOW);
    
    //motor control disable
    PB12_IN1::init(MODE_INPUT, PULL_DOWN);
    PB13_EN1::init(MODE_INPUT, PULL_DOWN);
    PB14_IN2::init(MODE_INPUT, PULL_DOWN);
    PB15_EN2::init(MODE_INPUT, PULL_DOWN);
    Timer::wait(100);

    //default low (VEXT disabled)
    CHGTX::reset();
    CHGTX::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_LOW);
    //Contacts::powerOff(0,0);
    
    //VBAT controls
    ENBAT_LC::set();
    ENBAT_LC::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    NBAT::set();
    NBAT::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    NBATSINK::set();
    NBATSINK::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    disableVBAT();
    
    //DUT_PROG controls
    PROG_HV::set();
    PROG_HV::type(TYPE_OPENDRAIN);
    PROG_HV::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    PROG_LV::set();
    PROG_LV::type(TYPE_OPENDRAIN);
    PROG_LV::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    disableDUTPROG();
    
    m_board_revision = revision_detect_();
    
    //ADC
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    ADC_DeInit();
    //ADC_CommonInitStructure.ADC_Mode = ADC_DualMode_RegSimult;
    //ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
    //ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1; // 2 half-words one by one, 1 then 2
    //ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    //ADC_CommonInit(&ADC_CommonInitStructure);
    ADC_InitTypeDef ADC_InitStructure;
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 1 Channel
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // Conversions Triggered
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_Cmd(ADC1, ENABLE);
    
    //Buttons
    BTN1::init(MODE_INPUT, PULL_UP);
    BTN2::init(MODE_INPUT, PULL_UP);
    
    //XXX: Fix 1.5 signals force OLED to reset/deselected in case it's populated
    XXX_0_OLED_RESN::reset();
    XXX_0_OLED_RESN::init(MODE_OUTPUT, PULL_NONE);
    XXX_1_OLED_CSN::set();
    XXX_1_OLED_CSN::init(MODE_OUTPUT, PULL_NONE);
    
    inited = true;
  }

  board_rev_t revision() {
    return m_board_revision;
  }

  char* revString(void) 
  {
    char* s;
    switch( revision() ) {
      case BOARD_REV_1_0_REV1:
      case BOARD_REV_1_0_REV2:
      case BOARD_REV_1_0_REV3:  s = (char*)"1.0.r{1,2,3}"; break;
      case BOARD_REV_1_5_1:     s = (char*)"1.5.1"; break;
      default:                  s = (char*)"?"; break;
    }
    return s;
  }

  void ledOn(led_e led) {
    switch( led ) {
      case LED_RED:    LEDPIN_RED::reset(); break;
      case LED_GREEN:  LEDPIN_GRN::reset(); break;
    }
  }

  void ledOff(led_e led) {
    switch( led ) {
      case LED_RED:    LEDPIN_RED::set(); break;
      case LED_GREEN:  LEDPIN_GRN::set(); break;
    }
  }

  void ledToggle(led_e led) {
    switch( led ) {
      case LED_RED:    LEDPIN_RED::write( !LEDPIN_RED::read() ); break;
      case LED_GREEN:  LEDPIN_GRN::write( !LEDPIN_GRN::read() ); break;
    }
  }

  static bool btnPressed_(btn_e btn) {
    switch( btn ) {
      case Board::BTN_1: return !(BTN1::read() > 0); //break;
      case Board::BTN_2: return !(BTN2::read() > 0); //break;
      default: return 0; //break;
    }
  }
  
  bool btnPressed(btn_e btn, int oversample)
  {
    int sum = 0;
    for(int x=0; x < (1<<oversample); x++)
      sum += btnPressed_(btn);
    return (sum >> oversample);
  }
  
  int btnEdgeDetect(btn_e btn, int sample_period_us, int sample_limit)
  {
    static uint32_t Tsample[Board::BTN_NUM];
    static uint32_t cnt[Board::BTN_NUM];
    static uint32_t state[Board::BTN_NUM];
    
    int x = (int)btn - 1;
    if( !btn || btn > Board::BTN_NUM ) //guarantee valid btn index
      return 0;
    
    if( sample_period_us < 0 || sample_limit < 0 ) { //reset
      state[x] = 0;
      cnt[x] = 0;
      Tsample[x] = 0;
      return 0;
    }
    
    if( Timer::elapsedUs(Tsample[x]) > sample_period_us )
    {
      Tsample[x] = Timer::get();
      
      if( btnPressed(btn, 0) ) //don't oversample; debounce filter averages
      {
        if( cnt[x] < sample_limit )
          cnt[x]++;
        if( !state[x] && cnt[x] >= sample_limit ) {
          state[x] = 1; //pressed
          return 1; //Rising Edge
        }
      }
      else
      {
        if( cnt[x] > 0 )
          cnt[x]--;
        if( state[x] && cnt[x] == 0 ) {
          state[x] = 0; //released
          return -1; //Falling Edge
        }
      }
    }
    return 0; //no change
  }
  
  static u8 vbatIsEnabled = 1; //force init sync
  void enableVBAT()
  {
    NBATSINK::set(); // Disable sink (to prevent blowing up the fixture)
    NBAT::reset();
    ENCHG::set();
    vbatIsEnabled = 1;
  }

  void disableVBAT()
  {
    if (vbatIsEnabled)
    {
      NBAT::set();        //GPIO_SetBits(GPIOC, GPIO_Pin_2);
      //ENCHG::set();       //GPIO_SetBits(GPIOA, GPIO_Pin_15);
      ENCHG::reset();     //GPIO_ResetBits(GPIOA, GPIOA_ENCHG);
      Timer::wait(1);
      NBATSINK::reset();  //GPIO_ResetBits(GPIOD, GPIO_Pin_2);  // Enable sink to quickly discharge any remaining power
      ENBAT_LC::reset();  //GPIO_ResetBits(GPIOC, GPIO_Pin_1);  // Sink even more current (down to 0.3V at least)
      Timer::wait(50000);
      NBATSINK::set();    //GPIO_SetBits(GPIOD, GPIO_Pin_2);    // Disable sink (to prevent blowing up the fixture)  
      ENBAT_LC::set();    //GPIO_SetBits(GPIOC, GPIO_Pin_1);
    }
    vbatIsEnabled = 0;
  }

  bool VBATisEnabled() {
    return vbatIsEnabled;
  }

  static u8 dutprogIsEnabled = 1; //force init sync
  void enableDUTPROG(int turn_on_delay_ms)
  {
    PROG_LV::set();   //disable current sink
    PROG_HV::reset(); //enable HV PFET
    if( !dutprogIsEnabled )
      Timer::delayMs(turn_on_delay_ms);
    dutprogIsEnabled = 1;
  }
  
  void disableDUTPROG(int turn_off_delay_ms, bool force)
  {
    if( dutprogIsEnabled )
    {
      PROG_HV::set(); //disable HV PFET
      if( force )
        PROG_LV::reset(); //current sink to discharge quicklier
      Timer::delayMs(turn_off_delay_ms);
      PROG_LV::set();     //disable current sink
    }
    dutprogIsEnabled = 0;
  }
  
  bool DUTPROGisEnabled() {
    return dutprogIsEnabled;
  }
  
  void buzz(u8 f_kHz, u16 duration_ms)
  {
    u32 half_period_us = f_kHz > 0 && f_kHz <= 20 ? (1000/2)/f_kHz : 125 /*4kHz*/; //half-period in us for delay loop
    u32 sqw_start = Timer::get();
    
    while( Timer::get() - sqw_start < duration_ms*1000 ) {
      buzzerOn();
      Timer::wait( half_period_us );
      buzzerOff();
      Timer::wait( half_period_us );
    }
  }

  void buzzerOn() {
    BZZ::mode(MODE_OUTPUT); //drive 0
  }

  void buzzerOff() {
    BZZ::mode(MODE_INPUT);  //pu-1
  }
  
  uint32_t getAdcMv(adc_chan_e chan, int oversample, int pin_chg_us)
  {
    //map channel to mcu pin
    GPIO_TypeDef *bank; int pin;
    if( chan <= ADC_Channel_7 ) {
      bank = GPIOA; pin = chan;
    } else if( chan <= ADC_Channel_9 ) {
      bank = GPIOB; pin = chan - 8;
    } else {
      bank = GPIOC; pin = chan - 10;
    }
    
    ADC_RegularChannelConfig(ADC1, chan, 1, ADC_SampleTime_15Cycles);
    
    // Turn off prefetch, then oversample to get a stable reading
    FLASH->ACR &= ~FLASH_ACR_PRFTEN;
    FLASH->ACR |= (FLASH_ACR_ICEN | FLASH_ACR_DCEN);
    
    //cfg ADC input pin
    if( pin_chg_us > -1 ) { //< 0 pin cfg is handled by caller
      PIN_IN(bank, pin);
      Timer::wait( pin_chg_us );
      PIN_ANA(bank, pin);
    }
    
    //see STM32F215 ADC erratum (see AN4703)
    uint32_t adc_raw = 0;
    for(int x=0; x < (1 << oversample); x++)
    {
      ADC_SoftwareStartConv(ADC1);
      while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC) == RESET)
        ;
      adc_raw += ADC_GetConversionValue(ADC1);
    }
    adc_raw >>= oversample;
    
    //de-cfg pin
    if( pin_chg_us > -1 ) { //< 0 pin cfg is handled by caller
      PIN_IN(bank, pin);
    }
    
    FLASH->ACR |= FLASH_ACR_PRFTEN;
    
    const int VCC = 2800;
    return (adc_raw * VCC) >> 12;
  }
}

#include "contacts.h"
namespace Board
{
  //DEPRECIATED
  //XXX: remove from board API - update app to use Contacts interface
  void enableVEXT() { Contacts::powerOn(0); }
  void disableVEXT() { Contacts::powerOff(0,0); }
  bool VEXTisEnabled() { return Contacts::powerIsOn(); }
  void VEXTon(int turn_on_delay_ms) { Contacts::powerOn(turn_on_delay_ms); }
  void VEXToff(int turn_off_delay_ms) { Contacts::powerOff(turn_off_delay_ms); }
}

