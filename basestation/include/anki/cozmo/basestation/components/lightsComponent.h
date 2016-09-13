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
#include "anki/common/basestation/math/point.h"
#include "anki/common/types.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/types/ledTypes.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include "json/json.h"
#include <map>
#include <list>
#include <set>

namespace Anki {
namespace Cozmo {

class Robot;

typedef std::array<u32, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS> ObjectLEDArray;
struct ObjectLights {
  ObjectLEDArray onColors;
  ObjectLEDArray offColors;
  ObjectLEDArray onPeriod_ms;
  ObjectLEDArray offPeriod_ms;
  ObjectLEDArray transitionOnPeriod_ms;
  ObjectLEDArray transitionOffPeriod_ms;
  std::array<s32, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS> offset;
  u32 rotationPeriod_ms;
  MakeRelativeMode makeRelative;
  Point2f relativePoint;
};

typedef std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS> BackpackLEDArray;
struct BackpackLights {
  BackpackLEDArray onColors;
  BackpackLEDArray offColors;
  BackpackLEDArray onPeriod_ms;
  BackpackLEDArray offPeriod_ms;
  BackpackLEDArray transitionOnPeriod_ms;
  BackpackLEDArray transitionOffPeriod_ms;
  std::array<s32,(size_t)LEDId::NUM_BACKPACK_LEDS> offset;
};

class LightsComponent : private Util::noncopyable
{
public:
  LightsComponent(Robot& robot);

  // TODO:(bn) handle robot lights here as well as cube

  void Update();

  // Tell this component to stop controlling these lights (e.g. because game is going to do it), or start
  // again. Disabling turns them all to "off"
  void SetEnableComponent(const bool enable);
  void SetEnableCubeLights(const ObjectID objectID, const bool enable);
  const bool AreAllCubesEnabled() const;
  const bool IsCubeEnabled(const ObjectID objectID) const;

  void SetInteractionObject(ObjectID objectID);
  void UnSetInteractionObject(ObjectID objectID);
  
  Result SetObjectLights(const ObjectID& objectID, const ObjectLights& lights);
  Result SetObjectLights(const ObjectID& objectID,
                         const WhichCubeLEDs whichLEDs,
                         const u32 onColor,
                         const u32 offColor,
                         const u32 onPeriod_ms,
                         const u32 offPeriod_ms,
                         const u32 transitionOnPeriod_ms,
                         const u32 transitionOffPeriod_ms,
                         const bool turnOffUnspecifiedLEDs,
                         const MakeRelativeMode makeRelative,
                         const Point2f& relativeToPoint,
                         const u32 rotationPeriod_ms);
  
  Result SetBackpackLights(const BackpackLights& lights);
  
  void SetHeadlight(bool on);
  
  void StartLoopingBackpackLights(const BackpackLights& lights);
  void StopLoopingBackpackLights();
  
  Result TurnOffObjectLights(const ObjectID& objectID);
  Result TurnOffBackpackLights();
  
  template<typename T>
  void HandleMessage(const T& msg);
  
private:

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
  std::map<CubeLightsState, ObjectLights> _stateToValues;
  
  enum class BackpackLightsState {
    OffCharger,
    Charging,
    Charged,
    BadCharger,
  };
  
  std::map<BackpackLightsState, BackpackLights> _backpackStateToValues;
  BackpackLightsState _curBackpackState = BackpackLightsState::OffCharger;

  struct ObjectInfo {
    ObjectInfo();

    bool enabled;

    // Desired state is what we want, currState is the last thing we've sent down (this avoids spamming too
    // many messages)
    
    CubeLightsState desiredState;
    CubeLightsState currState;
    CubeLightsState prevState;
    TimeStamp_t     lastObservedTime_ms;
    TimeStamp_t     startCurrStateTime;
    // When we are fading between states this keeps track of what the starting and ending states are
    std::pair<CubeLightsState, CubeLightsState> fadeFromTo;
    
    // The time this cube started flashing
    TimeStamp_t flashStartTime;
  };
  
  std::map< ObjectID, ObjectInfo > _cubeInfo;
  
  std::multiset<ObjectID> _interactionObjects;
  
  void UpdateToDesiredLights(const bool force = false);
  void UpdateToDesiredLights(const ObjectID objectID, const bool force = false);
  void SetLights(ObjectID object, CubeLightsState state, bool forceSet = false);
  
  void UpdateBackpackLights();

  // Returns true if we should change the lights from currState to nextState
  bool ShouldOverrideState(CubeLightsState currState, CubeLightsState nextState);
  
  // Returns true if we should fade between the two states
  bool FadeBetween(CubeLightsState from, CubeLightsState to,
                   std::pair<CubeLightsState, CubeLightsState>& fadeFromTo);
  
  void AddLightStateValues(CubeLightsState state, const Json::Value& data);
  void AddBackpackLightStateValues(BackpackLightsState state, const Json::Value& data);
  template<class T>
  T JsonColorValueToArray(const Json::Value& value);
  template<class T>
  T JsonValueToU32Array(const Json::Value& value);
  template<class T>
  T JsonValueToS32Array(const Json::Value& value);
  
  void RestorePrevStates();
  void RestorePrevState(const ObjectID objectID);
  void CheckAndUpdatePrevState(const ObjectID objectID);
  
  // Send a mesage to game to let them know the objects current light values
  // Will only send if _sendTransitionMessages is true
  void SendTransitionMessage(const ObjectID& objectID, const ObjectLights& values);
  
  bool CanSetObjectLights(const ObjectID& objectID);
  
  Result SetObjectLightsInternal(const ObjectID& objectID, const ObjectLights& values);
  Result SetObjectLightsInternal(const ObjectID& objectID,
                                 const WhichCubeLEDs whichLEDs,
                                 const u32 onColor,
                                 const u32 offColor,
                                 const u32 onPeriod_ms,
                                 const u32 offPeriod_ms,
                                 const u32 transitionOnPeriod_ms,
                                 const u32 transitionOffPeriod_ms,
                                 const bool turnOffUnspecifiedLEDs,
                                 const MakeRelativeMode makeRelative,
                                 const Point2f& relativeToPoint,
                                 const u32 rotationPeriod_ms);
  
  Result SetBackpackLightsInternal(const BackpackLights& lights);

  Robot& _robot;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  u32 _wakeupTime_ms    = 0;
  u32 _wakeupFadeOut_ms = 0;
  u32 _fadePeriod_ms    = 0;
  u32 _fadeTime_ms      = 0;
  u32 _sleepTime_ms     = 0;
  
  // Variables for making the cubes flash
  ObjectLEDArray _flashColor  = {{0,0,0,0}};
  u32 _flashPeriod_ms   = 0;
  u32 _flashTimes       = 0;
  
  
  bool _allCubesEnabled = true;
  
  bool _loopingBackpackLights = false;
  BackpackLights _currentLoopingBackpackLights;
  BackpackLights _currentBackpackLights;

  bool _sendTransitionMessages = false;
  
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
