#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

namespace Radio {
  void init();
  void advertise();
  void shutdown();

  void manage(void* userdata = NULL);
  void setPropState(unsigned int slot, const uint16_t *state);
  void assignProp(unsigned int slot, uint32_t accessory);
}

#endif
