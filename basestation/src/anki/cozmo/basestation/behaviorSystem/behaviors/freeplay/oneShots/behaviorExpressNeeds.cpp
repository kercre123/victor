/**
 * File: behaviorExpressNeeds.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-06-08
 *
 * Description: Play a one-shot animation to express needs states with a built-in cooldown based on needs
 *              levels
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/oneShots/behaviorExpressNeeds.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/cozmoContext.h"

namespace Anki {
namespace Cozmo {

BehaviorExpressNeeds::BehaviorExpressNeeds(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _cooldownEvaluator( new Util::GraphEvaluator2d() )
{
  {
    const auto& needStr = JsonTools::ParseString(config,
                                                 "need",
                                                 "BehaviorExpressNeeds.ConfigError.Need");
    _need = NeedIdFromString(needStr);
  }

  {
    const auto& needBracketStr = JsonTools::ParseString(config,
                                                        "needBracket",
                                                        "BehaviorExpressNeeds.ConfigError.NeedLevel");
    _requiredBracket = NeedBracketIdFromString(needBracketStr);
  }

  {
    const Json::Value& cooldownEvaluatorConfig = config["cooldown"];
    if( ANKI_VERIFY( !cooldownEvaluatorConfig.isNull(),
                     "BehaviorExpressNeeds.ConfigError.NoCooldownConfig",
                     "Behavior %s did not specify a cooldown config",
                     GetIDStr().c_str())) {
      const bool result = _cooldownEvaluator->ReadFromJson(cooldownEvaluatorConfig);

      if( !result ) {
        PRINT_NAMED_ERROR("BehaviorExpressNeeds.ConfigError.CooldownParsingFailed",
                          "Behavior '%s' failed to parse cooldown graph evaluator",
                          GetIDStr().c_str() );
      }
    }
  }

  {
    const auto& animTriggerString = JsonTools::ParseString(config,
                                                           "animTrigger",
                                                           "BehaviorExpressNeeds.ConfigError.AnimTrigger");
    _animTrigger = AnimationTriggerFromString(animTriggerString);
  }
}

bool BehaviorExpressNeeds::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  const Robot& robot = preReqData.GetRobot();

  // first check if we are in the right needs bracket
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if( !currNeedState.IsNeedAtBracket(_need, _requiredBracket) ) {
    return false;
  }

  // now check if we're on cooldown
  
  const float cooldown_s = GetCooldownSec(robot);
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const bool onCooldown = _lastTimeExpressed + cooldown_s >= currTime_s;

  return !onCooldown;  
}


Result BehaviorExpressNeeds::InitInternal(Robot& robot)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  const bool ignoreFailure = true;
  action->AddAction(new TurnTowardsLastFacePoseAction(robot), ignoreFailure);

  action->AddAction(new TriggerAnimationAction(robot, _animTrigger));
  
  StartActing(action, [this](ActionResult res) {
      if( IActionRunner::GetActionResultCategory(res) == ActionResultCategory::SUCCESS ) {
        // only update the variable for the last time we express the animation if it completes successfully
        // (wasn't interrupted)
        _lastTimeExpressed = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    });

  return Result::RESULT_OK;
}

Result BehaviorExpressNeeds::ResumeInternal(Robot& robot)
{
  // don't resume, since it will run again anyway if it wants to
  return Result::RESULT_FAIL;
}

void BehaviorExpressNeeds::StopInternal(Robot& robot)
{

}

float BehaviorExpressNeeds::GetCooldownSec(const Robot& robot) const
{
  const NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsState();
  const float level = currNeedState.GetNeedLevel(_need);

  if( ANKI_VERIFY( _cooldownEvaluator != nullptr,
                   "BehaviorExpressNeeds.GetCooldown.NullEvaluator",
                   "%s: Cooldown graph evaluator was null, must be a config error",
                   GetIDStr().c_str()) ) {
    const float cooldown = _cooldownEvaluator->EvaluateY( level );
    return cooldown;
  }

  return 0.0f;
}

}
}
