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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/signals/simpleSignal_fwd.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class BehaviorManager;
class BehaviorSystemManager;
class ContinuityComponent;
class IActionRunner;
class IBehavior;

class Delegator : private Util::noncopyable{
public:
  Delegator(BehaviorSystemManager& bsm, 
            ContinuityComponent& continuityComp);
  virtual ~Delegator(){};
  
  bool Delegate(IBehavior* delegatingBehavior, IActionRunner* action);
  bool Delegate(IBehavior* delegatingBehavior, IBehavior* delegated);
  
protected:
  friend class DelegationComponent;

  void HandleActionComplete(u32 actionTag);
  
private:  
  // Naive tracking for action delegation
  IBehavior* _behaviorThatDelegatedAction;
  u32 _lastActionTag;

  BehaviorSystemManager& _bsm;
  ContinuityComponent& _continuityComp;
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
    components.insert(BCComponentID::AIComponent);
  }
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void Init(BehaviorSystemManager* bsm, ContinuityComponent* continuityComp);
  
  bool IsControlDelegated(const IBehavior* delegatingBehavior);
  void CancelDelegates(IBehavior* delegatingBehavior);
  void CancelSelf(IBehavior* delegatingBehavior);

  bool HasDelegator(IBehavior* delegatingBehavior);
  Delegator& GetDelegator(IBehavior* delegatingBehavior);

  // called when an action completes (with any status). Argument is the action id
  void HandleActionComplete(u32 actionTag);

  // If control of the passed in behavior is delegated (to another behavior), return the pointer of the
  // behavior that it was delegated to. Otherwise, return nullptr (including if control was delegated to an
  // action)
  const IBehavior* GetBehaviorDelegatedTo(const IBehavior* delegatingBehavior) const;

  // Return the last tick when the behavior stack was updated (i.e. new behavior delegated to or one was
  // canceled)
  size_t GetLastTickBehaviorStackChanged() const;
  
private:
  std::unique_ptr<Delegator>   _delegator;
  std::vector<::Signal::SmartHandle> _eventHandles;
    
  BehaviorSystemManager* _bsm = nullptr;
  ContinuityComponent* _continuityComp = nullptr;
  
  void CancelActionIfRunning(IBehavior* delegatingBehavior = nullptr);
  
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
