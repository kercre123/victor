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
#include "clad/externalInterface/messageEngineToGame.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"


namespace Anki {
namespace Vector {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotorCalibration::BehaviorReactToMotorCalibration(const Json::Value& config)
: ICozmoBehavior(config)
{
  SubscribeToTags({
    EngineToGameTag::MotorCalibration
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
  PRINT_NAMED_INFO("BehaviorReactToMotorCalibration.InitInternalReactionary.Start", "");
  auto& robotInfo = GetBEI().GetRobotInfo();

  // Start a hang action just to keep this behavior alive until the calibration complete message is received
  DelegateIfInControl(new WaitAction(_kTimeout_sec), [&robotInfo](ActionResult res)
    {
      if (IActionRunner::GetActionResultCategory(res) != ActionResultCategory::CANCELLED  &&
          (!robotInfo.IsHeadCalibrated() || !robotInfo.IsLiftCalibrated())) {
        PRINT_NAMED_WARNING("BehaviorReactToMotorCalibration.Timeout",
                            "Calibration didn't complete (lift: %d, head: %d)",
                            robotInfo.IsLiftCalibrated(), robotInfo.IsHeadCalibrated());
      }
    });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::OnBehaviorDeactivated()
{
  // can be deactivated by cancelling itself, or timing out waiting
  // for the calibration finished messages from the robot
  _headMotorCalibrationStarted = false;
  _liftMotorCalibrationStarted = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::HandleWhileInScopeButNotActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::MotorCalibration:
    {
      auto& payload = event.GetData().Get_MotorCalibration();
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
      PRINT_NAMED_ERROR("BehaviorReactToMotorCalibration.HandleWhileRunning.BadEventType",
                        "Calling HandleWhileRunning with an event we don't care about, this is a bug");
      break;
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::MotorCalibration:
    {
      auto& payload = event.GetData().Get_MotorCalibration();
      if (!payload.calibStarted) {
        if (payload.motorID == MotorID::MOTOR_HEAD) {
          _headMotorCalibrationStarted = false;
        }
        if (payload.motorID == MotorID::MOTOR_LIFT) {
          _liftMotorCalibrationStarted = false;
        }
      }
      // calibration stop messages are received
      if (!_headMotorCalibrationStarted && !_liftMotorCalibrationStarted) {
        CancelSelf();
      }
      break;
    }
    default:
      PRINT_NAMED_ERROR("BehaviorReactToMotorCalibration.HandleWhileRunning.BadEventType",
                        "Calling HandleWhileRunning with an event we don't care about, this is a bug");
      break;
  }
}


}
}
