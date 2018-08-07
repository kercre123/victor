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
#include "coretech/common/shared/types.h"
#include <array>

namespace Anki {
namespace Vector {
namespace BackpackLightAnimation {

using BackpackLEDArray = std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>;
struct BackpackAnimation {  
  BackpackLEDArray onColors;
  BackpackLEDArray offColors;
  BackpackLEDArray onPeriod_ms;
  BackpackLEDArray offPeriod_ms;
  BackpackLEDArray transitionOnPeriod_ms;
  BackpackLEDArray transitionOffPeriod_ms;
  std::array<s32,(size_t)LEDId::NUM_BACKPACK_LEDS> offset;
};

bool DefineFromJSON(const Json::Value& jsonDef, BackpackAnimation& outAnim);

} // namespace BackpackLightAnimation
} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_BackpackLights_BackpackLightAnimation_H__

