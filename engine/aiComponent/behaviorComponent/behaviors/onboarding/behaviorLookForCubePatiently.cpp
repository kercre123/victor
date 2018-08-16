/**
 * File: BehaviorLookForCubePatiently.cpp
 *
 * Author: ross
 * Created: 2018-08-15
 *
 * Description: a T3MP behavior to patiently face the current direction, turning infrequently.
 *              It's T3MP since more generic cube search behaviors are being worked on, but adding params
 *              to cover this "patient" case is more trouble than it's worth for 1.0
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorLookForCubePatiently.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

namespace Anki {
namespace Vector {
  
namespace {
  const char* const kAnimLoopKey = "animLoop";
  
  const float kMinDuration = 3.0f; // actual time will round up to a multiple of the animation loop
  const float kMaxDuration = 5.0f;
  const float kMinAngle_rad = DEG_TO_RAD(5.0f);
  const float kMaxAngle_rad = DEG_TO_RAD(30.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookForCubePatiently::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookForCubePatiently::DynamicVariables::DynamicVariables()
{
  cancelLoopTime_s = 0.0f;
  lastAngle_rad = 0.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookForCubePatiently::BehaviorLookForCubePatiently(const Json::Value& config)
 : ICozmoBehavior(config)
{
  JsonTools::GetCladEnumFromJSON(config, kAnimLoopKey, _iConfig.animLoop, GetDebugLabel(), false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookForCubePatiently::~BehaviorLookForCubePatiently()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookForCubePatiently::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForCubePatiently::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // this will always delegate if the animation is provided. otherwise it may not
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForCubePatiently::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForCubePatiently::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAnimLoopKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForCubePatiently::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  auto* moveHeadAction = new MoveHeadToAngleAction{ 0.0f };
  DelegateIfInControl( moveHeadAction, &BehaviorLookForCubePatiently::TurnAndLoopAnim );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForCubePatiently::BehaviorUpdate() 
{
  if( IsActivated() ) {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( (_dVars.cancelLoopTime_s != 0.0f) && (currTime_s >= _dVars.cancelLoopTime_s) ) {
      auto animAction = _dVars.animAction.lock();
      if( animAction ) {
        auto castAnim = std::dynamic_pointer_cast<ReselectingLoopAnimationAction>(animAction);
        if( castAnim != nullptr ) {
          // has no effect if already called
          castAnim->StopAfterNextLoop();
        }
      } else {
        TurnAndLoopAnim();
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForCubePatiently::TurnAndLoopAnim()
{
  auto* parallelAction = new CompoundActionParallel{};
  
  const float duration_s = GetRNG().RandDblInRange(kMinDuration,kMaxDuration);
  _dVars.cancelLoopTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + duration_s;
  
  // fluctuate around a general center angle, but allow drift
  float randAngle_rad = -GetRNG().RandDblInRange(kMinAngle_rad,kMaxAngle_rad);
  if( std::signbit(_dVars.lastAngle_rad) ) {
    randAngle_rad = -randAngle_rad;
  }
  const float angle = Radians{randAngle_rad - _dVars.lastAngle_rad}.ToFloat();
  _dVars.lastAngle_rad = randAngle_rad;
  const bool isAbs = false;
  auto* turnAction = new TurnInPlaceAction{ angle, isAbs };
  
  parallelAction->AddAction( turnAction );
  
  if( _iConfig.animLoop != AnimationTrigger::Count ) {
    // loop forever, but will get cut short once the duration has expired
    auto* animAction = new ReselectingLoopAnimationAction{ _iConfig.animLoop, 0 };
    _dVars.animAction = parallelAction->AddAction( animAction );
  }
  
  DelegateIfInControl( parallelAction );
}

}
}
