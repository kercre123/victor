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
#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToMotorCalibration.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/robot.h"
#include "engine/robotManager.h"


namespace Anki {
namespace Cozmo {
  
namespace {

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersMotorCalibrationArray = {
  {ReactionTrigger::CliffDetected,                true},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  true},
  {ReactionTrigger::RobotOnSide,                  true},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersMotorCalibrationArray),
              "Reaction triggers duplicate or non-sequential");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToMotorCalibration::BehaviorReactToMotorCalibration(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{  
  SubscribeToTags({
    EngineToGameTag::MotorCalibration
  });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToMotorCalibration::IsRunnableInternal(const Robot& robot) const
{
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToMotorCalibration::InitInternal(Robot& robot)
{
  LOG_EVENT("BehaviorReactToMotorCalibration.InitInternalReactionary.Start", "");
  SmartDisableReactionsWithLock(GetIDStr(), kAffectTriggersMotorCalibrationArray);
  
  // Start a hang action just to keep this behavior alive until the calibration complete message is received
  StartActing(new WaitAction(robot, _kTimeout_sec), [&robot](ActionResult res)
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
