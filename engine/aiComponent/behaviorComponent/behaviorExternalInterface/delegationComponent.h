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
#include "engine/aiComponent/behaviorComponent/behaviors/ICozmoBehavior_fwd.h"

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

class Delegator{
public:
  Delegator(Robot& robot, BehaviorSystemManager& bsm);
  virtual ~Delegator(){};
  
  bool Delegate(IBehavior* delegatingRunnable, IActionRunner* action,
                BehaviorRobotCompletedActionCallback callback);
  bool Delegate(IBehavior* delegatingRunnable, IBehavior* delegated);
  bool Delegate(IBehavior* delegatingRunnable,
                BehaviorExternalInterface& behaviorExternalInterface,
                HelperHandle helper,
                BehaviorSimpleCallbackWithExternalInterface successCallback,
                BehaviorSimpleCallbackWithExternalInterface failureCallback);
  
protected:
  friend class DelegationComponent;
  
private:
  Robot& _robot;
  
  // Naive tracking for action delegation
  IBehavior* _runnableThatDelegatedAction;
  u32 _lastActionTag;

  // hold the callback along with a copy of it's arguments so it can be called during Update
  BehaviorRobotCompletedActionCallback _actionCallback;
  std::shared_ptr<ExternalInterface::RobotCompletedAction> _lastCompletedMsgCopy;
  
  // Naive tracking for helper delegation
  IBehavior* _runnableThatDelegatedHelper;
  WeakHelperHandle _delegateHelperHandle;
  BehaviorSystemManager* _bsm;

  void EnsureHandleIsUpdated();
};

  
class DelegationComponent {
public:
  DelegationComponent() {}; // Constructor for tests
  DelegationComponent(Robot& robot, BehaviorSystemManager& bsm);
  virtual ~DelegationComponent(){};

  void Update();
  
  bool IsControlDelegated(const IBehavior* delegatingRunnable);
  void CancelDelegates(IBehavior* delegatingRunnable);
  void CancelSelf(IBehavior* delegatingRunnable);

  // TODO:(bn) // TEMP: try to avoid the needing to pass in 'this'. Should be easy to just make "delegator"
  // optional like we did for components in the external interface
  std::weak_ptr<Delegator> GetDelegator(IBehavior* delegatingRunnable);
  
private:
  // For supporting legacy code
  friend class ICozmoBehavior;
  bool IsActing(const IBehavior* delegatingRunnable);
  std::shared_ptr<Delegator>   _delegator;
  std::weak_ptr<Delegator>     _invalidDelegator; // TODO:(bn) change to not weak? Keep consistent with components
  std::vector<::Signal::SmartHandle> _eventHandles;
    
  BehaviorSystemManager* _bsm;

#if !USE_BSM
  // needs to be public so ICozmoBehavior can call in. With USE_BSM this handler is the "real" one that gets the
  // event, with !USE_BSM, the ICozmoBehavior one is "real" and call this one
public:
#endif
  
  // this is an internal handler just for StartActing
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
