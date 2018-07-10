/**
* File: behaviorSystemManager.h
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Manages and enforces the lifecycle and transitions
* of parts of the behavior system
*
* Copyright: Anki, Inc. 2017
**/

#ifndef COZMO_BEHAVIOR_SYSTEM_MANAGER_H
#define COZMO_BEHAVIOR_SYSTEM_MANAGER_H

#include "coretech/common/shared/types.h"

#include "clad/types/robotCompletedAction.h"
#include "engine/aiComponent/behaviorComponent/asyncMessageGateComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/iBehaviorRunner.h"
#include "json/json-forwards.h"
#include "util/signals/simpleSignal_fwd.h"

#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorExternalInterface;
class BehaviorStack;
class IBehavior;
class Robot;

struct BehaviorRunningInfo;

class BehaviorSystemManager : public IDependencyManagedComponent<BCComponentID>, 
                              public IBehaviorRunner, 
                              private Util::noncopyable
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorSystemManager();
  virtual ~BehaviorSystemManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override;
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override 
  {
    dependencies.insert(BCComponentID::BehaviorsBootLoader);
    dependencies.insert(BCComponentID::BehaviorExternalInterface);
    dependencies.insert(BCComponentID::AsyncMessageComponent);
  }

  virtual void AdditionalUpdateAccessibleComponents(BCCompIDSet& components) const override
  {
    components.insert(BCComponentID::BehaviorExternalInterface);
  };
  //////
  // end IDependencyManagedComponent functions
  //////

  // initialize this behavior manager from the given Json config
  Result InitConfiguration(Robot& robot,
                           IBehavior* baseBehavior,
                           BehaviorExternalInterface& behaviorExternalInterface,
                           AsyncMessageGateComponent* asyncMessageComponent);
  
  // destroy the current behavior stack and setup a new one - provides
  // no gaurentees that other aspects of behavior system or component state are reset
  void ResetBehaviorStack(IBehavior* baseBehavior);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  bool IsControlDelegated(const IBehavior* delegator) override;
  bool CanDelegate(IBehavior* delegator) override;

  // delegate and return true if delegation was successful
  bool Delegate(IBehavior* delegator, IBehavior* delegated) override;

  // cancel delegates, and return true if any were canceled (note that false here isn't an error, it's OK for
  // a behavior with no delegates to call this function and have false returned)
  bool CancelDelegates(IBehavior* delegator) override;
  
  void CancelSelf(IBehavior* delegator) override;

  // If control of the passed in behavior is delegated (to another behavior), return the pointer of the
  // behavior that it was delegated to. Otherwise, return nullptr (including if control was delegated to an
  // action or helper)
  const IBehavior* GetBehaviorDelegatedTo(const IBehavior* delegatingBehavior) const;
  
  // Return the pointer of the behavior which delegated to the passed in behavior. If at the base behavior,
  // return nullptr
  const IBehavior* GetBehaviorDelegatedFrom(const IBehavior* behavior) const;

  // Returns the behavior at the base of the stack
  const IBehavior* GetBaseBehavior() const;

  // Returns the behavior at the top of the stack
  const IBehavior* GetTopBehavior() const;
  
  // Returns string label of the behavior at the top of the stack
  const std::string& GetTopBehaviorDebugLabel() const;
  
  // calls upon the behavior stack to build and return the behavior tree as a flat array in json
  Json::Value BuildDebugBehaviorTree(BehaviorExternalInterface& bei) const;

  // Return the last tick when the behavior stack was updated (i.e. new behavior delegated to or one was
  // canceled)  
  size_t GetLastTickBehaviorStackChanged() const { return _lastBehaviorStackUpdateTick; }

private:
  enum class InitializationStage{
    SystemNotInitialized,
    StackNotInitialized,
    Initialized
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  InitializationStage _initializationStage;
  // Store the base behavior until the stack is initialized
  IBehavior* _baseBehaviorTmp;
  
  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  BehaviorExternalInterface* _behaviorExternalInterface;
  
  AsyncMessageGateComponent* _asyncMessageComponent;
  std::vector<ExternalInterface::RobotCompletedAction> _actionsCompletedThisTick;
  std::vector<Signal::SmartHandle> _eventHandles;

  std::unique_ptr<BehaviorStack> _behaviorStack;

  size_t _lastBehaviorStackUpdateTick = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void UpdateInActivatableScope(BehaviorExternalInterface& behaviorExternalInterface, const std::set<IBehavior*>& tickedInStack);
  
}; // class BehaviorSystemManager

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_SYSTEM_MANAGER_H
