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

#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToPlacedOnSlope.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersPlacedOnSlopeArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersPlacedOnSlopeArray),
              "Reaction triggers duplicate or non-sequential");
  
} // end namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPlacedOnSlope::BehaviorReactToPlacedOnSlope(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPlacedOnSlope::IsRunnableInternal(const Robot& robot) const
{
  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToPlacedOnSlope::InitInternal(Robot& robot)
{
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersPlacedOnSlopeArray);

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
    
    AnimationTrigger reactionAnim = AnimationTrigger::ReactToPerchedOnBlock;
    
    // special animations for maintaining eye shape in severe need states
    const NeedId severeNeedExpressed = robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression();
    if(NeedId::Energy == severeNeedExpressed){
      reactionAnim = AnimationTrigger::NeedsSevereLowEnergySlopeReact;
    }else if(NeedId::Repair == severeNeedExpressed){
      reactionAnim = AnimationTrigger::NeedsSevereLowRepairSlopeReact;
    }
    
    StartActing(new TriggerAnimationAction(robot, reactionAnim),
                &BehaviorReactToPlacedOnSlope::CheckPitch);
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
