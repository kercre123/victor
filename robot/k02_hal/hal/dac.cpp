#include <string.h>
#include <stdint.h>
#include <math.h>

#include "hal/portable.h"
#include "anki/cozmo/robot/hal.h"
#include "MK02F12810.h"
#include "hardware.h"

#include "anki/cozmo/robot/drop.h"
#include "dac.h"

// 49,996,800

static const int PERF_CLOCK = DROPS_PER_SECOND * 8 * DROP_SPACING * 5;
static const int SAMPLE_RATE = MAX_AUDIO_BYTES_PER_DROP * DROPS_PER_SECOND;
static const int CLOCK_MOD = PERF_CLOCK / SAMPLE_RATE;

static volatile uint16_t* DAC_WRITE = (volatile uint16_t*) &DAC0_DAT0L;

static uint16_t MuLawDecompressTable[] =
{
  2048, 2049, 2050, 2051, 2052, 2053, 2054, 2055, 
  2056, 2057, 2058, 2059, 2060, 2061, 2062, 2063, 
  2064, 2065, 2066, 2067, 2068, 2069, 2070, 2071, 
  2072, 2073, 2074, 2075, 2076, 2077, 2078, 2079, 
  2080, 2082, 2084, 2086, 2088, 2090, 2092, 2094, 
  2096, 2098, 2100, 2102, 2104, 2106, 2108, 2110, 
  2112, 2116, 2120, 2124, 2128, 2132, 2136, 2140, 
  2144, 2148, 2152, 2156, 2160, 2164, 2168, 2172, 
  2176, 2184, 2192, 2200, 2208, 2216, 2224, 2232, 
  2240, 2248, 2256, 2264, 2272, 2280, 2288, 2296, 
  2304, 2320, 2336, 2352, 2368, 2384, 2400, 2416, 
  2432, 2448, 2464, 2480, 2496, 2512, 2528, 2544, 
  2560, 2592, 2624, 2656, 2688, 2720, 2752, 2784, 
  2816, 2848, 2880, 2912, 2944, 2976, 3008, 3040, 
  3072, 3136, 3200, 3264, 3328, 3392, 3456, 3520, 
  3584, 3648, 3712, 3776, 3840, 3904, 3968, 4032, 
  2047, 2046, 2045, 2044, 2043, 2042, 2041, 2040, 
  2039, 2038, 2037, 2036, 2035, 2034, 2033, 2032, 
  2031, 2030, 2029, 2028, 2027, 2026, 2025, 2024, 
  2023, 2022, 2021, 2020, 2019, 2018, 2017, 2016, 
  2015, 2013, 2011, 2009, 2007, 2005, 2003, 2001, 
  1999, 1997, 1995, 1993, 1991, 1989, 1987, 1985, 
  1983, 1979, 1975, 1971, 1967, 1963, 1959, 1955, 
  1951, 1947, 1943, 1939, 1935, 1931, 1927, 1923, 
  1919, 1911, 1903, 1895, 1887, 1879, 1871, 1863, 
  1855, 1847, 1839, 1831, 1823, 1815, 1807, 1799, 
  1791, 1775, 1759, 1743, 1727, 1711, 1695, 1679, 
  1663, 1647, 1631, 1615, 1599, 1583, 1567, 1551, 
  1535, 1503, 1471, 1439, 1407, 1375, 1343, 1311, 
  1279, 1247, 1215, 1183, 1151, 1119, 1087, 1055, 
  1023,  959,  895,  831,  767,  703,  639,  575, 
   511,  447,  383,  319,  255,  191,  127,   63
};

void Anki::Cozmo::HAL::DACInit(void) {
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
  PDB0_MOD = CLOCK_MOD;                 // This is our clock divided by our sampling rate
  PDB0_DACINT0 = CLOCK_MOD;             // Effective after writting PDBSC_DACTOE = 1, DAC output changes are base on the interval defined by this value  
  PDB0_SC |= PDB_SC_LDOK_MASK ;         // Load values into registers
 
  // Start counting
  PDB0_SC |= PDB_SC_SWTRIG_MASK ;       // Trigger the PDB because why not
}

void Anki::Cozmo::HAL::EnableAudio(bool enable) {
  #ifdef EP1_HEADBOARD
  if (enable) {
    GPIO_SET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  } else {
    GPIO_RESET(GPIO_AUDIO_STANDBY, PIN_AUDIO_STANDBY);
  }
  #endif
}

void Anki::Cozmo::HAL::FeedDAC(uint8_t* samples, int length) {  
  static int write_pointer = 0;
  
  while (length-- > 0) {
    DAC_WRITE[write_pointer] = MuLawDecompressTable[*(samples++)];
    write_pointer = (write_pointer+1) % 16;
  }
}

void Anki::Cozmo::HAL::DACTone(void) {
  EnableAudio(true);
  for (int i = 0; i < 16; i++) {
    DAC_WRITE[i] = 0x100 + 0xFF * sinf(i * M_PI_2 / 16);
  }
  
  MicroWait(100000);
  
  for (int i = 0; i < 16; i++) {
    DAC_WRITE[i] = 0;
  }
  EnableAudio(false);
}
