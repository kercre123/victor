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
#include "engine/components/bodyLightComponentTypes.h"
#include "clad/types/ledTypes.h"
#include "json/json.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <array>
#include <list>
#include <map>
#include <memory>

namespace Anki {
namespace Cozmo {

using BackpackLEDArray_V1 = std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS + 1>;
struct BackpackLights_V1 {
  BackpackLEDArray_V1 onColors;
  BackpackLEDArray_V1 offColors;
  BackpackLEDArray_V1 onPeriod_ms;
  BackpackLEDArray_V1 offPeriod_ms;
  BackpackLEDArray_V1 transitionOnPeriod_ms;
  BackpackLEDArray_V1 transitionOffPeriod_ms;
  std::array<s32,(size_t)LEDId::NUM_BACKPACK_LEDS + 1> offset;
};
  
  
using BackpackLEDArray = std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>;
struct BackpackLights {
  
  // Convert from V1 to V2 lights
  // Left arrow maps to LED_BACKPACK_0 "system" light.
  // Front, middle, back map to LED_BACKPACK_1 to 3.
  // Right arrow light is ignored.
  BackpackLights& operator=(const BackpackLights_V1& other) {
    size_t kNumLights = (size_t)LEDId::NUM_BACKPACK_LEDS;
    std::copy_n(other.onColors.begin(), kNumLights, onColors.begin());
    std::copy_n(other.offColors.begin(), kNumLights, offColors.begin());
    std::copy_n(other.onPeriod_ms.begin(), kNumLights, onPeriod_ms.begin());
    std::copy_n(other.offPeriod_ms.begin(), kNumLights, offPeriod_ms.begin());
    std::copy_n(other.transitionOnPeriod_ms.begin(), kNumLights, transitionOnPeriod_ms.begin());
    std::copy_n(other.transitionOffPeriod_ms.begin(), kNumLights, transitionOffPeriod_ms.begin());
    std::copy_n(other.offset.begin(), kNumLights, offset.begin());
    return *this;
  }
  
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
  
  void SetHeadlight(bool on);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  static const BackpackLights& GetOffBackpackLights();
  
  void ClearAllBackpackLightConfigs() { _backpackLightMap.clear(); }
  
  // Allows the caller to start a looping light pattern on Cozmo's backpack and get a handle to cancel it later.
  // Note that the source specified will be used in determining which pattern should be used when multiple have
  // been triggered.
  void StartLoopingBackpackLights(BackpackLights lights, BackpackLightSource source, BackpackLightDataLocator& lightLocator_out);
  
  // Related to the above start looping call, allows the caller to cancel a currently looping light pattern
  bool StopLoopingBackpackLights(const BackpackLightDataLocator& lightDataLocator);
  
  // General purpose call to set backpack lights. The light pattern will persist until this function is called again.
  // Uses a private source value with lower priority than the types specified in the above StartLooping call.
  void SetBackpackLights(const BackpackLights& lights);
  
private:
  
  Robot&                          _robot;
  
  std::list<Signal::SmartHandle>  _eventHandles;

  // Contains overall mapping of light config sources to list of configurations
  BackpackLightMap                _backpackLightMap;
  
  // Reference to the most recently used light configuration
  BackpackLightDataRefWeak        _curBackpackLightConfig;
  
  // Locator handle for the shared light configuration associated with SetBackpackLights above
  BackpackLightDataLocator        _sharedLightConfig{};
  
  // Reference to the container of backpack light animations
  BackpackLightAnimationContainer& _backpackLightAnimations;
  
  enum class BackpackLightsState {
    OffCharger,
    Charging,
    Charged,
    BadCharger,
  };
  
  BackpackLightsState             _curBackpackChargeState = BackpackLightsState::OffCharger;
  
  Result SetBackpackLightsInternal(const BackpackLights& lights);
  void StartLoopingBackpackLightsInternal(BackpackLights lights, BackpackLightSourceType source, BackpackLightDataLocator& lightLocator_out);
  
  const char* StateToString(const BackpackLightsState& state) const;
  
  void UpdateChargingLightConfig();
  static std::vector<BackpackLightSourceType> GetLightSourcePriority();
  BackpackLightDataRefWeak GetBestLightConfig();
  
};

}
}

#endif

