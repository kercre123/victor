/**
 * File: BehaviorUserDefinedBehaviorTreeRouter.cpp
 *
 * Author: Hamzah Khan
 * Created: 2018-07-11
 *
 * Description: A behavior that uses the user-defined behavior component to delegate to the user-defined behavior, given a condition.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/userDefinedBehaviorTree/behaviorUserDefinedBehaviorTreeRouter.h"
#include <string>

#include "clad/types/behaviorComponent/beiConditionTypes.h"

namespace Anki {
namespace Vector {

namespace {
const BehaviorID selectorBehaviorId = BehaviorID::UserDefinedBehaviorSelector;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorTreeRouter::InstanceConfig::InstanceConfig()
  : hasSetUpCustomizableConditions(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorTreeRouter::DynamicVariables::DynamicVariables()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorTreeRouter::BehaviorUserDefinedBehaviorTreeRouter(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorTreeRouter::~BehaviorUserDefinedBehaviorTreeRouter()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::InitBehavior()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorUserDefinedBehaviorTreeRouter::WantsToBeActivatedBehavior() const
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  for( auto& condition : _iConfig.customizableConditions) {
    if (condition->AreConditionsMet(GetBEI())) {
      // do not activate unless user-defined behavior tree is activated in console
      return userBehaviorTreeComp.IsEnabled();
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  // add all possible delegates of the router
  userBehaviorTreeComp.GetAllDelegates(delegates);
  
  // add the selection behavior to the possible delegates
  auto& behaviorContainer = GetBehaviorComp<BehaviorContainer>();
  ICozmoBehaviorPtr selectorBehaviorPtr = behaviorContainer.FindBehaviorByID(BehaviorID::UserDefinedBehaviorSelector);
  if(nullptr != selectorBehaviorPtr) {
    delegates.emplace(selectorBehaviorPtr.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::OnBehaviorActivated() 
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  // do not enable unless user-defined behavior tree is activated in console
  if(!userBehaviorTreeComp.IsEnabled()) {
    CancelSelf();
    return;
  }

  bool isAtLeastOneConditionMet = false;

  for( auto& condition : _iConfig.customizableConditions) {
    // if condition is not met, ignore it and move on to the next one
    const bool isConditionMet = condition->AreConditionsMet(GetBEI());
    if(!isConditionMet) {
      continue;
    }
    isAtLeastOneConditionMet = isConditionMet || isAtLeastOneConditionMet;

    // if condition is met, get the current settings for the condition
    BEIConditionType beiCondType = condition->GetConditionType();
    userBehaviorTreeComp.SetCurrentCondition(beiCondType);
    auto behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(beiCondType);

    // if a custom behavior hasn't yet been set, use the selector behavior to set one
    if(nullptr == behaviorPtr.get()) {
      SetNewCustomBehaviorWithSelector();
    }
    else {
      UsePreviouslySetCustomBehavior(beiCondType);
    }
  }

  if(!isAtLeastOneConditionMet) {
    PRINT_NAMED_ERROR("BehaviorUserDefinedBehaviorTreeRouter.OnBehaviorActivated.NoConditionWantsToBeActivated",
                      "Selector behavior activated, but no condition is activated.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::OnReturnFromSelectorBehavior()
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  BEIConditionType currentBeiCondType = userBehaviorTreeComp.GetCurrentCondition();
  auto behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(currentBeiCondType);
  if(behaviorPtr->WantsToBeActivated()) {
    DelegateNow(behaviorPtr.get(), &BehaviorUserDefinedBehaviorTreeRouter::EraseCondition);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::EraseCondition()
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  // now that we are done with the condition, remove it from being current.
  userBehaviorTreeComp.EraseCurrentCondition();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::SetNewCustomBehaviorWithSelector() {
  auto& behaviorContainer = GetBehaviorComp<BehaviorContainer>();

  ICozmoBehaviorPtr selectorBehaviorPtr = behaviorContainer.FindBehaviorByID(selectorBehaviorId);
  if(selectorBehaviorPtr->WantsToBeActivated()) {
    DelegateNow(selectorBehaviorPtr.get(), &BehaviorUserDefinedBehaviorTreeRouter::OnReturnFromSelectorBehavior);
  }
  else {
    PRINT_NAMED_WARNING("BehaviorUserDefinedBehaviorTreeRouter.OnBehaviorActivated.SelectorDoesNotWantToBeActivated",
                        "Selector behavior did not want to be activated.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::UsePreviouslySetCustomBehavior(BEIConditionType beiCondType)
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  auto behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(beiCondType);
  const bool behaviorDoesNotWantToBeActivated = !behaviorPtr->WantsToBeActivated();

  if(behaviorDoesNotWantToBeActivated) {
    PRINT_NAMED_ERROR("BehaviorUserDefinedBehaviorTreeRouter.OnBehaviorActivated.BehaviorActivationReqsTooStrict",
                      "Behavior %s did not want to be activated. Check the conditions under which it is activated.", 
                      BehaviorIDToString(behaviorPtr->GetID()));
    return;
  }

  // otherwise, delegate to the behavior!
  DelegateNow(behaviorPtr.get(), &BehaviorUserDefinedBehaviorTreeRouter::EraseCondition);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::BehaviorUpdate() 
{
  // we can avoid a circular dependency of the user-defined behavior component on behavior container
  // by setting the customizable conditions on the first tick
  if(!_iConfig.hasSetUpCustomizableConditions) {
    auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

    std::vector<BEIConditionType> beiCondTypes;
    userBehaviorTreeComp.GetCustomizableConditions(beiCondTypes);

    // get list of allowed BEIConditionTypes and turn them into BEIConditions
    for(const auto& beiCondType : beiCondTypes) {
      auto condition = BEIConditionFactory::CreateBEICondition(beiCondType, GetDebugLabel());

      // initialize the triggers and get them ready to check for events
      condition->Init(GetBEI());
      condition->SetActive(GetBEI(), true);
      _iConfig.customizableConditions.emplace_back(condition);
    }

    _iConfig.hasSetUpCustomizableConditions = true;
  }
}

}
}
