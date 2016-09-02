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
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/fileUtils/fileUtils.h"
#include "util/cpuProfiler/cpuProfiler.h"

namespace Anki {
namespace Cozmo {

LightsComponent::LightsComponent(Robot& robot)
  : _robot(robot)
{
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::ObjectMoved>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::ObjectConnectionState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableLightStates>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableCubeSleep>();
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
      }
    }
  }
}

void LightsComponent::AddLightStateValues(CubeLightsState state, const Json::Value& data)
{
  LightValues values = {
    .onColors               = JsonColorValueToArray(data["onColors"]),
    .offColors              = JsonColorValueToArray(data["offColors"]),
    .onPeriod_ms            = JsonValueToU32Array(data["onPeriod_ms"]),
    .offPeriod_ms           = JsonValueToU32Array(data["offPeriod_ms"]),
    .transitionOnPeriod_ms  = JsonValueToU32Array(data["transitionOnPeriod_ms"]),
    .transitionOffPeriod_ms = JsonValueToU32Array(data["transitionOffPeriod_ms"]),
    .offset                 = JsonValueToS32Array(data["offset"]),
    .rotationPeriod_ms      = data["rotationPeriod_ms"].asUInt()
  };
  _stateToValues.emplace(state, values);
}

std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS> LightsComponent::JsonValueToU32Array(const Json::Value& value)
{
  LEDArray arr;
  for(u8 i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
  {
    arr[i] = value[i].asUInt();
  }
  return arr;
}

std::array<s32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS> LightsComponent::JsonValueToS32Array(const Json::Value& value)
{
  std::array<s32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS> arr;
  for(u8 i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
  {
    arr[i] = value[i].asInt();
  }
  return arr;
}

std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS> LightsComponent::JsonColorValueToArray(const Json::Value& value)
{
  LEDArray arr;
  for(u8 i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i)
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
  
  for( auto& cubeInfoPair : _cubeInfo )
  {
    if(!cubeInfoPair.second.enabled)
    {
      continue;
    }
  
    CubeLightsState newState = cubeInfoPair.second.desiredState;
    CubeLightsState currState = cubeInfoPair.second.currState;
    const TimeStamp_t timeDiff = currTime - cubeInfoPair.second.startCurrStateTime;
    auto& fadeFromTo = cubeInfoPair.second.fadeFromTo;
    
    bool endingFade = false;
    
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
    
    // Only fade if we did not just end a fade and we can fade from current state to the new state
    if(!endingFade && FadeBetween(currState, newState, fadeFromTo))
    {
      newState = CubeLightsState::Fade;
    }
    
    if(ShouldOverrideState(currState, newState))
    {
      cubeInfoPair.second.desiredState = newState;
    }
    
    UpdateToDesiredLights(cubeInfoPair.first);
  }
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

bool LightsComponent::ShouldOverrideState(CubeLightsState currState, CubeLightsState nextState)
{
  // We should never transition to Invalid, override the Fade state since it manages itself,
  // or override WakeUp with anything other than WakeUpFadeOut
  if(nextState == CubeLightsState::Invalid ||
     currState == CubeLightsState::Fade ||
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
    LightValues fadeValues = {
      .onColors               = _stateToValues[from].onColors,
      .offColors              = _stateToValues[to].onColors,
      .onPeriod_ms            = {{_fadePeriod_ms,_fadePeriod_ms,_fadePeriod_ms,_fadePeriod_ms}},
      .offPeriod_ms           = {{_fadePeriod_ms*2,_fadePeriod_ms*2,_fadePeriod_ms*2,_fadePeriod_ms*2}},
      .transitionOnPeriod_ms  = {{0,0,0,0}},
      .transitionOffPeriod_ms = {{_fadeTime_ms,_fadeTime_ms,_fadeTime_ms,_fadeTime_ms}},
      .offset                 = {{0,0,0,0}},
      .rotationPeriod_ms      = 0
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
    PRINT_NAMED_INFO("LightsComponent.SetEnableCubeLights", "Setting object %d to %s",
                     id.GetValue(), (enable ? "enabled" : "disabled"));
    
    auto cube = _cubeInfo.find(id);
    if(cube == _cubeInfo.end())
    {
      PRINT_NAMED_WARNING("LightsComponent.SetEnableCubeLights",
                          "No object with id %d in _cubeInfo map",
                          id.GetValue());
      return;
    }
    
    if(cube->second.enabled && !enable)
    {
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
    PRINT_NAMED_WARNING("LightsComponent.SetLights",
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
  
  PRINT_NAMED_INFO("LightsComponent.SetLights",
                   "Setting object %d to state %d",
                   object.GetValue(),
                   state);
  
  const LightValues& values = _stateToValues[state];
  _robot.SetObjectLights(object,
                         values.onColors, values.offColors,
                         values.onPeriod_ms, values.offPeriod_ms,
                         values.transitionOnPeriod_ms, values.transitionOffPeriod_ms,
                         values.offset,
                         MakeRelativeMode::RELATIVE_LED_MODE_OFF,
                         Point2f{0,0}, values.rotationPeriod_ms);
}


template<>
void LightsComponent::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  if( msg.objectFamily != ObjectFamily::LightCube ) {
    return;
  }

  if( msg.markersVisible  ) {
    // Safe to do automatic insertion into map since we know we are observing a cube
    _cubeInfo[msg.objectID].lastObservedTime_ms = msg.timestamp;
    
    // If all cubes have been disabled then make sure to also disable new cubes that we connect to
    if(!_allCubesEnabled)
    {
      _cubeInfo[msg.objectID].enabled = false;
    }
    
    if( ShouldOverrideState( _cubeInfo[msg.objectID].desiredState, CubeLightsState::Visible ) ) {
      
      // Update our prevState if we are disabled so if we are moved while disabled our state will
      // be properly restored
      if(!_cubeInfo[msg.objectID].enabled)
      {
        _cubeInfo[msg.objectID].prevState = CubeLightsState::Visible;
      }
    
      _cubeInfo[msg.objectID].desiredState = CubeLightsState::Visible;
    }
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

