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
#include "util/cpuProfiler/cpuProfiler.h"

static const u32 kTimeToMarkCubeNonVisible_ms = 3000;

namespace Anki {
namespace Cozmo {

LightsComponent::LightsComponent(Robot& robot)
  : _robot(robot)
{
  if( _robot.HasExternalInterface() ) {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
  }
}

void LightsComponent::Update()
{
  ANKI_CPU_PROFILE("LightsComponent::Update");
  
  if( !_enabled ) {
    return;
  }

  // TODO: check for cubes that we can hear but not see

  // check if any cubes are no longer visible
  TimeStamp_t currTS = _robot.GetLastImageTimeStamp();
  
  for( auto& cubeInfoPair : _cubeInfo )
  {
    CubeLightsState newState = CubeLightsState::Visible;
    
    // If we're interacting with this object, put it in interacting state
    if(_interactionObjects.count(cubeInfoPair.first) > 0)
    {
      newState = CubeLightsState::Interacting;
    }
    
    // Ignore age timeout for carried object
    else if(_robot.GetCarryingObject() != cubeInfoPair.first)
    {
      u32 age = currTS - cubeInfoPair.second.lastObservedTime_ms;
      if( age > kTimeToMarkCubeNonVisible_ms ) {
        newState = CubeLightsState::Connected;
      }
    }
    
    if( ShouldOverrideState( cubeInfoPair.second.currState, newState )) {
      cubeInfoPair.second.desiredState = newState;
    }
  }

  UpdateToDesiredLights();
}

void LightsComponent::UpdateToDesiredLights()
{
  for( const auto& cubeInfoPair : _cubeInfo ) {
    if( cubeInfoPair.second.currState != cubeInfoPair.second.desiredState ) {
      SetLights( cubeInfoPair.first, cubeInfoPair.second.desiredState );
    }
  }
}

bool LightsComponent::ShouldOverrideState(CubeLightsState currState, CubeLightsState nextState)
{
  // This is a placeholder for more complicated logic to come later
  return true;
}

void LightsComponent::SetEnableComponent(bool enable)
{
  if( _enabled && !enable) {
    for( auto& cube : _cubeInfo ) {
      cube.second.desiredState = CubeLightsState::Off;
    }

    UpdateToDesiredLights();
  }

  _enabled = enable;
}


void LightsComponent::SetLights(ObjectID object, CubeLightsState state)
{
  if( _cubeInfo[object].currState == state ){
    return;
  }
  
  switch( state ) {      
    case CubeLightsState::Off:
      _robot.TurnOffObjectLights(object);
      
      PRINT_NAMED_DEBUG("LightsComponent.SetLights",
                        "Object %d set to off",
                        object.GetValue());
      break;
      
    case CubeLightsState::Connected:
      _robot.SetObjectLights(object,
                             WhichCubeLEDs::ALL,
                             ColorRGBA(0.6f,0.6f,0.6f), ColorRGBA(0.35f, 0.35f, 0.35f),
                             250, 100,
                             2000, 2000,
                             true,
                             MakeRelativeMode::RELATIVE_LED_MODE_OFF,
                             Point2f{0.0, 0.0});

      // TODO:(bn) need to set this to cubes I haven't seen yet, but can't figure out how
      //_robot.TurnOffObjectLights(object);
      
      PRINT_NAMED_DEBUG("LightsComponent.SetLights",
                        "Object %d set to connected",
                        object.GetValue());

      break;
      
    case CubeLightsState::Visible:
      _robot.SetObjectLights(object,
                             WhichCubeLEDs::ALL,
                             NamedColors::CYAN, NamedColors::CYAN,
                             1000, 0,
                             500, 500,
                             true,
                             MakeRelativeMode::RELATIVE_LED_MODE_OFF,
                             Point2f{0.0, 0.0});

      PRINT_NAMED_DEBUG("LightsComponent.SetLights",
                        "Object %d set to visible",
                        object.GetValue());

      break;
      
    case CubeLightsState::Interacting:
      _robot.SetObjectLights(object,
                             WhichCubeLEDs::ALL,
                             NamedColors::GREEN, NamedColors::GREEN,
                             1000, 0,
                             500, 500,
                             true,
                             MakeRelativeMode::RELATIVE_LED_MODE_OFF,
                             Point2f{0.0, 0.0});
      
      PRINT_NAMED_DEBUG("LightsComponent.SetLights",
                        "Object %d set to interacting",
                        object.GetValue());

      break;
  }

  _cubeInfo[object].currState = state;
}

// TODO:(bn) handle disconnected? can't really change the lights in that case anyway....

template<>
void LightsComponent::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  if( msg.objectFamily != ObjectFamily::LightCube ) {
    return;
  }

  if( msg.markersVisible  ) {
    _cubeInfo[msg.objectID].lastObservedTime_ms = msg.timestamp;
    if( ShouldOverrideState( _cubeInfo[msg.objectID].desiredState, CubeLightsState::Visible ) ) {
      _cubeInfo[msg.objectID].desiredState = CubeLightsState::Visible;
    }
  }
}

LightsComponent::ObjectInfo::ObjectInfo()
  : desiredState( LightsComponent::CubeLightsState::Off )
  , currState(  LightsComponent::CubeLightsState::Off )
  , lastObservedTime_ms(0)
{
}


} // namespace Cozmo
} // namespace Anki

