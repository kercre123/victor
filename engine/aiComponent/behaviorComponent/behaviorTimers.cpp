/**
 * File: behaviorTimers.h
 *
 * Author: ross
 * Created: 2018 feb 2
 *
 * Description: BehaviorTimerManager holds a collection of named timers BehaviorTimer
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviorTimers.h"
#include "clad/types/behaviorComponent/behaviorTimerTypes.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimerManager::BehaviorTimerManager()
 : IDependencyManagedComponent(this, BCComponentID::BehaviorTimerManager)
{
  for( size_t i=0; i<BehaviorTimerTypesNumEntries; ++i ) {
    const auto timerId = static_cast<BehaviorTimerTypes>(i);
    _timers.emplace( std::piecewise_construct,
                     std::forward_as_tuple(timerId),
                     std::forward_as_tuple(timerId) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerManager::IsValidName( const std::string& name )
{
  BehaviorTimerTypes waste;
  return EnumFromString( name, waste );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimerTypes BehaviorTimerManager::BehaviorTimerFromString(const std::string& name)
{
  return BehaviorTimerTypesFromString( name );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimer& BehaviorTimerManager::GetTimer(const BehaviorTimerTypes& timerId)
{
  const auto it = _timers.find( timerId );
  if( !ANKI_VERIFY(it != _timers.end(),
                   "BehaviorTimerManager.GetTimer.Missing",
                   "Could not find timer '%s'",
                   BehaviorTimerTypesToString(timerId)) )
  {
    return _timers.find( BehaviorTimerTypes::Invalid )->second;
  } else {
    return it->second;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimer::BehaviorTimer( const BehaviorTimerTypes& timerId )
  : _timerType( timerId )
  , _hasBeenReset( false )
{
  _baseTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimer::Reset()
{
  if( ANKI_VERIFY( _timerType != BehaviorTimerTypes::Invalid,
                   "BehaviorTimer.Reset",
                   "Cannot reset the default timer") )
  {
    
    _baseTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _hasBeenReset = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorTimer::GetElapsedTimeInSeconds() const
{
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float diffTime = currentTime - _baseTime;
  return diffTime;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimer::HasCooldownExpired(float cooldown, bool valueIfNoReset) const
{
  if( ANKI_VERIFY( cooldown >= 0.0f,
                   "BehaviorTimer.HasCooldownExpired.Negative",
                   "Cooldown %f must be non-negative",
                   cooldown) )
  {
    if( !HasBeenReset() ) {
      return valueIfNoReset;
    } else {
      return (GetElapsedTimeInSeconds() > cooldown);
    }
  } else {
    return true;
  }
}

} // namespace Cozmo
} // namespace Anki
