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
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviors/behaviorDemoFearEdge.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/vision/basestation/observableObject.h"
#include "json/json.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

#define DEBUG_PRINT_CUBE_STATE_CHANGES 0

#define DEBUG_SKIP_DIRECTLY_TO_CUBES 0
#define DEBUG_HOLD_IN_CUBES_STATE 0

namespace Anki {
namespace Cozmo {

static const char* kWakeUpBehavior = "demo_wakeUp";
static const char* kDriveOffChargerBehavior = "DriveOffCharger";
static const char* kFearEdgeBehavior = "demo_fearEdge";
// static const char* kCliffBehavior = "ReactToCliff";
static const char* kFlipDownFromBackBehavior = "ReactToRobotOnBack";
// static const char* kSleepOnChargerBehavior = "ReactToOnCharger";
static const char* kFindFacesBehavior = "demo_lookInPlaceForFaces";
//static const char* kKnockOverStackBehavior = "AdmireStack";
static const char* kDemoGameRequestBehaviorName = "demo_RequestSpeedTap";

static const float kTimeCubesMustBeUpright_s = 3.0f;

// score to force a behavior to run immediately
static const float kRunBehaviorNowScore = 100.0f;

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
    helper.SubscribeGameToEngine<MessageGameToEngineTag::WakeUp>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DemoResetState>();
  }

  _blockworldFilter->OnlyConsiderLatestUpdate(false);
  _blockworldFilter->SetFilterFcn( std::bind( &DemoBehaviorChooser::FilterBlocks, this, std::placeholders::_1) );

  _faceSearchBehavior = _robot.GetBehaviorFactory().FindBehaviorByName(kFindFacesBehavior);
  if( nullptr == _faceSearchBehavior ) {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.NoFaceSearchBehavior",
                      "couldnt get pointer to behavior '%s', won't be able to search for faces in mini game scene",
                      kFindFacesBehavior);
  }

  _gameRequestBehavior = _robot.GetBehaviorFactory().FindBehaviorByName(kDemoGameRequestBehaviorName);
  if( nullptr == _gameRequestBehavior ) {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.NoGameRequestBehavior",
                      "couldnt get pointer to behavior '%s', won't be able to search for faces in mini game scene",
                      kDemoGameRequestBehaviorName);
  }

  _fearEdgeBehavior = static_cast<BehaviorDemoFearEdge*>(
    _robot.GetBehaviorFactory().FindBehaviorByName(kFearEdgeBehavior) );

  if( nullptr == _fearEdgeBehavior ) {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.NoFearEdgeBehavior",
                      "couldn't find behavior '%s', demo won't work",
                      kFearEdgeBehavior);
  }

  //_admireStackBehavior = static_cast<BehaviorAdmireStack*>(
  //  _robot.GetBehaviorFactory().FindBehaviorByName(kKnockOverStackBehavior));

//  if( nullptr == _admireStackBehavior ) {
//    PRINT_NAMED_ERROR("DemoBehaviorChooser.NoAdmireStackBehavior",
//                      "couldn't find behavior '%s', demo won't work",
//                      kKnockOverStackBehavior);
//  }
  
  _name = "Demo[]";

  SetAllBehaviorsEnabled(false);
}

unsigned int DemoBehaviorChooser::GetUISceneNumForState(State state)
{
  switch( state ) {
    case State::None: return 0;
    case State::WakeUp: return 1;
    case State::DriveOffCharger: return 1;
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
  if( DEBUG_SKIP_DIRECTLY_TO_CUBES ) {
    TransitionToCubes();
  }

  // if we overrode any scores, reset those now (this will happen when coming out of activities and back to
  // demo)
  _modifiedBehaviorScores.clear();
}

Result DemoBehaviorChooser::Update()
{
  if( _checkTransition && _checkTransition() ) {
    TransitionToNextState();
  }

  if( _state == State::MiniGame ) {
    Pose3d waste;
    TimeStamp_t lastFaceTime = _robot.GetFaceWorld().GetLastObservedFace(waste);

    if( lastFaceTime == 0 ) {
      // we don't have any face, so we need to search for it. Make sure this happens now
      _modifiedBehaviorScores[ _faceSearchBehavior ] = kRunBehaviorNowScore;
    }
    else {
      // otherwise don't run this behavior at all in this mode
      _modifiedBehaviorScores[ _faceSearchBehavior ] = 0.0f;
    }
  }
  else {
    // no forced scores in any other states
    _modifiedBehaviorScores.clear();
  }
  
  return Result::RESULT_OK;
}

void DemoBehaviorChooser::ModifyScore(const IBehavior* behavior, float& score) const
{
  const auto& modPair = _modifiedBehaviorScores.find(behavior);
  if( modPair != _modifiedBehaviorScores.end() ) {
    score = modPair->second;
  }
}

void DemoBehaviorChooser::TransitionToNextState()
{
  switch( _state ) {
    case State::None: {
      TransitionToWakeUp();
      break;
    }
    case State::WakeUp: {
      TransitionToDriveOffCharger();
      break;
    }
    case State::DriveOffCharger: {
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

  _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::DemoState(GetUISceneNumForState(_state))));
}

bool DemoBehaviorChooser::IsBehaviorRunning(const char* behaviorName) const
{
  IBehavior* behavior = _robot.GetBehaviorFactory().FindBehaviorByName(behaviorName);
  if( nullptr == behavior ) {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.NoNamedBehavior",
                      "couldn't get behavior behavior '%s' by name",
                      behaviorName);
    return false;
  }

  return behavior->IsRunning();
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
  
void DemoBehaviorChooser::TransitionToDriveOffCharger()
{
  SET_STATE(DriveOffCharger);
  SetAllBehaviorsEnabled(false);
  SetBehaviorEnabled(kDriveOffChargerBehavior, true);
  
  // Deals with case where no connected charger has been added
  _checkTransition = [this]() {
    return DidBehaviorRunAndStop(kDriveOffChargerBehavior) || !_robot.IsOnChargerPlatform();
  };
}

void DemoBehaviorChooser::TransitionToFearEdge()
{
  SET_STATE(FearEdge);

  _robot.SetEnableCliffSensor(true);

  SetAllBehaviorsEnabled(false);
  SetBehaviorEnabled(kFearEdgeBehavior, true);

  // will transition when the fear edge is interrupted by cliff
  _checkTransition = [this]() {
    return _fearEdgeBehavior != nullptr && _fearEdgeBehavior->HasFinished();
  };
}

void DemoBehaviorChooser::TransitionToPounce()
{
  SET_STATE(Pounce);

  _robot.SetEnableCliffSensor(true);
  
  SetAllBehaviorsEnabled(false);
  SetBehaviorGroupEnabled(BehaviorGroup::DemoFingerPounce);

  // the "flip down on back" behavior is a special behavior which runs a reaction when the robot is up on it's
  // back. Once this behavior has run and stopped, we are ready to transition
  _checkTransition = std::bind(&DemoBehaviorChooser::DidBehaviorRunAndStop, this, kFlipDownFromBackBehavior);
}

void DemoBehaviorChooser::TransitionToFaces()
{
  SET_STATE(Faces);
  
  SetAllBehaviorsEnabled(false);
  SetBehaviorGroupEnabled(BehaviorGroup::DemoFaces);

  _checkTransition = [this]() { return _hasSeenBlock; };
}

void DemoBehaviorChooser::TransitionToCubes()
{
  SET_STATE(Cubes);

  _robot.GetProgressionUnlockComponent().SetUnlock(UnlockId::RollCube, true);
  _robot.GetProgressionUnlockComponent().SetUnlock(UnlockId::StackTwoCubes, true);

  SetAllBehaviorsEnabled(false);
  SetBehaviorGroupEnabled(BehaviorGroup::DemoCubes);

  _cubesUprightTime_s = -1.0f;

  if( DEBUG_HOLD_IN_CUBES_STATE ) {
    _checkTransition = nullptr;
  }
  else {
    _checkTransition = std::bind(&DemoBehaviorChooser::ShouldTransitionOutOfCubesState, this);
  }
}

void DemoBehaviorChooser::TransitionToMiniGame()
{
  SET_STATE(MiniGame);

  // default face search behavior to disabled
  _modifiedBehaviorScores[ _faceSearchBehavior ] = 0.0f;
  
  _robot.GetBehaviorManager().SetAvailableGame(BehaviorGameFlag::SpeedTap);
  
  // leave block behaviors active, but also enable speed tap game request
  SetBehaviorGroupEnabled(BehaviorGroup::DemoRequestSpeedTap);

  // when mini game starts, will go to selection chooser, then back to this chooser
  // need to fix COZMO-2700 then re-enable this
  _checkTransition = nullptr; // [this]() { return IsBehaviorRunning(kSleepOnChargerBehavior); };
}

void DemoBehaviorChooser::TransitionToSleep()
{
  SET_STATE(Sleep);
  SetAllBehaviorsEnabled(false);

  // no transition out of this one
  _checkTransition = nullptr;
}

bool DemoBehaviorChooser::ShouldTransitionOutOfCubesState()
{
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  //if( ! DidBehaviorRunAndStop(kKnockOverStackBehavior) || ! _admireStackBehavior->DidKnockOverStack() ) {
  //  return false;
  //}
  //else {
    // disable checking the cube states for now, just let it transition
    return true;
  //}
  
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

bool DemoBehaviorChooser::FilterBlocks(const ObservableObject* obj) const
{
  if( nullptr == obj ) {
    return false;
  }

  if( obj->GetFamily() != ObjectFamily::LightCube ) {
    return false;
  }

  const AxisName upAxis = obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();

  const bool ret = !obj->IsMoving() &&
    obj->IsRestingFlat() &&
    // ignore object we are carrying
    ( !_robot.IsCarryingObject() || _robot.GetCarryingObject() != obj->GetID() ) &&
    upAxis == AxisName::Z_POS;
  
  if( DEBUG_PRINT_CUBE_STATE_CHANGES ) {
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
  
  // We specifically want to not force transition to the sleep state from UI; we require the reactToCharger behavior to
  // start to make that transition
  if (State::MiniGame != _state)
  {
    TransitionToNextState();
  }
}

template<>
void DemoBehaviorChooser::HandleMessage(const ExternalInterface::WakeUp& msg)
{
  _hasEdge = msg.hasEdge;
  
  // If we're in the sleep state reset everything so we can start again
  if (State::Sleep == _state)
  {
    ResetDemoRelatedState();
  }

  // This serves as the "start demo" message, so also transition to the next state if we are at the start
  if( _state == State::None) { 
    TransitionToNextState();
  }
}

// TODO:(bn) This message name should probably change to TriggerDemoHack or something because it isn't
// actually resetting the state
template<>
void DemoBehaviorChooser::HandleMessage(const ExternalInterface::DemoResetState& msg)
{
  if( State::MiniGame == _state ) {
    // this is the "oh shit game request isn't happening" button. Force it to run
    PRINT_NAMED_WARNING("DemoBehaviorChooser.ForceGameRequest", "Forcing selection of game request behavior");

    _modifiedBehaviorScores[_gameRequestBehavior] = kRunBehaviorNowScore;

    _robot.GetBehaviorManager().ForceStopCurrentBehavior("DemoStateResetButton");

    // cancel any other actions just in case
    _robot.GetActionList().Cancel(RobotActionType::UNKNOWN);
  }
}

  
void DemoBehaviorChooser::ResetDemoRelatedState()
{
  // Clean up the demo behavior chooser state
  SET_STATE(None);
  _hasSeenBlock = false;
  
  // Helper lambda for resetting the start count of a behavior
  auto resetBehaviorStartCount = [this](const char* behaviorName)
  {
    auto behaviorPtr = _robot.GetBehaviorFactory().FindBehaviorByName(behaviorName);
    ASSERT_NAMED(nullptr != behaviorPtr, std::string("DemoBehaviorChooser.HandleStartDemoWithEdge.MissingBehavior_").append(behaviorName).c_str());
    if (nullptr != behaviorPtr)
    {
      behaviorPtr->ResetStartCount();
    }
  };
  
  // Clean up the wake up behavior
  resetBehaviorStartCount(kWakeUpBehavior);
  
  // Clean up the drive off charger behavior
  resetBehaviorStartCount(kDriveOffChargerBehavior);
  
  // Clean up the fear edge behavior state
  resetBehaviorStartCount(kFearEdgeBehavior);
  
  // Clean up the flip down from back behavior
  resetBehaviorStartCount(kFlipDownFromBackBehavior);
  
//  // Clean up the knock blocks down behavior
//  ASSERT_NAMED(nullptr != _admireStackBehavior, "DemoBehaviorChooser.HandleStartDemoWithEdge.MissingAdmireStackBehavior");
//  if (nullptr != _admireStackBehavior)
//  {
//    //_admireStackBehavior->ResetDidKnockOverStack();
//    //_admireStackBehavior->ResetStartCount();
//  }
  
}

void DemoBehaviorChooser::SetState_internal(State state, const std::string& stateName)
{
  PRINT_NAMED_INFO("DemoBehaviorChooser.SetState", "%s", stateName.c_str());
  _state = state;
  _name = "Demo[" + stateName + "]";
}

}
}
