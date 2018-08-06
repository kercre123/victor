/**
 * File: behaviorReactToUnexpectedMovement.cpp
 *
 * Author: Al Chaussee
 * Created: 7/11/2016
 *
 * Description: Behavior for reacting to unexpected movement like being spun while moving
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToUnexpectedMovement.h"
#include "engine/aiComponent/beiConditions/conditions/conditionUnexpectedMovement.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/robotManager.h"
#include "engine/moodSystem/moodManager.h"
#include "util/helpers/templateHelpers.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL    "BehaviorReactToUnexpectedMovement"

namespace Anki {
namespace Cozmo {

namespace {
  const char* kRepeatedActivationWindowKey = "repeatedActivationWindow_sec";
  const char* kNumRepeatedActivationsAllowedKey = "numRepeatedActivationsAllowed";
  const char* kRetreatDistanceKey = "retreatDistance_mm";
  const char* kRetreatSpeedKey = "retreatSpeed_mmps";
}

void BehaviorReactToUnexpectedMovement::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kRepeatedActivationWindowKey,
    kNumRepeatedActivationsAllowedKey,
    kRetreatDistanceKey,
    kRetreatSpeedKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToUnexpectedMovement::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  repeatedActivationCheckWindow_sec = JsonTools::ParseFloat(config, kRepeatedActivationWindowKey, debugName);
  numRepeatedActivationsAllowed = JsonTools::ParseUInt32(config, kNumRepeatedActivationsAllowedKey, debugName);
  retreatDistance_mm = JsonTools::ParseFloat(config, kRetreatDistanceKey, debugName);
  if (ANKI_VERIFY(Util::IsFltLEZero(retreatDistance_mm),
                  (debugName + ".NonPositiveDistance").c_str(),
                  "Retreat distance should always be positive (not %f). Making positive.",
                  retreatDistance_mm))
  {
    retreatDistance_mm *= -1.0;
  }
  retreatSpeed_mmps = JsonTools::ParseFloat(config, kRetreatSpeedKey, debugName);
  if (ANKI_VERIFY(Util::IsFltLEZero(retreatSpeed_mmps),
                  (debugName + ".NonPositiveSpeed").c_str(),
                  "Retreat speed should always be positive (not %f). Making positive.",
                  retreatSpeed_mmps))
  {
    retreatSpeed_mmps *= -1.0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToUnexpectedMovement::BehaviorReactToUnexpectedMovement(const Json::Value& config)
: ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig = InstanceConfig(config, debugName);

  _unexpectedMovementCondition
    = BEIConditionFactory::CreateBEICondition( IBEICondition::GenerateBaseConditionConfig(BEIConditionType::UnexpectedMovement),
                                               GetDebugLabel() );
  DEV_ASSERT( _unexpectedMovementCondition != nullptr, "BehaviorReactToUnexpectedMovement.Ctor.NullCond" );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToUnexpectedMovement::WantsToBeActivatedBehavior() const
{
  return _unexpectedMovementCondition->AreConditionsMet(GetBEI());;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnexpectedMovement::InitBehavior()
{
  _unexpectedMovementCondition->Init(GetBEI());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnexpectedMovement::OnBehaviorEnteredActivatableScope() {
  _unexpectedMovementCondition->SetActive(GetBEI(), true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnexpectedMovement::OnBehaviorLeftActivatableScope()
{
  _unexpectedMovementCondition->SetActive(GetBEI(), false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnexpectedMovement::OnBehaviorActivated()
{
  const auto persistent = _dVars.persistent;
  _dVars = DynamicVariables();
  _dVars.persistent = persistent;

  if(GetBEI().HasMoodManager()){
    auto& moodManager = GetBEI().GetMoodManager();
    // Make Cozmo more frustrated if he keeps running into things/being turned
    moodManager.TriggerEmotionEvent("ReactToUnexpectedMovement",
                                    MoodManager::GetCurrentTimeInSeconds());
  }

  // Have we been activated a lot recently?
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& times = _dVars.persistent.activatedTimes;
  times.insert(now_sec);
  // Discard records of activations outside of time window.
  times.erase(times.begin(), times.lower_bound(now_sec - _iConfig.repeatedActivationCheckWindow_sec));

  // If we have been activated too many times within a short window of time, then just delegate control to an action that
  // attempts to raise the lift and backs up, instead of continuing with the ReactToUnexpectedMovement behavior, since we
  // may be stuck in a loop, in the event that somehow the lift has been caught on something low to the ground plane.
  if (times.size() > _iConfig.numRepeatedActivationsAllowed) {
    LOG_WARNING("BehaviorReactToUnexpectedMovement.OnBehaviorActivated.RepeatedlyActivated",
                "Activated %zu times in the past %.1f seconds.",
                times.size(), _iConfig.repeatedActivationCheckWindow_sec);
    // Reset the records of activation times to prevent getting stuck in a loop with this emergency maneuver.
    times.clear();
    // Command the emergency maneuver of raising the lift as high as possible and then retreating slowly.
    CompoundActionSequential* seq_action = new CompoundActionSequential({
      new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::CARRY),
      new DriveStraightAction(-_iConfig.retreatDistance_mm, _iConfig.retreatSpeed_mmps)});
    DelegateIfInControl(seq_action);
    return;
  }

  // Otherwise, Lock the wheels if the unexpected movement is behind us so we don't drive backward and delete the created obstacle
  // TODO: Consider using a different animation that drives forward instead of backward? (COZMO-13035)
  const auto& unexpectedMovementSide = GetBEI().GetMovementComponent().GetUnexpectedMovementSide();
  const u8 tracksToLock = Util::EnumToUnderlying(unexpectedMovementSide == UnexpectedMovementSide::BACK ?
                                                 AnimTrackFlag::BODY_TRACK :
                                                 AnimTrackFlag::NO_TRACKS);
  
  const u32  kNumLoops = 1;
  const bool kInterruptRunning = true;
  
  AnimationTrigger reactionAnimation = AnimationTrigger::ReactToUnexpectedMovement;
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(reactionAnimation,
                                                         kNumLoops, kInterruptRunning, tracksToLock), [this]()
  {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToUnexpectedMovement);
  });  
}

  
}
}
