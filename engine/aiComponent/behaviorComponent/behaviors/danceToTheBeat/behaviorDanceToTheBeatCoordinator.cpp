/**
 * File: BehaviorDanceToTheBeatCoordinator.cpp
 *
 * Author: Matt Michini
 * Created: 2018-08-20
 *
 * Description: Coordinates listening for beats and dancing to the beat
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/behaviorDanceToTheBeatCoordinator.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* kListeningBehavior_key = "listeningBehavior";
  const char* kLongListeningBehavior_key = "longListeningBehavior";
  const char* kOffChargerDancingBehavior_key = "offChargerDancingBehavior";
  const char* kOnChargerDancingBehavior_key  = "onChargerDancingBehavior";
  
  #define CONSOLE_GROUP "BehaviorDanceToTheBeatCoordinator"
  CONSOLE_VAR_RANGED(f32, kDancingCooldown_sec, CONSOLE_GROUP, 20.0f, 0.0f, 3600.0f);
  CONSOLE_VAR_RANGED(f32, kListeningCooldown_sec, CONSOLE_GROUP, 20.0f, 0.0f, 3600.0f);
}
  
#define LOG_FUNCTION_NAME() PRINT_CH_INFO("Behaviors", "BehaviorDanceToTheBeatCoordinator", "BehaviorDanceToTheBeatCoordinator.%s", __func__);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeatCoordinator::BehaviorDanceToTheBeatCoordinator(const Json::Value& config)
 : ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  
  _iConfig.listeningBehaviorStr         = JsonTools::ParseString(config, kListeningBehavior_key, debugName);
  _iConfig.longListeningBehaviorStr     = JsonTools::ParseString(config, kLongListeningBehavior_key, debugName);
  _iConfig.offChargerDancingBehaviorStr = JsonTools::ParseString(config, kOffChargerDancingBehavior_key, debugName);
  _iConfig.onChargerDancingBehaviorStr  = JsonTools::ParseString(config, kOnChargerDancingBehavior_key, debugName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeatCoordinator::~BehaviorDanceToTheBeatCoordinator()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.listeningBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.listeningBehaviorStr));
  DEV_ASSERT(_iConfig.listeningBehavior != nullptr,
             "BehaviorDanceToTheBeatCoordinator.InitBehavior.NullBehavior");
  
  _iConfig.longListeningBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.longListeningBehaviorStr));
  DEV_ASSERT(_iConfig.longListeningBehavior != nullptr,
             "BehaviorDanceToTheBeatCoordinator.InitBehavior.NullBehavior");
  
  _iConfig.offChargerDancingBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.offChargerDancingBehaviorStr));
  DEV_ASSERT(_iConfig.offChargerDancingBehavior != nullptr,
             "BehaviorDanceToTheBeatCoordinator.InitBehavior.NullBehavior");
  
  _iConfig.onChargerDancingBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.onChargerDancingBehaviorStr));
  DEV_ASSERT(_iConfig.onChargerDancingBehavior != nullptr,
             "BehaviorDanceToTheBeatCoordinator.InitBehavior.NullBehavior");
  
  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DriveOffChargerStraight));
  _iConfig.goHomeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(FindAndGoToHome));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeatCoordinator::WantsToBeActivatedBehavior() const
{
  // Do not try to activate this behavior if the backpack has been fiddled with recently, since that is sometimes
  // interpreted as a rhythmic 'beat' by the BeatDetectorComponent
  if (RecentBackpackActivity()) {
    return false;
  }

  // Check if either dancing or listening cooldown is active
  auto& timerManager = GetBEI().GetBehaviorTimerManager();
  const auto& danceTimer = timerManager.GetTimer(BehaviorTimerTypes::DancingCooldown);
  const auto& listeningTimer = timerManager.GetTimer(BehaviorTimerTypes::ListenForBeatsCooldown);
  bool timersExpired = (danceTimer.HasCooldownExpired(kDancingCooldown_sec) &&
                        listeningTimer.HasCooldownExpired(kListeningCooldown_sec));

  // Override cooldowns if a 'strong' beat is detected
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  const bool strongBeatDetected = beatDetector.IsBeatDetected();
  
  return (timersExpired || strongBeatDetected);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject  = false;
  modifiers.wantsToBeActivatedWhenOffTreads       = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.listeningBehavior.get());
  delegates.insert(_iConfig.longListeningBehavior.get());
  delegates.insert(_iConfig.offChargerDancingBehavior.get());
  delegates.insert(_iConfig.onChargerDancingBehavior.get());
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  delegates.insert(_iConfig.goHomeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kListeningBehavior_key,
    kLongListeningBehavior_key,
    kOffChargerDancingBehavior_key,
    kOnChargerDancingBehavior_key,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // Begin listening to confirm that a beat is happening
  const bool wantsToRun = _iConfig.listeningBehavior->WantsToBeActivated();
  DEV_ASSERT(wantsToRun, "BehaviorDanceToTheBeatCoordinator.OnBehaviorActivated.ListenDoesNotWantToRun");
  DelegateIfInControl(_iConfig.listeningBehavior.get(),
                      &BehaviorDanceToTheBeatCoordinator::CheckIfBeatDetected);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::CheckIfBeatDetected()
{
  LOG_FUNCTION_NAME();
  
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  if (!beatDetector.IsBeatDetected()) {
    // After listening, we do not have a strong beat detected. Bail out of the behavior.
    PRINT_CH_INFO("Behaviors", "BehaviorDanceToTheBeatCoordinator.TransitionToCheckForBeat.NoBeat",
                  "Exiting behavior since no beat was detected during the listening phase");
    return;
  }
  
  // We have a detected beat! Take action based on our current onCharger/battery state
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const bool isOnCharger = robotInfo.IsOnChargerPlatform();
  const bool isBatteryFull = (robotInfo.GetBatteryLevel() == BatteryLevel::Full);
  
  if (isOnCharger) {
    if (isBatteryFull) {
      // Battery is full, so we can drive off the charger to dance
      TransitionToDriveOffCharger();
    } else {
      // Battery not full, so just do the simpler "on charger dancing" behavior
      TransitionToOnChargerDance();
    }
  } else {
    TransitionToOffChargerDance();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::TransitionToOffChargerDance()
{
  LOG_FUNCTION_NAME();
  
  const bool wantsToRun = _iConfig.offChargerDancingBehavior->WantsToBeActivated();
  if (wantsToRun) {
    DelegateIfInControl(_iConfig.offChargerDancingBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::TransitionToOnChargerDance()
{
  LOG_FUNCTION_NAME();
  
  const bool wantsToRun = _iConfig.onChargerDancingBehavior->WantsToBeActivated();
  DEV_ASSERT(wantsToRun, "BehaviorDanceToTheBeatCoordinator.TransitionToOnChargerDance.onChargerDanceDoesNotWantToRun");
  DelegateIfInControl(_iConfig.onChargerDancingBehavior.get());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::TransitionToDriveOffCharger()
{
  LOG_FUNCTION_NAME();

  const bool wantsToRun = _iConfig.driveOffChargerBehavior->WantsToBeActivated();
  DEV_ASSERT(wantsToRun, "BehaviorDanceToTheBeatCoordinator.TransitionToDriveOffCharger.driveOffChargerDoesNotWantToRun");
  DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(),
                      &BehaviorDanceToTheBeatCoordinator::TransitionToOffChargerListening);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeatCoordinator::TransitionToOffChargerListening()
{
  LOG_FUNCTION_NAME();
 
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const bool isOnCharger = robotInfo.IsOnChargerPlatform();
  if (isOnCharger) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeatCoordinator.TransitionToOffChargerListening.OnCharger",
                        "Somehow we are still on the charger. Exiting behavior");
    return;
  }
  
  // Use the 'long' version of listening here, since we may have messed up our beat detector by driving off the charger
  // (and thus making a bunch of motor noise)
  const bool wantsToRun = _iConfig.longListeningBehavior->WantsToBeActivated();
  DEV_ASSERT(wantsToRun, "BehaviorDanceToTheBeatCoordinator.TransitionToOffChargerListening.LongListenDoesNotWantToRun");
  DelegateIfInControl(_iConfig.longListeningBehavior.get(),
                      [this](){
                        const bool wantsToDance = _iConfig.offChargerDancingBehavior->WantsToBeActivated();
                        if (wantsToDance) {
                          TransitionToOffChargerDance();
                        } else {
                          // Dancing does not want to run - just go back home in this case
                          PRINT_CH_INFO("Behaviors", "BehaviorDanceToTheBeatCoordinator.TransitionToOffChargerListening.DoesNotWantToDance",
                                        "Returning to charger since the offChargerDancing behavior does not want to run");
                          if (_iConfig.goHomeBehavior->WantsToBeActivated()) {
                            DelegateIfInControl(_iConfig.goHomeBehavior.get());
                          }
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeatCoordinator::RecentBackpackActivity() const
{
  // Has the backpack been fiddled with recently enough to mess up beat detection? A cooldown is necessary since the
  // beat detector can retain its "beat detected" state for some time after the last touch.
  const float kButtonPressCooldown_sec = 10.f;
  
  const auto& touchSensor = GetBEI().GetTouchSensorComponent();
  const auto now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  float touchTime = touchSensor.GetTouchPressTime();
  // time defaults to FLT_MAX if never touched, so set this to zero in that case (shouldn't be in the future...)
  if( touchTime > now_sec + kButtonPressCooldown_sec ) {
    touchTime = 0.0f;
  }

  const bool recentTouchSensorActivity = touchSensor.GetIsPressed() ||
                                         touchTime > now_sec - kButtonPressCooldown_sec;
  
  return recentTouchSensorActivity;
}

}
}
