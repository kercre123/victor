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

#ifndef __Cozmo_Basestation_Behaviors_BehaviorFidget_H__
#define __Cozmo_Basestation_Behaviors_BehaviorFidget_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
  
  class BehaviorFidget : public IBehavior
  {
  public:
    
    BehaviorFidget(Robot& robot, const Json::Value& config);
    virtual ~BehaviorFidget();
    
    virtual bool IsRunnable(double currentTime_sec) const override { return true; }
    
  protected:
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
    
  private:
    
    enum class FidgetType {
      HeadTwitch = 0,
      LiftWiggle,
      LiftTap,
      TurnInPlace,
      Yawn,
      Sneeze,
      Stretch,
      
      NumFidgets
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

#endif // __Cozmo_Basestation_Behaviors_BehaviorFidget_H__
