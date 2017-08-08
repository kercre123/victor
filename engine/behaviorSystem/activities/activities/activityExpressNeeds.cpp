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

#include "engine/behaviorSystem/activities/activities/activityExpressNeeds.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/robot.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr ReactionTriggerHelpers::FullReactionArray kTriggersToDisable = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  true},
  {ReactionTrigger::RobotOnSide,                  true},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           false},
  {ReactionTrigger::VC,                           false}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kTriggersToDisable),
              "Reaction triggers duplicate or non-sequential");

}

ActivityExpressNeeds::ActivityExpressNeeds(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _params(config)
{
}

ActivityExpressNeeds::Params::Params(const Json::Value& config)
{
  _shouldDisableReactions = JsonTools::ParseBool(config,
                                                 "disableReactionTriggers",
                                                 "ActivityExpressNeeds.ConfigError.disableReactionTrigger");
}

void ActivityExpressNeeds::OnSelectedInternal(Robot& robot)
{
  if( _params._shouldDisableReactions ) {
    robot.GetBehaviorManager().DisableReactionsWithLock(GetIDStr(), kTriggersToDisable);
  }
}

void ActivityExpressNeeds::OnDeselectedInternal(Robot& robot)
{
  if( _params._shouldDisableReactions ) {
    robot.GetBehaviorManager().RemoveDisableReactionsLock(GetIDStr());
  }
}

}
}
