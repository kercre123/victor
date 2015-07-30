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
    
    virtual ~IBehavior() { }
    
    // Step through the behavior and deliver rewards to the robot along the way
    virtual Status Update(Robot& robot) = 0;
    
    virtual Status Interrupt(Robot& robot) = 0;
    
    // Figure out the reward this behavior will offer, given the robot's current
    // state. Returns true if the Behavior is runnable, false if not. (In the
    // latter case, the returned reward is not populated.)
    virtual bool GetRewardBid(Robot& robot, Reward& reward) = 0;
    
    
  }; // class IBehavior

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_INTERFACE_H