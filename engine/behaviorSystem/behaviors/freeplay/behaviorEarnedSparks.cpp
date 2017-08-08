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

#include "engine/behaviorSystem/behaviors/freeplay/behaviorEarnedSparks.h"
#include "engine/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "engine/actions/animActions.h"
#include "engine/cozmoContext.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEarnedSparks::BehaviorEarnedSparks(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorEarnedSparks::~BehaviorEarnedSparks()
{  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorEarnedSparks::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  return preReqData.GetRobot().GetContext()->GetNeedsManager()->IsPendingSparksRewardMsg();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorEarnedSparks::InitInternal(Robot& robot)
{
  const AnimationTrigger animTrigger = AnimationTrigger::EarnedSparks;
  StartActing(new TriggerLiftSafeAnimationAction(robot, animTrigger));

  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorEarnedSparks::ResumeInternal(Robot& robot)
{
  // Don't allow resuming
  return RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorEarnedSparks::StopInternal(Robot& robot)
{
  if (robot.GetContext()->GetNeedsManager()->IsPendingSparksRewardMsg())
  {
    robot.GetContext()->GetNeedsManager()->SparksRewardCommunicatedToUser();
  }
}


} // namespace Cozmo
} // namespace Anki
