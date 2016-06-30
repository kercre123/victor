#ifndef __TEMP_H
#define __TEMP_H

#include <stdint.h>

namespace Temp {
  void init();
  void manage();
  int32_t getTemp();
}

#endif
