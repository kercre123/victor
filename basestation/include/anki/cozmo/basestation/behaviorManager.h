/**
 * File: behaviorManager.h
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 *
 * Overhaul: 7/30/15, Andrew Stein
 *
 * Description: High-level module that is a container for available behaviors,
 *              determines what behavior a robot should be executing at a given
 *              time and handles ticking the running behavior forward.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef COZMO_BEHAVIOR_MANAGER_H
#define COZMO_BEHAVIOR_MANAGER_H

#include "anki/common/types.h"

#include "util/random/randomGenerator.h"

#include "json/json.h"

#include <unordered_map>
#include <string>
#include <random>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class IBehavior;
  class Reward;
  class Robot;
  
  class BehaviorManager
  {
  public:
    
    BehaviorManager(Robot& robot);
    ~BehaviorManager();
    
    Result Init(const Json::Value& config);
    
    // Calls the currently-selected behavior's Update() method until it
    // returns COMPLETE or FAILURE. Once current behavior completes
    // switches to next behavior (including an "idle" behavior?).
    Result Update(float currentTime_sec);
    
    // Picks next behavior based on robot's current state. This does not
    // transition immediately to running that behavior, but will let the
    // current beheavior know it needs wind down with a call to its
    // Interrupt() method.
    Result SelectNextBehavior();
    
    // Forcefully select the next behavior by name (versus by letting the
    // selection mechanism choose based on current state). Fails if that
    // behavior does not exist or the selected behavior is not runnable.
    Result SelectNextBehavior(const std::string& name);
    
    // Add a new behavior to the available ones to be selected from.
    // The behavior will be stored keyed by the name returned by its GetName() method.
    // NOTE: The BehaviorManager will handle deleting newBehavior when it
    //  destructs.
    Result AddBehavior(IBehavior* newBehavior);
    
    // Specify the minimum time we should stay in each behavior before
    // considering switching
    void SetMinBehaviorTime(float time_sec) { _minBehaviorTime_sec = time_sec; }
    
    // Returns nullptr if there is no current behavior
    const IBehavior* GetCurrentBehavior() const;
    
  private:
    
    bool _isInitialized;
    bool _forceReInit;
    
    Robot& _robot;
    
    void SwitchToNextBehavior();
    Result InitNextBehaviorHelper();
    
    // Map from behavior name to available behaviors
    using BehaviorContainer = std::unordered_map<std::string, IBehavior*>;
    BehaviorContainer _behaviors;
    
    BehaviorContainer::iterator _currentBehavior;
    BehaviorContainer::iterator _nextBehavior;
    
    // Minimum amount of time to stay in each behavior
    float _minBehaviorTime_sec;
    float _lastSwitchTime_sec;
    
    // For random numbers
    Util::RandomGenerator _rng;
    
  }; // class BehaviorManager
  
  
  class Reward
  {
  public:
    
    float value;
    
  }; // class Reward
  
} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
