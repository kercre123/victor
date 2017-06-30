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

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {

constexpr ReactionTriggerHelpers::FullReactionArray kTriggersToDisable = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
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
  {ReactionTrigger::RobotShaken,                  true},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           true},
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
  _severeNeedExpression = NeedIdFromString(
    JsonTools::ParseString(config, "severeNeedExpression", "ActivityExpressNeeds.ConfigError.SevereNeedExpressed"));

  _clearSevereNeedExpressionOnExit = JsonTools::ParseBool(config,
                                                        "clearSevereNeedExpressionOnExit",
                                                        "ActivityExpressNeeds.ConfigError.clearStateOnExit");
  _shouldDisableReactions = JsonTools::ParseBool(config,
                                                 "disableReactionTriggers",
                                                 "ActivityExpressNeeds.ConfigError.disableReactionTrigger");
}

void ActivityExpressNeeds::OnSelectedInternal(Robot& robot)
{
    // set severe needs expression
  if( _params._severeNeedExpression != NeedId::Count ) {
    robot.GetAIComponent().GetWhiteboard().SetSevereNeedExpression( _params._severeNeedExpression );
  }

  if( _params._shouldDisableReactions ) {
    robot.GetBehaviorManager().DisableReactionsWithLock(GetIDStr(), kTriggersToDisable);
  }
}

void ActivityExpressNeeds::OnDeselectedInternal(Robot& robot)
{
  if( _params._shouldDisableReactions ) {
    robot.GetBehaviorManager().RemoveDisableReactionsLock(GetIDStr());
  }

  if( _params._severeNeedExpression != NeedId::Count && _params._clearSevereNeedExpressionOnExit ) {
    robot.GetAIComponent().GetWhiteboard().ClearSevereNeedExpression();
  }
}

}
}
