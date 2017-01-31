/**
 * File: reactionTriggerStrategyCubeMoved.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyCubeMoved.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
  
namespace {
const float kMinTimeMoving_ms = 1000;
const float kRadiusRobotTolerence = 50;
static const char* kTriggerStrategyName = "Trigger Strategy Cube Moved";
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
  
ReactionTriggerStrategyCubeMoved::ReactionTriggerStrategyCubeMoved(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
, _robot(robot)
{
  SubscribeToTags({{
    EngineToGameTag::ObjectMoved,
    EngineToGameTag::ObjectStoppedMoving,
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::ObjectUpAxisChanged,
    EngineToGameTag::RobotDelocalized
  }});
}

  
bool ReactionTriggerStrategyCubeMoved::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  for(auto& object: _reactionObjects){
    const ObservableObject* cube = robot.GetBlockWorld().GetConnectedActiveObjectByID(object.GetObjectID());
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
       && !isVisible)
    {
      object.ResetObject();
      BehaviorPreReqAcknowledgeObject preReqData(object.GetObjectID());
      
      return behavior->IsRunnable(preReqData, behavior->IsRunning() );
    }
  }
  return false;
}
  
  
void ReactionTriggerStrategyCubeMoved::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
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


void ReactionTriggerStrategyCubeMoved::EnabledStateChanged(bool enabled)
{
  if(enabled){
    // Mark any blocks with a pose state still known as observed so that
    // we respond to future messaging
    std::vector<const ObservableObject*> blocksOnly;
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    _robot.GetBlockWorld().FindLocatedMatchingObjects(blocksOnlyFilter, blocksOnly);
    
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


void ReactionTriggerStrategyCubeMoved::HandleObjectMoved(const Robot& robot, const ObjectMoved& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStartedMoving(robot, msg);
}

void ReactionTriggerStrategyCubeMoved::HandleObjectStopped(const Robot& robot, const ObjectStoppedMoving& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStoppedMoving(robot);
}

void ReactionTriggerStrategyCubeMoved::HandleObjectUpAxisChanged(const Robot& robot, const ObjectUpAxisChanged& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectUpAxisHasChanged(robot);
}


void ReactionTriggerStrategyCubeMoved::HandleObservedObject(const Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectObserved(robot);
}

void ReactionTriggerStrategyCubeMoved::HandleRobotDelocalized()
{
  for(auto object : _reactionObjects){
    object.ResetObject();
  }
  
}

ReactionTriggerStrategyCubeMoved::Reaction_iter ReactionTriggerStrategyCubeMoved::GetReactionaryIterator(ObjectID objectID)
{
  Reaction_iter iter = std::find(_reactionObjects.begin(), _reactionObjects.end(), objectID);
  if(iter == _reactionObjects.end()){
    _reactionObjects.push_back(ReactionObjectData(objectID));
    iter = _reactionObjects.end();
    iter--;
  }
  
  return iter;
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
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
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
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  return _hasUpAxisChanged && _observedSinceLastReaction;
}

bool ReactionObjectData::ObjectOutsideIgnoreArea(const Robot& robot)
{
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
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
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
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
  const ObservableObject* object = robot.GetBlockWorld().GetLocatedObjectByID(_objectID);
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
