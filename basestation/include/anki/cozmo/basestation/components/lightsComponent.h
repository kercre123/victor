/**
 * File: lightsComponent.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-21
 *
 * Description: Central place for logic and control of robot and cube lights
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_LightsComponent_H__
#define __Anki_Cozmo_Basestation_Components_LightsComponent_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/types.h"
#include "clad/types/activeObjectTypes.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include "json/json.h"
#include <map>
#include <list>
#include <set>

namespace Anki {
namespace Cozmo {

class Robot;

class LightsComponent : private Util::noncopyable
{
public:
  LightsComponent(Robot& robot);

  // TODO:(bn) handle robot lights here as well as cube

  void Update();

  // Tell this component to stop controlling these lights (e.g. because game is going to do it), or start
  // again. Disabling turns them all to "off"
  void SetEnableComponent(bool enable);

  void SetInteractionObject(ObjectID objectID);
  void UnSetInteractionObject(ObjectID objectID);
  
  template<typename T>
  void HandleMessage(const T& msg);
  
private:
  
  typedef std::array<u32, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS> LEDArray;
  struct LightValues {
    LEDArray onColors;
    LEDArray offColors;
    LEDArray onPeriod_ms;
    LEDArray offPeriod_ms;
    LEDArray transitionOnPeriod_ms;
    LEDArray transitionOffPeriod_ms;
    LEDArray onOffset;
    LEDArray offOffset;
    u32 rotationPeriod_ms;
  };

  // The various states cube lights can be in
  enum class CubeLightsState {
    Invalid,
    Off,
    WakeUp,         // For first time power on and coming out of sleep
    WakeUpFadeOut,  // Ending of the wake up sequence
    Connected,      // We are connected to cube but can't see it or it has moved
    Visible,        // Have seen the cube and it hasn't moved
    Interacting,    // Actively doing something with the cube
    Sleep,
    Fade,
  };
  
  // Maps light states to actual light values for that state
  std::map<CubeLightsState, LightValues> _stateToValues;

  struct ObjectInfo {
    ObjectInfo();

    // Desired state is what we want, currState is the last thing we've sent down (this avoids spamming too
    // many messages)
    
    CubeLightsState desiredState;
    CubeLightsState currState;
    TimeStamp_t     lastObservedTime_ms;
    TimeStamp_t     startCurrStateTime;
    // When we are fading between states this keeps track of what the starting and ending states are
    std::pair<CubeLightsState, CubeLightsState> fadeFromTo;
  };
  
  std::map< ObjectID, ObjectInfo > _cubeInfo;
  
  std::multiset<ObjectID> _interactionObjects;
  
  void UpdateToDesiredLights();
  void SetLights(ObjectID object, CubeLightsState state);

  // Returns true if we should change the lights from currState to nextState
  bool ShouldOverrideState(CubeLightsState currState, CubeLightsState nextState);
  
  // Returns true if we should fade between the two states
  bool FadeBetween(CubeLightsState from, CubeLightsState to,
                   std::pair<CubeLightsState, CubeLightsState>& fadeFromTo);
  
  void AddLightStateValues(CubeLightsState state, const Json::Value& data);
  LEDArray JsonColorValueToArray(const Json::Value& value);
  LEDArray JsonValueToArray(const Json::Value& value);

  Robot& _robot;

  bool _enabled = true;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  u32 _wakeupTime_ms = 0;
  u32 _wakeupFadeOut_ms = 0;
  u32 _fadePeriod_ms = 0;
  u32 _fadeTime_ms = 0;
  
}; // class LightsComponent

  
inline void LightsComponent::SetInteractionObject(ObjectID objectID) {
  _interactionObjects.insert(objectID);
}
  
inline void LightsComponent::UnSetInteractionObject(ObjectID objectID) {
  auto iter = _interactionObjects.find(objectID);
  if(iter != _interactionObjects.end()) {
    _interactionObjects.erase(iter);
  }
}
  
} // namespace Cozmo
} // namespace Anki

#endif
