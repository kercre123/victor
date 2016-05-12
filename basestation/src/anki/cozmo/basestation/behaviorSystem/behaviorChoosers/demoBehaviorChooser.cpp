/**
 * File: demoBehaviorChooser.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-25
 *
 * Description: This is a chooser for the PR / announce demo. It handles enabling / disabling behavior groups,
 *              as well as any other special logic that is needed for the demo
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "demoBehaviorChooser.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "json/json.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

#define DEBUG_PRINT_ALL_CUBE_STATES 0

namespace Anki {
namespace Cozmo {

static const char* kWakeUpBehavior = "demo_wakeUp";
static const char* kFearEdgeBehavior = "demo_fearEdge";
static const char* kCliffBehavior = "ReactToCliff";
static const char* kFlipDownFromBackBehavior = "ReactToRobotOnBack";
static const char* kSleepBehavior = "demo_sleep";
static const float kTimeCubesMustBeUpright_s = 3.0f;

DemoBehaviorChooser::DemoBehaviorChooser(Robot& robot, const Json::Value& config)
  : SimpleBehaviorChooser(robot, config)
  , _blockworldFilter( new BlockWorldFilter )
  , _robot(robot)
{
  if ( _robot.HasExternalInterface() )
  {
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::TransitionToNextDemoState>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::StartDemoWithEdge>();
  }

  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &DemoBehaviorChooser::FilterBlocks, this, std::placeholders::_1) );
    
  _name = "Demo[]";
}

unsigned int DemoBehaviorChooser::GetStateNum(State state)
{
  switch( state ) {
    case State::None: return 0;
    case State::WakeUp: return 1;
    case State::FearEdge: return 2;
    case State::Pounce: return 3;
    case State::Faces: return 4;
    case State::Cubes: return 5;
    case State::MiniGame: return 6;
    case State::Sleep: return 7;
  }
}

void DemoBehaviorChooser::OnSelected()
{
  SetAllBehaviorsEnabled(false);
  _initCalled = true;

  // TransitionToCubes();
}

Result DemoBehaviorChooser::Update()
{
  if( _checkTransition && _checkTransition() ) {
    TransitionToNextState();
  }

  return Result::RESULT_OK;
}

void DemoBehaviorChooser::TransitionToNextState()
{
  switch( _state ) {
    case State::None: {
      TransitionToWakeUp();
      break;
    }
    case State::WakeUp: {
      if( _hasEdge ) {
        TransitionToFearEdge();
      }
      else {
        TransitionToPounce();
      }            
      break;
    }
    case State::FearEdge: {
      TransitionToPounce();
      break;
    }
    case State::Pounce: {
      TransitionToFaces();
      break;
    }
    case State::Faces: {
      TransitionToCubes();
      break;
    }
    case State::Cubes: {
      TransitionToMiniGame();
      break;
    }
    case State::MiniGame: {
      TransitionToSleep();
      break;
    }
    case State::Sleep: {
      PRINT_NAMED_WARNING("DemoBehaviorChooser.InvalidTransition",
                          "Tried to transition to next state from Sleep, but Sleep is the last state");
      break;
    }
  }

  _robot.Broadcast( ExternalInterface::MessageEngineToGame( ExternalInterface::DemoState( GetStateNum( _state ))));
}

bool DemoBehaviorChooser::DidBehaviorRunAndStop(const char* behaviorName) const
{
  IBehavior* behavior = _robot.GetBehaviorFactory().FindBehaviorByName(behaviorName);
  if( nullptr == behavior ) {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.NoNamedBehavior",
                      "couldn't get behavior behavior '%s' by name, skipping it",
                      behaviorName);
    // return true so that we'll transition out of the broken behavior (which will probably just mean we are
    // running no behavior)
    return true;
  }
  else {
    // make sure it has run, but isn't currently running (aka it stopped)
    return behavior->GetNumTimesBehaviorStarted() > 0 && !behavior->IsRunning();
  }
}

void DemoBehaviorChooser::TransitionToWakeUp()
{
  SET_STATE(WakeUp);

  _robot.SetEnableCliffSensor(false);
  
  SetAllBehaviorsEnabled(false);
  SetBehaviorEnabled(kWakeUpBehavior, true);

  _checkTransition = std::bind(&DemoBehaviorChooser::DidBehaviorRunAndStop, this, kWakeUpBehavior);
}

void DemoBehaviorChooser::TransitionToFearEdge()
{
  SET_STATE(FearEdge);

  _robot.SetEnableCliffSensor(true);

  SetAllBehaviorsEnabled(false);
  SetBehaviorEnabled(kFearEdgeBehavior, true);

  _checkTransition = std::bind(&DemoBehaviorChooser::DidBehaviorRunAndStop, this, kCliffBehavior);
}

void DemoBehaviorChooser::TransitionToPounce()
{
  SET_STATE(Pounce);

  _robot.SetEnableCliffSensor(true);

  _robot.GetVisionComponent().EnableMode(VisionMode::DetectingMotion, true);
  
  SetAllBehaviorsEnabled(false);
  SetBehaviorGroupEnabled(BehaviorGroup::DemoFingerPounce);

  // the "flip down on back" behavior is a special behavior which runs a reaction when the robot is up on it's
  // back. Once this behavior has run and stopped, we are ready to transition
  _checkTransition = std::bind(&DemoBehaviorChooser::DidBehaviorRunAndStop, this, kFlipDownFromBackBehavior);
}

void DemoBehaviorChooser::TransitionToFaces()
{
  SET_STATE(Faces);

  _robot.GetVisionComponent().EnableMode(VisionMode::DetectingMotion, false);
  
  SetAllBehaviorsEnabled(false);
  SetBehaviorGroupEnabled(BehaviorGroup::DemoFaces);

  // TODO:(bn) figure out how enrollment will work
  
  _checkTransition = [this]() { return _hasSeenBlock; };
}

void DemoBehaviorChooser::TransitionToCubes()
{
  SET_STATE(Cubes);

  _robot.GetProgressionUnlockComponent().SetUnlock(UnlockId::CubeRollAction, true);
  _robot.GetProgressionUnlockComponent().SetUnlock(UnlockId::CubeStackAction, true);

  SetAllBehaviorsEnabled(false);
  SetBehaviorGroupEnabled(BehaviorGroup::DemoCubes);

  _cubesUprightTime_s = -1.0f;
  _checkTransition = std::bind(&DemoBehaviorChooser::ShouldTransitionOutOfCubesState, this);
}

void DemoBehaviorChooser::TransitionToMiniGame()
{
  SET_STATE(MiniGame);

  _robot.GetMoodManager().SetEmotion(EmotionType::WantToPlay, 1.0f);
  
  // leave block behaviors active, but also enable speed tap game request
  SetBehaviorGroupEnabled(BehaviorGroup::RequestSpeedTap);

  // when mini game starts, will go to selection chooser, then back to this chooser
  _initCalled = false;
  _checkTransition = [this]() { return _initCalled; };
}

void DemoBehaviorChooser::TransitionToSleep()
{
  SET_STATE(Sleep);
  SetAllBehaviorsEnabled(false);
  SetBehaviorEnabled(kSleepBehavior, true);

  // no transition out of this one
  _checkTransition = nullptr;
}

bool DemoBehaviorChooser::ShouldTransitionOutOfCubesState()
{
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // check block world for three cubes in the correct state for transition
  
  std::vector<ObservableObject*> uprightCubes;
  _robot.GetBlockWorld().FindMatchingObjects(*_blockworldFilter, uprightCubes);

  if( uprightCubes.size() < 3 ) {
    _cubesUprightTime_s = -1.0f;
    return false;
  }  

  if( _cubesUprightTime_s >= 0.0f) {
    if(currentTime_s > _cubesUprightTime_s ) {
      return true;
    }
  }
  else {
    _cubesUprightTime_s = currentTime_s + kTimeCubesMustBeUpright_s;
    PRINT_NAMED_DEBUG("DemoBehaviorChooser.CubesUpright",
                      "cubes started being upright at t=%f, will complete at t=%f",
                      currentTime_s,
                      _cubesUprightTime_s);
  }

  return false;
}

bool DemoBehaviorChooser::FilterBlocks(ObservableObject* obj) const
{
  if( nullptr == obj ) {
    return false;
  }

  if( obj->GetFamily() != ObjectFamily::LightCube ) {
    return false;
  }

  const AxisName upAxis = obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();

  const bool ret =  obj->IsPoseStateKnown() &&
    !obj->IsMoving() &&
    obj->IsRestingFlat() &&
    // ignore object we are carrying
    ( !_robot.IsCarryingObject() || _robot.GetCarryingObject() != obj->GetID() ) &&
    upAxis == AxisName::Z_POS &&
    // ignore objects that aren't clear
    _robot.CanStackOnTopOfObject( *obj );

  
  if( DEBUG_PRINT_ALL_CUBE_STATES ) {
    static std::map< ObjectID, bool > _lastValues;

    auto it = _lastValues.find(obj->GetID());
    if( it == _lastValues.end() || it->second != ret ) {
      _lastValues[obj->GetID()] = ret;
    
      PRINT_NAMED_DEBUG("DemoBehaviorChooser.BlockState.Change",
                        "%d: known?%d !moving?%d flat?%d !carried?%d upright?%d stackable?%d status:%d",
                        obj->GetID().GetValue(),
                        obj->IsPoseStateKnown() ,
                        !obj->IsMoving() ,
                        obj->IsRestingFlat() ,
                        ( !_robot.IsCarryingObject() || _robot.GetCarryingObject() != obj->GetID() ) ,
                        upAxis == AxisName::Z_POS ,
                        _robot.CanStackOnTopOfObject( *obj ),
                        ret);
    }
  }

  return ret;  
}

template<>
void DemoBehaviorChooser::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  if( msg.objectFamily == ObjectFamily::LightCube ) {
    _hasSeenBlock = true;
  }
}

template<>
void DemoBehaviorChooser::HandleMessage(const ExternalInterface::TransitionToNextDemoState& msg)
{
  PRINT_NAMED_INFO("DemoBehaviorChooser.TransitionFromUI",
                   "got message from UI to transition to next demo state");
  TransitionToNextState();
}

template<>
void DemoBehaviorChooser::HandleMessage(const ExternalInterface::StartDemoWithEdge& msg)
{
  _hasEdge = msg.hasEdge;

  // This serves as the "start demo" message, so also transition to the next state if we are at the start
  if( _state == State::None) { 
    TransitionToNextState();
  }
}

void DemoBehaviorChooser::SetState_internal(State state, const std::string& stateName)
{
  PRINT_NAMED_INFO("DemoBehaviorChooser.SetState", "%s", stateName.c_str());
  _state = state;
  _name = "Demo[" + stateName + "]";
}

}
}
