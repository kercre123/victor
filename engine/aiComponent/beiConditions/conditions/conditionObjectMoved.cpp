/**
* File: ConditionObjectMoved.h
*
* Author: Matt Michini - Kevin M. Karol
* Created: 2017/01/11  - 7/5/17
*
* Description: Strategy that indicates when an object Cozmo cant see starts moving
*
* Copyright: Anki, Inc. 2017
*
*
**/

#include "engine/aiComponent/beiConditions/conditions/conditionObjectMoved.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/beiConditions/beiConditionMessageHelper.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const float kMinTimeMoving_ms = 1000;
const float kRadiusRobotTolerance = 50;
}
  
  
class ReactionObjectData{
public:
  ReactionObjectData(ObjectID objectID);
  ObjectID GetObjectID() const;
  bool ObjectHasMovedLongEnough(BehaviorExternalInterface& behaviorExternalInterface);
  bool ObjectUpAxisHasChanged(BehaviorExternalInterface& behaviorExternalInterface);
  bool ObjectOutsideIgnoreArea(BehaviorExternalInterface& behaviorExternalInterface);
  
  // update values for reactionObject
  void ObjectStartedMoving(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectMoved& msg);
  void ObjectStoppedMoving(BehaviorExternalInterface& behaviorExternalInterface);
  void ObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectUpAxisChanged& msg);
  void ObjectObserved(BehaviorExternalInterface& behaviorExternalInterface);
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectMoved::ConditionObjectMoved(const Json::Value& config, BEIConditionFactory& factory)
  : IBEICondition(config, factory)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectMoved::~ConditionObjectMoved()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::InitInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  _messageHelper.reset(new BEIConditionMessageHelper(this, behaviorExternalInterface));

  _messageHelper->SubscribeToTags({{
        EngineToGameTag::ObjectMoved,
        EngineToGameTag::ObjectStoppedMoving,
        EngineToGameTag::RobotObservedObject,
        EngineToGameTag::ObjectUpAxisChanged,
        EngineToGameTag::RobotDelocalized
      }});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionObjectMoved::AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  
  auto objIter = _reactionObjects.begin();
  while(objIter != _reactionObjects.end()){
    const ObservableObject* cube = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(objIter->GetObjectID());
    if(cube == nullptr){
      objIter = _reactionObjects.erase(objIter);
      continue;
    }
    
    static constexpr float kMaxNormalAngle = DEG_TO_RAD(45); // how steep of an angle we can see
    static constexpr float kMinImageSizePix = 0.0f; // just check if we are looking at it

    
    if(objIter->ObjectOutsideIgnoreArea(behaviorExternalInterface)
       && ((objIter->ObjectHasMovedLongEnough(behaviorExternalInterface)) || objIter->ObjectUpAxisHasChanged(behaviorExternalInterface)))
    {
      VisionComponent& visComp = behaviorExternalInterface.GetVisionComponent();
      const bool isVisible = cube->IsVisibleFrom(visComp.GetCamera(),
                                                 kMaxNormalAngle,
                                                 kMinImageSizePix,
                                                 false);
      if(!isVisible){
        objIter->ResetObject();
        return true;
      }
    }
    
    objIter++;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::HandleEvent(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch(event.GetData().GetTag()){
    case EngineToGameTag::ObjectMoved:
    HandleObjectMoved(behaviorExternalInterface, event.GetData().Get_ObjectMoved());
    break;
    case EngineToGameTag::ObjectStoppedMoving:
    HandleObjectStopped(behaviorExternalInterface, event.GetData().Get_ObjectStoppedMoving());
    break;
    case EngineToGameTag::RobotObservedObject:
    HandleObservedObject(behaviorExternalInterface, event.GetData().Get_RobotObservedObject());
    break;
    case EngineToGameTag::ObjectUpAxisChanged:
    HandleObjectUpAxisChanged(behaviorExternalInterface, event.GetData().Get_ObjectUpAxisChanged());
    break;
    case EngineToGameTag::RobotDelocalized:
    HandleRobotDelocalized();
    default:
    break;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::HandleObjectMoved(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectMoved& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStartedMoving(behaviorExternalInterface, msg);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::HandleObjectStopped(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectStoppedMoving& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStoppedMoving(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::HandleObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectUpAxisChanged& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectUpAxisChanged(behaviorExternalInterface, msg);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::HandleObservedObject(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectObserved(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionObjectMoved::HandleRobotDelocalized()
{
  for(auto object : _reactionObjects){
    object.ResetObject();
  }
  
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionObjectMoved::Reaction_iter ConditionObjectMoved::GetReactionaryIterator(ObjectID objectID)
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
, _axisOfAccel(UpAxis::UnknownAxis)
, _isObjectMoving(false)
, _observedSinceLastReaction(false)
, _hasUpAxisChanged(false)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectID ReactionObjectData::GetObjectID() const
{
  return _objectID;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionObjectData::operator==(const ReactionObjectData &other) const
{
  return this->_objectID == other.GetObjectID();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionObjectData::ObjectHasMovedLongEnough(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  if(_isObjectMoving && _observedSinceLastReaction){
    TimeStamp_t time_ms = behaviorExternalInterface.GetRobotInfo().GetLastMsgTimestamp();
    if(_timeStartedMoving != 0 && time_ms - _timeStartedMoving > kMinTimeMoving_ms){
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionObjectData::ObjectUpAxisHasChanged(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  return _hasUpAxisChanged && _observedSinceLastReaction;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionObjectData::ObjectOutsideIgnoreArea(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(object == nullptr){
    return false;
  }
  
  const Pose3d& blockPose = object->GetPose();
  const Pose3d& robotPose = behaviorExternalInterface.GetRobotInfo().GetPose();
  f32 distance = 0;
  bool distanceComputed = ComputeDistanceBetween(blockPose, robotPose, distance);
  if(!distanceComputed){
    return false;
  }
  
  return distance > kRadiusRobotTolerance;
}


// update values for reactionObject
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionObjectData::ObjectStartedMoving(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectMoved& msg)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(object == nullptr){
    _isObjectMoving = false;
    return;
  }else if(!_isObjectMoving){
    _isObjectMoving = true;
    _timeStartedMoving = msg.timestamp;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionObjectData::ObjectStoppedMoving(BehaviorExternalInterface& behaviorExternalInterface)
{
  _isObjectMoving = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionObjectData::ObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::ObjectUpAxisChanged& msg)
{
  if (msg.upAxis != _axisOfAccel) {
    _hasUpAxisChanged = true;
    _axisOfAccel = msg.upAxis;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionObjectData::ObjectObserved(BehaviorExternalInterface& behaviorExternalInterface)
{
  const ObservableObject* object = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_objectID);
  if(object != nullptr){
    // don't trigger reactions while we're directly looking at the block
    ResetObject();
    _observedSinceLastReaction = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionObjectData::ResetObject()
{
  _isObjectMoving = false;
  _hasUpAxisChanged = false;
  _observedSinceLastReaction = false;
  _timeStartedMoving = 0;
}
  

} // namespace Cozmo
} // namespace Anki
