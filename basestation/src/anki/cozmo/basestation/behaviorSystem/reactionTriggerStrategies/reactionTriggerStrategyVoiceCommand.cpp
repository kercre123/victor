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
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyVoiceCommand.h"
#include "anki/cozmo/basestation/behaviors/behaviorObjectiveHelpers.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "VoiceCommands"
#define LOG_INFO(...) PRINT_CH_INFO(LOG_CHANNEL, ##__VA_ARGS__)

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Voice Command";
}
  
ReactionTriggerStrategyVoiceCommand::ReactionTriggerStrategyVoiceCommand(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({
    MessageEngineToGameTag::VoiceCommandEvent
  });
}

bool ReactionTriggerStrategyVoiceCommand::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  if(_shouldTrigger)
  {
    _shouldTrigger = false;
    
    std::set<Vision::FaceID_t> targets;
    targets.insert(_desiredFace);
    BehaviorPreReqAcknowledgeFace acknowledgeFacePreReqs(targets);
    
    LOG_INFO("ReactionTriggerStrategyVoiceCommand.ShouldTriggerBehavior.DesiredFace", "DesiredFaceID: %d", _desiredFace);
    return behavior->IsRunnable(acknowledgeFacePreReqs);
  }
  
  return false;
}

void ReactionTriggerStrategyVoiceCommand::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case EngineToGameTag::VoiceCommandEvent:
    {
      // All recently seen face IDs
      const auto& knownFaceIDs = robot.GetFaceWorld().GetKnownFaceIDs();
      Vision::FaceID_t desiredFace = Vision::UnknownFaceID;
      auto oldestTimeLookedAt_s = std::numeric_limits<float>::max();
      
      for (const auto& faceID : knownFaceIDs)
      {
        auto dataIter = _lookedAtTimesMap.find(faceID);
        if (dataIter == _lookedAtTimesMap.end())
        {
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
      
      if (Vision::UnknownFaceID != desiredFace)
      {
        _desiredFace = desiredFace;
        _lookedAtTimesMap[desiredFace] = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _shouldTrigger = true;
      }
      
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("ReactionTriggerStrategyVoiceCommand.AlwaysHandleInternal.UnhandledEventType",
                        "Type: %s", MessageEngineToGameTagToString(event.GetData().GetTag()));
      break;
    }
  }
}
  
#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
} // namespace Cozmo
} // namespace Anki
