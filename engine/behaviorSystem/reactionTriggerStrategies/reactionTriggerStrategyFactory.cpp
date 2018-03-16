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

#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFactory.h"

#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyCubeMoved.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFacePositionUpdated.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFistBump.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFrustration.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyGeneric.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyHiccup.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyNoPreDockPoses.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyObjectPositionUpdated.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPlacedOnCharger.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPetInitialDetection.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotShaken.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategySparked.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

#include "clad/types/behaviorSystem/strategyTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

namespace {
static const char* kStrategyToCreateKey = "strategyToCreate";
}

using namespace ExternalInterface;

IReactionTriggerStrategy* ReactionTriggerStrategyFactory::
                             CreateReactionTriggerStrategy(Robot& robot,
                                                           const Json::Value& config,
                                                           ReactionTrigger trigger)
{
  WantsToRunStrategyType strategyToCreate = WantsToRunStrategyType::Invalid;
  std::string strategyToCreateString = "";
  JsonTools::GetValueOptional(config, kStrategyToCreateKey, strategyToCreateString);
  if(!strategyToCreateString.empty())
  {
    strategyToCreate = WantsToRunStrategyTypeFromString(strategyToCreateString);
  }

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
      // If the current reaction trigger is already CliffDetected, then we don't want to trigger it again
      // unless it "canInterruptSelf". This prevents the CliffEvent message (which comes in ~0.5 sec after
      // the RobotStopped msg) from causing the ReactToCliff behavior to run a second time.
      auto callback = [genericStrategy](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, const Robot& robot) {
        const bool cliffDetectedTriggerEnabled = robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::CliffDetected);
          return cliffDetectedTriggerEnabled &&
                ((robot.GetBehaviorManager().GetCurrentReactionTrigger() != ReactionTrigger::CliffDetected) || genericStrategy->CanInterruptSelf());
        };
      genericStrategy->ConfigureRelevantEvents(relevantTypes, callback);
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::CubeMoved:
    {
      strategy = new ReactionTriggerStrategyCubeMoved(robot, config);
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
    case ReactionTrigger::Hiccup:
    {
      strategy = new ReactionTriggerStrategyHiccup(robot, config);
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
    case ReactionTrigger::RobotFalling:
    {
      const int timeout_ms = 3000;
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::FallingStarted
      };
      genericStrategy->ConfigureRelevantEventsWithTimeout(relevantTypes,[] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, const Robot& robot) {
        return robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::RobotFalling);
      }, timeout_ms);
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotPickedUp:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot) -> bool {
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
    case ReactionTrigger::ReturnedToTreads:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::RobotOffTreadsStateChanged
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes, [] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, const Robot& robot) {
        const bool returnToTreadsTriggerEnabled = robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::ReturnedToTreads);
        return returnToTreadsTriggerEnabled && (event.GetData().Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads);
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnBack:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot) -> bool {
        return robot.GetOffTreadsState() == OffTreadsState::OnBack;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnFace:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot) -> bool {
        return robot.GetOffTreadsState() == OffTreadsState::OnFace;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnSide:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(robot, config);
      genericStrategy->SetShouldTriggerCallback([] (const Robot& robot) -> bool {
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
