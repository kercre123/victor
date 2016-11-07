/**
 * File: lightsComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-05-21
 *
 * Description: Central place for logic and control of robot and cube lights
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/components/lightsComponent.h"

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/ledEncoding.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/fileUtils/fileUtils.h"
#include "util/cpuProfiler/cpuProfiler.h"

#define DEBUG_BLOCK_LIGHTS 0
#define DEBUG_BACKPACK_LIGHTS 0

namespace Anki {
namespace Cozmo {

static const BackpackLights kBackpackLightsOff = {
  .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0,0,0}},
  .offPeriod_ms           = {{0,0,0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0,0,0}},
  .offset                 = {{0,0,0,0,0}}
};

static const ObjectLights kCubeLightsOff = {
  .onColors               = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
  .onPeriod_ms            = {{0,0,0,0}},
  .offPeriod_ms           = {{0,0,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0,0}},
  .offset                 = {{0,0,0,0}},
  .rotationPeriod_ms      = 0,
  .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_OFF,
  .relativePoint          = {0,0}
};

LightsComponent::LightsComponent(Robot& robot)
  : _robot(robot),
  _currentBackpackLights(kBackpackLightsOff)
{
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::ObjectMoved>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::ObjectConnectionState>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotDelocalized>();
    
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetActiveObjectLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetAllActiveObjectLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetBackpackLEDs>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetHeadlight>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableLightStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCubeSleep>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCubeLightsStateTransitionMessages>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::FlashCurrentLightsState>();
  }
  
  if(robot.GetContextDataPlatform() != nullptr)
  {
    if(Util::FileUtils::FileExists(robot.GetContextDataPlatform()->pathToResource(Util::Data::Scope::Resources, "config/basestation/lightStates/lightStates.json")))
    {
      Json::Value json;
      if(robot.GetContextDataPlatform()->readAsJson(Util::Data::Scope::Resources, "config/basestation/lightStates/lightStates.json", json))
      {
        AddLightStateValues(CubeLightsState::WakeUp,        json["wakeUp"]);
        AddLightStateValues(CubeLightsState::WakeUpFadeOut, json["wakeUpFadeOut"]);
        AddLightStateValues(CubeLightsState::Connected,     json["connected"]);
        AddLightStateValues(CubeLightsState::Visible,       json["visible"]);
        AddLightStateValues(CubeLightsState::Interacting,   json["interacting"]);
        AddLightStateValues(CubeLightsState::Sleep,         json["sleep"]);
        
        _wakeupTime_ms    = json["wakeUpDuration_ms"].asUInt();
        _wakeupFadeOut_ms = json["wakeUpFadeOutDuration_ms"].asUInt();
        _fadePeriod_ms    = json["fadePeriod_ms"].asUInt();
        _fadeTime_ms      = json["fadeTransitionTime_ms"].asUInt();
        _sleepTime_ms     = json["sleepDuration_ms"].asUInt();
        
        _flashColor     = JsonColorValueToArray<ObjectLEDArray>(json["flashColor"]);
        _flashPeriod_ms = json["flashPeriod_ms"].asUInt();
        _flashTimes     = json["flashTimes"].asUInt();
        
        _backpackStateToValues.emplace(BackpackLightsState::OffCharger, kBackpackLightsOff);
        AddBackpackLightStateValues(BackpackLightsState::Charging,   json["backpack"]["charging"]);
        AddBackpackLightStateValues(BackpackLightsState::Charged,    json["backpack"]["charged"]);
        AddBackpackLightStateValues(BackpackLightsState::BadCharger, json["backpack"]["badCharger"]);
      }
    }
  }
}

void LightsComponent::AddLightStateValues(CubeLightsState state, const Json::Value& data)
{
  ObjectLights values = {
    .onColors               = JsonColorValueToArray<ObjectLEDArray>(data["onColors"]),
    .offColors              = JsonColorValueToArray<ObjectLEDArray>(data["offColors"]),
    .onPeriod_ms            = JsonValueToU32Array<ObjectLEDArray>(data["onPeriod_ms"]),
    .offPeriod_ms           = JsonValueToU32Array<ObjectLEDArray>(data["offPeriod_ms"]),
    .transitionOnPeriod_ms  = JsonValueToU32Array<ObjectLEDArray>(data["transitionOnPeriod_ms"]),
    .transitionOffPeriod_ms = JsonValueToU32Array<ObjectLEDArray>(data["transitionOffPeriod_ms"]),
    .offset                 = JsonValueToS32Array<std::array<s32, (size_t)ActiveObjectConstants::NUM_CUBE_LEDS>>(data["offset"]),
    .rotationPeriod_ms      = data["rotationPeriod_ms"].asUInt(),
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_OFF,
    .relativePoint          = {0,0}
  };
  _stateToValues.emplace(state, values);
}

void LightsComponent::AddBackpackLightStateValues(BackpackLightsState state, const Json::Value& data)
{
  BackpackLights values = {
    .onColors               = JsonColorValueToArray<BackpackLEDArray>(data["onColors"]),
    .offColors              = JsonColorValueToArray<BackpackLEDArray>(data["offColors"]),
    .onPeriod_ms            = JsonValueToU32Array<BackpackLEDArray>(data["onPeriod_ms"]),
    .offPeriod_ms           = JsonValueToU32Array<BackpackLEDArray>(data["offPeriod_ms"]),
    .transitionOnPeriod_ms  = JsonValueToU32Array<BackpackLEDArray>(data["transitionOnPeriod_ms"]),
    .transitionOffPeriod_ms = JsonValueToU32Array<BackpackLEDArray>(data["transitionOffPeriod_ms"]),
    .offset                 = JsonValueToS32Array<std::array<s32, (size_t)LEDId::NUM_BACKPACK_LEDS>>(data["offset"]),
  };
  _backpackStateToValues.emplace(state, values);
}

template<class T>
T LightsComponent::JsonValueToU32Array(const Json::Value& value)
{
  T arr;
  for(u8 i = 0; i < (int)arr.size(); ++i)
  {
    arr[i] = value[i].asUInt();
  }
  return arr;
}

template<class T>
T LightsComponent::JsonValueToS32Array(const Json::Value& value)
{
  T arr;
  for(u8 i = 0; i < arr.size(); ++i)
  {
    arr[i] = value[i].asInt();
  }
  return arr;
}

template <class T>
T LightsComponent::JsonColorValueToArray(const Json::Value& value)
{
  T arr;
  for(u8 i = 0; i < (int)arr.size(); ++i)
  {
    
    ColorRGBA color(value[i][0].asFloat(),
                    value[i][1].asFloat(),
                    value[i][2].asFloat(),
                    value[i][3].asFloat());
    arr[i] = color.AsRGBA();
  }
  return arr;
}

void LightsComponent::Update()
{
  ANKI_CPU_PROFILE("LightsComponent::Update");
  
  const TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  // We are going from delocalized to localized
  bool doRelocalizedUpdate = false;
  if(_robotDelocalized && _robot.IsLocalized())
  {
    doRelocalizedUpdate = true;
    _robotDelocalized = false;
  }
  
  for( auto& cubeInfoPair : _cubeInfo )
  {
    // Note this check is done before the "Is cubeInfoPair enabled" check so we can update the
    // prevState should this cube be disabled
    if(doRelocalizedUpdate)
    {
      // Set the desired state to visible for all cubes with known poses
      const ObservableObject* object = _robot.GetBlockWorld().GetObjectByID(cubeInfoPair.first);
      if(object != nullptr && !object->IsPoseStateUnknown())
      {
        if(ShouldOverrideState(cubeInfoPair.second.desiredState, CubeLightsState::Visible))
        {
          // Update our prevState if we are disabled so if we are moved while disabled our state will
          // be properly restored
          if(!cubeInfoPair.second.enabled)
          {
            cubeInfoPair.second.prevState = CubeLightsState::Visible;
          }
          
          cubeInfoPair.second.desiredState = CubeLightsState::Visible;
        }
      }
    }
  
    if(!cubeInfoPair.second.enabled)
    {
      continue;
    }
    
    bool isFlashing = cubeInfoPair.second.flashStartTime > 0;
    bool doForceLightUpdate = false;
    
    // The total duration of flashing is flashPeriod*2 (onPeriod and offPeriod are both set to flashPeriod)
    // * (the number of times the lights need to flash)
    if(isFlashing &&
       (currTime - cubeInfoPair.second.flashStartTime) > (_flashPeriod_ms*2 * _flashTimes))
    {
      cubeInfoPair.second.flashStartTime = 0;
      
      // When coming out of flashing we need to force update this cubes lights to stop the flashing
      doForceLightUpdate = true;
      isFlashing = false;
    }
    
    
    ///////
    // newState selection
    ///////
  
    CubeLightsState newState = cubeInfoPair.second.desiredState;
    CubeLightsState currState = cubeInfoPair.second.currState;
    const TimeStamp_t timeDiff = currTime - cubeInfoPair.second.startCurrStateTime;
    auto& fadeFromTo = cubeInfoPair.second.fadeFromTo;
    
    bool endingFade = false;
    
    ///////
    // Check for interactions to set the light state
    
    // If we're interacting with this object, put it in interacting state
    // but only if we are not already fading to an interacting state
    if(_interactionObjects.count(cubeInfoPair.first) > 0 &&
       fadeFromTo.second != CubeLightsState::Interacting)
    {
      newState = CubeLightsState::Interacting;
    }
    // Otherwise if we are just finished interacting with the cube and we aren't carrying the cube
    // go back to visible
    else if(_interactionObjects.count(cubeInfoPair.first) == 0 &&
            currState == CubeLightsState::Interacting &&
            _robot.GetCarryingObject() != cubeInfoPair.first)
    {
      newState = CubeLightsState::Visible;
    }
    
    
    ///////
    // Check for customPatterns that override other interactions
    
    // If a custom pattern has been set, fade to it
    if(cubeInfoPair.second.currState != CubeLightsState::CustomPattern
       && cubeInfoPair.second.currState != CubeLightsState::Fade
       && _customLightPatterns.find(cubeInfoPair.first) != _customLightPatterns.end()){
      newState = CubeLightsState::CustomPattern;
    }
    // If a custom pattern has been removed, fade back to the previous state
    else if(cubeInfoPair.second.currState != CubeLightsState::Fade
       && cubeInfoPair.second.currState == CubeLightsState::CustomPattern
       && _customLightPatterns.find(cubeInfoPair.first) == _customLightPatterns.end()){
      newState = cubeInfoPair.second.prevState;
    }
    
    ///////
    // Handle WakeUp and sleep transitions
    
    if(currState == CubeLightsState::WakeUp &&
       (timeDiff > _wakeupTime_ms))
    {
      newState = CubeLightsState::WakeUpFadeOut;
    }
    else if(currState == CubeLightsState::WakeUpFadeOut &&
            (timeDiff > _wakeupFadeOut_ms))
    {
      newState = CubeLightsState::Connected;
    }
    else if(currState == CubeLightsState::Fade &&
            (timeDiff > _fadeTime_ms + _fadePeriod_ms*2)) // Multiply fadePeriod by 2 for both onPeriod and offPeriod
    {
      currState = fadeFromTo.first;
      newState = fadeFromTo.second;
      endingFade = true;
      fadeFromTo.first = CubeLightsState::Invalid;
      fadeFromTo.second = CubeLightsState::Invalid;
    }
    else if(currState == CubeLightsState::Sleep &&
            timeDiff > _sleepTime_ms)
    {
      newState = CubeLightsState::Off;
    }
    
    
    ///////
    // Handle fades between all states
    // add any other state transitions above here
    
    // Only fade if we did not just end a fade and we can fade from current state to the new state
    if(!endingFade && FadeBetween(currState, newState, fadeFromTo))
    {
      PRINT_CH_INFO("LightsComponent", "LightsComponent.Update.FadeBetween", "Fading object %d from %d to %d",
                    cubeInfoPair.first.GetValue(),
                    fadeFromTo.first,
                    fadeFromTo.second);
      newState = CubeLightsState::Fade;
    }
    
    ///////
    // end newState selection
    ///////
    
    if(ShouldOverrideState(currState, newState))
    {
      cubeInfoPair.second.desiredState = newState;
    }
    
    // Only update lights if they aren't being flashed
    if(!isFlashing)
    {
      UpdateToDesiredLights(cubeInfoPair.first, doForceLightUpdate);
    }
  }
  
  UpdateBackpackLights();
}

void LightsComponent::UpdateToDesiredLights(const ObjectID objectID, const bool force)
{
  const auto cube = _cubeInfo.find(objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.UpdateToDesiredLights", "No object %d in _cubeInfo map", objectID.GetValue());
    return;
  }
  if( cube->second.currState != cube->second.desiredState || force) {
    SetLights( objectID, cube->second.desiredState, force );
  }
}

void LightsComponent::UpdateToDesiredLights(const bool force)
{
  for( const auto& cubeInfoPair : _cubeInfo ) {
    if( cubeInfoPair.second.currState != cubeInfoPair.second.desiredState || force) {
      SetLights( cubeInfoPair.first, cubeInfoPair.second.desiredState, force );
    }
  }
}

void LightsComponent::UpdateBackpackLights()
{
  BackpackLightsState state = BackpackLightsState::OffCharger;
  if(_robot.IsOnCharger())
  {
    if(_robot.IsChargerOOS())
    {
      state = BackpackLightsState::BadCharger;
    }
    else if(_robot.IsCharging())
    {
      state = BackpackLightsState::Charging;
    }
    else
    {
      state = BackpackLightsState::Charged;
    }
  }
  
  if(state != _curBackpackState)
  {
    _curBackpackState = state;
    
    // If we are going to the off state go back to what the lights were before
    if(state == BackpackLightsState::OffCharger)
    {
      SetBackpackLightsInternal(_currentBackpackLights);
    }
    else
    {
      SetBackpackLightsInternal(_backpackStateToValues[state]);
    }
  }
}

bool LightsComponent::ShouldOverrideState(CubeLightsState currState, CubeLightsState nextState)
{
  // We should never transition to Invalid, override the Fade or custom pattern states since they manages themselves,
  // or override WakeUp with anything other than WakeUpFadeOut
  if(nextState == CubeLightsState::Invalid ||
     currState == CubeLightsState::Fade ||
     currState == CubeLightsState::CustomPattern ||
     (currState == CubeLightsState::WakeUp && nextState != CubeLightsState::WakeUpFadeOut))
  {
    return false;
  }

  return true;
}

bool LightsComponent::FadeBetween(CubeLightsState from, CubeLightsState to,
                                  std::pair<CubeLightsState, CubeLightsState>& fadeFromTo)
{
  if(from == to || from == CubeLightsState::Fade || to == CubeLightsState::Fade)
  {
    return false;
  }
  
  // These are all the state transitions that we should fade between really only works well
  // when fading from/to states that are solid colors
  if((from == CubeLightsState::Visible      && to == CubeLightsState::Interacting) ||
     (from == CubeLightsState::Interacting  && to == CubeLightsState::Visible) ||
     (from == CubeLightsState::Visible      && to == CubeLightsState::Connected) ||
     (from == CubeLightsState::Interacting  && to == CubeLightsState::Connected) ||
     (from == CubeLightsState::Off          && to != CubeLightsState::WakeUp))
  {
    fadeFromTo.first = from;
    fadeFromTo.second = to;
  
    // If we are fading to connected than we actually want to fade the lights off but
    // the actual fadeTo state is still connected
    if(to == CubeLightsState::Connected)
    {
      to = CubeLightsState::Off;
    }
    
    // Values for dynamic fade
    // offPeriod is multiplied by 2 to ensure the fade lights don't loop before we transition out of fade
    ObjectLights fadeValues = {
      .onColors               = _stateToValues[from].onColors,
      .offColors              = _stateToValues[to].onColors,
      .onPeriod_ms            = {{_fadePeriod_ms,_fadePeriod_ms,_fadePeriod_ms,_fadePeriod_ms}},
      .offPeriod_ms           = {{_fadePeriod_ms*2,_fadePeriod_ms*2,_fadePeriod_ms*2,_fadePeriod_ms*2}},
      .transitionOnPeriod_ms  = {{0,0,0,0}},
      .transitionOffPeriod_ms = {{_fadeTime_ms,_fadeTime_ms,_fadeTime_ms,_fadeTime_ms}},
      .offset                 = {{0,0,0,0}},
      .rotationPeriod_ms      = 0,
      .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_OFF,
      .relativePoint          = {0,0}
    };
    _stateToValues[CubeLightsState::Fade] = fadeValues;
    
    return true;
  }
  return false;
}

void LightsComponent::SetEnableComponent(const bool enable)
{
  SetEnableCubeLights(-1, enable);
}

void LightsComponent::SetEnableCubeLights(const ObjectID objectID, const bool enable)
{
  auto enableHelper = [this, enable](ObjectID id)
  {
    PRINT_CH_INFO("LightsComponent", "LightsComponent.SetEnableCubeLights", "Setting object %d to %s",
                     id.GetValue(), (enable ? "enabled" : "disabled"));
    
    auto cube = _cubeInfo.find(id);
    if(cube == _cubeInfo.end())
    {
      PRINT_NAMED_WARNING("LightsComponent.SetEnableCubeLights.CubeNotFound",
                          "No object with id %d in _cubeInfo map",
                          id.GetValue());
      return;
    }
    
    if(!enable)
    {
      // Always force desiredState to off (even if currently disabled) otherwise UpdateToDesiredLights()
      // will force the light back on
      cube->second.desiredState = CubeLightsState::Off;
    }
    else if(!cube->second.enabled && enable)
    {
      RestorePrevState(id);
    }
  
    cube->second.enabled = enable;
  };

  if(objectID == -1)
  {
    _allCubesEnabled = enable;
    for(auto& cube : _cubeInfo)
    {
      enableHelper(cube.first);
    }
  }
  else
  {
    enableHelper(objectID);
  }
  
  UpdateToDesiredLights(true);
}


const bool LightsComponent::AreAllCubesEnabled() const
{
  bool res = true;
  for(const auto& cube : _cubeInfo)
  {
    res &= cube.second.enabled;
  }
  return res;
  
}

const bool LightsComponent::IsCubeEnabled(const ObjectID objectID) const
{
  const auto cube = _cubeInfo.find(objectID);
  if(cube != _cubeInfo.end())
  {
    return cube->second.enabled;
  }
  return false;
}

void LightsComponent::SetLights(ObjectID object, CubeLightsState state, bool forceSet)
{
  auto cube = _cubeInfo.find(object);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.SetLights.CubeNotFound",
                        "Trying to set lights for object %d but no object found in _cubeInfo map",
                        object.GetValue());
    return;
  }
  
  if(!forceSet && cube->second.currState == state)
  {
    return;
  }
  
  cube->second.startCurrStateTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();;
  
  // Don't update prevState if our current state is sleep
  if(cube->second.currState != CubeLightsState::Sleep &&
     cube->second.currState != CubeLightsState::Off)
  {
    cube->second.prevState = cube->second.currState;
  }
  
  cube->second.currState = state;
  
  PRINT_CH_INFO("LightsComponent", "LightsComponent.SetLights.ChangeState",
                "%s object %d to state %d",
                (forceSet ? "Force setting" : "Setting"),
                object.GetValue(),
                state);
  
  const ObjectLights& values = _stateToValues[state];
  SetObjectLightsInternal(object, values);
}
  
bool LightsComponent::SetCustomLightPattern(ObjectID objectID, ObjectLights pattern)
{
  auto insertionResult = _customLightPatterns.insert(std::make_pair(objectID, pattern));
  return insertionResult.second;
}


bool LightsComponent::ClearCustomLightPattern(ObjectID objectID)
{
  auto count = _customLightPatterns.erase(objectID);
  return count != 0;
}


template<>
void LightsComponent::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  if( msg.objectFamily != ObjectFamily::LightCube ) {
    return;
  }
  
  // Safe to do automatic insertion into map since we know we are observing a cube
  _cubeInfo[msg.objectID].lastObservedTime_ms = msg.timestamp;
  
  // If all cubes have been disabled then make sure to also disable new cubes that we connect to
  if(!_allCubesEnabled)
  {
    _cubeInfo[msg.objectID].enabled = false;
  }
  
  if(ShouldOverrideState(_cubeInfo[msg.objectID].desiredState, CubeLightsState::Visible))
  {
    // Update our prevState if we are disabled so if we are moved while disabled our state will
    // be properly restored
    if(!_cubeInfo[msg.objectID].enabled)
    {
      _cubeInfo[msg.objectID].prevState = CubeLightsState::Visible;
    }
    
    _cubeInfo[msg.objectID].desiredState = CubeLightsState::Visible;
  }
}

template<>
void LightsComponent::HandleMessage(const ObjectMoved& msg)
{
  auto cube = _cubeInfo.find(msg.objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.HandleObjectMoved",
                        "Got object moved message from object %d but no matching object in _cubeInfo map",
                        msg.objectID);
    return;
  }
  
  if( ShouldOverrideState( cube->second.desiredState, CubeLightsState::Connected ) )
  {
    // Update our prevState if we are disabled so if we are moved while disabled our state will
    // be properly restored
    if(!cube->second.enabled)
    {
      cube->second.prevState = CubeLightsState::Connected;
    }
    
    cube->second.desiredState = CubeLightsState::Connected;
  }
}

template<>
void LightsComponent::HandleMessage(const ObjectConnectionState& msg)
{
  // Note: A new objectID is automatically added to the _cubeInfo map if one doesn't
  // already exist
  if( msg.connected &&
     (msg.device_type == ActiveObjectType::OBJECT_CUBE1 ||
      msg.device_type == ActiveObjectType::OBJECT_CUBE2 ||
      msg.device_type == ActiveObjectType::OBJECT_CUBE3) &&
     ShouldOverrideState( _cubeInfo[msg.objectID].desiredState, CubeLightsState::WakeUp ) )
  {
    _cubeInfo[msg.objectID].desiredState = CubeLightsState::WakeUp;
    
    // If all cubes have been disabled then make sure to also disable new cubes that we connect to
    if(!_allCubesEnabled)
    {
      _cubeInfo[msg.objectID].enabled = false;
    }
  }
}
  
  
template<>
void LightsComponent::HandleMessage(const ExternalInterface::RobotDelocalized& msg)
{
  _robotDelocalized = true;
  
  // Set light states back to default since Cozmo has lost track of them
  for(auto& cube: _cubeInfo)
  {
    if( ShouldOverrideState( cube.second.desiredState, CubeLightsState::Connected ) )
    {
      // Update our prevState if we are disabled so if we are moved while disabled our state will
      // be properly restored
      if(!cube.second.enabled)
      {
        cube.second.prevState = CubeLightsState::Connected;
      }
      
      cube.second.desiredState = CubeLightsState::Connected;
    }
  }
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::EnableLightStates& msg)
{
  if(msg.objectID == -1)
  {
    SetEnableComponent(msg.enable);
  }
  else
  {
    SetEnableCubeLights(msg.objectID, msg.enable);
  }
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::EnableCubeSleep& msg)
{
  if(msg.enable)
  {
    for(auto& pair : _cubeInfo)
    {
      pair.second.desiredState = CubeLightsState::Sleep;
    }
  }
  else
  {
    RestorePrevStates();
  }
  // Make sure we are enabled
  SetEnableComponent(true);
  
  UpdateToDesiredLights();
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::EnableCubeLightsStateTransitionMessages& msg)
{
  _sendTransitionMessages = msg.enable;
  
  // Send messages for all objects so that game knows their current state
  for(const auto& pair : _cubeInfo)
  {
    SendTransitionMessage(pair.first, _stateToValues[pair.second.currState]);
  }
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::FlashCurrentLightsState& msg)
{
  auto cube = _cubeInfo.find(msg.objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_CH_INFO("LightsComponent", "LightsComponent.FlashCurrentLights.InvalidObjectID",
                  "No object with id %d", msg.objectID);
    return;
  }
  
  if(!cube->second.enabled)
  {
    PRINT_CH_INFO("LightsComponent", "LightsComponent.FlashCurrentLights.CubeNotEnabled",
                  "Object %d is not enabled (game has control of lights)", msg.objectID);
    return;
  }
  
  cube->second.flashStartTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  const ObjectLights flashLights = {
    .onColors               = _flashColor,
    .offColors              = _stateToValues[cube->second.currState].onColors,
    .onPeriod_ms            = {{_flashPeriod_ms, _flashPeriod_ms, _flashPeriod_ms, _flashPeriod_ms}},
    .offPeriod_ms           = {{_flashPeriod_ms, _flashPeriod_ms, _flashPeriod_ms, _flashPeriod_ms}},
    .transitionOnPeriod_ms  = {{0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0}},
    .offset                 = {{0,0,0,0}},
    .rotationPeriod_ms      = 0,
    .makeRelative           = MakeRelativeMode::RELATIVE_LED_MODE_OFF,
    .relativePoint          = {0,0}
  };
  
  SetObjectLightsInternal(msg.objectID, flashLights);
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::SetAllActiveObjectLEDs& msg)
{
  const ObjectLights lights {
    .onColors               = msg.onColor,
    .offColors              = msg.offColor,
    .onPeriod_ms            = msg.onPeriod_ms,
    .offPeriod_ms           = msg.offPeriod_ms,
    .transitionOnPeriod_ms  = msg.transitionOnPeriod_ms,
    .transitionOffPeriod_ms = msg.transitionOffPeriod_ms,
    .offset                 = msg.offset,
    .rotationPeriod_ms      = msg.rotationPeriod_ms,
    .makeRelative           = msg.makeRelative,
    .relativePoint          = {msg.relativeToX,msg.relativeToY}
  };

  SetObjectLights(msg.objectID, lights);
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::SetActiveObjectLEDs& msg)
{
  SetObjectLights(msg.objectID,
                  msg.whichLEDs,
                  msg.onColor,
                  msg.offColor,
                  msg.onPeriod_ms,
                  msg.offPeriod_ms,
                  msg.transitionOnPeriod_ms,
                  msg.transitionOffPeriod_ms,
                  msg.turnOffUnspecifiedLEDs,
                  msg.makeRelative,
                  {msg.relativeToX, msg.relativeToY},
                  msg.rotationPeriod_ms);
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::SetBackpackLEDs& msg)
{
  const BackpackLights lights = {
    .onColors               = msg.onColor,
    .offColors              = msg.offColor,
    .onPeriod_ms            = msg.onPeriod_ms,
    .offPeriod_ms           = msg.offPeriod_ms,
    .transitionOnPeriod_ms  = msg.transitionOnPeriod_ms,
    .transitionOffPeriod_ms = msg.transitionOffPeriod_ms,
    .offset                 = msg.offset
  };
  
  SetBackpackLights(lights);
}

template<>
void LightsComponent::HandleMessage(const ExternalInterface::SetHeadlight& msg)
{
  SetHeadlight(msg.enable);
}

bool LightsComponent::CanSetObjectLights(const ObjectID& objectID)
{
  if(IsCubeEnabled(objectID))
  {
    PRINT_CH_INFO("LightsComponent", "LightsComponent.CanSetCubeLights.LightsComponentEnabled",
                  "Not setting lights while cube lights for object %d are enabled by engine", objectID.GetValue());
    return false;
  }
  return true;
}

Result LightsComponent::SetObjectLights(const ObjectID& objectID, const ObjectLights& lights)
{
  if(CanSetObjectLights(objectID))
  {
    return SetObjectLightsInternal(objectID, lights);
  }
  return RESULT_FAIL;
}

Result LightsComponent::SetObjectLights(const ObjectID& objectID,
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
                                        const u32 rotationPeriod_ms)
{
  if(CanSetObjectLights(objectID))
  {
    return SetObjectLightsInternal(objectID,
                                   whichLEDs,
                                   onColor,
                                   offColor,
                                   onPeriod_ms,
                                   offPeriod_ms,
                                   transitionOnPeriod_ms,
                                   transitionOffPeriod_ms,
                                   turnOffUnspecifiedLEDs,
                                   makeRelative,
                                   relativeToPoint,
                                   rotationPeriod_ms);
  }
  return RESULT_FAIL;
}

Result LightsComponent::SetBackpackLights(const BackpackLights& lights)
{
  _currentBackpackLights = lights;

  // If we are looping backpack lights or our current backpack light state is not off
  // don't actually set the lights but keep track of what they were trying to be set
  // to so we can set them to it when the looping stops
  if(_loopingBackpackLights || _curBackpackState != BackpackLightsState::OffCharger)
  {
    return RESULT_OK;
  }
  
  return SetBackpackLightsInternal(lights);
}


ActiveObject* GetActiveObjectInAnyFrame(BlockWorld& blockWorld, const ObjectID& objectID)
{
  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.SetFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
  filter.AddAllowedID(objectID);
  ActiveObject* activeObject = dynamic_cast<ActiveObject*>(blockWorld.FindMatchingObject(filter));
  
  return activeObject;
}

Result LightsComponent::SetObjectLightsInternal(const ObjectID& objectID, const ObjectLights& values)
{
  ActiveObject* activeObject = GetActiveObjectInAnyFrame(_robot.GetBlockWorld(), objectID);
  
  if(activeObject == nullptr) {
    PRINT_NAMED_ERROR("Robot.SetObjectLights", "Null active object pointer.");
    return RESULT_FAIL_INVALID_OBJECT;
  } else {
    
    activeObject->SetLEDs(values.onColors,
                          values.offColors,
                          values.onPeriod_ms,
                          values.offPeriod_ms,
                          values.transitionOnPeriod_ms,
                          values.transitionOffPeriod_ms,
                          values.offset);
    
    
    ActiveCube* activeCube = dynamic_cast<ActiveCube*>(activeObject);
    if(activeCube != nullptr) {
      // NOTE: if make relative mode is "off", this call doesn't do anything:
      activeCube->MakeStateRelativeToXY(values.relativePoint, values.makeRelative);
    } else if (values.makeRelative != MakeRelativeMode::RELATIVE_LED_MODE_OFF) {
      PRINT_NAMED_WARNING("Robot.SetObjectLights.MakeRelativeOnNonCube", "");
      return RESULT_FAIL;
    }
    
    std::array<Anki::Cozmo::LightState, 4> lights;
    ASSERT_NAMED((int)ActiveObjectConstants::NUM_CUBE_LEDS == 4, "Robot.wrong.number.of.cube.ligths");
    for (int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i){
      const ActiveObject::LEDstate& ledState = activeObject->GetLEDState(i);
      lights[i].onColor  = ENCODED_COLOR(ledState.onColor);
      lights[i].offColor = ENCODED_COLOR(ledState.offColor);
      lights[i].onFrames  = MS_TO_LED_FRAMES(ledState.onPeriod_ms);
      lights[i].offFrames = MS_TO_LED_FRAMES(ledState.offPeriod_ms);
      lights[i].transitionOnFrames  = MS_TO_LED_FRAMES(ledState.transitionOnPeriod_ms);
      lights[i].transitionOffFrames = MS_TO_LED_FRAMES(ledState.transitionOffPeriod_ms);
      lights[i].offset = MS_TO_LED_FRAMES(ledState.offset);
      
      if(DEBUG_BLOCK_LIGHTS)
      {
         PRINT_NAMED_DEBUG("SetObjectLights(2)",
                           "LED %u, onColor 0x%x (0x%x), offColor 0x%x (0x%x), onFrames 0x%x (%ums), "
                           "offFrames 0x%x (%ums), transOnFrames 0x%x (%ums), transOffFrames 0x%x (%ums), offset 0x%x (%ums)",
                           i, lights[i].onColor, ledState.onColor.AsRGBA(),
                           lights[i].offColor, ledState.offColor.AsRGBA(),
                           lights[i].onFrames, ledState.onPeriod_ms,
                           lights[i].offFrames, ledState.offPeriod_ms,
                           lights[i].transitionOnFrames, ledState.transitionOnPeriod_ms,
                           lights[i].transitionOffFrames, ledState.transitionOffPeriod_ms,
                           lights[i].offset, ledState.offset);
      }
      
    }
    
    if( DEBUG_BLOCK_LIGHTS ) {
      PRINT_NAMED_DEBUG("Robot.SetObjectLights.Set2",
                        "Setting lights for object %d (activeID %d)",
                        objectID.GetValue(), activeObject->GetActiveID());
    }
    
    SendTransitionMessage(objectID, values);

    _robot.SendMessage(RobotInterface::EngineToRobot(SetCubeGamma(activeObject->GetLEDGamma())));
    _robot.SendMessage(RobotInterface::EngineToRobot(CubeID((uint32_t)activeObject->GetActiveID(),
                                                            MS_TO_LED_FRAMES(values.rotationPeriod_ms))));
    return _robot.SendMessage(RobotInterface::EngineToRobot(CubeLights(lights)));
  }
  
}

Result LightsComponent::SetObjectLightsInternal(const ObjectID& objectID,
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
                                                const u32 rotationPeriod_ms)
{
  ActiveObject* activeObject = GetActiveObjectInAnyFrame(_robot.GetBlockWorld(), objectID);
  
  if(activeObject == nullptr) {
    PRINT_NAMED_ERROR("Robot.SetObjectLights", "Null active object pointer.");
    return RESULT_FAIL_INVALID_OBJECT;
  } else {
    
    WhichCubeLEDs rotatedWhichLEDs = whichLEDs;
    
    ActiveCube* activeCube = dynamic_cast<ActiveCube*>(activeObject);
    if(activeCube != nullptr) {
      // NOTE: if make relative mode is "off", this call doesn't do anything:
      rotatedWhichLEDs = activeCube->MakeWhichLEDsRelativeToXY(whichLEDs, relativeToPoint, makeRelative);
    } else if (makeRelative != MakeRelativeMode::RELATIVE_LED_MODE_OFF) {
      PRINT_NAMED_WARNING("Robot.SetObjectLights.MakeRelativeOnNonCube", "");
      return RESULT_FAIL;
    }
    
    
    activeObject->SetLEDs(rotatedWhichLEDs, onColor, offColor, onPeriod_ms, offPeriod_ms,
                          transitionOnPeriod_ms, transitionOffPeriod_ms, 0,
                          turnOffUnspecifiedLEDs);
    
    std::array<Anki::Cozmo::LightState, 4> lights;
    ASSERT_NAMED((int)ActiveObjectConstants::NUM_CUBE_LEDS == 4, "Robot.wrong.number.of.cube.ligths");
    for (int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i){
      const ActiveObject::LEDstate& ledState = activeObject->GetLEDState(i);
      lights[i].onColor  = ENCODED_COLOR(ledState.onColor);
      lights[i].offColor = ENCODED_COLOR(ledState.offColor);
      lights[i].onFrames  = MS_TO_LED_FRAMES(ledState.onPeriod_ms);
      lights[i].offFrames = MS_TO_LED_FRAMES(ledState.offPeriod_ms);
      lights[i].transitionOnFrames  = MS_TO_LED_FRAMES(ledState.transitionOnPeriod_ms);
      lights[i].transitionOffFrames = MS_TO_LED_FRAMES(ledState.transitionOffPeriod_ms);
      lights[i].offset = MS_TO_LED_FRAMES(ledState.offset);
      
      if(DEBUG_BLOCK_LIGHTS)
      {
         PRINT_NAMED_DEBUG("SetObjectLights(1)",
                           "LED %u, onColor 0x%x (0x%x), offColor 0x%x (0x%x)",
                           i,
                           lights[i].onColor,
                           ledState.onColor.AsRGBA(),
                           lights[i].offColor,
                           ledState.offColor.AsRGBA());
      }
    }
    
    if( DEBUG_BLOCK_LIGHTS ) {
      PRINT_NAMED_DEBUG("Robot.SetObjectLights.Set1",
                        "Setting lights for object %d (activeID %d)",
                        objectID.GetValue(), activeObject->GetActiveID());
    }
    
    SendTransitionMessage(objectID, lights);
    
    _robot.SendMessage(RobotInterface::EngineToRobot(SetCubeGamma(activeObject->GetLEDGamma())));
    _robot.SendMessage(RobotInterface::EngineToRobot(CubeID((uint32_t)activeObject->GetActiveID(),
                                                            MS_TO_LED_FRAMES(rotationPeriod_ms))));
    return _robot.SendMessage(RobotInterface::EngineToRobot(CubeLights(lights)));
  }
}

Result LightsComponent::SetBackpackLightsInternal(const BackpackLights& lights)
{
  std::array<Anki::Cozmo::LightState, 2> turnSignals;
  u8 turnCount = 0;
  std::array<Anki::Cozmo::LightState, 3> middleLights;
  u8 middleCount = 0;
  for (int i = 0; i < (int)LEDId::NUM_BACKPACK_LEDS; ++i) {
    if(i == (int)LEDId::LED_BACKPACK_RIGHT || i == (int)LEDId::LED_BACKPACK_LEFT)
    {
      turnSignals[turnCount].onColor  = ENCODED_COLOR(lights.onColors[i]);
      turnSignals[turnCount].offColor = ENCODED_COLOR(lights.offColors[i]);
      turnSignals[turnCount].onFrames  = MS_TO_LED_FRAMES(lights.onPeriod_ms[i]);
      turnSignals[turnCount].offFrames = MS_TO_LED_FRAMES(lights.offPeriod_ms[i]);
      turnSignals[turnCount].transitionOnFrames  = MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]);
      turnSignals[turnCount].transitionOffFrames = MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]);
      turnSignals[turnCount].offset = MS_TO_LED_FRAMES(lights.offset[i]);
      ++turnCount;
    }
    else
    {
      middleLights[middleCount].onColor  = ENCODED_COLOR(lights.onColors[i]);
      middleLights[middleCount].offColor = ENCODED_COLOR(lights.offColors[i]);
      middleLights[middleCount].onFrames  = MS_TO_LED_FRAMES(lights.onPeriod_ms[i]);
      middleLights[middleCount].offFrames = MS_TO_LED_FRAMES(lights.offPeriod_ms[i]);
      middleLights[middleCount].transitionOnFrames  = MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]);
      middleLights[middleCount].transitionOffFrames = MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]);
      middleLights[middleCount].offset = MS_TO_LED_FRAMES(lights.offset[i]);
      ++middleCount;
    }
    
    if(DEBUG_BACKPACK_LIGHTS)
    {
      PRINT_NAMED_DEBUG("BackpackLights",
                       "0x%x 0x%x %u %u %u %u %d",
                       ENCODED_COLOR(lights.onColors[i]),
                       ENCODED_COLOR(lights.offColors[i]),
                       MS_TO_LED_FRAMES(lights.onPeriod_ms[i]),
                       MS_TO_LED_FRAMES(lights.offPeriod_ms[i]),
                       MS_TO_LED_FRAMES(lights.transitionOnPeriod_ms[i]),
                       MS_TO_LED_FRAMES(lights.transitionOffPeriod_ms[i]),
                       MS_TO_LED_FRAMES(lights.offset[i]));
    }
  }
  
  _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::BackpackLightsMiddle(middleLights, 0)));
  return _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::BackpackLightsTurnSignals(turnSignals, 0)));
}

void LightsComponent::RestorePrevStates()
{
  for(auto& pair : _cubeInfo)
  {
    RestorePrevState(pair.first);
  }
}

void LightsComponent::RestorePrevState(const ObjectID objectID)
{
  auto cube = _cubeInfo.find(objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.RestorePrevState", "No object %d in _cubeInfo map", objectID.GetValue());
    return;
  }
  
  cube->second.desiredState = cube->second.prevState;
  
  // If returning to a previous state of interacting or fading just go back to visible
  if(cube->second.desiredState == CubeLightsState::Interacting ||
     cube->second.fadeFromTo.first != CubeLightsState::Invalid)
  {
    cube->second.desiredState = CubeLightsState::Visible;
  }
  
  PRINT_CH_INFO("LightsComponent", "LightsComponent.RestorePrevState", "Restoring obejct %d to state %d from %d)",
                objectID.GetValue(), cube->second.desiredState, cube->second.currState);
}

void LightsComponent::StartLoopingBackpackLights(const BackpackLights& lights)
{
  if(_loopingBackpackLights)
  {
    PRINT_CH_INFO("LightsComponent", "LightsComponent.StartLoopingBackpackLights",
                  "Already looping backpack lights will override current lights");
  }

  _loopingBackpackLights = true;
  _currentLoopingBackpackLights = lights;
  SetBackpackLightsInternal(lights);
}

void LightsComponent::StopLoopingBackpackLights()
{
  if(_loopingBackpackLights)
  {
    _loopingBackpackLights = false;
    PRINT_CH_INFO("LightsComponent", "LightsComponent.StopLoopingBackpackLights",
                  "Stopping looping backpack lights returning to previous pattern");
    SetBackpackLightsInternal(_currentBackpackLights);
  }
}

Result LightsComponent::TurnOffObjectLights(const ObjectID& objectID)
{
  return SetObjectLights(objectID, kCubeLightsOff);
}

Result LightsComponent::TurnOffBackpackLights()
{
  return SetBackpackLights(kBackpackLightsOff);
}

void LightsComponent::SendTransitionMessage(const ObjectID& objectID, const ObjectLights& values)
{
  const auto cube = _cubeInfo.find(objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.SendTransitionMessage.CubeNotFound", "No cube in _cubeInfo with id %d", objectID.GetValue());
    return;
  }
  
  if(_sendTransitionMessages && cube->second.enabled)
  {
    ObservableObject* obj = _robot.GetBlockWorld().GetObjectByID(objectID);
    if(obj == nullptr)
    {
      PRINT_NAMED_WARNING("LightsComponent.SendTransitionMessage.NullObject",
                          "Got null object using id %d",
                          objectID.GetValue());
      return;
    }
    
    ExternalInterface::CubeLightsStateTransition msg;
    msg.objectID = objectID;
    msg.factoryID = obj->GetFactoryID();
    msg.objectType = obj->GetType();
    
    LightState lights;
    for(u8 i = 0; i < (u8)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
    {
      msg.lights[i].onColor = ENCODED_COLOR(values.onColors[i]);
      msg.lights[i].offColor = ENCODED_COLOR(values.offColors[i]);
      msg.lights[i].transitionOnFrames = MS_TO_LED_FRAMES(values.transitionOnPeriod_ms[i]);
      msg.lights[i].transitionOffFrames = MS_TO_LED_FRAMES(values.transitionOffPeriod_ms[i]);
      msg.lights[i].onFrames = MS_TO_LED_FRAMES(values.onPeriod_ms[i]);
      msg.lights[i].offFrames = MS_TO_LED_FRAMES(values.offPeriod_ms[i]);
      msg.lights[i].offset = MS_TO_LED_FRAMES(values.offset[i]);
    }
    
    msg.lightRotation_ms = values.rotationPeriod_ms;
    
    _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
  }
}

void LightsComponent::SendTransitionMessage(const ObjectID& objectID, const std::array<Anki::Cozmo::LightState, 4>& lights)
{
  const auto cube = _cubeInfo.find(objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.SendTransitionMessage.CubeNotFound", "No cube in _cubeInfo with id %d", objectID.GetValue());
    return;
  }
    
  if(_sendTransitionMessages && cube->second.enabled)
  {
    ObservableObject* obj = _robot.GetBlockWorld().GetObjectByID(objectID);
    if(obj == nullptr)
    {
      PRINT_NAMED_WARNING("LightsComponent.SendTransitionMessage.NullObject",
                          "Got null object using id %d",
                          objectID.GetValue());
      return;
    }
    
    ExternalInterface::CubeLightsStateTransition msg;
    msg.objectID = objectID;
    msg.factoryID = obj->GetFactoryID();
    msg.objectType = obj->GetType();
    msg.lights = lights;
    msg.lightRotation_ms = 0;

    _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(msg)));
  }
}

void LightsComponent::SetHeadlight(bool on)
{
  _robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::SetHeadlight(on)));
}

void LightsComponent::OnObjectPoseStateChanged(const ObjectID& objectID,
                                               const PoseState oldPoseState,
                                               const PoseState newPoseState)
{
  if(oldPoseState == newPoseState)
  {
    return;
  }
  
  // Known -> Dirty | Dirty -> Unknown | Known -> Unknown change to Connected state
  // Works based on the ordering of the PoseState enum
  static_assert(PoseState::Known < PoseState::Dirty &&
                PoseState::Dirty < PoseState::Unknown,
                "PoseState enum in unexpected order");
  
  auto cube = _cubeInfo.find(objectID);
  if(cube == _cubeInfo.end())
  {
    PRINT_NAMED_WARNING("LightsComponent.OnObjectPoseStateChanged.NoCube", "%d", objectID.GetValue());
    return;
  }
  
  if(oldPoseState < newPoseState)
  {
    cube->second.desiredState = CubeLightsState::Connected;
  }
  // If going to Known change to Visible
  else if(newPoseState == PoseState::Known)
  {
    cube->second.desiredState = CubeLightsState::Visible;
  }
  
  PRINT_CH_INFO("LightsComponent", "LightsComponent.OnObjectPoseStateChanged",
                "Changing object %d from state %d to %d based on pose change from %s to %s",
                objectID.GetValue(),
                cube->second.currState,
                cube->second.desiredState,
                EnumToString(oldPoseState),
                EnumToString(newPoseState));
}

LightsComponent::ObjectInfo::ObjectInfo()
  : enabled(true)
  , desiredState( LightsComponent::CubeLightsState::Off )
  , currState( LightsComponent::CubeLightsState::Off )
  , prevState(LightsComponent::CubeLightsState::Connected)
  , lastObservedTime_ms(0)
{
}


} // namespace Cozmo
} // namespace Anki

