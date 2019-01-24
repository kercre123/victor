#include "whiskeyToF/tofError_vicos.h"

#include "whiskeyToF/vicos/vl53l1/core/inc/vl53l1_api.h"

const char * const VL53L1ErrorToString(VL53L1_Error status)
{
  static char buf[VL53L1_MAX_STRING_LENGTH];
  VL53L1_GetPalErrorString(status, buf);
  return buf;
}
