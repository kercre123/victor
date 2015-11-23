/**
 * File: behaviorFactory
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Factory for creating behaviors from data / messages
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
// Behaviors:
#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"
#include "anki/cozmo/basestation/behaviors/behaviorFollowMotion.h"
#include "anki/cozmo/basestation/behaviors/behaviorInteractWithFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayAnim.h"
#include "anki/cozmo/basestation/behaviors/behaviorReactToCliff.h"
#include "anki/cozmo/basestation/behaviors/behaviorReactToPickup.h"
#include "anki/cozmo/basestation/behaviors/behaviorUnityDriven.h"


namespace Anki {
namespace Cozmo {


IBehavior* BehaviorFactory::CreateBehavior(BehaviorType behaviorType, Robot& robot, const Json::Value& config)
{
  IBehavior* newBehavior = nullptr;
  
  switch (behaviorType)
  {
    case BehaviorType::NoneBehavior:
    {
      newBehavior = new BehaviorNone(robot, config);
      break;
    }
    case BehaviorType::LookAround:
    {
      newBehavior = new BehaviorLookAround(robot, config);
      break;
    }
    case BehaviorType::OCD:
    {
      newBehavior = new BehaviorOCD(robot, config);
      break;
    }
    case BehaviorType::Fidget:
    {
      newBehavior = new BehaviorFidget(robot, config);
      break;
    }
    case BehaviorType::InteractWithFaces:
    {
      newBehavior = new BehaviorInteractWithFaces(robot, config);
      break;
    }
    case BehaviorType::ReactToPickup:
    {
      newBehavior = new BehaviorReactToPickup(robot, config);
      break;
    }
    case BehaviorType::ReactToCliff:
    {
      newBehavior = new BehaviorReactToCliff(robot, config);
      break;
    }
    case BehaviorType::FollowMotion:
    {
      newBehavior = new BehaviorFollowMotion(robot, config);
      break;
    }
    case BehaviorType::PlayAnim:
    {
      newBehavior = new BehaviorPlayAnim(robot, config);
      break;
    }
    case BehaviorType::UnityDriven:
    {
      newBehavior = new BehaviorUnityDriven(robot, config);
      break;
    }
    case BehaviorType::Count:
    {
      PRINT_NAMED_ERROR("BehaviorFactory.CreateBehavior.BadType", "Unexpected type '%s'", EnumToString(behaviorType));
      break;
    }
  }
  
  return newBehavior;
}

  
} // namespace Cozmo
} // namespace Anki

