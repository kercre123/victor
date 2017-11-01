#include <string.h>

#include "common.h"
#include "hardware.h"

#include "messages.h"

#include "analog.h"
#include "power.h"
#include "vectors.h"
#include "flash.h"
#include "lights.h"

static const int SELECTED_CHANNELS = 0
  | ADC_CHSELR_CHSEL2
  | ADC_CHSELR_CHSEL4
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL17
  ;

static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.5);
static const uint32_t FALLING_EDGE = ADC_WINDOW(TRANSITION_POINT, ~0);

static const int POWER_DOWN_TIME = 200 * 2;   // Shutdown
static const int POWER_WIPE_TIME = 200 * 10;  // Erase flash
static const int MINIMUM_VEXT_TIME = 20; // 0.1s

static const int BUTTON_THRESHOLD = ADC_VOLTS(6.0);
static const int BOUNCE_LENGTH = 3;

static volatile bool onBatPower;
static bool chargeAllowed;
static int vext_debounce;
static bool last_vext = false;
static bool bouncy_button;
static int bouncy_count;
static int hold_count;

uint16_t volatile Analog::values[ADC_CHANNELS];
bool Analog::button_pressed;

static inline void wait(int us) {
  __asm {
    _jump:  nop
            nop
            nop
            nop
            nop
            nop
            nop
            subs     us,us, #1
            cmp      us, #0
            bgt      _jump
  }
}

static void setBatteryPower(bool bat) {
  ADC1->ISR = ADC_ISR_AWD;
  onBatPower = bat;

  if (bat) {
    NVIC_DisableIRQ(ADC1_IRQn);

    nVEXT_EN::mode(MODE_INPUT);
    wait(40*5); // Just around 50us
    BAT_EN::set();
  } else {
    BAT_EN::reset();
    wait(40); // Just around 10us
    nVEXT_EN::reset();
    nVEXT_EN::mode(MODE_OUTPUT);

    NVIC_EnableIRQ(ADC1_IRQn);
  }
}

void Analog::init(void) {
  // Calibrate ADC1
  if ((ADC1->CR & ADC_CR_ADEN) != 0) {
    ADC1->CR |= ADC_CR_ADDIS;
  }
  while ((ADC1->CR & ADC_CR_ADEN) != 0) ;
  ADC1->CFGR1 &= ~ADC_CFGR1_DMAEN;
  ADC1->CR |= ADC_CR_ADCAL;
  while ((ADC1->CR & ADC_CR_ADCAL) != 0) ;

  // Setup the ADC for continuous mode
  ADC1->CHSELR = SELECTED_CHANNELS;
  ADC1->CFGR1 = 0
              | ADC_CFGR1_CONT
              | ADC_CFGR1_DMAEN
              | ADC_CFGR1_DMACFG
              | ADC_CFGR1_AWDEN
              | ADC_CFGR1_AWDSGL
              | (ADC_CFGR1_AWDCH_0 * 4) // ADC Channel 4 (VMAIN)
              ;
  ADC1->CFGR2 = 0
              | ADC_CFGR2_CKMODE_1
              ;
  ADC1->SMPR  = 0
              ;
  ADC1->IER = ADC_IER_AWDIE;
  ADC1->TR = FALLING_EDGE;

  ADC->CCR = ADC_CCR_VREFEN;

  // Enable ADC
  ADC1->ISR = ADC_ISR_ADRDY;
  ADC1->CR |= ADC_CR_ADEN;
  while (~ADC1->ISR & ADC_ISR_ADRDY) ;

  // Start sampling
  ADC1->CR |= ADC_CR_ADSTART;

  // DMA in continuous mode
  DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
  DMA1_Channel1->CMAR = (uint32_t)&values[0];
  DMA1_Channel1->CNDTR = ADC_CHANNELS;
  DMA1_Channel1->CCR |= 0
                     | DMA_CCR_MINC
                     | DMA_CCR_MSIZE_0
                     | DMA_CCR_PSIZE_0
                     | DMA_CCR_CIRC;
  DMA1_Channel1->CCR |= DMA_CCR_EN;

  #ifdef BOOTLOADER
  static const uint16_t MINIMUM_BATTERY = ADC_VOLTS(3.6);
  chargeAllowed = true;

  // This is a fresh boot (no head)
  if (~USART1->CR1 & USART_CR1_UE) {
    // Disable VMAIN power
    nVEXT_EN::mode(MODE_INPUT);
    BAT_EN::reset();
    BAT_EN::mode(MODE_OUTPUT);

    // Enable (low-current) charging and power
    CHG_HC::reset();
    CHG_EN::set();
    CHG_HC::mode(MODE_OUTPUT);
    CHG_EN::mode(MODE_OUTPUT);

    // Make sure battery is partially charged, and that the robot is on a charger
    // NOTE: Only one interrupt is enabled here, and it's the 200hz main timing loop
    // this lowers power consumption and interrupts fire regularly
    for (;;) {
      BAT_EN::set();
      __asm("wfi\nwfi");  // 5ms~10ms for power to stablize
      if (Analog::values[ADC_VMAIN] > MINIMUM_BATTERY) break ;
      BAT_EN::reset();
      for( int i = 0; i < 200; i++)  __asm("wfi") ;
    }
    
    BAT_EN::reset();
  }

  POWER_EN::mode(MODE_INPUT);
  POWER_EN::pull(PULL_UP);
  #endif

  // Startup external power interrupt / handler
  NVIC_SetPriority(ADC1_IRQn, PRIORITY_ADC);
  setBatteryPower(Analog::values[ADC_VEXT] < TRANSITION_POINT);
}

void Analog::stop(void) {
  // Disable the DMA
  DMA1_Channel1->CCR = 0;

  // Stop reading values
  ADC1->CR |= ADC_CR_ADSTP;
  while ((ADC1->CR & ADC_CR_ADSTP) != 0) ;

  ADC1->CR |= ADC_CR_ADDIS;
  while ((~ADC1->CR & ADC_CR_ADEN) != 0) ;
}

void Analog::transmit(BodyToHead* data) {
  data->battery.battery = values[ADC_VMAIN];
  data->battery.charger = values[ADC_VEXT];
  data->touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
}

void Analog::allowCharge(bool enable) {
  chargeAllowed = enable;
  vext_debounce = 0;
}

void Analog::delayCharge() {  
  vext_debounce = 0;
}

extern "C" void ADC1_IRQHandler(void) {
  setBatteryPower(true);
}

void Analog::tick(void) {
  // On-charger delay
  bool vext_now = Analog::values[ADC_VEXT] >= TRANSITION_POINT;

  // Debounced VEXT line
  if (vext_now && last_vext) {
    vext_debounce++;
  } else {
    vext_debounce = 0;
  }
  last_vext = vext_now;

  // VEXT logic
  if (vext_debounce >= MINIMUM_VEXT_TIME) {
    // VEXT Switchover when on charger
    if (onBatPower) {
      __disable_irq();
      setBatteryPower(false);
      __enable_irq();
    }

    // Charge logic
    if (chargeAllowed) {
      CHG_EN::mode(MODE_INPUT);
    }
  } else if (chargeAllowed) {
    CHG_EN::reset();
    CHG_EN::mode(MODE_OUTPUT);
  }

  // Button logic
  bool new_button = (values[ADC_BUTTON] >= BUTTON_THRESHOLD);

  if (bouncy_button != new_button) {
    bouncy_button = new_button;
    bouncy_count = 0;
  } else if (++bouncy_count > BOUNCE_LENGTH) {
    button_pressed = bouncy_button;
  }

  if (button_pressed) {
    if (hold_count < POWER_DOWN_TIME) {
      hold_count++;
    } else if (hold_count < POWER_WIPE_TIME) {
      Lights::disable();
    } else {
      Power::softReset(true);
    }
  } else {
    if (hold_count >= POWER_DOWN_TIME) {
      Power::stop();
    } else {
      hold_count = 0;
    }
  }
}
