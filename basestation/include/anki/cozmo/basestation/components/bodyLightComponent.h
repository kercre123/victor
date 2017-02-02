/**
 * File: bodyLightComponent.h
 *
 * Author: Al Chaussee
 * Created: 1/23/2017
 *
 * Description: Manages various lights on Cozmo's body. 
 *              Currently this includes the backpack lights and headlight.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_BodyLightComponent_H__
#define __Anki_Cozmo_Basestation_Components_BodyLightComponent_H__

#include "anki/common/types.h"
#include "clad/types/activeObjectTypes.h"
#include "json/json.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <array>
#include <list>

namespace Anki {
namespace Cozmo {

using BackpackLEDArray = std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>;
struct BackpackLights {
  BackpackLEDArray onColors;
  BackpackLEDArray offColors;
  BackpackLEDArray onPeriod_ms;
  BackpackLEDArray offPeriod_ms;
  BackpackLEDArray transitionOnPeriod_ms;
  BackpackLEDArray transitionOffPeriod_ms;
  std::array<s32,(size_t)LEDId::NUM_BACKPACK_LEDS> offset;
};

class Robot;
class CozmoContext;
class BackpackLightAnimationContainer;

class BodyLightComponent : private Util::noncopyable
{
public:
  BodyLightComponent(Robot& robot, const CozmoContext* context);
  
  void Update();

  Result SetBackpackLights(const BackpackLights& lights);
  
  void TurnOffBackpackLights();
  
  void SetHeadlight(bool on);
  
  void StartLoopingBackpackLights(const BackpackLights& lights);
  void StopLoopingBackpackLights();
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  static const BackpackLights& GetOffBackpackLights();
  
private:
  
  Robot& _robot;
  
  // Reference to the container of backpack light animations
  BackpackLightAnimationContainer& _backpackLightAnimations;
  
  enum class BackpackLightsState {
    OffCharger,
    Charging,
    Charged,
    BadCharger,
  };
  
  Result SetBackpackLightsInternal(const BackpackLights& lights);
  
  const char* StateToString(const BackpackLightsState& state) const;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  BackpackLightsState _curBackpackState = BackpackLightsState::OffCharger;
  
  bool _loopingBackpackLights = false;
  BackpackLights _currentLoopingBackpackLights;
  BackpackLights _currentBackpackLights;
  
};

}
}

#endif

