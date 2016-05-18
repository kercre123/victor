#include <stdint.h>

#ifndef __DTM_H
#define __DTM_H

namespace DTM {
  void start(void);
  void testCommand(int32_t command, int32_t freq, int32_t length, int32_t payload);
  void stop(void);
}

#endif
