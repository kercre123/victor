/**
* File: delegationComponent.cpp
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


#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/delegationComponent.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "engine/actions/actionInterface.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/iBSRunnable.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/externalInterface/externalInterface.h"

#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
namespace{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelegateWrapper::DelegateWrapper(Robot& robot)
: _robot(robot)
, _runnableThatQueuedAction(nullptr)
, _runnableThatQueuedHelper(nullptr)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegateWrapper::Delegate(IBSRunnable* delegator, IActionRunner* action)
{
  EnsureHandleIsUpdated();

  DEV_ASSERT_MSG(((_runnableThatQueuedHelper == nullptr) ||
                        (_runnableThatQueuedHelper == delegator)) &&
                 (_runnableThatQueuedAction == nullptr),
                 "DelegateWrapper.DelegateAction.RunnableAlreadySet",
                 "Queued Helper address %p, Queued Action address: %p",
                 _runnableThatQueuedHelper, _runnableThatQueuedAction);
  
  
  Result result = _robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);
  if (RESULT_OK != result) {
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.NotQueued",
                        "Behavior '%s' can't queue action '%s' (error %d)",
                        delegator->GetPrintableID().c_str(),
                        action->GetName().c_str(), result);
    delete action;
    return false;
  }
  
  _runnableThatQueuedAction = delegator;
  _lastActionTag = action->GetTag();
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegateWrapper::Delegate(IBSRunnable* delegator, IBSRunnable* delegated)
{
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegateWrapper::Delegate(IBSRunnable* delegator,
                               BehaviorExternalInterface& behaviorExternalInterface,
                               HelperHandle helper,
                               BehaviorSimpleCallbackWithExternalInterface successCallback,
                               BehaviorSimpleCallbackWithExternalInterface failureCallback)
{
  EnsureHandleIsUpdated();
  
  DEV_ASSERT_MSG((_runnableThatQueuedHelper == nullptr) &&
                 (_runnableThatQueuedAction == nullptr),
                 "DelegateWrapper.DelegateHelper.RunnableAlreadySet",
                 "Queued Helper address %p, Queued Action address: %p",
                 _runnableThatQueuedHelper, _runnableThatQueuedAction);
  _runnableThatQueuedHelper = delegator;
  _queuedHandle = helper;
  return _robot.GetAIComponent().GetBehaviorHelperComponent().
                     DelegateToHelper(behaviorExternalInterface,
                                      helper, successCallback, failureCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegateWrapper::EnsureHandleIsUpdated()
{
  // If the helper completes successfully we don't get notified
  // so clear the _runnable to avoid the DEV_ASSERTS
  if(_queuedHandle.expired()){
    _runnableThatQueuedHelper = nullptr;
    _queuedHandle.reset();
  }
}

  
///////////////////////
///////////////////////
///////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelegationComponent::DelegationComponent(Robot& robot,
                                         BehaviorSystemManager& bsm,
                                         BehaviorManager& bm)
//: _bsm(bsm)
//, _bm(bm)
: _delegateWrapper( new DelegateWrapper(robot))
{
  
  IExternalInterface* externalInterface = robot.GetExternalInterface();
  if(externalInterface != nullptr) {
    using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
    using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;
    // NOTE: this won't get sent down to derived classes (unless they also subscribe)
    _eventHandles.push_back(externalInterface->Subscribe(
       EngineToGameTag::RobotCompletedAction,
       [this](const EngineToGameEvent& event) {
         DEV_ASSERT(event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction,
                    "IBehavior.RobotCompletedAction.WrongEventTypeFromCallback");
         HandleActionComplete(event.GetData().Get_RobotCompletedAction());
       } ));
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg)
{
  if(IsControlDelegated(_delegateWrapper->_runnableThatQueuedAction) &&
     (msg.idTag == _delegateWrapper->_lastActionTag))
  {
    _delegateWrapper->_lastActionTag = ActionConstants::INVALID_TAG;
    _delegateWrapper->_runnableThatQueuedAction = nullptr;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsControlDelegated(const IBSRunnable* delegator)
{
  if(_delegateWrapper->_runnableThatQueuedAction == delegator){
    return IsActing(delegator);
  }else if(_delegateWrapper->_runnableThatQueuedHelper == delegator){
    return !_delegateWrapper->_queuedHandle.expired();
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsActing(const IBSRunnable* delegator)
{
  if(_delegateWrapper->_runnableThatQueuedAction == delegator){
    return _delegateWrapper->_lastActionTag != ActionConstants::INVALID_TAG;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::CancelDelegates(IBSRunnable* delegator)
{
  if(_delegateWrapper->_runnableThatQueuedAction != nullptr){
    _delegateWrapper->_runnableThatQueuedAction = nullptr;
    bool ret = false;
    u32 tagToCancel = _delegateWrapper->_lastActionTag;
    if(_delegateWrapper->_runnableThatQueuedAction == delegator){
      ret = _delegateWrapper->_robot.GetActionList().Cancel(tagToCancel);
    }
    // note that the callback, if there was one (and it was allowed to run), should have already been called
    // at this point, so it's safe to clear the tag. Also, if the cancel itself failed, that is probably a
    // bug, but somehow the action is gone, so no sense keeping the tag around (and it clearly isn't
    // running). If the callback called StartActing, we may have a new action tag, so only clear this if the
    // cancel didn't change it
    if( _delegateWrapper->_lastActionTag == tagToCancel ) {
      _delegateWrapper->_lastActionTag = ActionConstants::INVALID_TAG;
    }
    return ret;
  }else if(_delegateWrapper->_runnableThatQueuedHelper != nullptr){
    _delegateWrapper->_runnableThatQueuedHelper = nullptr;
    const bool res = _delegateWrapper->_robot.GetAIComponent().GetBehaviorHelperComponent().StopHelperWithoutCallback(_delegateWrapper->_queuedHandle.lock());
    _delegateWrapper->_queuedHandle.reset();
    return res;
  }else{
    return false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelSelf(IBSRunnable* delegator)
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::weak_ptr<DelegateWrapper> DelegationComponent::GetDelegateWrapper(IBSRunnable* delegator)
{
  // TODO: Add logic checking BSM to see if on top
  return _delegateWrapper;
}

} // namespace Cozmo
} // namespace Anki
