#include <stdint.h>

#include "random.h"

#ifdef NRF51
extern "C" {
  #include "nrf.h" 
  #include "nrf_sdm.h"
  #include "nrf_soc.h"
}

static inline void sd_rand(void* ptr, int length) {
  uint8_t* data = (uint8_t*)ptr;
  
  while (length > 0) {
    uint8_t avail = 0;
    
    sd_rand_application_bytes_available_get(&avail);

    if (avail == 0) {
      continue ;
    }

    if (avail > length) {
      avail = length;
    }

    sd_rand_application_vector_get((uint8_t*)data, avail);
    data += avail;
    length -= avail;
  }
}

void gen_random(void* ptr, int length) {
  uint8_t softdevice_is_enabled;
  sd_softdevice_is_enabled(&softdevice_is_enabled);

  if (softdevice_is_enabled) {
    sd_rand(ptr, length);
  } else {
    uint8_t* data = (uint8_t*)ptr;
    while (length-- > 0) {
      NRF_RNG->EVENTS_VALRDY = 0;
      NRF_RNG->TASKS_START = 1;
      
      while (!NRF_RNG->EVENTS_VALRDY) ;
      *(data++) = NRF_RNG->VALUE;
    }
  }
}
#else
#error "IMPLEMENTATION NEEDED FOR GEN_RANDOM FOR THIS ARCHITECTURE"
#endif
