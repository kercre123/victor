/**
 * File: behaviorDanceToTheBeat.cpp
 *
 * Author: Matt Michini
 * Created: 2018-05-07
 *
 * Description: Dance to a beat detected by the microphones/beat detection algorithm
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/danceToTheBeat/behaviorDanceToTheBeat.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/utils/cozmoFeatureGate.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/random/randomGenerator.h"
#include "util/time/universalTime.h"

namespace Anki {
namespace Vector {

namespace {
  
  // This is a fixed latency correction factor. Animation
  // commands are sent to the anim process early by this
  // amount to account for messaging/tick latency/timing.
  const float kLatencyCorrectionTime_sec = 0.050f;
  
  const char* kCooldown_key = "cooldown_s";
  const char* kUseBackpackLights_key = "useBackpackLights";
  const char* kBackpackAnim_key  = "backpackAnim";
  const char* kEyeHoldAnim_key   = "eyeHoldAnim";
  const char* kGetInAnim_key     = "getInAnim";
  const char* kGetOutAnim_key    = "getOutAnim";
  const char* kGetReadyAnim_key  = "getReadyAnim";
  const char* kListeningAnim_key = "listeningAnim";
  const char* kDanceSessions_key = "danceSessions";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::BehaviorDanceToTheBeat(const Json::Value& config)
  : ICozmoBehavior(config)
  , _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
  MakeMemberTunable(_iConfig.cooldown_sec, kCooldown_key, ("Behavior" + GetDebugLabel()).c_str());
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDanceToTheBeat::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
  : cooldown_sec(JsonTools::ParseFloat(config, kCooldown_key, debugName))
  , useBackpackLights(JsonTools::ParseBool(config, kUseBackpackLights_key, debugName))
{
  JsonTools::GetCladEnumFromJSON(config, kBackpackAnim_key,  backpackAnim,  debugName);
  JsonTools::GetCladEnumFromJSON(config, kEyeHoldAnim_key,   eyeHoldAnim,   debugName);
  JsonTools::GetCladEnumFromJSON(config, kGetInAnim_key,     getInAnim,     debugName);
  JsonTools::GetCladEnumFromJSON(config, kGetOutAnim_key,    getOutAnim,    debugName);
  JsonTools::GetCladEnumFromJSON(config, kGetReadyAnim_key,  getReadyAnim,  debugName);
  JsonTools::GetCladEnumFromJSON(config, kListeningAnim_key, listeningAnim, debugName);
  
  const auto& danceSessionConfig = config[kDanceSessions_key];
  DEV_ASSERT(!danceSessionConfig.isNull() && danceSessionConfig.isArray() && !danceSessionConfig.empty(),
             "BehaviorDanceToTheBeat.InstanceConfig.EmptyOrInvalidDanceSessionList");
  for (const auto& json : danceSessionConfig) {
    danceSessionConfigs.emplace_back(json);
    DEV_ASSERT(danceSessionConfigs.back().IsValid(), "BehaviorDanceToTheBeat.InstanceConfig.InvalidDanceSessionConfig");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::InstanceConfig::CheckValid()
{
  isValid = true;
  
  if (danceSessionConfigs.empty()) {
    isValid = false;
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.InstanceConfig.CheckValid.NoDanceSessions",
                        "Invalid config - no dance session config entries");
  }

  for (const auto& session : danceSessionConfigs) {
    if (!session.IsValid()) {
      isValid = false;
    }
  }

  return isValid;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kCooldown_key,
    kUseBackpackLights_key,
    kBackpackAnim_key,
    kEyeHoldAnim_key,
    kGetInAnim_key,
    kGetOutAnim_key,
    kGetReadyAnim_key,
    kListeningAnim_key,
    kDanceSessions_key,
  };
  expectedKeys.insert(std::begin(list), std::end(list));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDanceToTheBeat::WantsToBeActivatedBehavior() const
{
  const auto* featureGate = GetBEI().GetRobotInfo().GetContext()->GetFeatureGate();
  const bool featureEnabled = featureGate->IsFeatureEnabled(Anki::Vector::FeatureType::Dancing);
  if(!featureEnabled)
  {
    return false;
  }

  // Check cooldown using BaseStation timer
  const auto nowBasestationTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool canRun = (_lastRunningBasestationTime_sec <= 0.f) ||
                      (nowBasestationTime_sec - _lastRunningBasestationTime_sec > _iConfig.cooldown_sec);
  return canRun;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::InitBehavior()
{
  // Initialize "dance sessions"
  auto& dataAccessorComp = GetBEI().GetComponentWrapper(BEIComponentID::DataAccessor).GetComponent<DataAccessorComponent>();
  const auto* animContainer = dataAccessorComp.GetCannedAnimationContainer();
  if (animContainer == nullptr) {
    PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.InitBehavior.NullAnimContainer",
                      "Null CannedAnimationContainer!");
    return;
  }
  for (auto& session : _iConfig.danceSessionConfigs) {
    const bool success = session.Init(*animContainer);
    DEV_ASSERT(success, "BehaviorDanceToTheBeat.InitBehavior.FailedInitializingDanceSession");
  }
  
  // Double check that the config is still valid
  ANKI_VERIFY(_iConfig.CheckValid(), "BehaviorDanceToTheBeat.InitBehavior.InvalidConfig", "");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorActivated()
{
  if (!_iConfig.isValid) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.OnBehaviorActivated.InvalidConfig",
                        "Configuration is invalid - ending behavior");
    CancelSelf();
    return;
  }

  // Reset dynamic variables:
  _dVars = DynamicVariables();
  
  // Pre-generate the queue of all animations to play for each dance session
  for (const auto& sessionConfig : _iConfig.danceSessionConfigs) {
    _dVars.danceSessionAnims.push_back(sessionConfig.GenerateAnimSequence());
  }
  
  // Record the starting tempo
  auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  _dVars.initialTempo_bpm = beatDetector.GetCurrTempo_bpm();

  // Play the get-in animation then transition to listening mode
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.getInAnim),
                      &BehaviorDanceToTheBeat::TransitionToListening);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }

  _lastRunningBasestationTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if (_dVars.state != State::Dancing) {
    // Nothing to do in Update if we are not dancing.
    return;
  }

  // If we are currently listening for beats, double check that a beat
  // is still detected. If not, we should bail out of the behavior.
  const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  if (_dVars.listeningForBeats &&
      !beatDetector.IsBeatDetected()) {
    PRINT_NAMED_WARNING("BehaviorDanceToTheBeat.BehaviorUpdate.BeatNoLongerDetected",
                        "Cancelling behavior since we are currently listening for beats "
                        "and BeatDetectorComponent says beat is no longer detected");
    CancelSelf();
    return;
  }

  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());
  // If we're not listening for beats, we need to call OnBeat() 'manually'
  // when the next 'extrapolated' beat occurs.
  if (!_dVars.listeningForBeats) {
    if (now_sec > _dVars.nextBeatTime_sec) {
      OnBeat();
    }
    // Unregister the OnBeat callback since we no longer need it
    UnregisterOnBeatCallback();
  }

  if (_dVars.nextAnimTriggerTime_sec > 0.f &&
      now_sec >= _dVars.nextAnimTriggerTime_sec) {
    // Time to play the next animation in the queue
    DEV_ASSERT(!_dVars.danceSessionAnims.empty() && !_dVars.danceSessionAnims.front().empty(),
               "BehaviorDanceToTheBeat.BehaviorUpdate.NoAnimsInQueue");
    auto& thisDanceSession = _dVars.danceSessionAnims.front();
    const auto& thisAnim = thisDanceSession.front();
    auto* animAction = new PlayAnimationAction(thisAnim.GetAnimName());
    // If this animation is not marked "canListenForBeats", then stop listening for beats
    if (!thisAnim.CanListenForBeats()) {
      _dVars.listeningForBeats = false;
    }
    thisDanceSession.pop_front();
    if (thisDanceSession.empty()) {
      // No more animations to play after this one. Remove this dance session from the list, and if there are none
      // remaining then play the get-out after. Otherwise transition into listening after this anim.
      DelegateNow(animAction, [this](){
        _dVars.danceSessionAnims.pop_front();
        if (_dVars.danceSessionAnims.empty()) {
          // No more dancing sessions, so play the getout.
          DelegateIfInControl(new TriggerAnimationAction(_iConfig.getOutAnim));
        } else {
          // There are still remaining dance sessions, so go back to the listening phase
          TransitionToListening();
        }
      });
      _dVars.nextAnimTriggerTime_sec = -1.f;
    } else {
      // Delegate to this animation, and play the eye hold indefinitely after that
      DelegateNow(animAction, [this](){
        DelegateIfInControl(new TriggerAnimationAction(_iConfig.eyeHoldAnim, 0));
      });
      SetNextAnimTriggerTime();
    }
  }

  // If we have no animation queued and we're not currently doing anything, it's time to quit.
  if (!IsControlDelegated() &&
      _dVars.nextAnimTriggerTime_sec <= 0.f) {
    CancelSelf();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBehaviorDeactivated()
{
  StopBackpackLights();
  UnregisterOnBeatCallback();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::TransitionToListening()
{
  DEV_ASSERT(!_dVars.danceSessionAnims.empty(), "BehaviorDanceToTheBeat.TransitionToListening.NoDanceAnimsRemaining");
  
  _dVars.state = State::Listening;
  _dVars.listeningForBeats = true;
 
  StopBackpackLights();
  
  // Start listening loop.
  const float kListeningTime_sec = 8.f;
  
  // First, wait for kListeningTime_sec at least, while looping the "listening" anim. After that, play the "get ready"
  // anim and transition into dancing.
  
  auto* listenForBeatsAction = new CompoundActionParallel();
  listenForBeatsAction->SetShouldEndWhenFirstActionCompletes(true);
  listenForBeatsAction->AddAction(new TriggerAnimationAction(_iConfig.listeningAnim, 0)); // loop indefinitely
  listenForBeatsAction->AddAction(new WaitAction(kListeningTime_sec)); // wait a minimum amount of time to acquire the beat
  
  auto* compoundAction = new CompoundActionSequential();
  compoundAction->AddAction(listenForBeatsAction);
  compoundAction->AddAction(new TriggerAnimationAction(_iConfig.getReadyAnim));
  
  DelegateIfInControl(listenForBeatsAction,
                      &BehaviorDanceToTheBeat::TransitionToDancing);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::TransitionToDancing()
{
  _dVars.state = State::Dancing;

  auto& beatDetector = GetBEI().GetBeatDetectorComponent();
  // grab the next beat time here and wait to play the anim until the appropriate time
  _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
  _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();

  // Register an OnBeat callback with BeatDetectorComponent if we are to listen for beats during the dancing:
  if (_dVars.listeningForBeats) {
    _dVars.onBeatCallbackId = beatDetector.RegisterOnBeatCallback(std::bind(&BehaviorDanceToTheBeat::OnBeat, this));
  }

  // Kick off the dancing animations by setting the next trigger time.
  SetNextAnimTriggerTime();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::SetNextAnimTriggerTime()
{
  if (_dVars.danceSessionAnims.empty() || _dVars.danceSessionAnims.front().empty()) {
    _dVars.nextAnimTriggerTime_sec = -1.f;
    DEV_ASSERT(false, "BehaviorDanceToTheBeat.SetNextAnimTriggerTime.NoAnimsInQueue");
    return;
  }

  const auto now_sec = static_cast<float>(Util::Time::UniversalTime::GetCurrentTimeInSeconds());

  const auto& thisDanceSession = _dVars.danceSessionAnims.front();
  const auto& nextAnim = thisDanceSession.front();

  // Must start the animation beatDelay_sec before the next beat. Also include
  // the latency correction factor to account for messaging/tick times, etc.
  _dVars.nextAnimTriggerTime_sec = _dVars.nextBeatTime_sec - nextAnim.GetBeatDelay_sec() - kLatencyCorrectionTime_sec;

  // Make sure the animation trigger time is somewhere in the future
  BOUNDED_WHILE(1000, _dVars.nextAnimTriggerTime_sec < now_sec) {
    _dVars.nextAnimTriggerTime_sec += _dVars.beatPeriod_sec;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::OnBeat()
{
  if (!IsActivated()) {
    DEV_ASSERT(false, "BehaviorDanceToTheBeat.OnBeat.NotActivated");
    return;
  }
  
  DEV_ASSERT(_dVars.state == State::Dancing, "BehaviorDanceToTheBeat.OnBeat.ShouldBeDancing");

  PRINT_NAMED_INFO("BehaviorDanceToTheBeat.OnBeat",
                   "OnBeat() called. Currently%s listening for new beats. Current universal time (sec): %.3f",
                   _dVars.listeningForBeats ? "" : " NOT",
                   Util::Time::UniversalTime::GetCurrentTimeInSeconds());

  if (_iConfig.useBackpackLights) {
    StopBackpackLights();
    auto& blc = GetBEI().GetBackpackLightComponent();
    blc.SetBackpackAnimation(_iConfig.backpackAnim);
  }

  // If we're currently listening for beats, then this means we have new information from the BeatDetectorComponent.
  if (_dVars.listeningForBeats) {
    // Update some stuff now that we have new beat information from BeatDetectorComponent.
    const auto& beatDetector = GetBEI().GetBeatDetectorComponent();
    _dVars.nextBeatTime_sec = beatDetector.GetNextBeatTime_sec();
    _dVars.beatPeriod_sec = 60.f / beatDetector.GetCurrTempo_bpm();

    // Update the next animation trigger time if we still have animations in the queue.
    if (!_dVars.danceSessionAnims.empty() && !_dVars.danceSessionAnims.front().empty()) {
      SetNextAnimTriggerTime();
    }
  } else {
    // We are not currently listening for beats, so just manually extrapolate the next beat time.
    _dVars.nextBeatTime_sec += _dVars.beatPeriod_sec;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDanceToTheBeat::StopBackpackLights()
{
  auto& blc = GetBEI().GetBackpackLightComponent();
  blc.ClearAllBackpackLightConfigs();
}

void BehaviorDanceToTheBeat::UnregisterOnBeatCallback()
{
  if (_dVars.onBeatCallbackId > 0) {
    auto& beatDetector = GetBEI().GetBeatDetectorComponent();
    if (!beatDetector.UnregisterOnBeatCallback(_dVars.onBeatCallbackId)) {
      PRINT_NAMED_ERROR("BehaviorDanceToTheBeat.UnregisterOnBeatCallback.FailedToUnregisterCallback",
                        "Failed to unregister OnBeat callback (id %d)", _dVars.onBeatCallbackId);
    }
  }
  _dVars.onBeatCallbackId = -1;
}
  

}
}
