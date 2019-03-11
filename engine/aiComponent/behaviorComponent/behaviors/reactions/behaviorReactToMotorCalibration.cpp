/**
 * File: BehaviorReactToMotorCalibration.cpp
 *
 * Author: Kevin Yoon
 * Created: 11/2/2016
 *
 * Description: Behavior for reacting to automatic motor calibration
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMotorCalibration.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotorCalibration::BehaviorReactToMotorCalibration(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    RobotInterface::RobotToEngineTag::motorCalibration
  });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToMotorCalibration::WantsToBeActivatedBehavior() const
{
  return _headMotorCalibrationStarted || _liftMotorCalibrationStarted;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::OnBehaviorActivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();

  // motor calibration currently can interrupt streaming and listening behaviors. If this happens an
  // in-opportune time, we could drop a voice command (intent) and respond with "can't do that" even though
  // the robot should be able to do it. This is a bit of a complex issue and requires more thought, but for
  // now just disable the intent timeout while calibration is active to allow us to respond to the voice
  // command after calibrating. Note that if no intent is pending (now or later while this behavior is
  // active), this will have no effect
  GetBehaviorComp<UserIntentComponent>().SetUserIntentTimeoutEnabled( false );

  // Start a hang action just to keep this behavior alive until the calibration complete message is received
  auto waitLambda = [&robotInfo](Robot& robot) {
    return robotInfo.IsHeadCalibrated() && robotInfo.IsLiftCalibrated();
  };
  auto timedoutLambda = [&robotInfo]() {
    if (!robotInfo.IsHeadCalibrated() || !robotInfo.IsLiftCalibrated()) {
      LOG_WARNING("BehaviorReactToMotorCalibration.Timedout", "");
    }
  };
  DelegateIfInControl(new WaitForLambdaAction(waitLambda, _kTimeout_sec), timedoutLambda);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::OnBehaviorDeactivated()
{
  // can be deactivated by cancelling itself, or timing out waiting
  // for the calibration finished messages from the robot
  _headMotorCalibrationStarted = false;
  _liftMotorCalibrationStarted = false;

  // re-enable user intent (voice command) timeout. See comment in OnBehaviorActivated
  GetBehaviorComp<UserIntentComponent>().SetUserIntentTimeoutEnabled( true );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::HandleWhileInScopeButNotActivated(const RobotToEngineEvent& event)
{
  switch(event.GetData().GetTag()) {
    case RobotInterface::RobotToEngineTag::motorCalibration:
    {
      auto& payload = event.GetData().Get_motorCalibration();
      if (payload.calibStarted && payload.autoStarted) {
        if (payload.motorID == MotorID::MOTOR_HEAD) {
          _headMotorCalibrationStarted = true;
        }
        if (payload.motorID == MotorID::MOTOR_LIFT) {
          _liftMotorCalibrationStarted = true;
        }
      }
      break;
    }
    default:
      LOG_ERROR("BehaviorReactToMotorCalibration.HandleWhileRunning.BadEventType",
                "Calling HandleWhileRunning with an event we don't care about, this is a bug");
      break;
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::AlwaysHandleInScope(const RobotToEngineEvent& event)
{
  switch(event.GetData().GetTag()) {
    case RobotInterface::RobotToEngineTag::motorCalibration:
    {
      auto& payload = event.GetData().Get_motorCalibration();
      if (!payload.calibStarted) {
        if (payload.motorID == MotorID::MOTOR_HEAD) {
          _headMotorCalibrationStarted = false;
        }
        if (payload.motorID == MotorID::MOTOR_LIFT) {
          _liftMotorCalibrationStarted = false;
        }
      }
      break;
    }
    default:
      LOG_ERROR("BehaviorReactToMotorCalibration.HandleWhileRunning.BadEventType",
                "Calling HandleWhileRunning with an event we don't care about, this is a bug");
      break;
  }
}


}
}
