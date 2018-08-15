/*
 * File: userDefinedBehaviorTreeComponent.cpp
 *
 * Author: Hamzah Khan
 * Created: 7/11/2018
 *
 * Description: This is a component meant to provide functionality that maintains a user-defined
 *              behavior tree. It keeps track of the conditions that can be given user-defined
 *              response behaviors and provides an interface to add/modify them.
 *
 * Copyright: Anki, Inc. 2018
 */

#include "engine/aiComponent/behaviorComponent/userDefinedBehaviorTreeComponent/userDefinedBehaviorTreeComponent.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#include "clad/types/variableSnapshotIds.h"


#ifndef __Cozmo_Basestation_UserDefinedBehaviorTreeComponent__
#define __Cozmo_Basestation_UserDefinedBehaviorTreeComponent__

namespace Anki {
namespace Vector {

CONSOLE_VAR(bool, kEnableUserDefinedBehaviorTree, "UserDefinedBehaviorTree", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UserDefinedBehaviorTreeComponent::UserDefinedBehaviorTreeComponent()
  : IDependencyManagedComponent<BCComponentID>(this, BCComponentID::UserDefinedBehaviorTreeComponent),
    _conditionMappingsPtr(std::make_shared<std::unordered_map<BEIConditionType, BehaviorID>>()),
    _possibleMappingsPtr(nullptr),
    _editModeCondition(BEIConditionType::Invalid),
    _behaviorContainer(nullptr),
    _currentCondition(BEIConditionType::Invalid)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
UserDefinedBehaviorTreeComponent::~UserDefinedBehaviorTreeComponent() 
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserDefinedBehaviorTreeComponent::InitDependent(Vector::Robot* robot, const BCCompMap& dependentComponents)
{ 
  _behaviorContainer = dependentComponents.GetComponentPtr<BehaviorContainer>();
  auto& bei = dependentComponents.GetComponent<BehaviorExternalInterface>();

  // get pointer to all possible mappings
  auto& dataAccessorComp = bei.GetDataAccessorComponent();
  _possibleMappingsPtr = dataAccessorComp.GetUserDefinedConditionToBehaviorsMap();

  // get edit behavior ID
  _editModeCondition = dataAccessorComp.GetUserDefinedEditCondition();

  // load customized mappings from memory
  auto& variableSnapshotComp = bei.GetVariableSnapshotComponent();
  variableSnapshotComp.InitVariable<std::unordered_map<BEIConditionType, BehaviorID>>(VariableSnapshotId::UserDefinedBehaviorTreeComp_CondToBehaviorMappings,
                                                                                      _conditionMappingsPtr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr UserDefinedBehaviorTreeComponent::GetDelegationBehavior(BEIConditionType beiCondType) const
{
  const auto mappingIter = _conditionMappingsPtr->find(beiCondType);
  const bool userDefinedBehaviorExists = (mappingIter != _conditionMappingsPtr->end());

  if(!userDefinedBehaviorExists) {
    return ICozmoBehaviorPtr{};
  }

  BehaviorID behaviorId = mappingIter->second;
  return _behaviorContainer->FindBehaviorByID(behaviorId);
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserDefinedBehaviorTreeComponent::AddCondition(BEIConditionType beiCondType, BehaviorID behaviorId)
{
  auto mappingIter = _conditionMappingsPtr->find(beiCondType);

  const bool willBeOverwritten = (mappingIter != _conditionMappingsPtr->end());
  if(willBeOverwritten) {
    mappingIter->second = behaviorId;
  }
  else {
    _conditionMappingsPtr->emplace(beiCondType, behaviorId);
  }

  return willBeOverwritten;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool UserDefinedBehaviorTreeComponent::IsEnabled() const
{
  return kEnableUserDefinedBehaviorTree;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserDefinedBehaviorTreeComponent::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for(const auto& condMapping : *_possibleMappingsPtr) {
    for(const auto& behaviorId : condMapping.second) {
      ICozmoBehaviorPtr behaviorPtr = _behaviorContainer->FindBehaviorByID(behaviorId);
      delegates.emplace(behaviorPtr.get());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserDefinedBehaviorTreeComponent::GetCustomizableConditions(std::vector<BEIConditionType>& conditions) const
{
  for(const auto& condMapping : *_possibleMappingsPtr) {
    BEIConditionType beiCond = condMapping.first;
    // if there are more than zero options for behaviors, add it to the list
    if(!condMapping.second.empty()) {
      conditions.emplace_back(beiCond);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void UserDefinedBehaviorTreeComponent::SetCurrentCondition(BEIConditionType beiCond)
{
  // ensure that set current condition only works if current condition is erased before being set
  if(!ANKI_VERIFY(_currentCondition == BEIConditionType::Invalid,
                  "UserDefinedBehaviorTreeComponent.SetCurrentCondition.ConditionHasAlreadyBeenSet",
                  "Condition must be erased before using a SetCurrentCondition.")) {
    return;
  }

  const bool isCondValid = _possibleMappingsPtr->find(beiCond) != _possibleMappingsPtr->end();
  if(isCondValid) {
    _currentCondition = beiCond;
  }
  else {
    PRINT_NAMED_ERROR("UserDefinedBehaviorTreeComponent.SetCurrentCondition.BEIConditionTypeNotCustomizable", 
                      "BEIConditionType %s not a customizable condition",
                      BEIConditionTypeToString(beiCond));
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::set<BehaviorID> UserDefinedBehaviorTreeComponent::GetConditionOptions(BEIConditionType beiCond) const
{
  const auto& mapIter = _possibleMappingsPtr->find(beiCond);
  const bool isCondValid = mapIter != _possibleMappingsPtr->end();
  if(!isCondValid) {
    PRINT_NAMED_ERROR("UserDefinedBehaviorTreeComponent.GetConditionOptions.BEIConditionTypeNotCustomizable", 
                      "BEIConditionType %s not a customizable condition",
                      BEIConditionTypeToString(beiCond));
  }

  return mapIter->second;
};

} // Cozmo
} // Anki

#endif
