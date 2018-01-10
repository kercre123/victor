/**
* File: delegationComponent.h
*
* Author: Kevin M. Karol
* Created: 09/22/17
*
* Description: Component which handles delegation requests from behaviors
* and forwards them to the appropriate behavior/robot system
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__

#include "coretech/common/shared/types.h"

#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/helperHandle.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class BehaviorManager;
class BehaviorSystemManager;
class IActionRunner;
class IBehavior;
class Robot;

class Delegator : private Util::noncopyable{
public:
  Delegator(Robot& robot, BehaviorSystemManager& bsm);
  virtual ~Delegator(){};
  
  bool Delegate(IBehavior* delegatingBehavior, IActionRunner* action);
  bool Delegate(IBehavior* delegatingBehavior, IBehavior* delegated);
  bool Delegate(IBehavior* delegatingBehavior,
                BehaviorExternalInterface& behaviorExternalInterface,
                HelperHandle helper,
                BehaviorSimpleCallbackWithExternalInterface successCallback,
                BehaviorSimpleCallbackWithExternalInterface failureCallback);
  
protected:
  friend class DelegationComponent;

  void HandleActionComplete(u32 actionTag);
  
private:
  Robot& _robot;
  
  // Naive tracking for action delegation
  IBehavior* _behaviorThatDelegatedAction;
  u32 _lastActionTag;

  // Naive tracking for helper delegation
  IBehavior* _behaviorThatDelegatedHelper;
  WeakHelperHandle _delegateHelperHandle;
  BehaviorSystemManager* _bsm;

  void EnsureHandleIsUpdated();
};

  
class DelegationComponent : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable {
public:
  DelegationComponent();
  virtual ~DelegationComponent(){};

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComponents) override;
  virtual void UpdateDependent(const BCCompMap& dependentComponents) override {};
  virtual void AdditionalInitAccessibleComponents(BCCompIDSet& components) const override {
    components.insert(BCComponentID::BehaviorSystemManager);
  }
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void Init(Robot& robot, BehaviorSystemManager& bsm);
  
  bool IsControlDelegated(const IBehavior* delegatingBehavior);
  void CancelDelegates(IBehavior* delegatingBehavior);
  void CancelSelf(IBehavior* delegatingBehavior);

  bool HasDelegator(IBehavior* delegatingBehavior);
  Delegator& GetDelegator(IBehavior* delegatingBehavior);

  // called when an action completes (with any status). Argument is the action id
  void HandleActionComplete(u32 actionTag);

  // If control of the passed in behavior is delegated (to another behavior), return the pointer of the
  // behavior that it was delegated to. Otherwise, return nullptr (including if control was delegated to an
  // action or helper)
  const IBehavior* GetBehaviorDelegatedTo(const IBehavior* delegatingBehavior) const;
  
private:
  std::unique_ptr<Delegator>   _delegator;
  std::vector<::Signal::SmartHandle> _eventHandles;
    
  BehaviorSystemManager* _bsm;
  
  // For supporting legacy code
  friend class ICozmoBehavior;
  bool IsActing(const IBehavior* delegatingBehavior);
  
  // TMP for helpers only
  friend class IHelper;
  void CancelActionIfRunning(IBehavior* delegatingBehavior = nullptr);
  
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
