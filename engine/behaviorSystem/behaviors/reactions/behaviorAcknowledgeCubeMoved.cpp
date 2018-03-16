/**
 * File: BehaviorAcknowledgeCubeMoved.cpp
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/behaviorSystem/behaviors/reactions/behaviorAcknowledgeCubeMoved.h"

#include "engine/behaviorSystem/behaviorManager.h"
#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/components/visionComponent.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {
  

namespace{
  
#define SET_STATE(s) SetState_internal(State::s, #s)

const float kDelayForUserPresentBlock_s = 1.0f;
const float kDelayToRecognizeBlock_s = 0.5f;

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersAcknowledgeCubeArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersAcknowledgeCubeArray),
              "Reaction triggers duplicate or non-sequential");

}
  
  
  
using namespace ExternalInterface;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorAcknowledgeCubeMoved::BehaviorAcknowledgeCubeMoved(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _robot(robot)
, _state(State::PlayingSenseReaction)
, _activeObjectSeen(false)
{  
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorAcknowledgeCubeMoved::IsRunnableInternal(const Robot& robot) const
{
  return _activeObjectID.IsSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorAcknowledgeCubeMoved::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersAcknowledgeCubeArray);
  _activeObjectSeen = false;
  switch(_state){
    case State::TurningToLastLocationOfBlock:
      TransitionToTurningToLastLocationOfBlock(robot);
      break;
      
    default:
      TransitionToPlayingSenseReaction(robot);
      break;
  }
  
  return Result::RESULT_OK;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorAcknowledgeCubeMoved::UpdateInternal(Robot& robot)
{
  // object seen - cancel turn and play response
  if(_state == State::TurningToLastLocationOfBlock
     && _activeObjectSeen)
  {
    StopActing(false);
    StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::AcknowledgeObject));
    SET_STATE(ReactingToBlockPresence);
  }
  
  return IBehavior::UpdateInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::StopInternal(Robot& robot)
{  
  _activeObjectID.UnSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::TransitionToPlayingSenseReaction(Robot& robot)
{
  SET_STATE(PlayingSenseReaction);
  StartActing(new CompoundActionParallel(robot, {
    new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::CubeMovedSense),
    new WaitAction(robot, kDelayForUserPresentBlock_s) }),
              &BehaviorAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock(Robot& robot)
{
  SET_STATE(TurningToLastLocationOfBlock);
  
  const ObservableObject* obj = robot.GetBlockWorld().GetLocatedObjectByID(_activeObjectID );
  if(obj == nullptr)
  {
    PRINT_NAMED_WARNING("BehaviorAcknowledgeCubeMoved.TransitionToTurningToLastLocationOfBlock.NullObject",
                        "The robot's context has changed and the block's location is no longer valid. (ObjectID=%d)",
                        _activeObjectID.GetValue());
    return;
  }
  const Pose3d& blockPose = obj->GetPose();
  
  StartActing(new CompoundActionParallel(robot, {
    new TurnTowardsPoseAction(robot, blockPose),
    new WaitAction(robot, kDelayToRecognizeBlock_s) }),
              &BehaviorAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence(Robot& robot)
{
  SET_STATE(ReactingToBlockAbsence);
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::CubeMovedUpset));
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedAcknowledgedCubeMoved);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  SetDebugStateName(stateName);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
    switch(event.GetData().GetTag()){
      case EngineToGameTag::RobotObservedObject:
      {
        HandleObservedObject(robot, event.GetData().Get_RobotObservedObject());
        break;
      }
      default:
      break;
    }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorAcknowledgeCubeMoved::HandleObservedObject(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  if(_activeObjectID.IsSet() && msg.objectID == _activeObjectID){
    _activeObjectSeen = true;
  }
}

  
} // namespace Cozmo
} // namespace Anki
