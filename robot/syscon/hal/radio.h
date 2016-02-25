#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

namespace Radio {
  void init();
  void manage(void* userdata = NULL);
  void discover();
  void setPropState(unsigned int slot, const uint16_t *state);
  void assignProp(unsigned int slot, uint32_t accessory);
  void sendPropConnectionState();
}

#endif
