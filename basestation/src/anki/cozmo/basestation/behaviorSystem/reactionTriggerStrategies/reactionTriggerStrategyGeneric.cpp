/**
* File: reactionTriggerStrategyGeneric.cpp
*
* Author: Lee Crippen
* Created: 02/15/2017
*
* Description: Generic Reaction Trigger strategy for responding to many configurations of reaction triggers.
*
* Copyright: Anki, Inc. 2017
*
*
**/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyGeneric.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/messageHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/logging/logging.h"


namespace{
static const char* kReactionConfigKey = "genericStrategyParams";
static const char* kShouldResumeLastKey = "shouldResumeLast";
static const char* kCanInterruptOtherTriggeredBehaviorKey = "canInterruptOtherTriggeredBehavior";
static const char* kTriggerStrategyNameKey = "debugStrategyName";
static const char* kNeedsRobotPreReqKey = "needsRobotPreReq";
}

namespace Anki {
namespace Cozmo {
  

// We have a static function for creating an instance because we want to pull some data out of the config
// and pass it into the constructor
ReactionTriggerStrategyGeneric* ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(Robot& robot,
                                                                                                     const Json::Value& config)
{
  std::string strategyName = "Trigger Strategy Generic";
  const Json::Value& genericParams = config[kReactionConfigKey];
  if(genericParams.isNull())
  {
    DEV_ASSERT_MSG(!genericParams.isNull(), "ReactionTriggerStrategyGeneric.CreateReactionTriggerStrategyGeneric.NoGenericParams",
                   "No Params provided for setting up this ReactionTriggerStrategyGeneric");
    return nullptr;
  }
  
  JsonTools::GetValueOptional(genericParams, kTriggerStrategyNameKey, strategyName);
  return new ReactionTriggerStrategyGeneric(robot, config, strategyName);
}

// Private constructor so that only the static creation function above can make an instance
ReactionTriggerStrategyGeneric::ReactionTriggerStrategyGeneric(Robot& robot,
                                                               const Json::Value& config,
                                                               std::string strategyName)
: IReactionTriggerStrategy(robot, config, strategyName)
, _strategyName(std::move(strategyName))
, _canInterruptOtherTriggeredBehavior(IReactionTriggerStrategy::CanInterruptOtherTriggeredBehavior())
{
  // Pull out config values from the Json
  const Json::Value& genericParams = config[kReactionConfigKey];
  if(!genericParams.isNull())
  {
    JsonTools::GetValueOptional(genericParams, kShouldResumeLastKey, _shouldResumeLast);
    JsonTools::GetValueOptional(genericParams, kCanInterruptOtherTriggeredBehaviorKey, _canInterruptOtherTriggeredBehavior);
    JsonTools::GetValueOptional(genericParams, kNeedsRobotPreReqKey, _needsRobotPreReq);
  }
  else
  {
    DEV_ASSERT_MSG(!genericParams.isNull(), "ReactionTriggerStrategyGeneric.Constructor.NoGenericParams",
                   "No Params provided for setting up this ReactionTriggerStrategyGeneric");
  }
}

void ReactionTriggerStrategyGeneric::SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  if (_needsRobotPreReq)
  {
    behavior->IsRunnable(BehaviorPreReqRobot(robot));
  }
  else
  {
    behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
}
  
// Overrides interface to allow for instances with different configurations to decide how they should trigger.
// If the ShouldTriggerCallback isn't set, there are no relevant events being watched, and other values are
// default, this boils down to only checking behavior->IsRunnable with no prereqs.
bool ReactionTriggerStrategyGeneric::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior)
{
  if(_relevantEvents.empty() || _shouldTrigger)
  {
    _shouldTrigger = false;
    
    // If the callback isn't set we treat it as returning true
    if (!_shouldTriggerCallback || _shouldTriggerCallback(robot, behavior))
    {
      if (_needsRobotPreReq)
      {
        return behavior->IsRunnable(BehaviorPreReqRobot(robot));
      }
      else
      {
        return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
      }
    }
  }
  
  return false;
}
  
// Overrides interface to allow for instances with different configurations to decide how they want to handle events
// they're listening to. Includes a check to make sure an event being handled is one that has actually been
// requested when the class was configured. With no callback specified this simply marks that computational switch
// should occur whenever a relevant event is handled.
void ReactionTriggerStrategyGeneric::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  // Don't do anything if this trigger can't interrupt itself and this reaction trigger
  //  caused the current behavior to run.
  if (!CanInterruptSelf() &&
      robot.GetBehaviorManager().GetCurrentReactionTrigger() == GetReactionTrigger())
  {
    return;
  }
  
  if (_relevantEvents.find(event.GetData().GetTag()) == _relevantEvents.end())
  {
    PRINT_NAMED_ERROR("ReactionTriggerStrategyGeneric.AlwaysHandleInternal.BadEventType",
                      "GenericStrategy named %s not configured to handle tag type %s",
                      _strategyName.c_str(),
                      MessageEngineToGameTagToString(event.GetData().GetTag()));
    return;
  }
  
  if (!_eventHandleCallback || _eventHandleCallback(event, robot))
  {
    _shouldTrigger = true;
  }
}

void ReactionTriggerStrategyGeneric::ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents, EventHandleCallbackType callback)
{
  if (!ANKI_VERIFY(_relevantEvents.empty(), "ReactionTriggerStrategyGeneric::SetEventHandleCallback.EventsAlreadySet", ""))
  {
    return;
  }
  
  if (!ANKI_VERIFY(!relevantEvents.empty(), "ReactionTriggerStrategyGeneric::SetEventHandleCallback.NewEventsNotSpecified", ""))
  {
    return;
  }
  
  _relevantEvents = relevantEvents; // keep a copy on this instance for validation later
  SubscribeToTags(std::move(relevantEvents));
  _eventHandleCallback = callback;
}

} // namespace Cozmo
} // namespace Anki
