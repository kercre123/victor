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

#include "clad/types/actionTypes.h"

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

class DelegateWrapper{
public:
  DelegateWrapper(Robot& robot);
  virtual ~DelegateWrapper(){};
  
  bool Delegate(IBSRunnable* delegator, IActionRunner* action);
  bool Delegate(IBSRunnable* delegator, IBSRunnable* delegated);
  bool Delegate(IBSRunnable* delegator,
                BehaviorExternalInterface& behaviorExternalInterface,
                HelperHandle helper,
                BehaviorSimpleCallbackWithExternalInterface successCallback,
                BehaviorSimpleCallbackWithExternalInterface failureCallback);
  
protected:
  friend class DelegationComponent;
  
private:
  Robot& _robot;
  
  // Naive tracking for action delegation
  IBSRunnable* _runnableThatQueuedAction;
  u32 _lastActionTag = ActionConstants::INVALID_TAG;
  
  // Naive tracking for helper delegation
  IBSRunnable* _runnableThatQueuedHelper;
  WeakHelperHandle _queuedHandle;
  
  void EnsureHandleIsUpdated();
};

  
class DelegationComponent {
public:
  DelegationComponent() {}; // Constructor for tests
  DelegationComponent(Robot& robot, BehaviorSystemManager& bsm, BehaviorManager& bm);
  virtual ~DelegationComponent(){};
  
  bool IsControlDelegated(const IBSRunnable* delegator);
  bool CancelDelegates(IBSRunnable* delegator);
  void CancelSelf(IBSRunnable* delegator);
  
  std::weak_ptr<DelegateWrapper> GetDelegateWrapper(IBSRunnable* delegator);
  
private:
  // For supporting legacy code
  friend class IBehavior;
  bool IsActing(const IBSRunnable* delegator);
  std::shared_ptr<DelegateWrapper>       _delegateWrapper;
  std::vector<::Signal::SmartHandle> _eventHandles;
  
  // this is an internal handler just for StartActing
  void HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg);
  
  //BehaviorSystemManager& _bsm;
  //BehaviorManager&       _bm;
  
};
  
  


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_DelegationComponent_H__
