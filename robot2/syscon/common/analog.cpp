#include <string.h>

#include "common.h"
#include "hardware.h"

#include "messages.h"

#include "analog.h"
#include "power.h"
#include "vectors.h"
#include "flash.h"

static const int SELECTED_CHANNELS = 0
  | ADC_CHSELR_CHSEL2
  | ADC_CHSELR_CHSEL4
  | ADC_CHSELR_CHSEL6
  | ADC_CHSELR_CHSEL17
  ;

// Since the analog portion is shared with the bootloader, this has to be in the shared ram space
uint16_t volatile Analog::values[ADC_CHANNELS];
static const uint16_t TRANSITION_POINT = ADC_VOLTS(4.5);
static const uint32_t RISING_EDGE =  ADC_WINDOW(0, TRANSITION_POINT);
static const uint32_t FALLING_EDGE = ADC_WINDOW(TRANSITION_POINT, ~0);
static const uint16_t MINIMUM_BATTERY = ADC_VOLTS(3.6);
static const int CHARGE_DELAY = 200; // 1 second

bool Analog::button_pressed;
static bool onBatPower;
static bool chargeAllowed;
static int chargeEnableDelay = 0;

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
            cmp      us,#0
            bgt      _jump
  }
}

static void setBatteryPower(bool offVext) {
  if (offVext) {
    nVEXT_EN::mode(MODE_INPUT);
    wait(40*5); // Just around 10us
    BAT_EN::set();
  } else {
    BAT_EN::reset();
    wait(40); // Just around 10us
    nVEXT_EN::mode(MODE_OUTPUT);
  }

  onBatPower = offVext;
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
  ADC->CCR = ADC_CCR_VREFEN;
  ADC1->CHSELR = SELECTED_CHANNELS;
  ADC1->CFGR1 = 0
              | ADC_CFGR1_CONT
              | ADC_CFGR1_DMAEN
              | ADC_CFGR1_DMACFG
              | ADC_CFGR1_AWDEN
              | ADC_CFGR1_AWDSGL
              | (ADC_CFGR1_AWDCH_0 * 2) // ADC Channel 2 (VEXT)
              ;
  ADC1->CFGR2 = 0
              | ADC_CFGR2_CKMODE_1
              ;
  ADC1->SMPR  = 0
              ;
  ADC1->IER = ADC_IER_AWDIE;
  ADC1->TR = FALLING_EDGE;

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
  chargeAllowed = true;

  // This is a fresh boot (no head)
  if (~USART1->CR1 & USART_CR1_UE) {
    // Set to VEXT power
    nVEXT_EN::reset();
    BAT_EN::reset();
    nVEXT_EN::mode(MODE_OUTPUT);
    BAT_EN::mode(MODE_OUTPUT);
    
    // Enable (low-current) charging and power
    nCHG_HC::set();
    nCHG_HC::mode(MODE_OUTPUT);
    CHG_EN::mode(MODE_INPUT);

    // Make sure battery is partially charged, and that the robot is on a charger
    // NOTE: Only one interrupt is enabled here, and it's the 200hz main timing loop
    // this lowers power consumption and interrupts fire regularly
    do {
      setBatteryPower(false);
      for( int i = 0; i < 100; i++)  __asm("wfi") ;
      setBatteryPower(true);
      __asm("wfi\nwfi");  // 5ms~10ms for power to stablize
    } while (Analog::values[ADC_VBAT] <= MINIMUM_BATTERY);

    // Power the head now that we are adiquately charged
    POWER_EN::mode(MODE_INPUT);
    POWER_EN::pull(PULL_UP);
  }
  #endif

  // Startup external power interrupt / handler
  NVIC_SetPriority(ADC1_IRQn, PRIORITY_ADC);
  NVIC_EnableIRQ(ADC1_IRQn);
  NVIC_SetPendingIRQ(ADC1_IRQn);
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
  data->battery.battery = values[ADC_VBAT];
  data->battery.charger = values[ADC_VEXT];
  data->touchLevel[1] = button_pressed ? 0xFFFF : 0x0000;
}

void Analog::allowCharge(bool enable) {
  chargeAllowed = enable;
}

void Analog::delayCharge() {
  chargeEnableDelay = 0;
}

extern "C" void ADC1_IRQHandler(void) {
  // Stop the ADC so we can change our watchdog window
  ADC1->CR |= ADC_CR_ADSTP;
  
  setBatteryPower(Analog::values[ADC_VEXT] < TRANSITION_POINT);

  // Clear interrupt and restart ADC
  ADC1->TR = onBatPower ? RISING_EDGE : FALLING_EDGE;
  ADC1->ISR = ADC_ISR_AWD;
  ADC1->CR |= ADC_CR_ADSTART;
}

#ifndef BOOTLOADER
#include "lights.h"
#endif

static const int BUTTON_THRESHOLD = ADC_VOLTS(4.0);
static const int BOUNCE_LENGTH = 3;

void Analog::tick(void) {
  static bool bouncy_button;
  static int bouncy_count;

  // Debounce buttons
  bool new_button = (values[ADC_BUTTON] >= BUTTON_THRESHOLD);

  if (bouncy_button != new_button) {
    bouncy_button = new_button;
    bouncy_count = 0;
  } else if (++bouncy_count > BOUNCE_LENGTH) {
    button_pressed = bouncy_button;
  }

  if (chargeAllowed && !onBatPower) {
    if (chargeEnableDelay > CHARGE_DELAY) {
      CHG_EN::mode(MODE_INPUT);
    } else {
      CHG_EN::reset();
      CHG_EN::mode(MODE_OUTPUT);
      
      chargeEnableDelay++;
    }
  } else {
    chargeEnableDelay = 0;
  }

  #ifndef BOOTLOADER
  static const int POWER_DOWN_TIME = 200 * 2;   // Shutdown
  static const int POWER_WIPE_TIME = 200 * 10;  // Erase flash
  static int hold_count;

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
  #endif
}
