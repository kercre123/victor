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

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyCubeMoved.h"


#include "engine/activeObject.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeCubeMoved.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/visionComponent.h"
#include "engine/robot.h"
#include "clad/types/behaviorComponent/behaviorTypes.h"

namespace Anki {
namespace Cozmo {
  
  
namespace {
static const char* kTriggerStrategyName = "Trigger Strategy Cube Moved";
}
  
  
class ReactionObjectData{
public:
  ReactionObjectData(ObjectID objectID);
  ObjectID GetObjectID() const;
  bool ObjectHasMovedLongEnough(BehaviorExternalInterface& behaviorExternalInterface);
  bool ObjectUpAxisHasChanged(BehaviorExternalInterface& behaviorExternalInterface);
  bool ObjectOutsideIgnoreArea(BehaviorExternalInterface& behaviorExternalInterface);
  
  // update values for reactionObject
  void ObjectStartedMoving(BehaviorExternalInterface& behaviorExternalInterface, const ObjectMoved& msg);
  void ObjectStoppedMoving(BehaviorExternalInterface& behaviorExternalInterface);
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
  
  
//////
/// ReactAcknowledge Cube Moved
/////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyCubeMoved::ReactionTriggerStrategyCubeMoved(BehaviorExternalInterface& behaviorExternalInterface,
                                                                   IExternalInterface* robotExternalInterface,
                                                                   const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, robotExternalInterface,
                           config, kTriggerStrategyName)
{
  SubscribeToTags({{
    EngineToGameTag::ObjectMoved,
    EngineToGameTag::ObjectStoppedMoving,
    EngineToGameTag::RobotObservedObject,
    EngineToGameTag::ObjectUpAxisChanged,
    EngineToGameTag::RobotDelocalized
  }});
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  std::shared_ptr<BehaviorAcknowledgeCubeMoved> directPtr;
  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                                               BehaviorClass::ReactToCubeMoved,
                                                                               directPtr);
  
  //  already failed to find a suitable cube so just grab the first valid cube
  auto objIter = _reactionObjects.begin();
  while(objIter != _reactionObjects.end()){
    const ObservableObject* cube = behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(objIter->GetObjectID());
    if(cube == nullptr){
      objIter = _reactionObjects.erase(objIter);
      continue;
    }
    
    objIter->ResetObject();
    directPtr->SetObjectToAcknowledge(objIter->GetObjectID());
    return;
  }
  
  PRINT_NAMED_WARNING("ReactionTriggerStrategyCubeMoved.SetupForceTriggerBehavior", "No cubes found to force behavior");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyCubeMoved::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  std::shared_ptr<BehaviorAcknowledgeCubeMoved> directPtr;
  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                         BehaviorClass::ReactToCubeMoved,
                                                         directPtr);
  
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
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      const Robot& robot = behaviorExternalInterface.GetRobot();
      const bool isVisible = cube->IsVisibleFrom(robot.GetVisionComponent().GetCamera(),
                                                 kMaxNormalAngle,
                                                 kMinImageSizePix,
                                                 false);
      if(!isVisible){
        objIter->ResetObject();
        directPtr->SetObjectToAcknowledge(objIter->GetObjectID());
        
        // To avoid the assert that IsActivatable can't be checked while the behavior
        // is running, grab direct access to WantsToBeActivatedInternal instead
        if(behavior->IsActivated()){
          return true;
        }else{
          return behavior->WantsToBeActivated(behaviorExternalInterface);
        }
      }
    }
    
    objIter++;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::AlwaysHandleInternal(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  if(robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::ObjectPositionUpdated)){
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
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::EnabledStateChanged(BehaviorExternalInterface& behaviorExternalInterface, bool enabled)
{
  if(enabled){
    // Mark any blocks with a pose state still known as observed so that
    // we respond to future messaging
    std::vector<const ObservableObject*> blocksOnly;
    BlockWorldFilter blocksOnlyFilter;
    blocksOnlyFilter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(blocksOnlyFilter, blocksOnly);
    
    for(const auto& block: blocksOnly){
      if(block->IsPoseStateKnown()){
        Reaction_iter it = GetReactionaryIterator(block->GetID());
        it->ObjectObserved(behaviorExternalInterface);
      }
    }
  }
  else{
    for(auto& object: _reactionObjects){
      object.ResetObject();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::HandleObjectMoved(BehaviorExternalInterface& behaviorExternalInterface, const ObjectMoved& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStartedMoving(behaviorExternalInterface, msg);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::HandleObjectStopped(BehaviorExternalInterface& behaviorExternalInterface, const ObjectStoppedMoving& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectStoppedMoving(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::HandleObjectUpAxisChanged(BehaviorExternalInterface& behaviorExternalInterface, const ObjectUpAxisChanged& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectUpAxisHasChanged(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::HandleObservedObject(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg)
{
  auto iter = GetReactionaryIterator(msg.objectID);
  iter->ObjectObserved(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyCubeMoved::HandleRobotDelocalized()
{
  for(auto object : _reactionObjects){
    object.ResetObject();
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

  
  
} // namespace Cozmo
} // namespace Anki
