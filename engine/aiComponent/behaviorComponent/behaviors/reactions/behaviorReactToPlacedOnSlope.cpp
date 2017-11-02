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

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPlacedOnSlope.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/robot.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPlacedOnSlope::BehaviorReactToPlacedOnSlope(const Json::Value& config)
: ICozmoBehavior(config)
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPlacedOnSlope::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToPlacedOnSlope::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  const double now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasBehaviorRunRecently = (now - _lastBehaviorTime < 10.0);
  
  // Double check that we should play the animation or recalibrate:
  if (hasBehaviorRunRecently && _endedOnInclineLastTime) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // Don't run the animation. Instead, run a motor cal since his head may be out of calibration.
    LOG_EVENT("BehaviorReactToPlacedOnSlope.CalibratingHead", "%f", robot.GetPitchAngle().getDegrees());
    DelegateIfInControl(new CalibrateMotorAction(robot, true, false));
    _endedOnInclineLastTime = false;
  } else {
    // Play the animation then check if we're still on a slope or if we were perched on something:
    
    AnimationTrigger reactionAnim = AnimationTrigger::ReactToPerchedOnBlock;
    
    // special animations for maintaining eye shape in severe need states
    const NeedId severeNeedExpressed = behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression();
    if(NeedId::Energy == severeNeedExpressed){
      reactionAnim = AnimationTrigger::NeedsSevereLowEnergySlopeReact;
    }else if(NeedId::Repair == severeNeedExpressed){
      reactionAnim = AnimationTrigger::NeedsSevereLowRepairSlopeReact;
    }
    
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new TriggerAnimationAction(robot, reactionAnim),
                &BehaviorReactToPlacedOnSlope::CheckPitch);
  }

  _lastBehaviorTime = now;
  return Result::RESULT_OK;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPlacedOnSlope::CheckPitch(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  // Was the robot on an inclined surface or was the lift simply perched on something?
  _endedOnInclineLastTime = (robot.GetPitchAngle().getDegrees() > 10.0f);
}


} // namespace Cozmo
} // namespace Anki
