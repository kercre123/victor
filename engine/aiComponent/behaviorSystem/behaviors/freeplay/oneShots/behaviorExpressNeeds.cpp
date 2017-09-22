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

#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/oneShots/behaviorExpressNeeds.h"

#include "anki/common/basestation/jsonTools.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "engine/actions/compoundActions.h"
#include "engine/cozmoContext.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kAnimTriggersKey              = "animTriggers";
static const char* kNeedConfigKey                = "need";
static const char* kNeedBracketConfigKey         = "needBracket";
static const char* kCooldownConfigKey            = "cooldown";

static const char* kClearExpressedStateConfigKey      = "shouldClearExpressedState";
static const char* kCaresAboutExpressedStateConfigKey = "caresAboutExpressedState";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExpressNeeds::BehaviorExpressNeeds(const Json::Value& config)
: IBehavior(config)
, _need(NeedId::Count)
, _requiredBracket(NeedBracketId::Count)
, _cooldownEvaluator( new Util::GraphEvaluator2d() )
, _shouldClearExpressedState(false)
, _caresAboutExpressedState(false)
{
  {
    const auto& needStr = JsonTools::ParseString(config,
                                                 kNeedConfigKey,
                                                 "BehaviorExpressNeeds.ConfigError.Need");
    _need = NeedIdFromString(needStr);
    ANKI_VERIFY(_need != NeedId::Count,
                "BehaviorExpressNeeds.Constructor.InvalidNeed",
                "Need should not be count for behavior %s",
                GetIDStr().c_str());

  }

  {
    const auto& needBracketStr = JsonTools::ParseString(config,
                                                        kNeedBracketConfigKey,
                                                        "BehaviorExpressNeeds.ConfigError.NeedLevel");
    _requiredBracket = NeedBracketIdFromString(needBracketStr);
  }

  {
    const Json::Value& cooldownEvaluatorConfig = config[kCooldownConfigKey];
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
  
  // Check should clear expressed state
  JsonTools::GetValueOptional(config, kClearExpressedStateConfigKey, _shouldClearExpressedState);

  JsonTools::GetValueOptional(config, kCaresAboutExpressedStateConfigKey, _caresAboutExpressedState);
  
  // load anim triggers
  for (const auto& trigger : config[kAnimTriggersKey])
  {
    // make sure each trigger is valid
    const AnimationTrigger animTrigger = AnimationTriggerFromString(trigger.asString().c_str());
    DEV_ASSERT_MSG(animTrigger != AnimationTrigger::Count, "BehaviorExpressNeeds.InvalidTriggerString",
                   "'%s'", trigger.asString().c_str());
    if (animTrigger != AnimationTrigger::Count) {
      _animTriggers.emplace_back( animTrigger );
    }
  }
  // make sure we loaded at least one trigger
  DEV_ASSERT_MSG(!_animTriggers.empty(), "BehaviorExpressNeeds.NoTriggers",
                 "Behavior '%s'", GetIDStr().c_str());  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExpressNeeds::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if(_caresAboutExpressedState &&
     (_requiredBracket == NeedBracketId::Critical)){
    if(_need != behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression()){
      return false;
    }
  }else{
    auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
    if(needsManager != nullptr){
      // first check if we are in the right needs bracket
      NeedsState& currNeedState = needsManager->GetCurNeedsStateMutable();
      if( !currNeedState.IsNeedAtBracket(_need, _requiredBracket) ) {
        return false;
      }
    }
  }

  // now check if we're on cooldown
  
  const float cooldown_s = GetCooldownSec(behaviorExternalInterface);
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const bool onCooldown = _lastTimeExpressed + cooldown_s >= currTime_s;

  return !onCooldown;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExpressNeeds::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  CompoundActionSequential* action = new CompoundActionSequential(robot);

  const bool ignoreFailure = true;
  action->AddAction(new TurnTowardsLastFacePoseAction(robot), ignoreFailure);

  for( const auto& trigger : _animTriggers ) {
    action->AddAction(new TriggerAnimationAction(robot, trigger));
  }
  
  StartActing(action, [this](ActionResult res) {
      if( IActionRunner::GetActionResultCategory(res) == ActionResultCategory::SUCCESS ) {
        // only update the variable for the last time we express the animation if it completes successfully
        // (wasn't interrupted)
        _lastTimeExpressed = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    });

  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExpressNeeds::ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // don't resume, since it will run again anyway if it wants to
  return Result::RESULT_FAIL;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExpressNeeds::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(_shouldClearExpressedState){
    behaviorExternalInterface.GetAIComponent().GetSevereNeedsComponent().ClearSevereNeedExpression();
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorExpressNeeds::GetCooldownSec(BehaviorExternalInterface& behaviorExternalInterface) const
{
  auto needsManager = behaviorExternalInterface.GetNeedsManager().lock();
  if(needsManager != nullptr){
    const NeedsState& currNeedState = needsManager->GetCurNeedsState();
    const float level = currNeedState.GetNeedLevel(_need);

    if( ANKI_VERIFY( _cooldownEvaluator != nullptr,
                     "BehaviorExpressNeeds.GetCooldown.NullEvaluator",
                     "%s: Cooldown graph evaluator was null, must be a config error",
                     GetIDStr().c_str()) ) {
      const float cooldown = _cooldownEvaluator->EvaluateY( level );
      return cooldown;
    }
  }
  return 0.0f;
}

}
}
