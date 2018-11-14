#ifndef __ENCODERS_H
#define __ENCODERS_H

#include "messages.h"

namespace Encoders {
  extern bool disabled;
  extern bool head_invalid;
  extern bool lift_invalid;
  extern int stale_count;
  
  void init();
  void start();
  void stop();
  void tick_start();
  void tick_end();
  void flip(uint32_t* &time_last, int32_t* &delta_last);
}

#endif
