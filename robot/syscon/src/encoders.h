#ifndef __ENCODERS_H
#define __ENCODERS_H

#include "messages.h"

namespace Encoders {
  void init();
  void stop();
  void flip(uint32_t* &time_last, int32_t* &delta_last);
}

#endif
