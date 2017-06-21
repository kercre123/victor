/**
 * File: activityExpressNeeds.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-06-20
 *
 * Description: Activity to express severe needs states.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityExpressNeeds.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"

namespace Anki {
namespace Cozmo {

namespace {

static const char* kLockToDisableTriggers = "lockTriggersFullPyramid";

constexpr ReactionTriggerHelpers::FullReactionArray kTriggersToDisable = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kTriggersToDisable),
              "Reaction triggers duplicate or non-sequential");

}

ActivityExpressNeeds::ActivityExpressNeeds(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
{
}

void ActivityExpressNeeds::OnSelectedInternal(Robot& robot)
{
  robot.GetBehaviorManager().DisableReactionsWithLock(kLockToDisableTriggers, kTriggersToDisable);
}

void ActivityExpressNeeds::OnDeselectedInternal(Robot& robot)
{
  robot.GetBehaviorManager().RemoveDisableReactionsLock(kLockToDisableTriggers);
}

}
}
