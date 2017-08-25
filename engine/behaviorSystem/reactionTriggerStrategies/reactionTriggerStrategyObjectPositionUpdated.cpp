/**
 * File: reactionTriggerStrategyObject.cpp
 *
 * Author: Andrew Stein :: Kevin M. Karol
 * Created: 2016-06-14 :: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyObjectPositionUpdated.h"

#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/behaviors/reactions/behaviorAcknowledgeObject.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Strategy React To Object Position Updated";
CONSOLE_VAR(bool, kEnableObjectAcknowledgement, "BehaviorAcknowledgeObject", true);
  
std::set<ObjectFamily> _objectFamilies = {{
  ObjectFamily::LightCube,
  ObjectFamily::Block
}};
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyObjectPositionUpdated::ReactionTriggerStrategyObjectPositionUpdated(Robot& robot, const Json::Value& config)
:ReactionTriggerStrategyPositionUpdate(robot, config, kTriggerStrategyName, ReactionTrigger::ObjectPositionUpdated)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyObjectPositionUpdated::SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior) {
  if (HasDesiredReactionTargets(robot)){
    std::set<s32> targets;
    GetDesiredReactionTargets(robot, targets);
    
    std::shared_ptr<BehaviorAcknowledgeObject> directPtr;
    robot.GetBehaviorManager().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                           BehaviorClass::AcknowledgeObject,
                                                           directPtr);
    
    directPtr->SetObjectsToAcknowledge(targets);
  }
  else
  {
    PRINT_NAMED_WARNING("ReactionTriggerStrategyObjectPositionUpdated.SetupForceTriggerBehavior", "No target to update");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyObjectPositionUpdated::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior)
{
  const bool robotInValidState = kEnableObjectAcknowledgement &&
                                  !robot.GetCarryingComponent().IsCarryingObject() &&
                                  !robot.GetDockingComponent().IsPickingOrPlacing() &&
                                  !robot.IsOnChargerPlatform();
  
  if(robotInValidState && HasDesiredReactionTargets(robot)){
    std::set<s32> targets;
    GetDesiredReactionTargets(robot, targets);
 
    std::shared_ptr<BehaviorAcknowledgeObject> directPtr;
    robot.GetBehaviorManager().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                           BehaviorClass::AcknowledgeObject,
                                                           directPtr);
    
    directPtr->SetObjectsToAcknowledge(targets);
    return behavior->IsRunnable(robot);
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyObjectPositionUpdated::HandleObjectObserved(const Robot& robot,
                                                     const ExternalInterface::RobotObservedObject& msg)
{
  // Object must be in one of the families this behavior cares about
  const bool hasValidFamily = _objectFamilies.count(msg.objectFamily) > 0;
  if(!hasValidFamily) {
    return;
  }
  
  // check if we want to react based on pose and cooldown, and also update position data even if we don't
  // react
  Pose3d obsPose( msg.pose, robot.GetPoseOriginList() );
  
  // ignore cubes we are carrying or docking to (don't react to them)
  if(msg.objectID == robot.GetCarryingComponent().GetCarryingObject() ||
     msg.objectID == robot.GetDockingComponent().GetDockObject())
  {
    const bool considerReaction = false;
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp, considerReaction);
  }
  else {
    HandleNewObservation(robot, msg.objectID, obsPose, msg.timestamp);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyObjectPositionUpdated::AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject: {
      // only update target blocks if we are running
      const IBehaviorPtr currentBehavior = robot.GetBehaviorManager().GetCurrentBehavior();
      if(currentBehavior != nullptr &&
         currentBehavior->GetClass() != _classTriggerMapsTo){
        HandleObjectObserved(robot, event.GetData().Get_RobotObservedObject());
      }
      break;
    }
    
    case EngineToGameTag::RobotDelocalized:
    {
      // this is passed through from the parent class - it's valid to be receiving this
      // we just currently don't use it for anything.  Case exists so we don't print errors.
      break;
    }
    
    default:
    PRINT_NAMED_ERROR("BehaviorAcknowledgeObject.HandleWhileNotRunning.InvalidTag",
                      "Received event with unhandled tag %hhu.",
                      event.GetData().GetTag());
    break;
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyObjectPositionUpdated::BehaviorThatStrategyWillTriggerInternal(IBehaviorPtr behavior)
{
  behavior->AddListener(this);
  _classTriggerMapsTo = behavior->GetClass();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyObjectPositionUpdated::ReactedToID(Robot& robot, s32 id)
{
  RobotReactedToId(robot, id);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyObjectPositionUpdated::ClearDesiredTargets(Robot& robot)
{
  std::set<s32> targets;
  GetDesiredReactionTargets(robot, targets);
  for(auto target: targets){
    RobotReactedToId(robot, target);
  }
}
    
  
} // namespace Cozmo
} // namespace Anki
