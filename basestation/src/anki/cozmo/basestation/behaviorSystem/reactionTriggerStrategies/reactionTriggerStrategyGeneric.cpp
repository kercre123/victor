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

static const char* kWantsToRunStrategyConfigKey = "wantsToRunStrategyConfig";
static const char* kStrategyGenericJson =
  "{"
  "   \"strategyType\" : \"Generic\""
  "}";
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
  
  // Dynamically add a "wantsToRunStrategyConfig" json value to the passed in config
  // so that the generic wantsToRunStrategy is created by the base class
  Json::Reader reader;
  Json::Value genericStrategyConfig;
  const bool parsedOK = reader.parse(kStrategyGenericJson, genericStrategyConfig, false);
  DEV_ASSERT(parsedOK, "ReactionTriggerStrategyGeneric.CreateReactionTriggerStrategyGeneric.FailedToParseJson");
  
  Json::Value configCopy = config;
  configCopy[kWantsToRunStrategyConfigKey] = genericStrategyConfig;
  
  return new ReactionTriggerStrategyGeneric(robot, configCopy, strategyName);
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

void ReactionTriggerStrategyGeneric::SetupForceTriggerBehavior(const Robot& robot, const IBehaviorPtr behavior)
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
bool ReactionTriggerStrategyGeneric::ShouldTriggerBehaviorInternal(const Robot& robot, const IBehaviorPtr behavior)
{
  if(ANKI_VERIFY(_wantsToRunStrategy != nullptr,
                 "ReactionTriggerStrategyNoPreDockPoses.ShouldTriggerBehaviorInternal",
                 "WantsToRunStrategyNotSpecified")){
    
    const bool isRunnable = (behavior->IsRunning() ||
                             (_needsRobotPreReq ?
                              behavior->IsRunnable(BehaviorPreReqRobot(robot)) :
                              behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs)));
    
    return isRunnable && _wantsToRunStrategy->WantsToRun(robot);
  }
  
  return false;
}

void ReactionTriggerStrategyGeneric::SetShouldTriggerCallback(StrategyGeneric::ShouldTriggerCallbackType callback)
{
  StrategyGeneric* strategy = GetGenericWantsToRunStrategy();
  strategy->SetShouldTriggerCallback(callback);
}
  
void ReactionTriggerStrategyGeneric::ConfigureRelevantEvents(std::set<EngineToGameTag> relevantEvents,
                                                             StrategyGeneric::E2GHandleCallbackType callback)
{
  StrategyGeneric* strategy = GetGenericWantsToRunStrategy();
  strategy->ConfigureRelevantEvents(relevantEvents, callback);
}
  
void ReactionTriggerStrategyGeneric::ConfigureRelevantEvents(std::set<GameToEngineTag> relevantEvents,
                                                             StrategyGeneric::G2EHandleCallbackType callback)
{
  StrategyGeneric* strategy = GetGenericWantsToRunStrategy();
  strategy->ConfigureRelevantEvents(relevantEvents, callback);
}

StrategyGeneric* ReactionTriggerStrategyGeneric::GetGenericWantsToRunStrategy() const
{
  StrategyGeneric* strategy;
  if(ANKI_DEV_CHEATS)
  {
    strategy = dynamic_cast<StrategyGeneric*>(_wantsToRunStrategy.get());
    DEV_ASSERT(strategy != nullptr, "ReactionTriggerStrategyGeneric.GetGenericWantsToRunStrategy.Null");
  }
  else
  {
    strategy = static_cast<StrategyGeneric*>(_wantsToRunStrategy.get());
  }
  return strategy;
}

} // namespace Cozmo
} // namespace Anki
