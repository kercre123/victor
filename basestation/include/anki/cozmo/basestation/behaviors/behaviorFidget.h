/**
 * File: behaviorFidget.h
 *
 * Author: Andrew Stein
 * Date:   8/4/15
 *
 * Description: Defines Cozmo's "Fidgeting" behavior, which is an idle animation 
 *              that moves him around in little quick movements periodically.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef COZMO_BEHAVIOR_FIDGET_H
#define COZMO_BEHAVIOR_FIDGET_H

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
  
  class BehaviorFidget : public IBehavior
  {
  public:
    
    BehaviorFidget(Robot& robot, const Json::Value& config);
    virtual ~BehaviorFidget();
    
    virtual bool IsRunnable() const override { return true; }
    
    virtual Result Init() override;
    
    virtual Status Update(float currentTime_sec) override;
    
    // Finish placing current object if there is one, otherwise good to go
    virtual Result Interrupt() override;
    
    virtual bool GetRewardBid(Reward& reward) override;
    
    virtual const std::string& GetName() const override {
      return _name;
    }

  private:
    
    enum class FidgetType {
      HEAD_TWITCH = 0,
      LIFT_WIGGLE,
      LIFT_TAP,
      TURN_IN_PLACE,
      YAWN,
      SNEEZE,
      STRETCH,
      
      NUM_FIDGETS
    };
    
    bool _interrupted;
    
    f32 _lastFidgetTime_sec;
    f32 _nextFidgetWait_sec;
    
    f32 _minWait_sec, _maxWait_sec;
    
    std::string _name;
    
  }; // class BehaviorFidget

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_FIDGET_H
