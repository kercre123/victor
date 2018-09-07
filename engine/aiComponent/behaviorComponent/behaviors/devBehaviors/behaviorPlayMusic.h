/**
 * File: BehaviorPlayMusic.h
 *
 * Author: ross
 * Created: 2018-09-06
 *
 * Description: plays online music
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlayMusic__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlayMusic__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorPlayMusic : public ICozmoBehavior
{
public: 
  virtual ~BehaviorPlayMusic();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorPlayMusic(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual void AlwaysHandleInScope(const RobotToEngineEvent& event) override;

private:
  
  void FixStimAtMax();
  void UnFixStim();

  struct InstanceConfig {
    InstanceConfig();
    // TODO: put configuration variables here
  };

  struct DynamicVariables {
    DynamicVariables();
    std::string request;
    bool isStimMaxed = false;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorPlayMusic__
