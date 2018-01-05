/**
 * File: behaviorObservingOnCharger.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-11-20
 *
 * Description: "idle" looking behavior to observe things while Victor is sitting on the charger
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorObservingOnCharger.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "util/graphEvaluator/graphEvaluator2d.h"

#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

struct BehaviorObservingOnCharger::Params
{
  Params(const Json::Value& config);
    
  // x = time robot has been on the charger, in seconds
  // y = desired period of head motions, in seconds
  Util::GraphEvaluator2d _smallMotionPeriod;

  // add +/-_smallMotionPeriodRandomFactor * period "jitter" to each period above
  float _smallMotionPeriodRandomFactor;

  float _lookUpPeriodMultiplier;
  float _lookStraightPeriodMultiplier;
};

BehaviorObservingOnCharger::Params::Params(const Json::Value& config)
{
  _smallMotionPeriod.ReadFromJson(config["small_motion_period_s"]);
  _smallMotionPeriodRandomFactor = config.get("small_motion_period_random_factor", 0.0f).asFloat();


  _lookUpPeriodMultiplier = JsonTools::ParseFloat(config,
                                                  "look_up_period_multiplier",
                                                  "BehaviorObservingOnCharger.Params.LookUpPeriodMultiplier");

  
  _lookStraightPeriodMultiplier = JsonTools::ParseFloat(
    config,
    "look_straight_period_multiplier",
    "BehaviorObservingOnCharger.Params.LookStraightPeriodMultiplier");
}

BehaviorObservingOnCharger::BehaviorObservingOnCharger(const Json::Value& config)
  : ICozmoBehavior(config)
{
  _params = std::make_unique<Params>(config);
}

BehaviorObservingOnCharger::~BehaviorObservingOnCharger()
{

}

bool BehaviorObservingOnCharger::CanBeGentlyInterruptedNow(BehaviorExternalInterface& bei) const
{
  return !_isTransitioning;
}

void BehaviorObservingOnCharger::OnBehaviorActivated(BehaviorExternalInterface& bei)
{
  TransitionToLookingUp(bei);
  
  
}

void BehaviorObservingOnCharger::TransitionToLookingUp(BehaviorExternalInterface& bei)
{
  SET_STATE(LookingUp);
  _isTransitioning = true;
  ResetSmallHeadMoveTimer(bei);
  
  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ObservingLookUp),
                      &BehaviorObservingOnCharger::Loop);
}

void BehaviorObservingOnCharger::TransitionToLookingStraight(BehaviorExternalInterface& bei)
{
  SET_STATE(LookingStraight);
  _isTransitioning = true;
  ResetSmallHeadMoveTimer(bei);

  DelegateIfInControl(new TriggerAnimationAction(AnimationTrigger::ObservingLookStraight),
                      &BehaviorObservingOnCharger::Loop);
}

void BehaviorObservingOnCharger::ResetSmallHeadMoveTimer(BehaviorExternalInterface& bei)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _lastSmallHeadMoveTime_s = currTime_s;

  const TimeStamp_t lastChargingTime_ms = bei.GetRobotInfo().GetLastChargingStateChangeTimestamp();
  const TimeStamp_t currRobotTime_ms = bei.GetRobotInfo().GetLastMsgTimestamp();
  const TimeStamp_t timeOnCharger_ms = currRobotTime_ms > lastChargingTime_ms ?
                                       currRobotTime_ms - lastChargingTime_ms :
                                       0;
  
  _currSmallMovePeriod_s = _params->_smallMotionPeriod.EvaluateY( timeOnCharger_ms / 1000.0f );
  if( _params->_smallMotionPeriodRandomFactor > 0.0f ) {
    const float randomRange = _currSmallMovePeriod_s * _params->_smallMotionPeriodRandomFactor;
    _currSmallMovePeriod_s += GetRNG().RandDblInRange( -randomRange, randomRange );
  }
  
  PRINT_CH_DEBUG("Behaviors", "BehaviorObservingOnCharger.NextSmallMotion",
                 "Next small motion will be in %f seconds",
                 _currSmallMovePeriod_s);
}

void BehaviorObservingOnCharger::Loop(BehaviorExternalInterface& bei)
{
  _isTransitioning = false;

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float stateChangePeriod_s = GetStateChangePeriod();

  if( currTime_s >= _lastStateChangeTime_s + stateChangePeriod_s ) {
    SwitchState(bei);
  }
  else {
    AnimationTrigger animToPlay = AnimationTrigger::Count;
    
    if( currTime_s >= _lastSmallHeadMoveTime_s + _currSmallMovePeriod_s ) {
      animToPlay = GetSmallHeadMoveAnim();
      ResetSmallHeadMoveTimer(bei);
    }
    else {
      animToPlay = AnimationTrigger::ObservingIdleEyesOnly;
    }

    DEV_ASSERT(animToPlay != AnimationTrigger::Count, "BehaviorObservingOnCharger.Loop.NoAnimation");

    DelegateIfInControl(new TriggerAnimationAction(animToPlay), &BehaviorObservingOnCharger::Loop);
  }
}

AnimationTrigger BehaviorObservingOnCharger::GetSmallHeadMoveAnim() const
{
  switch( _state ) {
    case State::LookingUp: return AnimationTrigger::ObservingIdleWithHeadLookingUp;
    case State::LookingStraight: return AnimationTrigger::ObservingIdleWithHeadLookingStraight;
  } 
}

float BehaviorObservingOnCharger::GetStateChangePeriod() const
{
  switch(_state) {
    case State::LookingUp:
      return _params->_lookUpPeriodMultiplier * _currSmallMovePeriod_s;
    case State::LookingStraight:
      return _params->_lookStraightPeriodMultiplier * _currSmallMovePeriod_s;
  }
}

void BehaviorObservingOnCharger::SwitchState(BehaviorExternalInterface& bei)
{
  switch(_state) {
    case State::LookingUp:
      TransitionToLookingStraight(bei);
      break;
    case State::LookingStraight:
      TransitionToLookingUp(bei);
      break;
  }
}


void BehaviorObservingOnCharger::SetState_internal(State state, const std::string& stateName)
{
  _lastStateChangeTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _state = state;
  SetDebugStateName(stateName);
}



}
}
