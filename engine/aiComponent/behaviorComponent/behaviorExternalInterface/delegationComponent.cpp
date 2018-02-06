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
#include "engine/aiComponent/continuityComponent.h"
#include "engine/externalInterface/externalInterface.h"

#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
namespace{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Delegator::Delegator(Robot& robot,
                     BehaviorSystemManager& bsm,
                     ContinuityComponent& continuityComp)
: _robot(robot)
, _behaviorThatDelegatedAction(nullptr)
, _lastActionTag(ActionConstants::INVALID_TAG)
, _behaviorThatDelegatedHelper(nullptr)
, _bsm(bsm)
, _continuityComp(continuityComp)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingBehavior,
                         IActionRunner* action)
{
  EnsureHandleIsUpdated();

  DEV_ASSERT_MSG(((_behaviorThatDelegatedHelper == nullptr) ||
                        (_behaviorThatDelegatedHelper == delegatingBehavior)) &&
                 (_behaviorThatDelegatedAction == nullptr),
                 "Delegator.DelegateAction.BehaviorAlreadySet",
                 "Queued Helper address %p, Queued Action address: %p",
                 _behaviorThatDelegatedHelper, _behaviorThatDelegatedAction);
  
  const bool getInSuccess = _continuityComp.GetIntoAction(action);
  if (getInSuccess) {
    _behaviorThatDelegatedAction = delegatingBehavior;
    _lastActionTag = action->GetTag();
  }else{
    PRINT_NAMED_WARNING("ICozmoBehavior.Delegate.Failure.ActionNotSet",
                        "Behavior '%s' failed to set action '%s'",
                        delegatingBehavior->GetDebugLabel().c_str(),
                        action->GetName().c_str());
  }
  
  return getInSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingBehavior, IBehavior* delegated)
{
  DEV_ASSERT(dynamic_cast<IHelper*>(delegated) == nullptr,
              "Delegator.Delegate.WrongDelegationFunction.UseIHelperFunction");
  return _bsm.Delegate(delegatingBehavior, delegated);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingBehavior,
                         HelperHandle helper,
                         BehaviorSimpleCallback successCallback,
                         BehaviorSimpleCallback failureCallback)
{
  EnsureHandleIsUpdated();
  
  DEV_ASSERT_MSG((_behaviorThatDelegatedHelper == nullptr) &&
                 (_behaviorThatDelegatedAction == nullptr),
                 "Delegator.DelegateHelper.BehaviorAlreadySet",
                 "Queued Helper address %p, Queued Action address: %p",
                 _behaviorThatDelegatedHelper, _behaviorThatDelegatedAction);
  _behaviorThatDelegatedHelper = delegatingBehavior;
  _delegateHelperHandle = helper;
  return _robot.GetAIComponent().GetBehaviorHelperComponent().
                     DelegateToHelper(helper, successCallback, failureCallback);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Delegator::EnsureHandleIsUpdated()
{
  // If the helper completes successfully we don't get notified
  // so clear the _behavior to avoid the DEV_ASSERTS
  if(_delegateHelperHandle.expired()){
    _behaviorThatDelegatedHelper = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Delegator::HandleActionComplete(u32 actionTag)
{
  if( actionTag == _lastActionTag ) {
    _lastActionTag = ActionConstants::INVALID_TAG;
    _behaviorThatDelegatedAction = nullptr;
  }
}

  
///////////////////////
///////////////////////
///////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelegationComponent::DelegationComponent()
: IDependencyManagedComponent(BCComponentID::DelegationComponent)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::InitDependent(Robot* robot, const BCCompMap& dependentComponents)
{
  auto& bsm = dependentComponents.find(BCComponentID::BehaviorSystemManager)->second.GetValue<BehaviorSystemManager>();
  Init(*robot, &bsm, &robot->GetAIComponent().GetComponent<ContinuityComponent>(AIComponentID::ContinuityComponent));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::Init(Robot& robot, BehaviorSystemManager* bsm, ContinuityComponent* continuityComp)
{
  _delegator = std::make_unique<Delegator>(robot, *bsm, *continuityComp);
  _bsm = bsm;
  _continuityComp = continuityComp;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsControlDelegated(const IBehavior* delegatingBehavior)
{
  if( delegatingBehavior == nullptr ) {
    return false;
  }
  
  if(_bsm->IsControlDelegated(delegatingBehavior)){
    return true;
  }
  
  if(_delegator->_behaviorThatDelegatedAction == delegatingBehavior){
    return IsActing(delegatingBehavior);
  }else if(_delegator->_behaviorThatDelegatedHelper == delegatingBehavior){
    return !_delegator->_delegateHelperHandle.expired();
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::IsActing(const IBehavior* delegatingBehavior)
{
  if(_delegator->_behaviorThatDelegatedAction == delegatingBehavior){
    return _delegator->_lastActionTag != ActionConstants::INVALID_TAG;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelDelegates(IBehavior* delegatingBehavior)
{
  CancelActionIfRunning(delegatingBehavior);
  
  if(_delegator->_behaviorThatDelegatedHelper != nullptr &&
           _delegator->_behaviorThatDelegatedHelper == delegatingBehavior){
    _delegator->_behaviorThatDelegatedHelper = nullptr;
    _delegator->_robot.GetAIComponent().GetBehaviorHelperComponent().StopHelperWithoutCallback(_delegator->_delegateHelperHandle.lock());
    _delegator->_delegateHelperHandle.reset();
  }
  
  _bsm->CancelDelegates(delegatingBehavior);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelActionIfRunning(IBehavior* delegatingBehavior)
{
  if(_delegator->_behaviorThatDelegatedAction != nullptr &&
     (delegatingBehavior == nullptr ||
     _delegator->_behaviorThatDelegatedAction == delegatingBehavior) ){
    bool ret = false;
    u32 tagToCancel = _delegator->_lastActionTag;
    if(_delegator->_behaviorThatDelegatedAction == delegatingBehavior ||
       delegatingBehavior == nullptr){
      _delegator->_behaviorThatDelegatedAction = nullptr; // TEMP:  // TODO:(bn) redundant checks now
      ret = _continuityComp->GetOutOfAction(tagToCancel);
    }
    // note that the callback, if there was one (and it was allowed to run), should have already been called
    // at this point, so it's safe to clear the tag. Also, if the cancel itself failed, that is probably a
    // bug, but somehow the action is gone, so no sense keeping the tag around (and it clearly isn't
    // running). If the callback called DelegateIfInControl, we may have a new action tag, so only clear this if the
    // cancel didn't change it
    if( _delegator->_lastActionTag == tagToCancel ) {
      _delegator->_lastActionTag = ActionConstants::INVALID_TAG;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelSelf(IBehavior* delegatingBehavior)
{
  _bsm->CancelSelf(delegatingBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelegationComponent::HasDelegator(IBehavior* delegatingBehavior)
{
  return _bsm->CanDelegate(delegatingBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Delegator& DelegationComponent::GetDelegator(IBehavior* delegatingBehavior)
{
  DEV_ASSERT(_bsm->CanDelegate(delegatingBehavior),
              "DelegationComponent.GetDelegator.delegatingBehaviorNotValid");
  return *_delegator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::HandleActionComplete(u32 actionTag)
{
  _delegator->HandleActionComplete(actionTag);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const IBehavior* DelegationComponent::GetBehaviorDelegatedTo(const IBehavior* delegatingBehavior) const
{
  return _bsm->GetBehaviorDelegatedTo(delegatingBehavior);
}

} // namespace Cozmo
} // namespace Anki
