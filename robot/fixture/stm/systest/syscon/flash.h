#ifndef __FLASH_H
#define __FLASH_H

#include "vectors.h"

namespace Flash {
  void writeFlash(const void* targ, const void* src, int size);
  void eraseApplication(void);
  void writeFaultReason(FaultType reason);
};

#endif
