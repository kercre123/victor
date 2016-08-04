/**
 * File: behaviorReactAcknowledgeCubeMoved.cpp
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactAcknowledgeCubeMoved.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

const float kMinTimeMoving = 0.2;
const float kDelayBetweenMessages = 1.0;
const float kDelayForUserPresentBlock_s = 1.0;
const float kDelayToRecognizeBlock_s = 0.5;
const float kRadiusRobotTolerence = 50;

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kBRACM_enableObjectMovedReact, "BehaviorCubeMoved", false);

ReactionObjectData::ReactionObjectData(int objectID, double nextUniqueTime, bool observedSinceLastResponse)
{
  _objectID = objectID;
  _nextUniqueTime = nextUniqueTime;
  _observedSinceLastResponse = observedSinceLastResponse;
}

using namespace ExternalInterface;

BehaviorReactAcknowledgeCubeMoved::BehaviorReactAcknowledgeCubeMoved(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IReactionaryBehavior(robot, config)
, _state(State::PlayingSenseReaction)
{
  SetDefaultName("ReactToCubeMoved");
  
  SubscribeToTriggerTags({
    EngineToGameTag::ObjectMovedWrapper
  });
  
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject
  });
  
}

bool BehaviorReactAcknowledgeCubeMoved::IsRunnableInternalReactionary(const Robot& robot) const
{
  return kBRACM_enableObjectMovedReact;
}

bool BehaviorReactAcknowledgeCubeMoved::ShouldRunForEvent(const MessageEngineToGame& event, const Robot& robot)
{
  bool shouldRun = false;
  double time_s =  BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  uint32_t objectID = event.Get_ObjectMovedWrapper().objectMoved.objectID;
  float_t timeSpentMoving = event.Get_ObjectMovedWrapper().timeMoving;
  
  std::vector<ReactionObjectData>::iterator iter;
  for(iter = _reactionObjects.begin(); iter != _reactionObjects.end(); ++iter){
    if (iter->_objectID == objectID){
      break;
    }
  }
  
  if(iter == _reactionObjects.end()){
    _reactionObjects.push_back(ReactionObjectData(objectID, time_s, false));
    iter = _reactionObjects.end();
    --iter;
  }
  
  //Only trigger reactionary behavior if enough time has elapsed and the object's position is known
  if(iter->_nextUniqueTime <= time_s && iter->_observedSinceLastResponse
     && timeSpentMoving > kMinTimeMoving){
    iter->_nextUniqueTime = time_s + kDelayBetweenMessages;
    iter->_observedSinceLastResponse = false;
    
    //Ensure object is in robot's current frame of reference
    const ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(objectID);
    if(obj != nullptr)
    {
      //Ensure the object's position is outside of a certain radius of the robot
      const Pose3d& blockPose = obj->GetPose();
      const Pose3d& robotPose = robot.GetPose();
      f32 distance = ComputeDistanceBetween(blockPose, robotPose);
      if(distance > kRadiusRobotTolerence){
        shouldRun = true;
        SET_STATE(PlayingSenseReaction);
        _activeObjectID = objectID;
      }
    }
  }
  
  return shouldRun;
}

Result BehaviorReactAcknowledgeCubeMoved::InitInternalReactionary(Robot& robot)
{
  switch(_state){
    case State::PlayingSenseReaction:
      TransitionToPlayingSenseReaction(robot);
      break;
    case State::TurningToLastLocationOfBlock:
      TransitionToTurningToLastLocationOfBlock(robot);
      break;
    case State::ReactingToBlockAbsence:
      TransitionToReactingToBlockAbsence(robot);
      break;
  }
  
  return Result::RESULT_OK;
}
  
void BehaviorReactAcknowledgeCubeMoved::TransitionToPlayingSenseReaction(Robot& robot)
{
  StartActing(new CompoundActionParallel(robot, {
    new TriggerAnimationAction(robot, AnimationTrigger::CubeMovedSense),
    new WaitAction(robot, kDelayForUserPresentBlock_s) }),
              &BehaviorReactAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock);
  
}

void BehaviorReactAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock(Robot& robot)
{
  SET_STATE(TurningToLastLocationOfBlock);
  
  const ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_activeObjectID );
  if(obj == nullptr)
  {
    PRINT_NAMED_WARNING("BehaviorReactAcknowledgeCubeMoved.TransitionToTurningToLastLocationOfBlock",
                        "The robot's context has changed and the block's location is no longer valid.");
    return;
  }
  const Pose3d& blockPose = obj->GetPose();
  
  StartActing(new CompoundActionParallel(robot, {
    new TurnTowardsPoseAction(robot, blockPose, PI),
    new WaitAction(robot, kDelayToRecognizeBlock_s) }),
              &BehaviorReactAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence);
}
  

void BehaviorReactAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence(Robot& robot)
{
  SET_STATE(ReactingToBlockAbsence);
  
  StartActing(new TriggerAnimationAction(robot, AnimationTrigger::CubeMovedUpset));
}
  
void BehaviorReactAcknowledgeCubeMoved::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  if(event.GetData().GetTag() == EngineToGameTag::RobotObservedObject){
    RobotObservedObject objOvserved = event.GetData().Get_RobotObservedObject();
    uint32_t objectID = objOvserved.objectID;

    for(auto iter = _reactionObjects.begin(); iter != _reactionObjects.end(); ++iter){
      if (iter->_objectID == objectID){
        iter->_observedSinceLastResponse = true;
        break;
      }
    }
  }
}

  
  
void BehaviorReactAcknowledgeCubeMoved::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorReactAcknowledgeCubeMoved.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}
  
} // namespace Cozmo
} // namespace Anki
