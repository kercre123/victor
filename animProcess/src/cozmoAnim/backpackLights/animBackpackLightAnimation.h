/**
 * File: backpackLightAnimation.h
 *
 * Authors: Kevin M. Karol
 * Created: 6/21/18
 *
 * Description: Definitions for building/parsing backpack lights
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Cozmo_BackpackLights_BackpackLightAnimation_H__
#define __Anki_Cozmo_BackpackLights_BackpackLightAnimation_H__

#include <array>

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/ledTypes.h"
#include "coretech/common/shared/types.h"

namespace Json {
class Value;
}

namespace Anki {
namespace Vector {
namespace Anim {

namespace BackpackLightAnimation {

// BackpackAnimation is just a container for the SetBackpackLights message
struct BackpackAnimation {
  RobotInterface::SetBackpackLights lights;
};

bool DefineFromJSON(const Json::Value& jsonDef, BackpackAnimation& outAnim);

}  // namespace BackpackLightAnimation

}  // namespace Anim
}  // namespace Vector
}  // namespace Anki

#endif  // __Anki_Cozmo_BackpackLights_BackpackLightAnimation_H__
