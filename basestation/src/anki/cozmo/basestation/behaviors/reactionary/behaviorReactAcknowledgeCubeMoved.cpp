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

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactAcknowledgeCubeMoved.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

const float kMinTimeMoving_ms = 1000;
const float kDelayForUserPresentBlock_s = 1.0;
const float kDelayToRecognizeBlock_s = 0.5;
const float kRadiusRobotTolerence = 50;

namespace Anki {
namespace Cozmo {
  
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
, _robot(robot)
, _state(State::PlayingSenseReaction)
, _activeObjectSeen(false)
{
  SetDefaultName("ReactToCubeMoved");
  
  SubscribeToTags({{
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::ObjectMoved,
    EngineToGameTag::ObjectStoppedMoving,
    EngineToGameTag::ObjectUpAxisChanged,
    EngineToGameTag::RobotDelocalized
  }});
  
}

bool BehaviorReactAcknowledgeCubeMoved::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
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
  SmartDisableReactionaryBehavior(BehaviorType::AcknowledgeObject);
  _activeObjectSeen = false;
  _activeObjectID = _switchObjectID;
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
  
IBehavior::Status BehaviorReactAcknowledgeCubeMoved::UpdateInternal(Robot& robot)
{
  if(ShouldComputationallySwitch(robot)){
    return Status::Complete;
  }
  
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


void BehaviorReactAcknowledgeCubeMoved::StopInternalReactionary(Robot& robot)
{  
  //Ensure that two cubes being moved does not cause the robot to throttle back and forth
  auto iter = GetReactionaryIterator(_activeObjectID);
  iter->ResetObject();
  _activeObjectID.UnSet();
}
  
void BehaviorReactAcknowledgeCubeMoved::TransitionToPlayingSenseReaction(Robot& robot)
{
  SET_STATE(PlayingSenseReaction);
  StartActing(new CompoundActionParallel(robot, {
    new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::CubeMovedSense),
    new WaitAction(robot, kDelayForUserPresentBlock_s) }),
              &BehaviorReactAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock);
  
}

void BehaviorReactAcknowledgeCubeMoved::TransitionToTurningToLastLocationOfBlock(Robot& robot)
{
  SET_STATE(TurningToLastLocationOfBlock);
  
  const ObservableObject* obj = robot.GetBlockWorld().GetObjectByID(_activeObjectID );
  if(obj == nullptr)
  {
    PRINT_NAMED_WARNING("BehaviorReactAcknowledgeCubeMoved.TransitionToTurningToLastLocationOfBlock.NullObject",
                        "The robot's context has changed and the block's location is no longer valid. (ObjectID=%d)",
                        _activeObjectID.GetValue());
    return;
  }
  const Pose3d& blockPose = obj->GetPose();
  
  StartActing(new CompoundActionParallel(robot, {
    new TurnTowardsPoseAction(robot, blockPose, M_PI_F),
    new WaitAction(robot, kDelayToRecognizeBlock_s) }),
              &BehaviorReactAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence);
}
  

void BehaviorReactAcknowledgeCubeMoved::TransitionToReactingToBlockAbsence(Robot& robot)
{
  SET_STATE(ReactingToBlockAbsence);
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::CubeMovedUpset));
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedAcknowledgedCubeMoved);
}
  
void BehaviorReactAcknowledgeCubeMoved::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  if(IsReactionEnabled()){
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
      case EngineToGameTag::RobotDelocalized:
        HandleRobotDelocalized();
      default:
        break;
    }
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
  if(_activeObjectID.IsSet() && msg.objectID == _activeObjectID){
    _activeObjectSeen = true;
  }
  
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectObserved(robot);
}
  
void BehaviorReactAcknowledgeCubeMoved::HandleRobotDelocalized()
{
  for(auto object : _reactionObjects){
    object.ResetObject();
  }
  
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
  PRINT_NAMED_DEBUG("BehaviorReactAcknowledgeCubeMovde.TransitionTo", "%s", stateName.c_str());
  SetDebugStateName(stateName);
}
  
  
void BehaviorReactAcknowledgeCubeMoved::EnabledStateChanged(bool enabled)
{
  if(enabled){
    // Mark any blocks with a pose state still known as observed so that
    // we respond to future messaging
    std::vector<const ObservableObject*> blocksOnly;
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    _robot.GetBlockWorld().FindMatchingObjects(blocksOnlyFilter, blocksOnly);
    
    for(const auto& block: blocksOnly){
      if(block->IsPoseStateKnown()){
        Reaction_iter it = GetReactionaryIterator(block->GetID());
        it->ObjectObserved(_robot);
      }
    }
  }
  else{
    for(auto& object: _reactionObjects){
      object.ResetObject();
    }
  }
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
    if(_timeStartedMoving != 0 && time_ms - _timeStartedMoving > kMinTimeMoving_ms){
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
  
  return _hasUpAxisChanged && _observedSinceLastReaction;
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
    // don't trigger reactions while we're directly looking at the block
    ResetObject();
    _observedSinceLastReaction = true;
  }
}

void ReactionObjectData::ResetObject()
{
  _isObjectMoving = false;
  _hasUpAxisChanged = false;
  _observedSinceLastReaction = false;
  _timeStartedMoving = 0;
}
  
} // namespace Cozmo
} // namespace Anki
