#include <assert.h>
#include "board.h"
#include "contacts.h"
#include "portable.h"
#include "timer.h"

#define BOARD_DEBUG 0

#if BOARD_DEBUG > 0
#warning "BOARD_DEBUG"
#include "console.h"
#endif

namespace Board
{
  static board_rev_t m_board_revision = BOARD_REV_INVALID;
  
  //-----------------------------------------------------------------------------
  //                  Init
  //-----------------------------------------------------------------------------
  
  enum pinstate_e { HIGH = '1', LOW = '0', Z = 'Z', CAP2GND = 'C' };
  static pinstate_e test_pin_state_(GPIO_TypeDef* GPIOx, uint32_t pin)
  {
    uint16_t GPIO_Pin = (1 << pin);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit( &GPIO_InitStructure ); //set struct defaults
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
    
    //initial state, pull down & discharge
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    Timer::wait(1000);
    
    //flip on the internal pull-up, measure rise time (if it rises) and final pin state
    //GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP
    //GPIO_Init(GPIOx, &GPIO_InitStructure);
    const int ctr_max = 2000; //100us @ ~50ns loop time
    int ctr=0, read=0;
    GPIOx->PUPDR = (GPIOx->PUPDR & ~(GPIO_PUPDR_PUPDR0<<(pin*2))) | ((GPIO_PuPd_UP)<<(pin*2));
    do {
      read = (GPIOx->IDR & GPIO_Pin) > 0; //GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
    } while( !read && ++ctr < ctr_max );
    int pu_val = read; //final pin value
    int pu_ctr = ctr;  //rise time
    
    //test value with internal pull-down
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOx, &GPIO_InitStructure);
    Timer::wait(100);
    int pd_val = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
    
    pinstate_e state;
    if( !pu_val && !pd_val )    //pulled low externally
      state = LOW;
    else if( pu_val && pd_val ) //pulled high exeternally
      state = HIGH;
    else if( pu_val && !pd_val ) //floating or capacitor to gnd
      state = pu_ctr > 2 ? CAP2GND : Z;
    else //if( !pu_val && pd_val ) //bizarro world?
      assert( false );
    
    //if we're not floating, remove pin pull to save power
    if( state != Z ) {
      GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
      GPIO_Init(GPIOB, &GPIO_InitStructure);
    }
    
    #if BOARD_DEBUG > 0
    ConsolePrintf("test_pin_state_(%i)=%c : up/dn=%i/%i upctr=%i\n", pin, (char)state, pu_val, pd_val, pu_ctr );
    #endif
    
    return state;
  }

  static board_rev_t revision_detect_()
  {
    pinstate_e board_id0 = test_pin_state_( PC13_BRD_REV0::bank, PC13_BRD_REV0::pin );
    pinstate_e board_id1 = test_pin_state_( PC14_BRD_REV1::bank, PC14_BRD_REV1::pin );
    pinstate_e board_id2 = test_pin_state_( PC15_BRD_REV2::bank, PC15_BRD_REV2::pin );
    
    #if BOARD_DEBUG > 0
    ConsolePrintf("BOARD REVISION DETECT BITS: <%c%c%c>\n", (char)board_id2, (char)board_id1, (char)board_id0);
    #endif
    
    //ambiguity confuses and infuriates us! Ahhh!
    if( board_id0 == Z || board_id1 == Z || board_id2 == Z )
      return BOARD_REV_INVALID;
    
    if( board_id0 == LOW && board_id1 == LOW && board_id2 == LOW )
      return BOARD_REV_1_0;
    
    if( board_id0 == HIGH && board_id1 == LOW && board_id2 == LOW )
      return BOARD_REV_2_0;
    
    if( board_id0 == HIGH && board_id1 == CAP2GND && board_id2 == CAP2GND )
      return BOARD_REV_A;
    
    return BOARD_REV_FUTURE; //time travel is possible
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
    
    m_board_revision = revision_detect_();
    
    //LEDs
    LEDPIN_RED::set(); //off
    LEDPIN_RED::init(MODE_OUTPUT, PULL_UP, TYPE_PUSHPULL, SPEED_LOW);
    LEDPIN_GRN::set(); //off
    LEDPIN_GRN::init(MODE_OUTPUT, PULL_UP, TYPE_PUSHPULL, SPEED_LOW);
    LEDPIN_YLW::set(); //off
    LEDPIN_YLW::init(MODE_OUTPUT, PULL_UP, TYPE_PUSHPULL, SPEED_LOW);
    
    //Piezo Buzzer (BZZ): no-pull (input state), out 0 (output state)
    BZZ::init(MODE_INPUT, PULL_NONE);
    BZZ::reset();
    
    //motor control disable
    MOTDRV_IN1::init(MODE_INPUT, PULL_DOWN);
    MOTDRV_EN1::init(MODE_INPUT, PULL_DOWN);
    MOTDRV_IN2::init(MODE_INPUT, PULL_DOWN);
    MOTDRV_EN2::init(MODE_INPUT, PULL_DOWN);
    Timer::wait(100);

    //DUT_VEXT off
    Contacts::init();
    Contacts::powerOff();
    
    //VBAT controls
    ENBAT_LC::set();
    ENBAT_LC::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    NBAT::set();
    NBAT::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    NBATSINK::set();
    NBATSINK::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    ENCHG::reset();
    ENCHG::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_LOW);
    Board::powerOff(PWR_VBAT,0);
    
    //CUBEBAT controls
    ENCHG_CUBE::reset();
    ENCHG_CUBE::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_LOW);
    Board::powerOff(PWR_CUBEBAT,0);
    
    //DUT_PROG controls
    PROG_HV::set();
    PROG_HV::type(TYPE_OPENDRAIN);
    PROG_HV::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    PROG_LV::set();
    PROG_LV::type(TYPE_OPENDRAIN);
    PROG_LV::init(MODE_OUTPUT, PULL_NONE, TYPE_OPENDRAIN, SPEED_LOW);
    Board::powerOff(PWR_DUTPROG,0);
    
    //UAMP/DUT_VDD controls
    Board::powerOff(PWR_DUTVDD,0,0);
    Board::powerOff(PWR_UAMP,0,0);
    
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
    PB1::init(MODE_INPUT, PULL_UP);
    PB2::init(MODE_INPUT, PULL_UP);
    PB3::init(MODE_INPUT, PULL_UP);
    PB4::init(MODE_INPUT, PULL_UP);
    
    inited = true;
  }

  //-----------------------------------------------------------------------------
  //                  Revision
  //-----------------------------------------------------------------------------
  
  board_rev_t revision() {
    return m_board_revision;
  }

  char* revString(void) 
  {
    if( revision() <= BOARD_REV_INVALID )
      return (char*)"INVALID";
    switch( revision() ) {
      case BOARD_REV_1_0:       return (char*)"1.0"; //break;
      case BOARD_REV_2_0:       return (char*)"2.0"; //break;
      case BOARD_REV_A:         return (char*)"A";   //break;
      default:                  return (char*)"?";   //break;
    }
  }

  //-----------------------------------------------------------------------------
  //                  LEDs & Buttons
  //-----------------------------------------------------------------------------
  
  void ledOn(led_e led) {
    switch( led ) {
      case LED_RED:    LEDPIN_RED::reset(); break;
      case LED_GREEN:  LEDPIN_GRN::reset(); break;
      case LED_YLW:    LEDPIN_YLW::reset(); break;
      default: break;
    }
  }

  void ledOff(led_e led) {
    switch( led ) {
      case LED_RED:    LEDPIN_RED::set(); break;
      case LED_GREEN:  LEDPIN_GRN::set(); break;
      case LED_YLW:    LEDPIN_YLW::set(); break;
      default: break;
    }
  }

  void ledToggle(led_e led) {
    switch( led ) {
      case LED_RED:    LEDPIN_RED::write( !LEDPIN_RED::read() ); break;
      case LED_GREEN:  LEDPIN_GRN::write( !LEDPIN_GRN::read() ); break;
      case LED_YLW:    LEDPIN_YLW::write( !LEDPIN_YLW::read() ); break;
      default: break;
    }
  }

  static bool btnPressed_(btn_e btn) {
    switch( btn ) {
      case Board::BTN_1: return !(PB1::read() > 0); //break;
      case Board::BTN_2: return !(PB2::read() > 0); //break;
      case Board::BTN_3: return !(PB3::read() > 0); //break;
      case Board::BTN_4: return !(PB4::read() > 0); //break;
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
  
  //-----------------------------------------------------------------------------
  //                  Power
  //-----------------------------------------------------------------------------
  
  static u8 vbatIsEnabled = 1; //force init sync
  static u8 cubebatIsEnabled = 1; //force init sync
  static u8 dutprogIsEnabled = 1; //force init sync
  static u8 dutvddIsEnabled = 1; //force init sync
  static u8 uampIsEnabled = 1; //force init sync
  
  void powerOn(pwr_e net, int turn_on_delay_ms)
  {
    bool was_off = !Board::powerIsOn(net);
    
    switch( net )
    {
      case PWR_VEXT:
        Contacts::powerOn(0); //no delay
        break;
      
      case PWR_VBAT:
        NBATSINK::set(); //disable current sink
        ENBAT_LC::set();
        NBAT::reset();
        ENCHG::set();
        vbatIsEnabled = 1;
        break;
        
      case PWR_CUBEBAT:
        ENCHG_CUBE::set(); //enable cube regulator
        cubebatIsEnabled = 1;
        break;
      
      case PWR_DUTPROG:
        PROG_LV::set();   //disable current sink
        PROG_HV::reset(); //enable HV PFET
        dutprogIsEnabled = 1;
        break;
        
      case PWR_DUTVDD:
        Board::powerOff(PWR_UAMP,0,0); //UAMP-DUTVDD conflict
        DUT_VDD::set();
        DUT_VDD::init(MODE_OUTPUT,PULL_NONE);
        dutvddIsEnabled = 1;
        break;
      
      case PWR_UAMP:
        Board::powerOff(PWR_DUTVDD,0,0); //UAMP-DUTVDD conflict
        UAMP::set();
        UAMP::init(MODE_OUTPUT,PULL_NONE);
        uampIsEnabled = 1;
        break;
    }
    
    if( was_off )
      Timer::delayMs(turn_on_delay_ms);
  }
  
  void powerOff(pwr_e net, int turn_off_delay_ms, bool force)
  {
    switch( net )
    {
      case PWR_VEXT:
        Contacts::powerOff(turn_off_delay_ms, force);
        break;
      
      case PWR_VBAT:
        if( vbatIsEnabled )
        {
          NBAT::set();
          ENCHG::reset();
          if( force ) {
            Timer::wait(1);
            NBATSINK::reset();  // Enable sink to quickly discharge any remaining power
            ENBAT_LC::reset();  // Sink even more current (down to 0.3V at least)
          }
          Timer::delayMs(turn_off_delay_ms);
          NBATSINK::set();    // Disable sink (to prevent blowing up the fixture)  
          ENBAT_LC::set();
        }
        vbatIsEnabled = 0;
        break;
        
      case PWR_CUBEBAT:
        if( cubebatIsEnabled )
        {
          ENCHG_CUBE::reset();
          if( force ) {
            Timer::wait(1);
            //add some current sink to quickly discharge?
          }
          Timer::delayMs(turn_off_delay_ms);
        }
        cubebatIsEnabled = 0;
        break;
        
      case PWR_DUTPROG:
        if( dutprogIsEnabled )
        {
          PROG_HV::set(); //disable HV PFET
          if( force ) {
            Timer::wait(1);
            PROG_LV::reset(); //current sink to discharge quicklier
          }
          Timer::delayMs(turn_off_delay_ms);
          PROG_LV::set();     //disable current sink
        }
        dutprogIsEnabled = 0;
        break;
      
      case PWR_DUTVDD:
        if( dutvddIsEnabled )
        {
          if( force ) {
            DUT_VDD::reset(); //drive low to discharge
          } else {
            DUT_VDD::init(MODE_INPUT,PULL_NONE); //just disconnect
          }
          Timer::delayMs(turn_off_delay_ms);
        }
        DUT_VDD::init(MODE_INPUT,PULL_NONE);
        dutvddIsEnabled = 0;
        break;
        
      case PWR_UAMP:
        if( uampIsEnabled )
        {
          if( force ) {
            UAMP::reset(); //drive low to discharge
          } else {
            UAMP::init(MODE_INPUT,PULL_NONE); //just disconnect
          }
          Timer::delayMs(turn_off_delay_ms);
        }
        UAMP::init(MODE_INPUT,PULL_NONE);
        uampIsEnabled = 0;
        break;
    }
  }
  
  bool powerIsOn(pwr_e net)
  {
    switch( net ) {
      case PWR_VEXT:    return Contacts::powerIsOn();
      case PWR_VBAT:    return vbatIsEnabled;
      case PWR_CUBEBAT: return cubebatIsEnabled;
      case PWR_DUTPROG: return dutprogIsEnabled;
      case PWR_DUTVDD:  return dutvddIsEnabled;
      case PWR_UAMP:    return uampIsEnabled;
    }
    return false;
  }
  
  char* power2str(pwr_e net)
  {
    switch( net ) {
      case PWR_VEXT:    return (char*)"VEXT";
      case PWR_VBAT:    return (char*)"VBAT";
      case PWR_CUBEBAT: return (char*)"CUBEBAT";
      case PWR_DUTPROG: return (char*)"DUTPROG";
      case PWR_DUTVDD:  return (char*)"DUTVDD";
      case PWR_UAMP:    return (char*)"UAMP";
    }
    return (char*)"?";
  }
  
  //-----------------------------------------------------------------------------
  //                  buzzer
  //----------------------------------------------------------------------------
  
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
  
  //-----------------------------------------------------------------------------
  //                  ADC
  //----------------------------------------------------------------------------
  
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

