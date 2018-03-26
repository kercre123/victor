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

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorTimers_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorTimers_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include <unordered_map>

namespace Anki {
namespace Cozmo {

class BehaviorTimer;
enum class BehaviorTimerTypes : uint8_t;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorTimerManager : public IDependencyManagedComponent<BCComponentID>
                           , public Anki::Util::noncopyable
{
public:
  BehaviorTimerManager();

  virtual void InitDependent( Robot* robot, const BCCompMap& dependentComponents ) override { }
  virtual void GetInitDependencies( BCCompIDSet& dependencies ) const override { }
  virtual void GetUpdateDependencies( BCCompIDSet& dependencies ) const override { }

  static BehaviorTimerTypes BehaviorTimerFromString( const std::string& name );
  
  static bool IsValidName( const std::string& name );

  BehaviorTimer& GetTimer(const BehaviorTimerTypes& timerId);

protected:
  std::unordered_map<BehaviorTimerTypes, BehaviorTimer> _timers;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorTimer
{
public:
  explicit BehaviorTimer( const BehaviorTimerTypes& timerId );

  // sync the timer to the current BS time
  void Reset();

  // whether the timer has ever been reset (true) or not (false)
  bool HasBeenReset() const { return _hasBeenReset; }

  // difference between current time and the last time this was reset. If it was never reset, the
  // elapsed time is wrt the time of creation
  float GetElapsedTimeInSeconds() const;
  
  // shortcut for using the above functions for the purpose of cooldowns.
  // true if GetElapsedTimeInSeconds is > cooldown. If !HasBeenReset, the return value is valueIfNoReset
  bool HasCooldownExpired(float cooldown, bool valueIfNoReset = true) const;

protected:
  float _baseTime;
  BehaviorTimerTypes _timerType;
  bool _hasBeenReset;
};

} // end namespace Cozmo
} // end namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_BehaviorTimers_H__
