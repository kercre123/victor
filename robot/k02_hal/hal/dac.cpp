#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/hal.h"
#include "MK02F12810.h"
#include "hardware.h"

#include "dac.h"

void Anki::Cozmo::HAL::DACInit(void) {
  #ifdef EP1_HEADBOARD
  SOURCE_SETUP(GPIO_AUDIO_STANDBY, SOURCE_AUDIO_STANDBY, SourceGPIO);
  GPIO_RESET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  GPIO_OUT(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  #endif

  SIM_SCGC6 |= SIM_SCGC6_DAC0_MASK;
  DAC0_C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;
}

static const int SAMPLE_RATE = 24000;
static int tone_freq = 440;

void Anki::Cozmo::HAL::DACTone(void) {
  #ifdef EP1_HEADBOARD
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;   // Enable FTM1

  // Temporarly steal FTM0 for our own horrendous devices
  FTM0_MOD = BUS_CLOCK / SAMPLE_RATE - 1;   // 168 bytes at I2S_CLOCK
  FTM0_CNT = 0;
  FTM0_CNTIN = 0;

  FTM0_SC = FTM_SC_TOF_MASK |
            FTM_SC_TOIE_MASK |
            FTM_SC_CLKS(1) | // BUS_CLOCK
            FTM_SC_PS(0);

  GPIO_SET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  NVIC_EnableIRQ(FTM0_IRQn);
  NVIC_SetPriority(FTM0_IRQn, 1);
  
  for(int i = 0; i < 6; i++) {
    tone_freq = 440;
    MicroWait(500000);
    tone_freq = 1760;
    MicroWait(500000);
  }

  NVIC_DisableIRQ(FTM0_IRQn);
  GPIO_RESET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  #endif
}

static volatile uint16_t* dac_word = (volatile uint16_t*) &DAC0_DAT0L;

extern "C"
void FTM0_IRQHandler(void)
{
  const float wrap = 2.0f * PI;
  const uint16_t range = 0xBFF; // 75% intensity
  
  static float phase = 0;

  float increment = wrap * tone_freq / SAMPLE_RATE;

  FTM0_SC &= ~FTM_SC_TOF_MASK;

  *dac_word = range * (sinf(phase) + 1.0f) / 2.0f;
  if ((phase += increment) > wrap) {
    phase -= wrap;
  }
}
