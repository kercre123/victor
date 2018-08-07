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
#include "util/console/consoleInterface.h"
#include <string>

#include "clad/types/behaviorComponent/beiConditionTypes.h"

namespace Anki {
namespace Vector {

namespace {
const char* kAllowedBEIConditions = "allowedBEIConditions";
}

CONSOLE_VAR(bool, kEnableUserDefinedBehaviorTree, "UserDefinedBehaviorTreeComponent", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorTreeRouter::InstanceConfig::InstanceConfig()
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
  std::vector<std::string> beiConds;
  JsonTools::GetVectorOptional(config, kAllowedBEIConditions, beiConds);

  // get list of allowed BEIConditions
  for(const auto& beiCondString : beiConds) {
    // turn them into BEIConditionTypes
    BEIConditionType beiCondType = BEIConditionType::Invalid;
    const bool beiCondLoaded = BEIConditionTypeFromString(beiCondString, beiCondType);
    if( !beiCondLoaded ) {
      PRINT_NAMED_WARNING("UserDefinedBehaviorTreeComponent.BehaviorUserDefinedBehaviorTreeRouter.UnknownBEIConditionType",
                          "%s was not recognized as a valid BEIConditionType value, will be dropped", beiCondString.c_str());
    }

    _iConfig.customizableConditions.emplace(
      BEIConditionFactory::CreateBEICondition(beiCondType, GetDebugLabel())
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorUserDefinedBehaviorTreeRouter::~BehaviorUserDefinedBehaviorTreeRouter()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::InitBehavior()
{
  // initialize the triggers and get them ready to check for events
  for(const auto& condition: _iConfig.customizableConditions) {
    condition->Init(GetBEI());
    condition->SetActive(GetBEI(), true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorUserDefinedBehaviorTreeRouter::WantsToBeActivatedBehavior() const
{
  for( auto& condition : _iConfig.customizableConditions) {
    if (condition->AreConditionsMet(GetBEI())) {
      // do not activate unless enabled by developer
      return kEnableUserDefinedBehaviorTree;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();

  // add all possible delegates of the router
  for( auto& condition : _iConfig.customizableConditions) {
    auto behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(condition->GetConditionType());
    if(nullptr != behaviorPtr.get()) {
      delegates.emplace(behaviorPtr.get());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAllowedBEIConditions
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorUserDefinedBehaviorTreeRouter::OnBehaviorActivated() 
{
  // do not enable unless enabled by developer
  if(!kEnableUserDefinedBehaviorTree) {
    CancelSelf();
  }

  auto& userBehaviorTreeComp = GetBehaviorComp<UserDefinedBehaviorTreeComponent>();
  for( auto& condition : _iConfig.customizableConditions) {
    if(condition->AreConditionsMet(GetBEI())) {
      auto behaviorPtr = userBehaviorTreeComp.GetDelegationBehavior(condition->GetConditionType());
      if(nullptr != behaviorPtr.get()) {
        if(behaviorPtr->WantsToBeActivated()) {
          DelegateNow(behaviorPtr.get());
        }
      }
    }
  }
}

}
}
