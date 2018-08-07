/**
 * File: behaviorObservingWithoutTurn.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-11-20
 *
 * Description: "idle" looking behavior to observe things without turning the body
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/observing/behaviorObservingWithoutTurn.h"

#include "clad/types/behaviorComponent/behaviorTimerTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "util/graphEvaluator/graphEvaluator2d.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Vector {
  
namespace {
const char* const kSmallMotionPeriodKey = "small_motion_period_s";
const char* const kSmallMotionPeriodRandomFactorKey = "small_motion_period_random_factor";
const char* const kLookUpPeriodMultiplierKey = "look_up_period_multiplier";
const char* const kLookStraightPeriodMultiplierKey = "look_straight_period_multiplier";
const char* const kBehaviorTimerKey = "behaviorTimer";
}

struct BehaviorObservingWithoutTurn::Params
{
  Params(const Json::Value& config);
    
  // x = Specified behavior timer value (e.g. time robot has been on the charger), in seconds
  // y = desired period of head motions, in seconds
  Util::GraphEvaluator2d _smallMotionPeriod;

  // add +/-_smallMotionPeriodRandomFactor * period "jitter" to each period above
  float _smallMotionPeriodRandomFactor;

  float _lookUpPeriodMultiplier;
  float _lookStraightPeriodMultiplier;

  BehaviorTimerTypes _behaviorTimer = BehaviorTimerTypes::Invalid;
};

BehaviorObservingWithoutTurn::Params::Params(const Json::Value& config)
{
  _smallMotionPeriod.ReadFromJson(config[kSmallMotionPeriodKey]);
  _smallMotionPeriodRandomFactor = config.get(kSmallMotionPeriodRandomFactorKey, 0.0f).asFloat();


  _lookUpPeriodMultiplier = JsonTools::ParseFloat(config,
                                                  kLookUpPeriodMultiplierKey,
                                                  "BehaviorObservingWithoutTurn.Params.LookUpPeriodMultiplier");

  const std::string& timerStr = JsonTools::ParseString(config,
                                                       kBehaviorTimerKey,
                                                       "BehaviorObservingWithoutTurn.Params.BehaviorTimer");
  _behaviorTimer = BehaviorTimerManager::BehaviorTimerFromString( timerStr );
  
  _lookStraightPeriodMultiplier = JsonTools::ParseFloat(
    config,
    kLookStraightPeriodMultiplierKey,
    "BehaviorObservingWithoutTurn.Params.LookStraightPeriodMultiplier");
}

BehaviorObservingWithoutTurn::BehaviorObservingWithoutTurn(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _params = std::make_unique<Params>(config);
}

BehaviorObservingWithoutTurn::~BehaviorObservingWithoutTurn()
{

}

void BehaviorObservingWithoutTurn::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSmallMotionPeriodKey,
    kSmallMotionPeriodRandomFactorKey,
    kLookUpPeriodMultiplierKey,
    kLookStraightPeriodMultiplierKey,
    kBehaviorTimerKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

bool BehaviorObservingWithoutTurn::CanBeGentlyInterruptedNow() const
{
  return !_isTransitioning;
}


void BehaviorObservingWithoutTurn::OnBehaviorActivated()
{
  const auto& timer = GetBEI().GetBehaviorTimerManager().GetTimer( _params->_behaviorTimer );
  const float currBehaviorTimerTime = timer.GetElapsedTimeInSeconds();
  PRINT_CH_INFO("Behaviors", "BehaviorObservingWithoutTurn.Activated",
                "%s: behavior timer '%s' current value is %f",
                GetDebugLabel().c_str(),
                BehaviorTimerTypesToString(_params->_behaviorTimer),
                currBehaviorTimerTime);                
  
  TransitionToLookingUp();
}

void BehaviorObservingWithoutTurn::TransitionToLookingUp()
{
  SET_STATE(LookingUp);
  _isTransitioning = true;
  ResetSmallHeadMoveTimer();
  
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ObservingLookUp),
                      &BehaviorObservingWithoutTurn::Loop);
}

void BehaviorObservingWithoutTurn::TransitionToLookingStraight()
{
  SET_STATE(LookingStraight);
  _isTransitioning = true;
  ResetSmallHeadMoveTimer();

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ObservingLookStraight),
                      &BehaviorObservingWithoutTurn::Loop);
}

void BehaviorObservingWithoutTurn::ResetSmallHeadMoveTimer()
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastSmallHeadMoveTime_s = currTime_s;

  const float time_s = GetBEI().GetBehaviorTimerManager().GetTimer( _params->_behaviorTimer ).GetElapsedTimeInSeconds();
  _currSmallMovePeriod_s = _params->_smallMotionPeriod.EvaluateY(time_s);
  if( _params->_smallMotionPeriodRandomFactor > 0.0f ) {
    const float randomRange = _currSmallMovePeriod_s * _params->_smallMotionPeriodRandomFactor;
    _currSmallMovePeriod_s += GetRNG().RandDblInRange( -randomRange, randomRange );
  }
  
  PRINT_CH_DEBUG("Behaviors", "BehaviorObservingWithoutTurn.NextSmallMotion",
                 "Next small motion will be in %f seconds",
                 _currSmallMovePeriod_s);
}

void BehaviorObservingWithoutTurn::Loop()
{
  _isTransitioning = false;

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float stateChangePeriod_s = GetStateChangePeriod();

  if( currTime_s >= _lastStateChangeTime_s + stateChangePeriod_s ) {
    SwitchState();
  }
  else {
    AnimationTrigger animToPlay = AnimationTrigger::Count;
    
    if( currTime_s >= _lastSmallHeadMoveTime_s + _currSmallMovePeriod_s ) {
      animToPlay = GetSmallHeadMoveAnim();
      ResetSmallHeadMoveTimer();
    }
    else {
      animToPlay = AnimationTrigger::ObservingIdleEyesOnly;
    }

    DEV_ASSERT(animToPlay != AnimationTrigger::Count, "BehaviorObservingWithoutTurn.Loop.NoAnimation");

    const u32 numLoops = 1;
    const bool interruptRunning = true;
    const float timeout = TriggerAnimationAction::GetDefaultTimeoutInSeconds();
    const bool strictCooldown = true;

    const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
    const u8 tracksToLock = onCharger ? (u8)AnimTrackFlag::BODY_TRACK : (u8)AnimTrackFlag::NO_TRACKS;
    
    auto* action = new TriggerAnimationAction(animToPlay, numLoops, interruptRunning, tracksToLock, timeout, strictCooldown);
    
    DelegateIfInControl(action, &BehaviorObservingWithoutTurn::Loop);
  }
}

AnimationTrigger BehaviorObservingWithoutTurn::GetSmallHeadMoveAnim() const
{
  switch( _state ) {
    case State::LookingUp: return AnimationTrigger::ObservingIdleWithHeadLookingUp;
    case State::LookingStraight: return AnimationTrigger::ObservingIdleWithHeadLookingStraight;
  } 
}

float BehaviorObservingWithoutTurn::GetStateChangePeriod() const
{
  switch(_state) {
    case State::LookingUp:
      return _params->_lookUpPeriodMultiplier * _currSmallMovePeriod_s;
    case State::LookingStraight:
      return _params->_lookStraightPeriodMultiplier * _currSmallMovePeriod_s;
  }
}

void BehaviorObservingWithoutTurn::SwitchState()
{
  switch(_state) {
    case State::LookingUp:
      TransitionToLookingStraight();
      break;
    case State::LookingStraight:
      TransitionToLookingUp();
      break;
  }
}


void BehaviorObservingWithoutTurn::SetState_internal(State state, const std::string& stateName)
{
  _lastStateChangeTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _state = state;
  SetDebugStateName(stateName);
}



}
}
