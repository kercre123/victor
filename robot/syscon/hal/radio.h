#include <stdint.h>

#ifndef RADIO_H
#define RADIO_H

namespace Radio {
  void init();
  void manage(void* userdata = NULL);
  void discover();
}

#endif
