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

#include "anki/common/types.h"

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
  
  bool Delegate(IBehavior* delegatingRunnable, IActionRunner* action);
  bool Delegate(IBehavior* delegatingRunnable, IBehavior* delegated);
  bool Delegate(IBehavior* delegatingRunnable,
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
  IBehavior* _runnableThatDelegatedAction;
  u32 _lastActionTag;

  // Naive tracking for helper delegation
  IBehavior* _runnableThatDelegatedHelper;
  WeakHelperHandle _delegateHelperHandle;
  BehaviorSystemManager* _bsm;

  void EnsureHandleIsUpdated();
};

  
class DelegationComponent : private Util::noncopyable {
public:
  DelegationComponent();
  virtual ~DelegationComponent(){};
  
  void Init(Robot& robot, BehaviorSystemManager& bsm);
  
  bool IsControlDelegated(const IBehavior* delegatingRunnable);
  void CancelDelegates(IBehavior* delegatingRunnable);
  void CancelSelf(IBehavior* delegatingRunnable);

  bool HasDelegator(IBehavior* delegatingRunnable);
  Delegator& GetDelegator(IBehavior* delegatingRunnable);

  // called when an action completes (with any status). Argument is the action id
  void HandleActionComplete(u32 actionTag);
  
private:
  std::unique_ptr<Delegator>   _delegator;
  std::vector<::Signal::SmartHandle> _eventHandles;
    
  BehaviorSystemManager* _bsm;
  
  // For supporting legacy code
  friend class ICozmoBehavior;
  bool IsActing(const IBehavior* delegatingRunnable);
  
  // TMP for helpers only
  friend class IHelper;
  void CancelActionIfRunning(IBehavior* delegatingRunnable = nullptr);
  
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
