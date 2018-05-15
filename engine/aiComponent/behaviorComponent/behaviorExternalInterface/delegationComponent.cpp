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
#include "engine/aiComponent/continuityComponent.h"
#include "engine/externalInterface/externalInterface.h"

#include "coretech/common/engine/utils/timer.h"

#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
namespace{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Delegator::Delegator(BehaviorSystemManager& bsm,
                     ContinuityComponent& continuityComp)
: _behaviorThatDelegatedAction(nullptr)
, _lastActionTag(ActionConstants::INVALID_TAG)
, _bsm(bsm)
, _continuityComp(continuityComp)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Delegator::Delegate(IBehavior* delegatingBehavior,
                         IActionRunner* action)
{  
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
  const bool delegatedOK = _bsm.Delegate(delegatingBehavior, delegated);
  return delegatedOK;
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
: IDependencyManagedComponent(this, BCComponentID::DelegationComponent)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::InitDependent(Robot* robot, const BCCompMap& dependentComponents)
{
  auto* bsm = dependentComponents.GetBasePtr<BehaviorSystemManager>();
  Init(bsm, dependentComponents.GetValue<AIComponent>().GetBasePtr<ContinuityComponent>());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::Init(BehaviorSystemManager* bsm, ContinuityComponent* continuityComp)
{
  _delegator = std::make_unique<Delegator>(*bsm, *continuityComp);
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
  
  return _delegator->_lastActionTag != ActionConstants::INVALID_TAG;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelegationComponent::CancelDelegates(IBehavior* delegatingBehavior)
{
  CancelActionIfRunning(delegatingBehavior);  
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
  return _bsm->CanDelegate(delegatingBehavior) && !IsControlDelegated(delegatingBehavior);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Delegator& DelegationComponent::GetDelegator(IBehavior* delegatingBehavior)
{
  DEV_ASSERT(HasDelegator(delegatingBehavior),
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t DelegationComponent::GetLastTickBehaviorStackChanged() const
{
  return _bsm->GetLastTickBehaviorStackChanged();
}

} // namespace Cozmo
} // namespace Anki
