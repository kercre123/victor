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
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMotorCalibration.h"
#include "engine/robot.h"
#include "engine/robotManager.h"


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
bool BehaviorReactToMotorCalibration::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToMotorCalibration::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  LOG_EVENT("BehaviorReactToMotorCalibration.InitInternalReactionary.Start", "");  
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  // Start a hang action just to keep this behavior alive until the calibration complete message is received
  DelegateIfInControl(new WaitAction(robot, _kTimeout_sec), [&robot](ActionResult res)
    {
      if (IActionRunner::GetActionResultCategory(res) != ActionResultCategory::CANCELLED  &&
          (!robot.IsHeadCalibrated() || !robot.IsLiftCalibrated())) {
        PRINT_NAMED_WARNING("BehaviorReactToMotorCalibration.Timeout",
                            "Calibration didn't complete (lift: %d, head: %d)",
                            robot.IsLiftCalibrated(), robot.IsHeadCalibrated());
      }
    });

  return RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToMotorCalibration::HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::MotorCalibration:
    {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      const Robot& robot = behaviorExternalInterface.GetRobot();

      if (robot.IsHeadCalibrated() && robot.IsLiftCalibrated()) {
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
