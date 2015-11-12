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

#include "anki/cozmo/basestation/viz/vizObjectBaseId.h"
#include <stdint.h>

namespace Anki {
namespace Cozmo {

// Base IDs for each VizObject type
const uint32_t VizObjectBaseID[(int)VizObjectType::NUM_VIZ_OBJECT_TYPES+1] = {
  0,    // VIZ_OJECT_ROBOT
  1000, // VIZ_OBJECT_CUBOID
  2000, // VIZ_OBJECT_RAMP
  3000, // VIZ_OBJECT_CHARGER
  4000, // VIZ_OJECT_PREDOCKPOSE
  7000, // VIZ_OBJECT_HUMAN_HEAD
  8000, // VIZ_OBJECT_CAMERA_FACE
  std::numeric_limits<uint32_t>::max() - 100 // Last valid object ID allowed
};

} // end namespace Cozmo
} // end namespace Anki

