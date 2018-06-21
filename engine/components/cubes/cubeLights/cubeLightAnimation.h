/**
 * File: cubeLightAnimation.h
 *
 * Author: Kevin M. Karol
 * Created: 6/19/18
 *
 * Description: Defines the structs/classes related to cube lights in engine
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_CubeLightAnimation_fwd_H__
#define __Anki_Cozmo_Basestation_Components_CubeLightAnimation_fwd_H__

#include "clad/types/ledTypes.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/shared/types.h"
#include "json/json-forwards.h"

#include <array>
#include <list>

namespace Anki {
namespace Cozmo {
namespace CubeLightAnimation {

using ObjectLEDArray = std::array<u32, static_cast<int>(CubeConstants::NUM_CUBE_LEDS)>;
struct ObjectLights {
  ObjectLEDArray onColors{};
  ObjectLEDArray offColors{};
  ObjectLEDArray onPeriod_ms{};
  ObjectLEDArray offPeriod_ms{};
  ObjectLEDArray transitionOnPeriod_ms{};
  ObjectLEDArray transitionOffPeriod_ms{};
  std::array<s32, static_cast<int>(CubeConstants::NUM_CUBE_LEDS)> offset{};
  bool rotate = false;
  MakeRelativeMode makeRelative = MakeRelativeMode::RELATIVE_LED_MODE_OFF;
  Point2f relativePoint;
  
  bool operator==(const ObjectLights& other) const;
  bool operator!=(const ObjectLights& other) const { return !(*this == other);}
};

// A single light pattern (multiple light patterns make up an animation)
struct LightPattern
{
  // The name of this pattern (used for debugging)
  std::string name = "";
  
  // The object lights for this pattern
  ObjectLights lights;
  
  // How long this pattern should be played
  u32 duration_ms = 0;
  
  // Whether or not this pattern can be overridden by another pattern
  bool canBeOverridden = true;
  
  // Prints information about this pattern
  void Print() const;
};

using Animation = std::list<LightPattern>;

bool ParseCubeAnimationFromJson(const Json::Value& jsonRoot, Animation& outAnimation);
bool ParseJsonToPattern(const Json::Value& json, LightPattern& pattern);


} // namespace CubeLightAnimation
} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_Components_CubeLightAnimation_fwd_H__
