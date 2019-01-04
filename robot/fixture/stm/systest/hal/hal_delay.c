#include <stdint.h>
//#include "hal_common.h"
#include "hardware.h"
#include <stdint.h>
//#include "hal_delay.h"

//#include "hal_cfg.h"

void hal_delay_us(uint32_t us) {
  uint32_t x;
  int cycleCount = (SYSTEM_CLOCK/1000000) / 6; //CYCLES_PER_INSTRUCTION
  for(x=0; x<us; x++) {
    volatile int cnt = cycleCount;
    while (cnt--) //little bit longer than 1us, the safer direction to err
      ;
  }
}
