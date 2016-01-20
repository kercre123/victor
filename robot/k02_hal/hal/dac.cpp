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
          | DAC_C0_DACRFS_MASK;
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
}

void Anki::Cozmo::HAL::DAC::EnableAudio(bool enable) {
  #ifdef EP1_HEADBOARD
  if (enable) {
    GPIO_SET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  } else {
    GPIO_RESET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  }
  #endif
}

static inline uint16_t MuLawDecompress(uint8_t byte) {
  uint8_t exp = (byte >> 4) & 0x7;
  uint8_t bits = byte & 0xF;

  if (exp) {
    bits = ((exp ? 0x10 : 0) | bits) << (exp - 1);
  }

  return 0x7FF + ((byte & 0x80) ? -bits : bits);
}

void Anki::Cozmo::HAL::DAC::Feed(uint8_t* samples, int length) {  
  static int write_pointer = 0;
  
  while (length-- > 0) {
    DAC_WRITE[write_pointer] = MuLawDecompress(*(samples++));
    write_pointer = (write_pointer+1) % 16;
  }
}

void Anki::Cozmo::HAL::DAC::Tone(void) {
  EnableAudio(true);
  for (int i = 0; i < 16; i++) {
    DAC_WRITE[i] = 0x100 + 0xFF * sinf(i * M_PI_2 / 16);
  }
}

void Anki::Cozmo::HAL::DAC::Mute(void) {
  for (int i = 0; i < 16; i++) {
    DAC_WRITE[i] = 0;
  }

  EnableAudio(false);
}
