/**
 * File: behaviorOCD.h
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines Cozmo's "OCD" behavior, which likes to keep blocks neat n' tidy.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef COZMO_BEHAVIOR_OCD_H
#define COZMO_BEHAVIOR_OCD_H

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectTypesAndIDs.h"

#include <set>

namespace Anki {
namespace Cozmo {
  
  // A behavior that tries to neaten up blocks present in the world
  class OCD_Behavior : public IBehavior
  {
  public:
    
    virtual Status Init(Robot& robot) override;
    
    virtual Status Update(Robot& robot) override;
    
    virtual bool IsRunnable(Robot &robot) const;
    
    virtual bool GetRewardBid(Robot& robot, Reward& reward) override;
    
  private:
    
    // Internally, this behavior is just a little state machine going back and
    // forth between picking up and placing blocks
    
    enum class State {
      PICKING_UP_BLOCK,
      PLACING_BLOCK
    };
    
    State _currentState;
    
    // Enumerate possible arrangements Cozmo "likes".
    enum class Arrangement {
      STACKS,
      LINE
    };
    
    Arrangement _currentArrangement;
    
    std::set<ObjectID> _objectsOfInterest;
    
  }; // class OCD_Behavior

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_OCD_H