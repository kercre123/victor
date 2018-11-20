/**
 * File: checkForAndReactToSalientPoint.h
 *
 * Author:  Andrew Stein
 * Created: 2018-11-14
 *
 * Description: Run a single frame of a VisionMode specified in Json and react if one of the Json-configured
 *              SalientPointTypes is seen. Does nothing otherwise. The reaction can either be an animation
 *              or another behavior.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Vector_Behaviors_Reactions_CheckForAndReactToSalientPoint_H__
#define __Vector_Behaviors_Reactions_CheckForAndReactToSalientPoint_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorCheckForAndReactToSalientPoint : public ICozmoBehavior
{
public:
  ~BehaviorCheckForAndReactToSalientPoint();
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorCheckForAndReactToSalientPoint(const Json::Value& config);
  
  virtual void InitBehavior() override;
  
  virtual void OnBehaviorActivated() override;
  
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override { }
  
private:
  
  void CheckForStimulus();

  struct InstanceConfig;
  std::unique_ptr<InstanceConfig> _iConfig;
  
  struct DynamicVariables;
  std::unique_ptr<DynamicVariables> _dVars;
  
};

}
}

#endif /* __Vector_Behaviors_Reactions_CheckForAndReactToSalientPoint_H__ */
