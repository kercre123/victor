/**
* File: behaviorFeedingSearchForCube.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to anticipate energy being loaded
* into a cube as the user prepares it
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFeedingSearchForCube::BehaviorFeedingSearchForCube(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFeedingSearchForCube::IsRunnableInternal(const BehaviorPreReqNone& preReqData ) const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorFeedingSearchForCube::InitInternal(Robot& robot)
{
  TransitionWaitForFood(robot);
  
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingSearchForCube::TransitionWaitForFood(Robot& robot)
{
  
  AnimationTrigger searchIdle = AnimationTrigger::Count;
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if(currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical)){
    searchIdle = AnimationTrigger::FeedingIdleSearch_Severe;
  }else{
    searchIdle = AnimationTrigger::FeedingIdleSearch_Normal;
  }
  
  IActionRunner* searchAction = new CompoundActionParallel(robot, {
    new SearchForNearbyObjectAction(robot),
    new TriggerAnimationAction(robot, searchIdle)
  });
  
  StartActing(searchAction,
              &BehaviorFeedingSearchForCube::TransitionWaitForFood);
}

} // namespace Cozmo
} // namespace Anki





