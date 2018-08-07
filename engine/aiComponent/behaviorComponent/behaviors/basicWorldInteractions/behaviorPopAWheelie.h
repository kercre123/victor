/**
 * File: behaviorPopAWheelie.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-06-15
 *
 * Description: A behavior which allows cozmo to use a block to pop himself onto his back
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__

#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"

#include <string>

namespace Anki {
namespace Vector {

class BlockWorldFilter;
class ObservableObject;

class BehaviorPopAWheelie : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPopAWheelie(const Json::Value& config);
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;

private:
  enum class DebugState {
    ReactingToBlock,
    PerformingAction
  };

  struct InstanceConfig {
    InstanceConfig();
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
    // UpdateWhileNotRunning
    mutable ObjectID targetBlock;
    ObjectID         lastBlockReactedTo;
    s32              numPopAWheelieActionRetries;
    bool             hasDisabledcliff;
  };

  InstanceConfig   _iConfig;
  DynamicVariables _dVars;
        
  void TransitionToReactingToBlock();
  void TransitionToPerformingAction();
  void TransitionToPerformingAction(bool isRetry);
  
  void SetupRetryAction(const ExternalInterface::RobotCompletedAction& msg);
  
  void ResetBehavior();
  
  virtual void UpdateTargetBlock() const;
};
  
}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPopAWheelie_H__
