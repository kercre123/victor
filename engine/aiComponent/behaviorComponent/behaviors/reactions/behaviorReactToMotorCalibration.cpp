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

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMotorCalibration.h"


namespace Anki {
namespace Cozmo {

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
  return true;
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
void BehaviorReactToMotorCalibration::HandleWhileActivated(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::MotorCalibration:
    {
      auto& robotInfo = GetBEI().GetRobotInfo();

      if (robotInfo.IsHeadCalibrated() && robotInfo.IsLiftCalibrated()) {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToMotorCalibration.HandleWhileRunning.Stop", "");
        CancelDelegates();
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
