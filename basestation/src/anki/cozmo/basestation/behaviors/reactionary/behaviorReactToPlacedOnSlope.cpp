/**
 * File: behaviorReactToPlacedOnSlope.cpp
 *
 * Author: Matt Michini
 * Created: 01/25/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPlacedOnSlope.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersPlacedOnSlopeArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::DoubleTapDetected,            false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::PyramidInitialDetection,      false},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::StackOfCubesInitialDetection, false},
  {ReactionTrigger::UnexpectedMovement,           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersPlacedOnSlopeArray),
              "Reaction triggers duplicate or non-sequential");
  
} // end namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPlacedOnSlope::BehaviorReactToPlacedOnSlope(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("ReactToPlacedOnSlope");

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPlacedOnSlope::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToPlacedOnSlope::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetName(), kAffectTriggersPlacedOnSlopeArray);

  const double now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasBehaviorRunRecently = (now - _lastBehaviorTime < 10.0);
  
  // Double check that we should play the animation or recalibrate:
  if (hasBehaviorRunRecently && _endedOnInclineLastTime) {
    // Don't run the animation. Instead, run a motor cal since his head may be out of calibration.
    LOG_EVENT("BehaviorReactToPlacedOnSlope.CalibratingHead", "%f", robot.GetPitchAngle().getDegrees());
    StartActing(new CalibrateMotorAction(robot, true, false));
    _endedOnInclineLastTime = false;
  } else {
    // Play the animation then check if we're still on a slope or if we were perched on something:
    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::ReactToPerchedOnBlock), &BehaviorReactToPlacedOnSlope::CheckPitch);
  }

  _lastBehaviorTime = now;
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPlacedOnSlope::CheckPitch(Robot& robot)
{
  // Was the robot on an inclined surface or was the lift simply perched on something?
  _endedOnInclineLastTime = (robot.GetPitchAngle().getDegrees() > 10.0f);
}


} // namespace Cozmo
} // namespace Anki
