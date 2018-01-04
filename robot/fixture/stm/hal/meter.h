#ifndef METER_H
#define METER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//current and voltage metering
namespace Meter
{
  void init();
  
  int32_t getVextVoltageMv(int oversample = 0);
  int32_t getVextCurrentMa(int oversample = 0);
  
  int32_t getVbatVoltageMv(int oversample = 0);
  int32_t getVbatCurrentMa(int oversample = 0);
  
  void setDoubleSpeed(bool enable = true);
}

#ifdef __cplusplus
}
#endif

#endif //METER_H
