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
  private:
    
    // Enforce creation through BehaviorFactory
    friend class BehaviorFactory;
    BehaviorFidget(Robot& robot, const Json::Value& config);
    virtual ~BehaviorFidget();
    
    // TODO REMOVE THIS HACK! This is temporarily being overridde for the investor demo
    virtual float EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const override { return 0.01f; }

  public:
    
    virtual bool IsRunnable(const Robot& robot, double currentTime_sec) const override { return true; }
    
  protected:
    
    virtual Result InitInternal(Robot& robot, double currentTime_sec) override;
    virtual Status UpdateInternal(Robot& robot, double currentTime_sec) override;
    virtual Result InterruptInternal(Robot& robot, double currentTime_sec) override;
    virtual void   StopInternal(Robot& robot, double currentTime_sec) override;
    
    virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
    
  private:
    
    using MakeFidgetAction = std::function<IActionRunner*()>;
    
    struct FidgetData
    {
      std::string _name;
      MakeFidgetAction _function;
      int32_t _frequency;
      bool _mustComplete;
      float _minTimeBetween_sec;
      double _lastTimeUsed_sec;
    };
    
    void AddFidget(const std::string& name,
                   MakeFidgetAction fcn,
                   s32 frequency,
                   float minTimeBetween = 0,
                   bool mustComplete = false);
    
    std::vector<FidgetData> _fidgets;
    
    bool _interrupted;
    
    f32 _lastFidgetTime_sec;
    f32 _nextFidgetWait_sec;
    
    f32 _minWait_sec, _maxWait_sec;
    
    uint32_t _queuedActionTag;
    bool _currentActionMustComplete = false;
    
  }; // class BehaviorFidget

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorFidget_H__
