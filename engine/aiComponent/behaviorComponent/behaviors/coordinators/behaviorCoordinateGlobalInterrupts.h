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


#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherPassThrough.h"

namespace Anki {
namespace Cozmo {

// forward declarations
class BehaviorHighLevelAI;
class BehaviorTimerUtilityCoordinator;


class BehaviorCoordinateGlobalInterrupts : public BehaviorDispatcherPassThrough
{
public:
  virtual ~BehaviorCoordinateGlobalInterrupts();

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorCoordinateGlobalInterrupts(const Json::Value& config);

  virtual void InitPassThrough() override;
  virtual void OnPassThroughActivated() override;
  virtual void PassThroughUpdate() override;
  virtual void OnPassThroughDeactivated() override;

private:
  
  void CreateConsoleVars();
  
  struct InstanceConfig{
    InstanceConfig();
    IBEIConditionPtr  triggerWordPendingCond;
    ICozmoBehaviorPtr wakeWordBehavior;
    std::shared_ptr<BehaviorTimerUtilityCoordinator> timerCoordBehavior;
    ICozmoBehaviorPtr highLevelAIBehavior;
    ICozmoBehaviorPtr sleepingBehavior;
    ICozmoBehaviorPtr reactToObstacleBehavior;
    
    std::unordered_map<ICozmoBehaviorPtr, bool> devActivatableOverrides;
  };

  struct DynamicVariables{
    DynamicVariables();
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;

  bool ShouldSuppressProxReaction() const;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Engine_Behaviors_BehaviorCoordinateGlobalInterrupts_H__
