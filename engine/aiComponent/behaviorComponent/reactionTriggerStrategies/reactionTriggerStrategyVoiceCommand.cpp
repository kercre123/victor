/**
* File: reactionTriggerStrategyVoiceCommand.cpp
*
* Author: Lee Crippen
* Created: 02/16/17
*
* Description: Reaction trigger strategy for hearing a voice command.
*
* Copyright: Anki, Inc. 2017
*
*
**/


#include "anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyVoiceCommand.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/robotIdleTimeoutComponent.h"
#include "engine/voiceCommands/voiceCommandComponent.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Voice Command";

static const char* kVoiceCommandParamsKey = "voiceCommandParams";
static const char* kIsWakeUpReaction      = "isWakeUpReaction";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyVoiceCommand::ReactionTriggerStrategyVoiceCommand(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, config, kTriggerStrategyName)
{
  const auto& params = config[kVoiceCommandParamsKey];
  JsonTools::GetValueOptional(params, kIsWakeUpReaction, _isWakeUpReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyVoiceCommand::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const BehaviorContainer& BC = behaviorExternalInterface.GetBehaviorContainer();
  std::shared_ptr<BehaviorReactToVoiceCommand> directPtr;
  BC.FindBehaviorByIDAndDowncast(behavior->GetID(),
                                 BehaviorClass::ReactToVoiceCommand,
                                 directPtr);
  directPtr->SetDesiredFace(GetDesiredFace(behaviorExternalInterface));
  behavior->WantsToBeActivated(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyVoiceCommand::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr behavior)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  auto* voiceCommandComponent = robot.GetContext()->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent, "ReactionTriggerStrategyVoiceCommand.ShouldTriggerBehaviorInternal", "VoiceCommandComponent invalid"))
  {
    return false;
  }
  
  if(voiceCommandComponent->KeyPhraseWasHeard())
  {
    const bool robotHasIdleTimeout = robot.GetIdleTimeoutComponent().IdleTimeoutSet();
   
    // If the robot has an idle timeout set (game sets this when Cozmo is going to sleep) and this is
    // the strategy instance that is responsible for managing the "Hey Cozmo" wake up from/cancel sleep
    // behavior then that behavior should run
    if(robotHasIdleTimeout && _isWakeUpReaction)
    {
      voiceCommandComponent->ClearHeardCommand();
      
      DEV_ASSERT(behavior->GetID() == BehaviorID::ReactToVoiceCommand_Wakeup,
                 "ReactionTriggerStrategyVoiceCommand.ShouldTriggerBehaviorInternal.ExpectedWakeUpReaction");
    
      return behavior->WantsToBeActivated(behaviorExternalInterface);
    }
    // Otherwise Cozmo is not going to sleep so the normal "Hey Cozmo" reaction can run
    else if(!robotHasIdleTimeout && !_isWakeUpReaction)
    {
      voiceCommandComponent->ClearHeardCommand();
      
      Vision::FaceID_t desiredFace = GetDesiredFace(behaviorExternalInterface);
      
      if (Vision::UnknownFaceID != desiredFace)
      {
        _lookedAtTimesMap[desiredFace] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
      
      const BehaviorContainer& BC = behaviorExternalInterface.GetBehaviorContainer();
      std::shared_ptr<BehaviorReactToVoiceCommand> directPtr;
      BC.FindBehaviorByIDAndDowncast(behavior->GetID(),
                                     BehaviorClass::ReactToVoiceCommand,
                                     directPtr);
      LOG_INFO("ReactionTriggerStrategyVoiceCommand.ShouldTriggerBehaviorInternal.DesiredFace", "DesiredFaceID: %d", desiredFace);
      directPtr->SetDesiredFace(desiredFace);
      return behavior->WantsToBeActivated(behaviorExternalInterface);
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vision::FaceID_t ReactionTriggerStrategyVoiceCommand::GetDesiredFace(BehaviorExternalInterface& behaviorExternalInterface) const
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  // All recently seen face IDs
  const auto& knownFaceIDs = robot.GetFaceWorld().GetFaceIDs();
  Vision::FaceID_t desiredFace = Vision::UnknownFaceID;
  auto oldestTimeLookedAt_s = std::numeric_limits<float>::max();
  
  for (const auto& faceID : knownFaceIDs)
  {
    // If we don't know where this face is right now, continue on
    const auto* face = robot.GetFaceWorld().GetFace(faceID);
    if(nullptr == face || !face->GetHeadPose().HasSameRootAs(robot.GetPose()))
    {
      continue;
    }
    
    auto dataIter = _lookedAtTimesMap.find(faceID);
    if (dataIter == _lookedAtTimesMap.end())
    {
      // If we don't have a time associated with looking at this face, break out now and use it cause it's new.
      desiredFace = faceID;
      break;
    }
    
    const auto& curLookedTime = dataIter->second;
    if (curLookedTime < oldestTimeLookedAt_s)
    {
      desiredFace = faceID;
      oldestTimeLookedAt_s = curLookedTime;
    }
  }
  
  return desiredFace;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyVoiceCommand::BehaviorThatStrategyWillTriggerInternal(ICozmoBehaviorPtr behavior)
{
  if(behavior != nullptr)
  {
    behavior->AddListener(this);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ReactionTriggerStrategyVoiceCommand::AnimationComplete(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  // When one of the behaviors this strategy is responsible for completes/decides to call
  // its listeners, we want to make sure to set the listening context back to TriggerPhrase
  robot.GetContext()->GetVoiceCommandComponent()->ForceListenContext(VoiceCommand::VoiceCommandListenContext::TriggerPhrase);
}

} // namespace Cozmo
} // namespace Anki
