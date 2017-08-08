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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorEarnedSparks.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/robot.h"

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
