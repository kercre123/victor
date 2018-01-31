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

namespace Anki {
namespace Cozmo {

namespace Factory {

  union EMR {
    struct {
      uint32_t PACKED_OUT;
      uint32_t ROBOT1_PASSED;
      uint32_t ROBOT2_PASSED;
      uint32_t ROBOT3_PASSED;
      uint32_t PLAYPEN_PASSED;

      uint32_t fixture[128];

      uint32_t playpen[8];

      uint32_t reserved[115];
    };
    uint32_t data[256];
  };

  static_assert(sizeof(EMR) == 1024);
}
}
}

#endif