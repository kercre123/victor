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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
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
namespace Vector {

namespace {
  const char* kRepeatedActivationWindowKey = "repeatedActivationWindow_sec";
  const char* kRepeatedActivationAngleWindowKey = "repeatedActivationAngleWindow_deg";
  const char* kNumRepeatedActivationsAllowedKey = "numRepeatedActivationsAllowed";
  const char* kRetreatDistanceKey = "retreatDistance_mm";
  const char* kRetreatSpeedKey = "retreatSpeed_mmps";
}

void BehaviorReactToUnexpectedMovement::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kRepeatedActivationWindowKey,
    kRepeatedActivationAngleWindowKey,
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
  // Convert user input setting from degrees to radians
  float possibleYawAngleWindow_rad = std::fabs( DEG_TO_RAD( JsonTools::ParseFloat(config,
                                                                                  kRepeatedActivationAngleWindowKey,
                                                                                  debugName)));
  if (!ANKI_VERIFY(Util::IsFltLE(possibleYawAngleWindow_rad, M_PI_F), (debugName + ".InvalidYawAngleWindow").c_str(),
                   "Yaw angle window should be less than PI radians (not %f). Clamping to PI radians.",
                   possibleYawAngleWindow_rad))
  {
    yawAngleWindow_rad = M_PI_F;
  } else {
    yawAngleWindow_rad = possibleYawAngleWindow_rad;
  }

  numRepeatedActivationsAllowed = JsonTools::ParseUInt32(config, kNumRepeatedActivationsAllowedKey, debugName);
  retreatDistance_mm = JsonTools::ParseFloat(config, kRetreatDistanceKey, debugName);
  if (!ANKI_VERIFY(Util::IsFltGTZero(retreatDistance_mm),
                  (debugName + ".NonPositiveDistance").c_str(),
                  "Retreat distance should always be positive (not %f). Making positive.",
                  retreatDistance_mm))
  {
    retreatDistance_mm *= -1.0;
  }
  retreatSpeed_mmps = JsonTools::ParseFloat(config, kRetreatSpeedKey, debugName);
  if (!ANKI_VERIFY(Util::IsFltGTZero(retreatSpeed_mmps),
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
void BehaviorReactToUnexpectedMovement::UpdateActivationHistory()
{
  // Have we been activated a lot recently while in the same pose?
  const auto& robotYawAngle = GetBEI().GetRobotInfo().GetPose().GetRotation().GetAngleAroundZaxis();
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& times = _dVars.persistent.activatedTimes_sec;
  auto& yawAngles = _dVars.persistent.yawAnglesAtActivation_rad;

  // Discard records of activations outside of time window.
  const auto earliestValidTimeIter = std::lower_bound(times.begin(), times.end(), now_sec - _iConfig.repeatedActivationCheckWindow_sec);
  const size_t earliestValidTimeIndex = earliestValidTimeIter - times.begin();

  LOG_INFO("BehaviorReactToUnexpectedMovement.CheckForRepeatedActivations.DiscardEntries",
           "Discarding %zu activation entries.", earliestValidTimeIndex);
  times.erase(times.begin(), earliestValidTimeIter);
  yawAngles.erase(yawAngles.begin(), yawAngles.begin() + earliestValidTimeIndex);

  if (!times.empty() && !robotYawAngle.IsNear(yawAngles.front(), _iConfig.yawAngleWindow_rad)) {
    LOG_INFO("BehaviorReactToUnexpectedMovement.CheckForRepeatedActivations.Reset",
             "New angle (%.2f) not within %.2f radians of the yaw angle at first activation: %.2f radians",
             robotYawAngle.ToFloat(), _iConfig.yawAngleWindow_rad.ToFloat(), yawAngles.front().ToFloat());
    // If the new orientation falls outside the yaw window, then reset the persistent variables, it's unlikely that
    // the robot's lift is caught on something if its yaw orientation is changing.
    ResetActivationHistory();
  }
  // Record the new activation instance.
  times.emplace_back(now_sec);
  yawAngles.emplace_back(robotYawAngle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToUnexpectedMovement::HasBeenReactivatedFrequently() const
{
  return _dVars.persistent.activatedTimes_sec.size() > _iConfig.numRepeatedActivationsAllowed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToUnexpectedMovement::ResetActivationHistory()
{
  _dVars.persistent.activatedTimes_sec.clear();
  _dVars.persistent.yawAnglesAtActivation_rad.clear();
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

  UpdateActivationHistory();

  // If we have been activated too many times within a short window of time, then just delegate control to an action that
  // attempts to raise the lift and backs up, instead of continuing with the ReactToUnexpectedMovement behavior, since we
  // may be stuck in a loop, in the event that somehow the lift has been caught on something low to the ground plane.
  if (HasBeenReactivatedFrequently()) {
    LOG_WARNING("BehaviorReactToUnexpectedMovement.OnBehaviorActivated.RepeatedlyActivated",
                "Activated %zu times in the past %.1f seconds.",
                _dVars.persistent.activatedTimes_sec.size(), _iConfig.repeatedActivationCheckWindow_sec);
    // Reset the records of activation times to prevent getting stuck in a loop with this emergency maneuver.
    ResetActivationHistory();
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
