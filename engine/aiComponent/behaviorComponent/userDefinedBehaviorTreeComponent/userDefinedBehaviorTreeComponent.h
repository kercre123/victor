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
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/variableSnapshot/variableSnapshotEncoder.h"
#include "util/entityComponent/iDependencyManagedComponent.h"

#include <unordered_map>

#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/beiConditionTypes.h"


#ifndef __Cozmo_Basestation_UserDefinedBehaviorTreeComponent_H__
#define __Cozmo_Basestation_UserDefinedBehaviorTreeComponent_H__

namespace Anki {
namespace Vector {

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
  virtual void InitDependent(Vector::Robot* robot, const BCCompMap& dependentComponents) override final;
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

  // returns true if the component is enabled (if the feature is enabled from the console)
  bool IsEnabled() const;

  // returns BEIConditionType of edit condition
  BEIConditionType GetEditModeCondition() const { return _editModeCondition; };

  // get all possible delegates - fills the passed in set with the possible delegates
  void GetAllDelegates(std::set<IBehavior*>&) const;

  // fills the vector with all of the conditions that have options for customization
  void GetCustomizableConditions(std::vector<BEIConditionType>&) const;

  RobotDataLoader::ConditionToBehaviorsMap* GetConditionToBehaviorsMap() const { return _possibleMappingsPtr; }

  // set and get the currently activated condition in the router - if an invalid condition is set, an error witll be thrown
  // NOTE: these functions assume that only the router/selector will be setting/getting this info
  void SetCurrentCondition(BEIConditionType);
  BEIConditionType GetCurrentCondition() const { assert(BEIConditionType::Invalid != _currentCondition); return _currentCondition; };

  // gets the possible behaviors of a given condition
  std::set<BehaviorID> GetConditionOptions(BEIConditionType) const;

  // reset current condition to invalid condition
  void EraseCurrentCondition() { _currentCondition = BEIConditionType::Invalid; };

private:
  // NOTE: Mapping to BehaviorIDs disallows the use of anonymous behaviors since there are no
  //       serialization functions for them.
  std::shared_ptr<std::unordered_map<BEIConditionType, BehaviorID>> _conditionMappingsPtr;
  RobotDataLoader::ConditionToBehaviorsMap* _possibleMappingsPtr;

  // edit mode condition - when triggered within some set of conditions, used to enter an 
  // "edit" mode for the user-defined behavior tree feature
  BEIConditionType _editModeCondition;

  // need to use BehaviorContainer, so store a pointer
  BehaviorContainer* _behaviorContainer;

  // used for storing the currently activated condition
  BEIConditionType _currentCondition;
};


} // Cozmo
} // Anki

#endif
