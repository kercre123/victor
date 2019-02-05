/**
 * File: behaviorResetState.h
 *
 * Author: ross
 * Created: 2018-08-12
 *
 * Description: Tries to reset things that may have caused behavior cycles
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorResetState__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorResetState__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorResetState : public ICozmoBehavior
{
public: 
  virtual ~BehaviorResetState() = default;
  
  void SetFollowupBehaviorID( BehaviorID behaviorID );

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorResetState(const Json::Value& config);

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override{}
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  
  virtual void BehaviorUpdate() override;

private:
  
  void PutDownCubeIfNeeded();
  void ResetComponentsAndDelegate();
  
  ICozmoBehaviorPtr _currBehavior = nullptr;
  ICozmoBehaviorPtr _nextBehavior = nullptr;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorResetState__
