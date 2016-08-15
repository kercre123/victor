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
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

const float kMinTimeMoving_ms = 500;
const float kDelayForUserPresentBlock_s = 1.0;
const float kDelayToRecognizeBlock_s = 0.5;
const float kRadiusRobotTolerence = 50;

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(bool, kEnableObjectMovedReact, "BehaviorReactAcknowledgeCubeMoved", false);
}

class ReactionObjectData{
public:
  ReactionObjectData(ObjectID objectID);
  ObjectID GetObjectID() const;
  bool ObjectHasMovedLongEnough(const Robot& robot);
  bool ObjectUpAxisHasChanged(const Robot& robot);
  bool ObjectOutsideIgnoreArea(const Robot& robot);
  
  // update values for reactionObject
  void ObjectStartedMoving(const Robot& robot, const ObjectMoved& msg);
  void ObjectStoppedMoving(const Robot& robot);
  void ObjectObserved(const Robot& robot);
  void ResetObject();
  
  bool operator==(const ReactionObjectData &other) const;
  
private:
  ObjectID _objectID;
  TimeStamp_t _timeStartedMoving;
  UpAxis _axisOfAccel;

  // tracking if the block has moved long enough
  bool _isObjectMoving;
  bool _observedSinceLastReaction;
  
  // tracking if the block has changed orientation since last observed
  bool _hasUpAxisChanged;
};
  

//////
/// ReactAcknowledge Cube Moved
/////
using namespace ExternalInterface;

BehaviorReactAcknowledgeCubeMoved::BehaviorReactAcknowledgeCubeMoved(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IReactionaryBehavior(robot, config)
, _state(State::PlayingSenseReaction)
{
  SetDefaultName("ReactToCubeMoved");
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::ObjectMoved,
    EngineToGameTag::ObjectStoppedMoving,
    EngineToGameTag::ObjectUpAxisChanged
  }});
  
}

bool BehaviorReactAcknowledgeCubeMoved::IsRunnableInternalReactionary(const Robot& robot) const
{
  return kEnableObjectMovedReact;
}
  
bool BehaviorReactAcknowledgeCubeMoved::ShouldComputationallySwitch(const Robot& robot)
{
  for(auto& object: _reactionObjects){
    const ObservableObject* cube = robot.GetBlockWorld().GetObjectByID(object.GetObjectID());
    if(cube == nullptr){
      continue;
    }
    
    static constexpr float kMaxNormalAngle = DEG_TO_RAD(45); // how steep of an angle we can see
    static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it
    bool isVisible = cube->IsVisibleFrom(robot.GetVisionComponent().GetCamera(),
                                        kMaxNormalAngle,
                                        kMinImageSizePix,
                                        false);
    
    if(object.ObjectOutsideIgnoreArea(robot)
       && ((object.ObjectHasMovedLongEnough(robot)) || object.ObjectUpAxisHasChanged(robot))
       && !isVisible
    ){
      SET_STATE(PlayingSenseReaction);
      //Ensure there's no throttling between two blocks if moved
      _switchObjectID = object.GetObjectID();
      object.ResetObject();
      return true;
    }
  }
  return false;
}

Result BehaviorReactAcknowledgeCubeMoved::InitInternalReactionary(Robot& robot)
{
  _activeObjectID = _switchObjectID;
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

void BehaviorReactAcknowledgeCubeMoved::StopInternalReactionary(Robot& robot)
{
  //Ensure that two cubes being moved does not cause the robot to throttle back and forth
  auto iter = GetReactionaryIterator(_activeObjectID);
  iter->ResetObject();
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
  switch(event.GetData().GetTag()){
    case EngineToGameTag::ObjectMoved:
      HandleObjectMoved(robot, event.GetData().Get_ObjectMoved());
      break;
    case EngineToGameTag::ObjectStoppedMoving:
      HandleObjectStopped(robot, event.GetData().Get_ObjectStoppedMoving());
      break;
    case EngineToGameTag::RobotObservedObject:
      HandleObservedObject(robot, event.GetData().Get_RobotObservedObject());
      break;
    case EngineToGameTag::ObjectUpAxisChanged:
      HandleObjectUpAxisChanged(robot, event.GetData().Get_ObjectUpAxisChanged());
      break;
    default:
      break;
  }
}
  
void BehaviorReactAcknowledgeCubeMoved::HandleObjectMoved(const Robot& robot, const ObjectMoved& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStartedMoving(robot, msg);
}
  
void BehaviorReactAcknowledgeCubeMoved::HandleObjectStopped(const Robot& robot, const ObjectStoppedMoving& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStoppedMoving(robot);
}
  
void BehaviorReactAcknowledgeCubeMoved::HandleObjectUpAxisChanged(const Robot& robot, const ObjectUpAxisChanged& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectUpAxisHasChanged(robot);
}

  
void BehaviorReactAcknowledgeCubeMoved::HandleObservedObject(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectObserved(robot);
}
  
BehaviorReactAcknowledgeCubeMoved::Reaction_iter BehaviorReactAcknowledgeCubeMoved::GetReactionaryIterator(ObjectID objectID)
{
  Reaction_iter iter = std::find(_reactionObjects.begin(), _reactionObjects.end(), objectID);
  if(iter == _reactionObjects.end()){
    _reactionObjects.push_back(ReactionObjectData(objectID));
    iter = _reactionObjects.end();
    iter--;
  }
  
  return iter;
}


void BehaviorReactAcknowledgeCubeMoved::SetState_internal(State state, const std::string& stateName)
{
  _state = state;
  PRINT_NAMED_DEBUG("BehaviorReactAcknowledgeCubeMoved.TransitionTo", "%s", stateName.c_str());
  SetStateName(stateName);
}
  
//////
/// ReactionObject
/////

ReactionObjectData::ReactionObjectData(ObjectID objectID)
: _objectID(objectID)
, _timeStartedMoving(0)
, _axisOfAccel(UpAxis::Unknown)
, _isObjectMoving(false)
, _observedSinceLastReaction(false)
, _hasUpAxisChanged(false)
{
}

ObjectID ReactionObjectData::GetObjectID() const
{
  return _objectID;
}
  
bool ReactionObjectData::operator==(const ReactionObjectData &other) const
{
  return this->_objectID == other.GetObjectID();
}

  
bool ReactionObjectData::ObjectHasMovedLongEnough(const Robot& robot)
{
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  if(_isObjectMoving && _observedSinceLastReaction){
    TimeStamp_t time_ms = robot.GetLastMsgTimestamp();
    if(time_ms - _timeStartedMoving > kMinTimeMoving_ms){
      return true;
    }
  }
  
  return false;
}
  
bool ReactionObjectData::ObjectUpAxisHasChanged(const Robot& robot)
{
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  return _hasUpAxisChanged;
}
  
bool ReactionObjectData::ObjectOutsideIgnoreArea(const Robot& robot)
{
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  const Pose3d& blockPose = object->GetPose();
  const Pose3d& robotPose = robot.GetPose();
  f32 distance = 0;
  bool distanceComputed = ComputeDistanceBetween(blockPose, robotPose, distance);
  if(!distanceComputed){
    return false;
  }
  
  return distance > kRadiusRobotTolerence;
}

// update values for reactionObject
void ReactionObjectData::ObjectStartedMoving(const Robot& robot, const ObjectMoved& msg)
{
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
  if(object == nullptr){
    _isObjectMoving = false;
    return;
  }else if(!_isObjectMoving){
    _isObjectMoving = true;
    _timeStartedMoving = msg.timestamp;
    _axisOfAccel = msg.axisOfAccel;
  }else{
    if(msg.axisOfAccel != _axisOfAccel){
      _hasUpAxisChanged = true;
      _axisOfAccel = msg.axisOfAccel;
    }
  }
}
  
void ReactionObjectData::ObjectStoppedMoving(const Robot& robot)
{
  _isObjectMoving = false;
}
  
void ReactionObjectData::ObjectObserved(const Robot& robot)
{
  const ObservableObject* object = robot.GetBlockWorld().GetObjectByID(_objectID);
  if(object != nullptr){
    _observedSinceLastReaction = true;
  }
}

void ReactionObjectData::ResetObject()
{
  _isObjectMoving = false;
  _hasUpAxisChanged = false;
  _observedSinceLastReaction = false;
}
  
} // namespace Cozmo
} // namespace Anki
