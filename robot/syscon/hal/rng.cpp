#include "nrf.h"
#include "nrf51_bitfields.h"

#include <stdint.h>
#include "rng.h"

void Random::init() {
  NRF_RNG->SHORTS = RNG_SHORTS_VALRDY_STOP_Msk;
  NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Msk;
  
  start();
}

void Random::start() {
  NRF_RNG->EVENTS_VALRDY = 0;
  NRF_RNG->TASKS_START = 1;
}

void Random::get(void* data, int length) {
  uint8_t *ptr = (uint8_t*) data;
  while (length--) {
    while (!NRF_RNG->EVENTS_VALRDY) ;

    *(ptr++) = NRF_RNG->VALUE;
    start();
  }
}
