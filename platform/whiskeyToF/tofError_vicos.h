
#ifndef __tof_error_h__
#define __tof_error_h__

#include "whiskeyToF/vicos/vl53l1/core/inc/vl53l1_error_codes.h"

const char * const VL53L1ErrorToString(VL53L1_Error status);

#define return_if_error(status, msg, ...) {         \
    if(status != VL53L1_ERROR_NONE) {               \
      PRINT_NAMED_ERROR("", "%s(%d) " msg, VL53L1ErrorToString(status), status, ##__VA_ARGS__); \
      return status;                                \
    }                                               \
  }

#endif
