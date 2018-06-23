/**
 * File: behaviorDispatchAfterShake.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-01-18
 *
 * Description: Simple behavior to wait for the robot to be shaken and placed down before delegating to
 *              another data-defined behavior
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDispatchAfterShake.h"

#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/components/backpackLights/backpackLightComponent.h"

#include "util/console/consoleInterface.h"

#include <set>

namespace Anki {
namespace Cozmo {

namespace{
  
const float kAccelMagnitudeShakingStartedThreshold = 16000.f;
  
const char* const kBehaviorsKey = "behaviors";

#if REMOTE_CONSOLE_ENABLED
// set to > 0 from console to fake the repeated "shakes" (shakes are hard to do in webots sim)
CONSOLE_VAR(unsigned int, kDevDispatchAfterShake, "DevBaseBehavior", 0);
#endif

// how long you have to shake/pause
CONSOLE_VAR_RANGED(float, kShakeTime, "DevBaseBehavior", 0.1f, 0.01f, 2.0f);
  
// if the robot is put down and no behavior wants to be active after this many ticks, the shake count is reset
const unsigned int kFailTicksBeforeReset = 10;
  
static const BackpackLightAnimation::BackpackAnimation kLightsSteady =
{
  .onColors               = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{100,0,0}},
  .offPeriod_ms           = {{100,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

static const BackpackLightAnimation::BackpackAnimation kLightsShake =
{
  .onColors               = {{NamedColors::RED,NamedColors::BLACK,NamedColors::BLACK}},
  .offColors              = {{NamedColors::RED,NamedColors::BLACK,NamedColors::BLACK}},
  .onPeriod_ms            = {{100,0,0}},
  .offPeriod_ms           = {{100,0,0}},
  .transitionOnPeriod_ms  = {{0,0,0}},
  .transitionOffPeriod_ms = {{0,0,0}},
  .offset                 = {{0,0,0}}
};

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatchAfterShake::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatchAfterShake::DynamicVariables::DynamicVariables()
{
  countShaken = 0;
  shakingSession = false;
  lastChangeTime_s = 0.0f;
  tickPlacedDown = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatchAfterShake::BehaviorDispatchAfterShake(const Json::Value& config)
  : ICozmoBehavior(config)
{
  for( const auto& behavior : config[kBehaviorsKey] ) {
    _iConfig.delegateIDs.push_back( BehaviorTypesWrapper::BehaviorIDFromString( behavior.asString() ) );
  }
  
  ANKI_VERIFY( !_iConfig.delegateIDs.empty(),
               "BehaviorDispatchAfterShake.Ctor.Empty",
               "No behavior delegates were found" );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchAfterShake::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchAfterShake::OnBehaviorActivated()
{
  _dVars.countShaken = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchAfterShake::InitBehavior()
{
  for( const auto& delegateID : _iConfig.delegateIDs ) {
    _iConfig.delegates.push_back( GetBEI().GetBehaviorContainer().FindBehaviorByID(delegateID) );
    
    ANKI_VERIFY( _iConfig.delegates.back() != nullptr,
                 "BehaviorDispatchAfterShake.Delegate.InvalidBehavior",
                 "could not get pointer for behavior '%s'",
                 BehaviorTypesWrapper::BehaviorIDToString(delegateID) );
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchAfterShake::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  // copy _iConfig.delegates into delegates, taking the raw ptrs
  std::transform( _iConfig.delegates.begin(),
                  _iConfig.delegates.end(),
                  std::inserter(delegates, delegates.end()),
                  [](const auto& x){ return x.get(); });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatchAfterShake::BehaviorUpdate()
{
  if(!IsActivated() || IsControlDelegated()){
    return;
  }

  const auto& robotInfo = GetBEI().GetRobotInfo();
  
#if REMOTE_CONSOLE_ENABLED
  if( kDevDispatchAfterShake > 0 ) {
    
    _dVars.countShaken = kDevDispatchAfterShake;
    kDevDispatchAfterShake = 0;
    _dVars.shakingSession = false;
    
  } else
#endif // REMOTE_CONSOLE_ENABLED
  {
    
    const bool isBeingShaken = (robotInfo.GetHeadAccelMagnitudeFiltered() > kAccelMagnitudeShakingStartedThreshold);
    const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if( (_dVars.shakingSession && isBeingShaken)
        || (!_dVars.shakingSession && !isBeingShaken) )
    {
      // last shake time / last steady time
      _dVars.lastChangeTime_s = currentTime;
    }
    
    const bool timeElapsed = (currentTime - _dVars.lastChangeTime_s >= kShakeTime);
    if( _dVars.shakingSession && (!isBeingShaken) && timeElapsed ) {
      // shaking stopped for a while
      _dVars.shakingSession = false;
      
      GetBEI().GetBackpackLightComponent().SetBackpackAnimation(kLightsSteady);
    }
    if( !_dVars.shakingSession && isBeingShaken && timeElapsed ) {
      // shaking started for a while
      _dVars.shakingSession = true;
      ++_dVars.countShaken;
      
      GetBEI().GetBackpackLightComponent().SetBackpackAnimation(kLightsShake);
    }
    // shaking is one of those words where if you write it enough times it starts to look wrong.
    
  }
  
  const bool isOnTreads = (robotInfo.GetOffTreadsState() == OffTreadsState::OnTreads);
  if( (_dVars.countShaken>0) && (!_dVars.shakingSession) && isOnTreads ) {
    TimeStamp_t currTimeStamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    if( _dVars.tickPlacedDown == 0 ) {
      _dVars.tickPlacedDown = currTimeStamp;
    }
    // time to delegate to the data defined delegate
    size_t idx = _dVars.countShaken - 1;
    if( _dVars.countShaken > _iConfig.delegates.size() ) {
      PRINT_NAMED_WARNING("BehaviorDispatchAfterShake.BehaviorUpdate.TooManyShakes",
                          "You shook the robot (%zu) times but there were only (%zu) behaviors",
                          _dVars.countShaken,
                          _iConfig.delegates.size());
      idx = std::min(idx, _iConfig.delegates.size()-1);
    }
    
    DEV_ASSERT( idx<_iConfig.delegates.size(), "BehaviorDispatchAfterShake.Update.OutOfRange" );
    auto& delegate = _iConfig.delegates[idx];
    if( ANKI_VERIFY( delegate != nullptr,
                     "BehaviorDispatchAfterShake.Update.NullDelegate",
                     "Behavior idx (%zu) is null",
                     idx ) )
    {
      if( delegate->WantsToBeActivated() ) {
        DelegateIfInControl( delegate.get() );
        // clear shaken count so you have to shake again to run the behavior again if it completes
        _dVars.countShaken = 0;
        _dVars.tickPlacedDown = 0;
      } else if( currTimeStamp > _dVars.tickPlacedDown + kFailTicksBeforeReset ) {
        // start over
        _dVars.countShaken = 0;
        _dVars.tickPlacedDown = 0;
        PRINT_NAMED_WARNING( "BehaviorDispatchAfterShake.BehaviorUpdate.NoWant",
                            "Behavior %s didn't want to be activated",
                            BehaviorTypesWrapper::BehaviorIDToString(delegate->GetID()) );
      }
    }
    
  }
}

} // namespace Cozmo
} // namespace Anki
