/**
* File: behaviorCoordinateGlobalInterrupts.h
*
* Author: Kevin M. Karol
* Created: 2/22/18
*
* Description: Behavior responsible for handling special case needs 
* that require coordination across behavior global interrupts
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__
#define __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

// forward declarations
class BehaviorHighLevelAI;
class BehaviorTimerUtilityCoordinator;


class BehaviorCoordinateGlobalInterrupts : public ICozmoBehavior
{
public:
  virtual ~BehaviorCoordinateGlobalInterrupts();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorCoordinateGlobalInterrupts(const Json::Value& config);

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  virtual bool WantsToBeActivatedBehavior() const override;

private:
  struct InstanceConfig{
    InstanceConfig();
    IBEIConditionPtr  triggerWordPendingCond;
    ICozmoBehaviorPtr wakeWordBehavior;
    ICozmoBehaviorPtr globalInterruptsBehavior;
    std::shared_ptr<BehaviorTimerUtilityCoordinator> timerCoordBehavior;
    ICozmoBehaviorPtr highLevelAIBehavior;
    ICozmoBehaviorPtr sleepingBehavior;
  };

  struct DynamicVariables{
    DynamicVariables();
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__
