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

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorExpressNeeds.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
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
static const char* kSupportCharger = "playOnChargerWithoutBody";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExpressNeeds::BehaviorExpressNeeds(const Json::Value& config)
: ICozmoBehavior(config)
, _need(NeedId::Count)
, _requiredBracket(NeedBracketId::Count)
, _cooldownEvaluator( new Util::GraphEvaluator2d() )
, _shouldClearExpressedState(false)
, _caresAboutExpressedState(false)
, _supportCharger(false)
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

  // do anims work on the charger?
  _supportCharger = config.get(kSupportCharger, false).asBool();
  
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
    if(behaviorExternalInterface.HasNeedsManager()){
      auto& needsManager = behaviorExternalInterface.GetNeedsManager();
      // first check if we are in the right needs bracket
      NeedsState& currNeedState = needsManager.GetCurNeedsStateMutable();
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
void BehaviorExpressNeeds::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{  
  CompoundActionSequential* action = new CompoundActionSequential();

  auto& robotInfo = behaviorExternalInterface.GetRobotInfo();

  if( !robotInfo.IsOnChargerPlatform() ) {
    // only turn towards the last face if we aren't on the charger
    // TODO:(bn) support "look with head and eyes" when we are on the charger.
    
    const bool ignoreFailure = true;
    action->AddAction(new TurnTowardsLastFacePoseAction(), ignoreFailure);
  }

  for( const auto& trigger : _animTriggers ) {
    const u32 numLoops = 1;
    const bool interruptRunning = true;
    const u8 tracksToLock = GetTracksToLock(behaviorExternalInterface);
    IAction* playAnim = new TriggerLiftSafeAnimationAction(trigger,
                                                           numLoops,
                                                           interruptRunning,
                                                           tracksToLock);

    action->AddAction(playAnim);
  }
  
  DelegateIfInControl(action, [this](ActionResult res) {
      if( IActionRunner::GetActionResultCategory(res) == ActionResultCategory::SUCCESS ) {
        // only update the variable for the last time we express the animation if it completes successfully
        // (wasn't interrupted)
        _lastTimeExpressed = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      }
    });

  
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
  if(behaviorExternalInterface.HasNeedsManager()){
    auto& needsManager = behaviorExternalInterface.GetNeedsManager();
    const NeedsState& currNeedState = needsManager.GetCurNeedsState();
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
u8 BehaviorExpressNeeds::GetTracksToLock(BehaviorExternalInterface& behaviorExternalInterface) const
{
  if( _supportCharger ) {
    auto& robotInfo = behaviorExternalInterface.GetRobotInfo();
    if( robotInfo.IsOnChargerPlatform() ) {
      // we are supporting the charger and are on it, so lock out the body
      return (u8)AnimTrackFlag::BODY_TRACK;
    }
  }

  // otherwise nothing to lock
  return (u8)AnimTrackFlag::NO_TRACKS;   
}

}
}
