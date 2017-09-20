/**
 * File: reactionTriggerStrategyNoPreDockPoses.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to an object with no predock poses
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "engine/aiComponent/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyNoPreDockPoses.h"

#include "engine/aiComponent/behaviorSystem/behaviorContainer.h"
#include "engine/aiComponent/behaviorSystem/behaviorManager.h"
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorRamIntoBlock.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/robot.h"

#include "anki/common/basestation/objectIDs.h"

namespace{
static const char* kTriggerStrategyName = "NoPreDockPoses";
}


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTriggerStrategyNoPreDockPoses::ReactionTriggerStrategyNoPreDockPoses(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IReactionTriggerStrategy(behaviorExternalInterface, config, kTriggerStrategyName)
{
}


void ReactionTriggerStrategyNoPreDockPoses::SetupForceTriggerBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior)
{
  behavior->IsRunnable(behaviorExternalInterface);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ReactionTriggerStrategyNoPreDockPoses::ShouldTriggerBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr behavior)
{
  std::shared_ptr<BehaviorRamIntoBlock> directPtr;
  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByIDAndDowncast(behavior->GetID(),
                                                         BehaviorClass::RamIntoBlock,
                                                         directPtr);
  
  const ObjectID& objID = behaviorExternalInterface.GetAIComponent().GetWhiteboard().GetNoPreDockPosesOnObject();
  if(objID.IsSet()){
    const ObjectID objectWithoutPosesCopy = objID;
    ObjectID unsetObj;
    behaviorExternalInterface.GetAIComponent().GetNonConstWhiteboard().SetNoPreDockPosesOnObject(unsetObj);
    directPtr->SetBlockToRam(objectWithoutPosesCopy.GetValue());
    return behavior->IsRunnable(behaviorExternalInterface);
  }
  return false;
}

  
} // namespace Cozmo
} // namespace Anki
