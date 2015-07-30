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
    
    // Calls the currently-selected behavior's Update() method until it
    // returns COMPLETE or FAILURE. Once current behavior completes
    // switches to next behavior (including an "idle" behavior?).
    Result Update();
    
    // Picks next behavior based on robot's current state. This does not
    // transition immediately to running that behavior, but will let the
    // current beheavior know it needs wind down with a call to its
    // Interrupt() method.
    Result SelectNextBehavior();
    
    // Forcefully select the next behavior by name (versus by letting the
    // selection mechanism choose based on current state)
    Result SelectNextBehavior(const std::string& name);
    
    Result AddBehavior(const std::string& name, IBehavior* newBehavior);
    
  private:
    
    void SwitchFromCurrentToNext();
    
    Robot& _robot;
    
    // Map from behavior name to available behaviors
    using BehaviorContainer = std::unordered_map<std::string, IBehavior*>;
    BehaviorContainer _behaviors;
    
    BehaviorContainer::iterator _currentBehavior;
    BehaviorContainer::iterator _nextBehavior;
    
    // For random numbers
    std::mt19937 _randomGenerator;
    
  }; // class BehaviorManager
  
  
  class Reward
  {
  public:
    
    float value;
    
  }; // class Reward
  
} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_MANAGER_H
