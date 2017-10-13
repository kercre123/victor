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
#include "clad/types/actionTypes.h"

#include "engine/actions/actionInterface.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/externalInterface/externalInterface.h"

#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
namespace{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Delegator::Delegator(Robot& robot,
                                 BehaviorSystemManager& bsm)
: _robot(robot)
, _runnableThatDelegatedAction(nullptr)
, _lastActionTag(ActionConstants::INVALID_TAG)
, _runnableThatDelegatedHelper(nullptr)
, _bsm(&bsm)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingRunnable,
                         IActionRunner* action)
{
  EnsureHandleIsUpdated();

  DEV_ASSERT_MSG(((_runnableThatDelegatedHelper == nullptr) ||
                        (_runnableThatDelegatedHelper == delegatingRunnable)) &&
                 (_runnableThatDelegatedAction == nullptr),
                 "Delegator.DelegateAction.RunnableAlreadySet",
                 "Queued Helper address %p, Queued Action address: %p",
                 _runnableThatDelegatedHelper, _runnableThatDelegatedAction);
  
  
  Result result = _robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);
  if (RESULT_OK != result) {
    PRINT_NAMED_WARNING("ICozmoBehavior.Delegate.Failure.NotQueued",
                        "Behavior '%s' can't queue action '%s' (error %d)",
                        delegatingRunnable->GetPrintableID().c_str(),
                        action->GetName().c_str(), result);
    delete action;
    return false;
  }
  
  _runnableThatDelegatedAction = delegatingRunnable;
  _lastActionTag = action->GetTag();
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingRunnable, IBehavior* delegated)
{
  if(USE_BSM){
    return _bsm->Delegate(delegatingRunnable, delegated);
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingRunnable,
                               BehaviorExternalInterface& behaviorExternalInterface,
                               HelperHandle helper,
                               BehaviorSimpleCallbackWithExternalInterface successCallback,
                               BehaviorSimpleCallbackWithExternalInterface failureCallback)
{
  EnsureHandleIsUpdated();
  
  DEV_ASSERT_MSG((_runnableThatDelegatedHelper == nullptr) &&
                 (_runnableThatDelegatedAction == nullptr),
                 "Delegator.DelegateHelper.RunnableAlreadySet",
                 "Queued Helper address %p, Queued Action address: %p",
                 _runnableThatDelegatedHelper, _runnableThatDelegatedAction);
  _runnableThatDelegatedHelper = delegatingRunnable;
  _delegateHelperHandle = helper;
  return _robot.GetAIComponent().GetBehaviorHelperComponent().
                     DelegateToHelper(behaviorExternalInterface,
                                      helper, successCallback, failureCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Delegator::EnsureHandleIsUpdated()
{
  // If the helper completes successfully we don't get notified
  // so clear the _runnable to avoid the DEV_ASSERTS
  if(_delegateHelperHandle.expired()){
    _runnableThatDelegatedHelper = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Delegator::HandleActionComplete(u32 actionTag)
{
  if( actionTag == _lastActionTag ) {
    _lastActionTag = ActionConstants::INVALID_TAG;
    _runnableThatDelegatedAction = nullptr;
  }
}

  
///////////////////////
///////////////////////
///////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelegationComponent::DelegationComponent()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::Init(Robot& robot, BehaviorSystemManager& bsm)
{
  _delegator = std::make_unique<Delegator>(robot, bsm);
  _bsm = &bsm;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsControlDelegated(const IBehavior* delegatingRunnable)
{
  if( delegatingRunnable == nullptr ) {
    return false;
  }
  
  if(USE_BSM){
    if(_bsm->IsControlDelegated(delegatingRunnable)){
      return true;
    }
  }
  
  if(_delegator->_runnableThatDelegatedAction == delegatingRunnable){
    return IsActing(delegatingRunnable);
  }else if(_delegator->_runnableThatDelegatedHelper == delegatingRunnable){
    return !_delegator->_delegateHelperHandle.expired();
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsActing(const IBehavior* delegatingRunnable)
{
  if(_delegator->_runnableThatDelegatedAction == delegatingRunnable){
    return _delegator->_lastActionTag != ActionConstants::INVALID_TAG;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelDelegates(IBehavior* delegatingRunnable)
{
  if(_delegator->_runnableThatDelegatedAction != nullptr &&
     _delegator->_runnableThatDelegatedAction == delegatingRunnable ){
    bool ret = false;
    u32 tagToCancel = _delegator->_lastActionTag;
    if(_delegator->_runnableThatDelegatedAction == delegatingRunnable){
      _delegator->_runnableThatDelegatedAction = nullptr; // TEMP:  // TODO:(bn) redundant checks now
      ret = _delegator->_robot.GetActionList().Cancel(tagToCancel);
    }
    // note that the callback, if there was one (and it was allowed to run), should have already been called
    // at this point, so it's safe to clear the tag. Also, if the cancel itself failed, that is probably a
    // bug, but somehow the action is gone, so no sense keeping the tag around (and it clearly isn't
    // running). If the callback called DelegateIfInControl, we may have a new action tag, so only clear this if the
    // cancel didn't change it
    if( _delegator->_lastActionTag == tagToCancel ) {
      _delegator->_lastActionTag = ActionConstants::INVALID_TAG;
    }
    
  }else if(_delegator->_runnableThatDelegatedHelper != nullptr &&
           _delegator->_runnableThatDelegatedHelper == delegatingRunnable){
    _delegator->_runnableThatDelegatedHelper = nullptr;
    _delegator->_robot.GetAIComponent().GetBehaviorHelperComponent().StopHelperWithoutCallback(_delegator->_delegateHelperHandle.lock());
    _delegator->_delegateHelperHandle.reset();
  }
  
  if(USE_BSM){
    _bsm->CancelDelegates(delegatingRunnable);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelSelf(IBehavior* delegatingRunnable)
{
  if(USE_BSM){
    _bsm->CancelSelf(delegatingRunnable);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::HasDelegator(IBehavior* delegatingRunnable)
{
  if(USE_BSM){
    return _bsm->CanDelegate(delegatingRunnable);
  }else{
    return true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Delegator& DelegationComponent::GetDelegator(IBehavior* delegatingRunnable)
{
  if(USE_BSM){
    DEV_ASSERT(_bsm->CanDelegate(delegatingRunnable),
               "DelegationComponent.GetDelegator.DelegatingRunnableNotValid");
  }
  return *_delegator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::HandleActionComplete(u32 actionTag)
{
  _delegator->HandleActionComplete(actionTag);
}


} // namespace Cozmo
} // namespace Anki
