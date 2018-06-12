#ifndef __COMMS_H
#define __COMMS_H

#include "messages.h"

namespace Comms {
  void init(void);
  void reset(void);
  void tick(void);
  void sendVersion(void);
  void enqueue(PayloadId kind, const void* packet, int size);
}

#endif
