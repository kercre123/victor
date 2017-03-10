/**
 * File: reactionTriggerStrategyFactory.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Factory for creating reactionTriggerStrategy
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFactory.h"

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyCubeMoved.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFacePositionUpdated.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFistBump.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFrustration.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyGeneric.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyNoPreDockPoses.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyObjectPositionUpdated.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPlacedOnCharger.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPetInitialDetection.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPyramidInitialDetection.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotShaken.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategySparked.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyStackOfCubesInitialDetection.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyVoiceCommand.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/types/behaviorTypes.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

IReactionTriggerStrategy* ReactionTriggerStrategyFactory::
                             CreateReactionTriggerStrategy(Robot& robot,
                                                           const Json::Value& config,
                                                           ReactionTrigger trigger)
{
  IReactionTriggerStrategy* strategy = nullptr;
  switch (trigger) {
    case ReactionTrigger::CliffDetected:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::CliffEvent,
        MessageEngineToGameTag::RobotStopped
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes);
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::CubeMoved:
    {
      strategy = new ReactionTriggerStrategyCubeMoved(robot, config);
      break;
    }
    case ReactionTrigger::DoubleTapDetected:
    {
      strategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      break;
    }
    case ReactionTrigger::FacePositionUpdated:
    {
      strategy = new ReactionTriggerStrategyFacePositionUpdated(robot, config);
      break;
    }
    case ReactionTrigger::FistBump:
    {
      strategy = new ReactionTriggerStrategyFistBump(robot, config);
      break;
    }
    case ReactionTrigger::Frustration:
    {
      strategy = new ReactionTriggerStrategyFrustration(robot, config);
      break;
    }
    case ReactionTrigger::MotorCalibration:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::MotorCalibration
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes, [] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, const Robot& robot) {
        const MotorCalibration& msg = event.GetData().Get_MotorCalibration();
        return (msg.autoStarted && msg.calibStarted);
      });
                                              
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::NoPreDockPoses:
    {
      strategy = new ReactionTriggerStrategyNoPreDockPoses(robot, config);
      break;
    }
    case ReactionTrigger::ObjectPositionUpdated:
    {
      strategy = new ReactionTriggerStrategyObjectPositionUpdated(robot, config);
      break;
    }
    case ReactionTrigger::PlacedOnCharger:
    {
      strategy = new ReactionTriggerStrategyPlacedOnCharger(robot, config);
      break;
    }
    case ReactionTrigger::PetInitialDetection:
    {
      strategy = new ReactionTriggerStrategyPetInitialDetection(robot, config);
      break;
    }
    case ReactionTrigger::RobotPickedUp:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot, const IBehavior* behavior) -> bool {
        return robot.GetOffTreadsState() == OffTreadsState::InAir;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotPlacedOnSlope:
    {
      strategy = new ReactionTriggerStrategyRobotPlacedOnSlope(robot, config);
      break;
    }
    case ReactionTrigger::PyramidInitialDetection:
    {
      strategy = new ReactionTriggerStrategyPyramidInitialDetection(robot, config);
      break;
    }
    case ReactionTrigger::ReturnedToTreads:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::RobotOffTreadsStateChanged
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes, [] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, const Robot& robot) {
        return (event.GetData().Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads);
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnBack:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot, const IBehavior* behavior) -> bool {
        return robot.GetOffTreadsState() == OffTreadsState::OnBack;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnFace:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot, const IBehavior* behavior) -> bool {
        return robot.GetOffTreadsState() == OffTreadsState::OnFace;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnSide:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot, const IBehavior* behavior) -> bool {
        return robot.GetOffTreadsState() == OffTreadsState::OnLeftSide
            || robot.GetOffTreadsState() == OffTreadsState::OnRightSide;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotShaken:
    {
      strategy = new ReactionTriggerStrategyRobotShaken(robot, config);
      break;
    }
    case ReactionTrigger::Sparked:
    {
      strategy = new ReactionTriggerStrategySparked(robot, config);
      break;
    }
    case ReactionTrigger::StackOfCubesInitialDetection:
    {
      strategy = new ReactionTriggerStrategyStackOfCubesInitialDetection(robot, config);
      break;
    }
    case ReactionTrigger::UnexpectedMovement:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::UnexpectedMovement
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes);
      strategy = genericStrategy;
      break;
    }
#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
    case ReactionTrigger::VoiceCommand:
    {
      strategy = new ReactionTriggerStrategyVoiceCommand(robot, config);
      break;
    }
#endif
    case ReactionTrigger::Count:
    case ReactionTrigger::NoneTrigger:
    {
      PRINT_NAMED_ERROR("ReactionTriggerStrategyFactory.InvalidTrigger", "Attempted to make reaction trigger with invalid type");
      strategy = nullptr;
      break;
    }
  }
  
  if(strategy != nullptr){
    strategy->SetReactionTrigger(trigger);
  }
  
  return strategy;
}
  
} // namespace Cozmo
} // namespace Anki
