#include <stdint.h>

#ifndef __DTM_H
#define __DTM_H

namespace DTM {
  void start(void);
  void enterTestMode(uint8_t mode, uint8_t channel);
  void stop(void);
}

#endif
