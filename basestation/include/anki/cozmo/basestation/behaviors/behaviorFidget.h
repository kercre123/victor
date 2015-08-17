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
    
    virtual bool IsRunnable(float currentTime_sec) const override { return true; }
    
    virtual Result Init() override;
    
    virtual Status Update(float currentTime_sec) override;
    
    // Finish placing current object if there is one, otherwise good to go
    virtual Result Interrupt(float currentTime_sec) override;
    
    virtual bool GetRewardBid(Reward& reward) override;

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
    
    using MakeFidgetAction = std::function<IActionRunner*()>;
    
    void AddFidget(const std::string& name, MakeFidgetAction fcn, s32 frequency);
    
    // Mapping from "probability" to a name paired with an action creator
    std::map<s32, std::pair<std::string, MakeFidgetAction> > _fidgets;
    
    bool _interrupted;
    
    f32 _lastFidgetTime_sec;
    f32 _nextFidgetWait_sec;
    
    f32 _minWait_sec, _maxWait_sec;
    
    s32 _totalProb;
    
  }; // class BehaviorFidget

} // namespace Cozmo
} // namespace Anki

#endif // COZMO_BEHAVIOR_FIDGET_H
