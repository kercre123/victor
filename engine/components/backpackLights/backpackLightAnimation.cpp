/**
* File: backpackLightAnimation.cpp
*
* Authors: Kevin M. Karol
* Created: 6/21/18
*
* Description: Definitions for building/parsing backpack lights
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/components/backpackLights/backpackLightAnimation.h"
#include "coretech/common/engine/jsonTools.h"

namespace Anki {
namespace Vector {
namespace BackpackLightAnimation {


bool DefineFromJSON(const Json::Value& jsonDef, BackpackAnimation& outAnim)
{  
  bool res = true;
  res &= JsonTools::GetColorValuesToArrayOptional(jsonDef, "onColors",  outAnim.onColors, true);
  res &= JsonTools::GetColorValuesToArrayOptional(jsonDef, "offColors", outAnim.offColors, true);
  res &= JsonTools::GetArrayOptional(jsonDef, "onPeriod_ms",            outAnim.onPeriod_ms);
  res &= JsonTools::GetArrayOptional(jsonDef, "offPeriod_ms",           outAnim.offPeriod_ms);
  res &= JsonTools::GetArrayOptional(jsonDef, "transitionOnPeriod_ms",  outAnim.transitionOnPeriod_ms);
  res &= JsonTools::GetArrayOptional(jsonDef, "transitionOffPeriod_ms", outAnim.transitionOffPeriod_ms);
  res &= JsonTools::GetArrayOptional(jsonDef, "offset",                 outAnim.offset);
  return res;
}

} // namespace BackpackLightAnimation
} // namespace Vector
} // namespace Anki
