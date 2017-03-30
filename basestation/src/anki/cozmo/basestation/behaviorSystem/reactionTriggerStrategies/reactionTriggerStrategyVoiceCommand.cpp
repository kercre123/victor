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

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyVoiceCommand.h"
#include "anki/cozmo/basestation/behaviors/behaviorObjectiveHelpers.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandComponent.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Voice Command";
}
  
ReactionTriggerStrategyVoiceCommand::ReactionTriggerStrategyVoiceCommand(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
}

bool ReactionTriggerStrategyVoiceCommand::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  auto* voiceCommandComponent = robot.GetContext()->GetVoiceCommandComponent();
  if (!ANKI_VERIFY(voiceCommandComponent, "ReactionTriggerStrategyVoiceCommand.ShouldTriggerBehavior", "VoiceCommandComponent invalid"))
  {
    return false;
  }
  
  if(voiceCommandComponent->KeyPhraseWasHeard())
  {
    voiceCommandComponent->ClearHeardCommand();
    
    Vision::FaceID_t desiredFace = GetDesiredFace(robot);
    
    if (Vision::UnknownFaceID != desiredFace)
    {
      _lookedAtTimesMap[desiredFace] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    }
    
    std::set<Vision::FaceID_t> targets;
    targets.insert(desiredFace);
    BehaviorPreReqAcknowledgeFace acknowledgeFacePreReqs(targets, robot);
    
    LOG_INFO("ReactionTriggerStrategyVoiceCommand.ShouldTriggerBehavior.DesiredFace", "DesiredFaceID: %d", desiredFace);
    return behavior->IsRunnable(acknowledgeFacePreReqs);
  }
  
  return false;
}

Vision::FaceID_t ReactionTriggerStrategyVoiceCommand::GetDesiredFace(const Robot& robot) const
{
  // All recently seen face IDs
  const auto& knownFaceIDs = robot.GetFaceWorld().GetFaceIDs();
  Vision::FaceID_t desiredFace = Vision::UnknownFaceID;
  auto oldestTimeLookedAt_s = std::numeric_limits<float>::max();
  
  for (const auto& faceID : knownFaceIDs)
  {
    // If we don't know where this face is right now, continue on
    const auto* face = robot.GetFaceWorld().GetFace(faceID);
    Pose3d pose;
    if(nullptr == face || !face->GetHeadPose().GetWithRespectTo(robot.GetPose(), pose))
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

} // namespace Cozmo
} // namespace Anki
#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
