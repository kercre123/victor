/**
 * File: behaviorStuckOnEdge.cpp
 *
 * Author: Kevin Yoon
 * Created: 2018-05-08
 *
 * Description: Behavior that periodically plays a "distressed" animation because
 *              he's stuck and needs help from the user to get out of his situation.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorStuckOnEdge.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/sensors/cliffSensorComponent.h"

namespace Anki {
namespace Cozmo {

static const float kWaitTimeBeforeRepeatAnim_s = 5.f;
static const u8 kTracksToLock = (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::LIFT_TRACK | (u8)AnimTrackFlag::HEAD_TRACK;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorStuckOnEdge::BehaviorStuckOnEdge(const Json::Value& config)
: ICozmoBehavior(config)
{
  _iConfig.stuckOnEdgeCondition = BEIConditionFactory::CreateBEICondition(BEIConditionType::StuckOnEdge, GetDebugLabel());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorStuckOnEdge::WantsToBeActivatedBehavior() const
{
  return _iConfig.stuckOnEdgeCondition->AreConditionsMet(GetBEI());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::InitBehavior()
{
  _iConfig.stuckOnEdgeCondition->Init(GetBEI());
  _iConfig.stuckOnEdgeCondition->SetActive(GetBEI(), true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::OnBehaviorActivated()
{
  TriggerStuckOnEdgeAnimation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }

  bool isPickedUp =  GetBEI().GetRobotInfo().IsPickedUp();
  bool noCliffs   = !GetBEI().GetRobotInfo().GetCliffSensorComponent().IsCliffDetected();
  if (isPickedUp || noCliffs) {
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::TriggerStuckOnEdgeAnimation()
{
  PRINT_NAMED_INFO("BehaviorStuckOnEdge.TriggerStuckOnEdgeAnimation", "");
  CompoundActionSequential *action = new CompoundActionSequential();
  action->AddAction(new TriggerAnimationAction(AnimationTrigger::StuckOnEdge, 1, true, kTracksToLock));
  action->AddAction(new WaitAction(kWaitTimeBeforeRepeatAnim_s));
  DelegateIfInControl(action, &BehaviorStuckOnEdge::TriggerStuckOnEdgeAnimation);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::OnBehaviorDeactivated()
{
}

} // namespace Cozmo
} // namespace Anki
