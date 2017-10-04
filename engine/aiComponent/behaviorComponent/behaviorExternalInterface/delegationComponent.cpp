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
#include "engine/aiComponent/behaviorComponent/iBSRunnable.h"
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
bool Delegator::Delegate(IBSRunnable* delegatingRunnable,
                         IActionRunner* action,
                         BehaviorRobotCompletedActionCallback callback)
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
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.NotQueued",
                        "Behavior '%s' can't queue action '%s' (error %d)",
                        delegatingRunnable->GetPrintableID().c_str(),
                        action->GetName().c_str(), result);
    delete action;
    return false;
  }
  
  _runnableThatDelegatedAction = delegatingRunnable;
  _lastActionTag = action->GetTag();
  _actionCallback = callback;
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBSRunnable* delegatingRunnable, IBSRunnable* delegated)
{
  if(USE_BSM){
    return _bsm->Delegate(delegatingRunnable, delegated);
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBSRunnable* delegatingRunnable,
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

  
///////////////////////
///////////////////////
///////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelegationComponent::DelegationComponent(Robot& robot,
                                         BehaviorSystemManager& bsm)
: _delegator( new Delegator(robot, bsm))
, _bsm(&bsm)
{
  if(robot.HasExternalInterface()){
    IExternalInterface* externalInterface = robot.GetExternalInterface();
    using EngineToGameTag   = ExternalInterface::MessageEngineToGameTag;
    using EngineToGameEvent = AnkiEvent<ExternalInterface::MessageEngineToGame>;

    if( USE_BSM ) {
      _eventHandles.push_back(externalInterface->Subscribe(
                                EngineToGameTag::RobotCompletedAction,
                                [this](const EngineToGameEvent& event) {
                                  DEV_ASSERT(event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction,
                                             "IBehavior.RobotCompletedAction.WrongEventTypeFromCallback");
                                  HandleActionComplete(event.GetData().Get_RobotCompletedAction());
                                } ));
    }
  }
  {
    // Create a weak ptr with no strong references to return when delegation is locked
    _invalidDelegator = std::weak_ptr<Delegator>();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg)
{
  if(IsControlDelegated(_delegator->_runnableThatDelegatedAction) &&
     (msg.idTag == _delegator->_lastActionTag))
  {
    _delegator->_lastActionTag = ActionConstants::INVALID_TAG;
    _delegator->_runnableThatDelegatedAction = nullptr;

    if( _delegator->_actionCallback) {
      // save a copy of the message so we can call the callback before Update
      _delegator->_lastCompletedMsgCopy.reset( new ExternalInterface::RobotCompletedAction(msg) );
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::Update()
{
  if( USE_BSM ) {
    if( _delegator != nullptr &&
        _delegator->_lastCompletedMsgCopy != nullptr ) {
      // deferred call of action callback
      if( _delegator->_actionCallback ) {
        _delegator->_actionCallback( *_delegator->_lastCompletedMsgCopy );
        _delegator->_lastCompletedMsgCopy.reset();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsControlDelegated(const IBSRunnable* delegatingRunnable)
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
bool DelegationComponent::IsActing(const IBSRunnable* delegatingRunnable)
{
  if(_delegator->_runnableThatDelegatedAction == delegatingRunnable){
    return _delegator->_lastActionTag != ActionConstants::INVALID_TAG;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelDelegates(IBSRunnable* delegatingRunnable)
{
  if(_delegator->_runnableThatDelegatedAction != nullptr &&
     _delegator->_runnableThatDelegatedAction == delegatingRunnable ){
    bool ret = false;
    u32 tagToCancel = _delegator->_lastActionTag;
    if(_delegator->_runnableThatDelegatedAction == delegatingRunnable){
      _delegator->_runnableThatDelegatedAction = nullptr; // TEMP:  // TODO:(bn) redundant checks now
      _delegator->_actionCallback = nullptr;
      _delegator->_lastCompletedMsgCopy.reset();
      ret = _delegator->_robot.GetActionList().Cancel(tagToCancel);
    }
    // note that the callback, if there was one (and it was allowed to run), should have already been called
    // at this point, so it's safe to clear the tag. Also, if the cancel itself failed, that is probably a
    // bug, but somehow the action is gone, so no sense keeping the tag around (and it clearly isn't
    // running). If the callback called StartActing, we may have a new action tag, so only clear this if the
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
void DelegationComponent::CancelSelf(IBSRunnable* delegatingRunnable)
{
  if(USE_BSM){
    _bsm->CancelSelf(delegatingRunnable);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::weak_ptr<Delegator> DelegationComponent::GetDelegator(IBSRunnable* delegatingRunnable)
{
  if(USE_BSM){
    if(_bsm->CanDelegate(delegatingRunnable)){
      return _delegator;
    }else{
      return _invalidDelegator;
    }
  }else{
    return _delegator;
  }
}

} // namespace Cozmo
} // namespace Anki
