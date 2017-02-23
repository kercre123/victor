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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyCliffDetected.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyCubeMoved.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyDoubleTapDetected.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFacePositionUpdated.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFistBump.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFrustration.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyMotorCalibration.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyObjectPositionUpdated.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPlacedOnCharger.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPetInitialDetection.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPickedUp.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotPlacedOnSlope.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPyramidInitialDetection.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyReturnedToTreads.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotOnBack.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotOnFace.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyRobotOnSide.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategySparked.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyStackOfCubesInitialDetection.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyUnexpectedMovement.h"

#include "clad/types/behaviorTypes.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Cozmo {

IReactionTriggerStrategy* ReactionTriggerStrategyFactory::
                             CreateReactionTriggerStrategy(Robot& robot,
                                                           const Json::Value& config,
                                                           ReactionTrigger trigger)
{
  IReactionTriggerStrategy* strategy = nullptr;
  switch (trigger) {
    case ReactionTrigger::CliffDetected:
    {
      strategy = new ReactionTriggerStrategyCliffDetected(robot, config);
      break;
    }
    case ReactionTrigger::CubeMoved:
    {
      strategy = new ReactionTriggerStrategyCubeMoved(robot, config);
      break;
    }
    case ReactionTrigger::DoubleTapDetected:
    {
      strategy = new ReactionTriggerStrategyDoubleTapDetected(robot, config);
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
      strategy = new ReactionTriggerStrategyMotorCalibration(robot, config);
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
      strategy = new ReactionTriggerStrategyRobotPickedUp(robot, config);
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
      strategy = new ReactionTriggerStrategyReturnedToTreads(robot, config);
      break;
    }    case ReactionTrigger::RobotOnBack:
    {
      strategy = new ReactionTriggerStrategyRobotOnBack(robot, config);
      break;
    }
    case ReactionTrigger::RobotOnFace:
    {
      strategy = new ReactionTriggerStrategyRobotOnFace(robot, config);
      break;
    }
    case ReactionTrigger::RobotOnSide:
    {
      strategy = new ReactionTriggerStrategyRobotOnSide(robot, config);
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
      strategy = new ReactionTriggerStrategyUnexpectedMovement(robot, config);
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
