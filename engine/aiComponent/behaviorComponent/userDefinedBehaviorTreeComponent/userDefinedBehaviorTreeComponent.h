/*
 * File: userDefinedBehaviorTreeComponent.h
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

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"
#include "engine/components/variableSnapshot/variableSnapshotEncoder.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include <unordered_map>

#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/beiConditionTypes.h"


#ifndef __Cozmo_Basestation_UserDefinedBehaviorTreeComponent_H__
#define __Cozmo_Basestation_UserDefinedBehaviorTreeComponent_H__

namespace Anki {
namespace Cozmo {

class BehaviorContainer;
class BehaviorExternalInterface;
class VariableSnapshotComponent;

class UserDefinedBehaviorTreeComponent : public IDependencyManagedComponent<BCComponentID> {
public:
  explicit UserDefinedBehaviorTreeComponent();
  virtual ~UserDefinedBehaviorTreeComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const BCCompMap& dependentComponents) override final;
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {
    dependencies.insert(BCComponentID::BehaviorContainer);
    dependencies.insert(BCComponentID::BehaviorExternalInterface);
  };
  virtual void UpdateDependent(const BCCompMap& dependentComponents) override {};


  //////
  // API functions
  //////

  // GetDelegationBehavior returns a pointer to the behavior that should be delegated to given the
  // condition. If there is no such behavior, it returns a null ICozmoBehaviorPtr.
  ICozmoBehaviorPtr GetDelegationBehavior(BEIConditionType) const;

  // AddCondition adds a mapping from a confition to a behavior to the user-defined behavior tree.
  // It returns true if it overwrites a previous mapping. It overwrites values by default.
  // TODO: Change this function to accept a map class with a serialize function that can define more 
  //       types of dispatching
  bool AddCondition(BEIConditionType, BehaviorID);

private:
  // NOTE: Mapping to BehaviorIDs disallows the use of anonymous behaviors since there are no
  //       serialization functions for them.
  std::shared_ptr<std::unordered_map<BEIConditionType, BehaviorID>> _conditionMappingsPtr;

  // need to use BehaviorContainer, so store a pointer
  BehaviorContainer* _behaviorContainer;
};


} // Cozmo
} // Anki

#endif
