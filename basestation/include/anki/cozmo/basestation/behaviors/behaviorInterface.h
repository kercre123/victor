/**
 * File: behaviorInterface.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines interface for a Cozmo "Behavior".
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef COZMO_BEHAVIOR_INTERFACE_H
#define COZMO_BEHAVIOR_INTERFACE_H

#include "anki/cozmo/basestation/actionContainers.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {
  
  // Forward declarations
  class Robot;
  class Reward;
  
  // Base Behavior Interface specification
  class IBehavior
  {
  public:
    enum class Status {
      FAILURE,
      RUNNING,
      COMPLETE
    };
    
    IBehavior(Robot& robot, const Json::Value& config) : _robot(robot) { }
    virtual ~IBehavior() { }
    
    // Returns true iff the state of the world/robot is sufficient for this
    // behavior to be executed
    virtual bool IsRunnable() const = 0;
    
    // Will be called upon first switching to a behavior before calling update.
    virtual Result Init() = 0;
    
    // Step through the behavior and deliver rewards to the robot along the way
    virtual Status Update(float currentTime_sec) = 0;
    
    // Tell this behavior to finish up ASAP so we can switch to a new one.
    // This should trigger any cleanup and get Update() to return COMPLETE
    // as quickly as possible.
    virtual Result Interrupt() = 0;
    
    // Figure out the reward this behavior will offer, given the robot's current
    // state. Returns true if the Behavior is runnable, false if not. (In the
    // latter case, the returned reward is not populated.)
    virtual bool GetRewardBid(Reward& reward) = 0;
    
    virtual const std::string& GetName() const = 0;
    
    // All behaviors run in a single "slot" in the AcitonList. (This seems icky.)
    static const ActionList::SlotHandle sActionSlot;
    
  protected:
    
    Robot &_robot;
    
  }; // class IBehavior

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_INTERFACE_H