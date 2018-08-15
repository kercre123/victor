/**
* File: vizObjectBaseId
*
* Author: damjan stulic
* Created: 9/16/15
*
* Description:
*
* Copyright: Anki, inc. 2015
*
*/

#include "engine/viz/vizObjectBaseId.h"
#include <stdint.h>

namespace Anki {
namespace Vector {

// Base IDs for each VizObject type
const uint32_t VizObjectBaseID[(int)VizObjectType::NUM_VIZ_OBJECT_TYPES+1] = {
  0,        // VIZ_OBJECT_ROBOT
  10000000, // VIZ_OBJECT_CUBOID
  20000000, // VIZ_OBJECT_RAMP
  30000000, // VIZ_OBJECT_CHARGER
  40000000, // VIZ_OBJECT_PREDOCKPOSE
  70000000, // VIZ_OBJECT_HUMAN_HEAD
  80000000, // VIZ_OBJECT_CAMERA_FACE
  std::numeric_limits<uint32_t>::max() - 100 // Last valid object ID allowed
};

} // end namespace Vector
} // end namespace Anki

