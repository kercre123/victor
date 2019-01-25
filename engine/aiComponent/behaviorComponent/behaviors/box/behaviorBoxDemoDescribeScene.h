/**
 * File: BehaviorBoxDemoDescribeScene.h
 *
 * Author: Andrew Stein
 * Created: 2019-01-09
 *
 * Description: Shows text describing the scene when asked or when The Box is repositioned
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoDescribeScene__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoDescribeScene__
#pragma once

#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BlockWorldFilter;

class BehaviorBoxDemoDescribeScene : public ICozmoBehavior
{
public: 
  virtual ~BehaviorBoxDemoDescribeScene();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorBoxDemoDescribeScene(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;

private:

  struct InstanceConfig {
    InstanceConfig();
    
    ICozmoBehaviorPtr showTextBehavior;
    float recentDelocTimeWindow_sec = 0.5f;
    float textDisplayTime_sec = 3.f;
    float visionRequestTimeout_sec = 5.f;
    IBEIConditionPtr touchAndReleaseCondition;
    
    std::unique_ptr<BlockWorldFilter> blockWorldFilter;
    TimeStamp_t recentObjectSeenTimeWindow_ms = 2000;
  };

  struct DynamicVariables {
    DynamicVariables();
    RobotTimeStamp_t lastImageTime_ms = 0;
    bool isUsingOCR = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  void DisplayDescription();
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorBoxDemoDescribeScene__
