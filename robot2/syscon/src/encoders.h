#ifndef __ENCODERS_H
#define __ENCODERS_H

#include "schema/messages.h"

namespace Encoders {
  void init();
  void flip(uint32_t* &time_last, int32_t* &delta_last);
}

#endif
