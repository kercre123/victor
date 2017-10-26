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

#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyFactory.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyCubeMoved.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyFacePositionUpdated.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyFistBump.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyFrustration.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyGeneric.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyHiccup.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyNoPreDockPoses.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyObjectPositionUpdated.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyPlacedOnCharger.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyPetInitialDetection.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyRobotShaken.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategySparked.h"
#include "engine/aiComponent/behaviorComponent/reactionTriggerStrategies/reactionTriggerStrategyVoiceCommand.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"

#include "clad/types/behaviorComponent/strategyTypes.h"

#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

namespace {
static const char* kStrategyToCreateKey = "strategyToCreate";
}

using namespace ExternalInterface;

IReactionTriggerStrategy* ReactionTriggerStrategyFactory::
                             CreateReactionTriggerStrategy(BehaviorExternalInterface& behaviorExternalInterface,
                                                           IExternalInterface* robotExternalInterface,
                                                           const Json::Value& config,
                                                           ReactionTrigger trigger)
{
  StateConceptStrategyType strategyToCreate = StateConceptStrategyType::Invalid;
  std::string strategyToCreateString = "";
  JsonTools::GetValueOptional(config, kStrategyToCreateKey, strategyToCreateString);
  if(!strategyToCreateString.empty())
  {
    strategyToCreate = StateConceptStrategyTypeFromString(strategyToCreateString);
  }

  IReactionTriggerStrategy* strategy = nullptr;
  switch (trigger) {
    case ReactionTrigger::CliffDetected:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::CliffEvent,
        MessageEngineToGameTag::RobotStopped
      };
      // If the current reaction trigger is already CliffDetected, then we don't want to trigger it again
      // unless it "canInterruptSelf". This prevents the CliffEvent message (which comes in ~0.5 sec after
      // the RobotStopped msg) from causing the ReactToCliff behavior to run a second time.
      auto callback = [genericStrategy](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, BehaviorExternalInterface& behaviorExternalInterface) {
        // DEPRECATED - Grabbing robot to support current cozmo code, but this should
        // be removed
        const Robot& robot = behaviorExternalInterface.GetRobot();
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
      strategy = new ReactionTriggerStrategyCubeMoved(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::FacePositionUpdated:
    {
      strategy = new ReactionTriggerStrategyFacePositionUpdated(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::FistBump:
    {
      strategy = new ReactionTriggerStrategyFistBump(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::Frustration:
    {
      strategy = new ReactionTriggerStrategyFrustration(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::Hiccup:
    {
      strategy = new ReactionTriggerStrategyHiccup(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::MotorCalibration:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::MotorCalibration
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes, [] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, BehaviorExternalInterface& behaviorExternalInterface) {
        const MotorCalibration& msg = event.GetData().Get_MotorCalibration();
        return (msg.autoStarted && msg.calibStarted);
      });
                                              
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::NoPreDockPoses:
    {
      strategy = new ReactionTriggerStrategyNoPreDockPoses(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::ObjectPositionUpdated:
    {
      strategy = new ReactionTriggerStrategyObjectPositionUpdated(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::PlacedOnCharger:
    {
      strategy = new ReactionTriggerStrategyPlacedOnCharger(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::PetInitialDetection:
    {
      strategy = new ReactionTriggerStrategyPetInitialDetection(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::RobotPickedUp:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      genericStrategy->SetShouldTriggerCallback([] (BehaviorExternalInterface& behaviorExternalInterface) -> bool {
        return behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::InAir;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotPlacedOnSlope:
    {
      strategy = new ReactionTriggerStrategyRobotPlacedOnSlope(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::ReturnedToTreads:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::RobotOffTreadsStateChanged
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes, [] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event, BehaviorExternalInterface& behaviorExternalInterface) {
        // DEPRECATED - Grabbing robot to support current cozmo code, but this should
        // be removed
        const Robot& robot = behaviorExternalInterface.GetRobot();
        const bool returnToTreadsTriggerEnabled = robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::ReturnedToTreads);
        return returnToTreadsTriggerEnabled && (event.GetData().Get_RobotOffTreadsStateChanged().treadsState == OffTreadsState::OnTreads);
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnBack:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      genericStrategy->SetShouldTriggerCallback([] (BehaviorExternalInterface& behaviorExternalInterface) -> bool {
        return behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnBack;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnFace:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      genericStrategy->SetShouldTriggerCallback([] (BehaviorExternalInterface& behaviorExternalInterface) -> bool {
        return behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnFace;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotOnSide:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      genericStrategy->SetShouldTriggerCallback([] (BehaviorExternalInterface& behaviorExternalInterface) -> bool {
        return behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnLeftSide
            || behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnRightSide;
      });
      
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::RobotShaken:
    {
      strategy = new ReactionTriggerStrategyRobotShaken(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::Sparked:
    {
      strategy = new ReactionTriggerStrategySparked(behaviorExternalInterface, robotExternalInterface, config);
      break;
    }
    case ReactionTrigger::UnexpectedMovement:
    {
      auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
      std::set<MessageEngineToGameTag> relevantTypes =
      {
        MessageEngineToGameTag::UnexpectedMovement
      };
      genericStrategy->ConfigureRelevantEvents(relevantTypes);
      strategy = genericStrategy;
      break;
    }
    case ReactionTrigger::VC:
    {
      const StateConceptStrategyType kGenericStrategyType = StateConceptStrategyType::Generic;
      if(strategyToCreate == kGenericStrategyType)
      {
        // Generic strategy wants to run when IdleTimeout is cancelled (cancelled by game when waking up while
        // going to sleep). The VC reaction trigger needs to be enabled and the current reaction can't be VC or
        // PlacedOnCharger (We expect to potentially receive a CancelIdleTimeout message from game when
        // on the charger and we don't want that to trigger the strategy).
        // This strategy is used to trigger reactToVoiceCommand_Wakeup when sleeping is cancelled via a cancel
        // button
        auto* genericStrategy = ReactionTriggerStrategyGeneric::CreateReactionTriggerStrategyGeneric(behaviorExternalInterface, robotExternalInterface, config);
        std::set<MessageGameToEngineTag> relevantTypes =
        {
          MessageGameToEngineTag::CancelIdleTimeout
        };
        genericStrategy->ConfigureRelevantEvents(relevantTypes,
         [](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event, BehaviorExternalInterface& behaviorExternalInterface)
         {
           // DEPRECATED - Grabbing robot to support current cozmo code, but this should
           // be removed
           const Robot& robot = behaviorExternalInterface.GetRobot();
           const bool currentTriggerNotVC = (robot.GetBehaviorManager().GetCurrentReactionTrigger() !=
                                             ReactionTrigger::VC);
           const bool currentTriggerNotOnCharger = (robot.GetBehaviorManager().GetCurrentReactionTrigger() !=
                                                    ReactionTrigger::PlacedOnCharger);
           const bool VCTriggerEnabled = robot.GetBehaviorManager().IsReactionTriggerEnabled(ReactionTrigger::VC);
           
           return (VCTriggerEnabled && currentTriggerNotVC && currentTriggerNotOnCharger);
         }
        );
        
        strategy = genericStrategy;
      }
      else
      {
        strategy = new ReactionTriggerStrategyVoiceCommand(behaviorExternalInterface, robotExternalInterface, config);
      }
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
