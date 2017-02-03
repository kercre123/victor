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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyObjectPositionUpdated.h"

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/robot.h"
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
  
  
ReactionTriggerStrategyObjectPositionUpdated::ReactionTriggerStrategyObjectPositionUpdated(Robot& robot, const Json::Value& config)
:ReactionTriggerStrategyPositionUpdate(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({
    EngineToGameTag::RobotObservedObject,
  });
}
  
  
bool ReactionTriggerStrategyObjectPositionUpdated::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  const bool robotInValidState = kEnableObjectAcknowledgement &&
                                  !robot.IsCarryingObject() &&
                                  !robot.IsPickingOrPlacing() &&
                                  !robot.IsOnCharger() &&
                                  !robot.IsOnChargerPlatform();
  
  if(robotInValidState && HasDesiredReactionTargets(robot)){
    std::set<s32> targets;
    GetDesiredReactionTargets(robot, targets);
    
    BehaviorPreReqAcknowledgeObject acknowledgeObjectPreReqs(targets);
    return behavior->IsRunnable(acknowledgeObjectPreReqs);
  }
  
  return false;
}
  
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
  if(msg.objectID == robot.GetCarryingObject() ||
     msg.objectID == robot.GetDockObject())
  {
    const bool considerReaction = false;
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp, considerReaction);
  }
  else {
    HandleNewObservation(msg.objectID, obsPose, msg.timestamp);
  }
}
  
void ReactionTriggerStrategyObjectPositionUpdated::AlwaysHandlePoseBasedInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotObservedObject: {
      // only update target blocks if we are running
      const IBehavior* currentBehavior = robot.GetBehaviorManager().GetCurrentBehavior();
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

void ReactionTriggerStrategyObjectPositionUpdated::BehaviorThatStrategyWillTrigger(IBehavior* behavior)
{
  behavior->AddListener(this);
  _classTriggerMapsTo = behavior->GetClass();
}

  
void ReactionTriggerStrategyObjectPositionUpdated::ReactedToID(Robot& robot, s32 id)
{
  RobotReactedToId(robot, id);
}
  
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
