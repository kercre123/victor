/**
* File: emr.h
*
* Author: Al Chaussee
* Date:   1/30/2018
*
* Description: Electronic medical(manufacturing) record written to the robot at the factory
*              DO NOT MODIFY this is shared across all processes and test fixtures
*
* Copyright: Anki, Inc. 2018
**/

#ifndef EMR_H
#define EMR_H

#include <stdint.h>

namespace Anki {
namespace Cozmo {

namespace Factory {

  union EMR {
    struct Fields {
      uint32_t ESN;
      uint32_t HW_VER;
      uint32_t MODEL;
      uint32_t LOT_CODE;
      uint32_t PLAYPEN_READY_FLAG; //fixture approves all previous testing. OK to run playpen
      uint32_t PLAYPEN_PASSED_FLAG;
      uint32_t PACKED_OUT_FLAG;
      uint32_t PACKED_OUT_DATE; //Unix time?
      uint32_t reserved[47];
      uint32_t playpenTestDisableMask;
      uint32_t playpen[8];
      uint32_t fixture[192];
    } fields;
    
    uint32_t data[256];
  };
  
  #ifdef __GNUC__
  static_assert(sizeof(EMR) == 1024);
  #else
  typedef char static_assertion_emr_size_check[(sizeof(EMR) == 1024) ? 1 : -1];
  #endif
}
}
}

#endif
