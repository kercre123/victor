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

#include "clad/types/variableSnapshotIds.h"


#ifndef __Cozmo_Basestation_UserDefinedBehaviorTreeComponent__
#define __Cozmo_Basestation_UserDefinedBehaviorTreeComponent__

namespace Anki {
namespace Vector {


UserDefinedBehaviorTreeComponent::UserDefinedBehaviorTreeComponent()
  : IDependencyManagedComponent<BCComponentID>(this, BCComponentID::UserDefinedBehaviorTreeComponent),
    _conditionMappingsPtr(std::make_shared<std::unordered_map<BEIConditionType, BehaviorID>>()),
    _behaviorContainer(nullptr)
{
}

UserDefinedBehaviorTreeComponent::~UserDefinedBehaviorTreeComponent() 
{
}

void UserDefinedBehaviorTreeComponent::InitDependent(Vector::Robot* robot, const BCCompMap& dependentComponents)
{ 
  _behaviorContainer = dependentComponents.GetComponentPtr<BehaviorContainer>();
  auto& bei = dependentComponents.GetComponent<BehaviorExternalInterface>();
  auto& variableSnapshotComp = bei.GetVariableSnapshotComponent();
  variableSnapshotComp.InitVariable<std::unordered_map<BEIConditionType, BehaviorID>>(VariableSnapshotId::UserDefinedBehaviorTreeComp_CondToBehaviorMappings,
                                                                                      _conditionMappingsPtr);
}

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

} // Cozmo
} // Anki

#endif
