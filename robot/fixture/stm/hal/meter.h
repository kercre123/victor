#ifndef METER_H
#define METER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "board.h"

//current and voltage metering
namespace Meter
{
  void init();
  
  int32_t getVoltageMv(pwr_e net, int oversample = 0); //unsupported nets return 0
  int32_t getCurrentMa(pwr_e net, int oversample = 0); //unsupported nets return 0
  int32_t getCurrentUa(pwr_e net, int oversample = 0); //UAMP only
  
  //void setDoubleSpeed(bool enable = true);
}

#ifdef __cplusplus
}
#endif

#endif //METER_H
