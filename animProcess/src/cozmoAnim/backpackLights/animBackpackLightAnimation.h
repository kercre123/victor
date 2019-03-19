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

#include "clad/types/ledTypes.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "coretech/common/shared/types.h"
#include <array>

#ifdef USES_CPPLITE
#define CLAD_VECTOR(ns) CppLite::Anki::Vector::ns
#else
#define CLAD_VECTOR(ns) ns
#endif

namespace Json {
  class Value;
}

namespace Anki {
namespace Vector {
namespace Anim {

namespace BackpackLightAnimation {

  // BackpackAnimation is just a container for the SetBackpackLights message
  struct BackpackAnimation {
    CLAD_VECTOR(RobotInterface)::SetBackpackLights lights;
  };
  
  bool DefineFromJSON(const Json::Value& jsonDef, BackpackAnimation& outAnim);

} // namespace BackpackLightAnimation

} // namespace Anim
} // namespace Vector
} // namespace Anki

#undef CLAD_VECTOR

#endif // __Anki_Cozmo_BackpackLights_BackpackLightAnimation_H__
