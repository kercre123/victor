#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/hal.h"
#include "MK02F12810.h"
#include "hardware.h"

#include "anki/cozmo/robot/drop.h"
#include "dac.h"

static const int PERF_CLOCK = DROPS_PER_SECOND * 8 * DROP_SPACING * 5;
static const int SAMPLE_RATE = MAX_AUDIO_BYTES_PER_DROP * DROPS_PER_SECOND;
static const int CLOCK_MOD = PERF_CLOCK / SAMPLE_RATE;

static volatile uint16_t* DAC_WRITE = (volatile uint16_t*) &DAC0_DAT0L;
static const int DAC_WORDS = 16;
static const int AUDIO_RAMP = 0x20;
static const int MIN_VOLUME = 0;

static unsigned int write_pointer = 0;
static uint16_t targetAudioVolume = 0;
static uint16_t audioVolume = 0;

void Anki::Cozmo::HAL::DAC::Init(void) {
  #ifdef EP1_HEADBOARD
  SOURCE_SETUP(GPIO_AUDIO_STANDBY, SOURCE_AUDIO_STANDBY, SourceGPIO);
  GPIO_RESET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  GPIO_OUT(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  #endif
  
  // Enable PDB, PIT and DAC0
  SIM_SCGC6 |= SIM_SCGC6_DAC0_MASK;
  
  // Configure and clear DAC
  DAC0_C0 = DAC_C0_DACEN_MASK 
          | DAC_C0_DACRFS_MASK
          | DAC_C0_LPEN_MASK;
  DAC0_C1 = DAC_C1_DACBFMD(0)
          | DAC_C1_DACBFEN_MASK;


  // Configure PDB to trigger the dac
  SIM_SCGC6 |= SIM_SCGC6_PDB_MASK;

  PDB0_SC = PDB_SC_PDBEN_MASK           // enable PDB module
          | PDB_SC_CONT_MASK            // Continuous mode
          | PDB_SC_TRGSEL(0xF);         // Software trigger
  PDB0_DACINTC0 = PDB_INTC_TOE_MASK ;   // DAC output delay from PDB Software trigger
  
  // Configure our timing
  PDB0_MOD = CLOCK_MOD-1;                 // This is our clock divided by our sampling rate
  PDB0_DACINT0 = CLOCK_MOD-1;             // Effective after writting PDBSC_DACTOE = 1, DAC output changes are base on the interval defined by this value  
  PDB0_SC |= PDB_SC_LDOK_MASK ;         // Load values into registers
 
  // Start counting
  PDB0_SC |= PDB_SC_SWTRIG_MASK ;       // Trigger the PDB because why not

  Mute();
  
  SetVolume(~0); // Default to max volume
}

void Anki::Cozmo::HAL::DAC::EnableAudio(bool enable) {
  if (enable) {
    GPIO_SET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  } else {
    GPIO_RESET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  }
}

static uint32_t MuLawDecompress(uint8_t byte) {
  uint16_t bits = byte & 0xF;
  uint8_t exp = (byte >> 4) & 0x7;

  if (exp) {
    bits = (0x10 | bits) << (exp - 1);
  }

  return 0x7FF + ((byte & 0x80) ? -bits : bits);
}

void Anki::Cozmo::HAL::DAC::Sync() {
  // Set the write pointer to be the current read pointer minus one
  // The next feed will have moved ~MAX_AUDIO_BYTES_PER_DROP bytes 
  // forward, making this the ideal write location
  write_pointer = (uint8_t)((DAC0_C2 >> 4) - 1) % DAC_WORDS;
}

static void adjustVolume(int target) {
  using namespace Anki::Cozmo::HAL::DAC;

  if (target > audioVolume) {
    audioVolume = MIN(audioVolume + AUDIO_RAMP, target);
  } else if (target < audioVolume) {
    audioVolume = MAX(audioVolume - AUDIO_RAMP, target);
  }

  EnableAudio(audioVolume > MIN_VOLUME);
}

void Anki::Cozmo::HAL::DAC::Feed(bool available, uint8_t* samples) {  
  static uint16_t last_sample;
  
  adjustVolume(available ? targetAudioVolume : 0);

  for (int length = MAX_AUDIO_BYTES_PER_DROP; length > 0; length--) {
    // Decode audio only if drop contains data
    if (available) {
      last_sample = MuLawDecompress(*(samples++));
    }

    DAC_WRITE[write_pointer] = (last_sample * audioVolume) >> 16;
    write_pointer = (write_pointer+1) % DAC_WORDS;
  }
}

void Anki::Cozmo::HAL::DAC::SetVolume(uint16_t volume) {
  targetAudioVolume = (3  * (int)volume) / 4;
}

void Anki::Cozmo::HAL::DAC::Mute(void) {
  for (int i = 0; i < DAC_WORDS; i++) {
    DAC_WRITE[i] = 0;
  }

  EnableAudio(false);
}

#include <math.h>

static int next_write_index(void) {
  static int last_index = -1;
  int index;

  do {
    index  = DAC0_C2 >> 4;
  } while (index == last_index);
  
  last_index = index;
  return (index - 1) & 0xF;
}

void GenerateTestTone(void) {
  using namespace Anki::Cozmo::HAL;
   
  __disable_irq();
  GPIO_RESET(GPIO_POWEREN, PIN_POWEREN);
  MCG_C1 |= MCG_C1_IREFS_MASK;
  
  // THIS ALL NEEDS TO BE ADJUSTED
  static const int CPU_OLD_CLOCK = 100000000;
  static const int CPU_NEW_CLOCK = 32768*2560;
  static const float FREQ_DILATION = (float)CPU_OLD_CLOCK / (float)CPU_NEW_CLOCK;
  
  static const float peak = 0x800;
  static const int ticks_per_freq = (int)(SAMPLE_RATE * FREQ_DILATION * 20 / 1000); // 40ms

  float freq = (float)(M_PI_2 * 8000.0 / SAMPLE_RATE * FREQ_DILATION);

  DAC::EnableAudio(true);
  for (int g = 0; g < 8; g++) {   
    // Silence
    for (int i = 0; i < ticks_per_freq; i++) {
      DAC_WRITE[next_write_index()] = (int)peak;
    }

    // Tone
    float phase = 0;
    for (int i = 0; i < ticks_per_freq; i++) {
      DAC_WRITE[next_write_index()] = (int)(peak * sinf(phase) + peak);
      phase += freq;
    }

    // Pitch shift
    freq /= 2;
  }
  
  NVIC_SystemReset();
}
