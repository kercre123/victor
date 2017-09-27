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

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/helperHandle.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior_fwd.h"

#include "util/signals/simpleSignal_fwd.h"

#include <memory>

namespace Anki {
namespace Cozmo {

// Forward Declaration
class BehaviorManager;
class BehaviorSystemManager;
class IActionRunner;
class IBSRunnable;
class Robot;

class Delegator{
public:
  Delegator(Robot& robot,
                  BehaviorSystemManager& bsm);
  virtual ~Delegator(){};
  
  bool Delegate(IBSRunnable* delegatingRunnable, IActionRunner* action);
  bool Delegate(IBSRunnable* delegatingRunnable, IBSRunnable* delegated);
  bool Delegate(IBSRunnable* delegatingRunnable,
                BehaviorExternalInterface& behaviorExternalInterface,
                HelperHandle helper,
                BehaviorSimpleCallbackWithExternalInterface successCallback,
                BehaviorSimpleCallbackWithExternalInterface failureCallback);
  
protected:
  friend class DelegationComponent;
  
private:
  Robot& _robot;
  
  // Naive tracking for action delegation
  IBSRunnable* _runnableThatDelegatedAction;
  u32 _lastActionTag;
  
  // Naive tracking for helper delegation
  IBSRunnable* _runnableThatDelegatedHelper;
  WeakHelperHandle _delegateHelperHandle;
  BehaviorSystemManager* _bsm;
  
  void EnsureHandleIsUpdated();
};

  
class DelegationComponent {
public:
  DelegationComponent() {}; // Constructor for tests
  DelegationComponent(Robot& robot, BehaviorSystemManager& bsm);
  virtual ~DelegationComponent(){};
  
  bool IsControlDelegated(const IBSRunnable* delegatingRunnable);
  void CancelDelegates(IBSRunnable* delegatingRunnable);
  void CancelSelf(IBSRunnable* delegatingRunnable);
  
  std::weak_ptr<Delegator> GetDelegator(IBSRunnable* delegatingRunnable);
  
private:
  // For supporting legacy code
  friend class IBehavior;
  bool IsActing(const IBSRunnable* delegatingRunnable);
  std::shared_ptr<Delegator>   _delegator;
  std::weak_ptr<Delegator>     _invalidDelegator;
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  // this is an internal handler just for StartActing
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);
  
  BehaviorSystemManager* _bsm;
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
