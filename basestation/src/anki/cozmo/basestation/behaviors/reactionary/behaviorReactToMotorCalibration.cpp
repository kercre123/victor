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

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToMotorCalibration.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"


namespace Anki {
namespace Cozmo {
  
BehaviorReactToMotorCalibration::BehaviorReactToMotorCalibration(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToMotorCalibration");
  
  SubscribeToTriggerTags({
    EngineToGameTag::MotorCalibration
  });
  
  SubscribeToTags({
    EngineToGameTag::MotorCalibration
  });
}

bool BehaviorReactToMotorCalibration::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactToMotorCalibration::InitInternalReactionary(Robot& robot)
{
  LOG_EVENT("BehaviorReactToMotorCalibration.InitInternalReactionary.Start", "");
  
  // Start a hang action just to keep this behavior alive until the calibration complete message is received
  StartActing(new WaitAction(robot, _kTimeout_sec), [this, &robot](ActionResult res) {
                                                    if (res != ActionResult::CANCELLED && (!robot.IsHeadCalibrated() || !robot.IsLiftCalibrated())) {
                                                      PRINT_NAMED_WARNING("BehaviorReactToMotorCalibration.Timeout",
                                                                          "Calibration didn't complete (lift: %d, head: %d)",
                                                                          robot.IsLiftCalibrated(), robot.IsHeadCalibrated());
                                                    }
                                                  });

  return RESULT_OK;
}

bool BehaviorReactToMotorCalibration::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event, const Robot& robot)
{
  switch(event.GetTag())
  {
    case EngineToGameTag::MotorCalibration:
    {
      const MotorCalibration& msg = event.Get_MotorCalibration();
      if(!IsRunning() && msg.autoStarted && msg.calibStarted)
      {
        return true;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToMotorCalibration.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;
    }
  }
  return false;
}

void BehaviorReactToMotorCalibration::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::MotorCalibration:
    {
      if (robot.IsHeadCalibrated() && robot.IsLiftCalibrated()) {
        PRINT_CH_INFO("Behaviors", "BehaviorReactToMotorCalibration.HandleWhileRunning.Stop", "");
        StopActing();
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