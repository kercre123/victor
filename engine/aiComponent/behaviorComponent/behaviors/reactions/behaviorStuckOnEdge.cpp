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
namespace Vector {

static const u8 kTracksToLock = (u8)AnimTrackFlag::BODY_TRACK | (u8)AnimTrackFlag::LIFT_TRACK;

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
  ICozmoBehavior::SmartRequestPowerSaveMode();
  TriggerGetInAnim();
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
void BehaviorStuckOnEdge::TriggerGetInAnim()
{
  PRINT_NAMED_INFO("BehaviorStuckOnEdge.TriggerGetInAnim", "");
  auto action = new TriggerAnimationAction(AnimationTrigger::StuckOnEdgeGetIn, 1, true, kTracksToLock);
  DelegateIfInControl(action, &BehaviorStuckOnEdge::TriggerIdleAnim);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::TriggerIdleAnim()
{
  PRINT_NAMED_INFO("BehaviorStuckOnEdge.TriggerIdleAnim", "");
  auto action = new TriggerAnimationAction(AnimationTrigger::StuckOnEdgeIdle, 1, true, kTracksToLock);
  DelegateIfInControl(action, &BehaviorStuckOnEdge::TriggerIdleAnim);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorStuckOnEdge::OnBehaviorDeactivated()
{
}

} // namespace Vector
} // namespace Anki
