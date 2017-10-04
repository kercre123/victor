/**
 * File: behaviorReactToRobotOnBack.h
 *
 * Author: Brad Neuman
 * Created: 2016-05-06
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnBack.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/components/cliffSensorComponent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const float kWaitTimeBeforeRepeatAnim_s = 0.5f;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToRobotOnBack::BehaviorReactToRobotOnBack(const Json::Value& config)
: IBehavior(config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToRobotOnBack::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToRobotOnBack::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  FlipDownIfNeeded(behaviorExternalInterface);
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::FlipDownIfNeeded(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnBack ) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    // Check if cliff detected
    // If not, then calibrate head because we're not likely to be on back if no cliff detected.
    const auto cliffDataRaw = robot.GetCliffSensorComponent().GetCliffDataRaw();
    if (cliffDataRaw < CLIFF_SENSOR_DROP_LEVEL) {
      AnimationTrigger anim = AnimationTrigger::FlipDownFromBack;
      
      if(behaviorExternalInterface.GetAIComponent().GetWhiteboard().HasHiccups())
      {
        anim = AnimationTrigger::HiccupRobotOnBack;
      }
    
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();
      DelegateIfInControl(new TriggerAnimationAction(robot, anim),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    } else {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      Robot& robot = behaviorExternalInterface.GetRobot();
      LOG_EVENT("BehaviorReactToRobotOnBack.FlipDownIfNeeded.CalibratingHead", "%d", cliffDataRaw);
      DelegateIfInControl(new CalibrateMotorAction(robot, true, false),
                  &BehaviorReactToRobotOnBack::DelayThenFlipDown);
    }
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::DelayThenFlipDown(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( behaviorExternalInterface.GetOffTreadsState() == OffTreadsState::OnBack ) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DelegateIfInControl(new WaitAction(robot, kWaitTimeBeforeRepeatAnim_s),
                &BehaviorReactToRobotOnBack::FlipDownIfNeeded);
  }
  else {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToRobotOnBack);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToRobotOnBack::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

} // namespace Cozmo
} // namespace Anki
