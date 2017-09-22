/**
 * File: BehaviorEarnedSparks
 *
 * Author: Paul Terry
 * Created: 07/18/2017
 *
 * Description: Simple behavior for Cozmo playing an animaton when he has earned sparks
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorEarnedSparks.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/cozmoContext.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEarnedSparks::BehaviorEarnedSparks(const Json::Value& config)
: IBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEarnedSparks::~BehaviorEarnedSparks()
{  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEarnedSparks::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
  if(needsManager != nullptr){
    return needsManager->IsPendingSparksRewardMsg();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorEarnedSparks::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  const AnimationTrigger animTrigger = AnimationTrigger::EarnedSparks;
  StartActing(new TriggerLiftSafeAnimationAction(robot, animTrigger));

  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorEarnedSparks::ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Don't allow resuming
  return RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEarnedSparks::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
  if(needsManager != nullptr &&
     needsManager->IsPendingSparksRewardMsg()){
    needsManager->SparksRewardCommunicatedToUser();
  }
}


} // namespace Cozmo
} // namespace Anki
