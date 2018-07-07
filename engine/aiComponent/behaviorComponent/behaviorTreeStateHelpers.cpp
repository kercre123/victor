/**
* File: behaviorTreeStateHelpers.cpp
*
* Author: Kevin M. Karol
* Created: 6/25/18
*
* Description: Functions that provide insight into the state of the behavior tree
*
* Copyright: Anki, Inc. 2018
*
**/

#include "engine/aiComponent/behaviorComponent/behaviorTreeStateHelpers.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AreBehaviorsActivatedHelper::AreBehaviorsActivatedHelper(const BehaviorContainer& bc, const std::set<BehaviorID>& behaviorIDs)
{
  for(const auto& id: behaviorIDs){
    AddBehavior(bc, id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AreBehaviorsActivatedHelper::AreBehaviorsActivatedHelper(const BehaviorContainer& bc, const std::set<BehaviorClass>& classes)
{
  for(const auto& classID: classes){
    AddBehavior(bc, classID);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AreBehaviorsActivatedHelper::AddBehavior(const BehaviorContainer& bc, BehaviorID behaviorID)
{
  _behaviorSet.insert(bc.FindBehaviorByID(behaviorID));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AreBehaviorsActivatedHelper::AddBehavior(const BehaviorContainer& bc, BehaviorClass behaviorClass)
{
  const std::set<ICozmoBehaviorPtr> classMatches = bc.FindBehaviorsByClass(behaviorClass);
  _behaviorSet.insert(classMatches.begin(), classMatches.end());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AreBehaviorsActivatedHelper::AddBehavior(ICozmoBehaviorPtr behavior)
{
  _behaviorSet.insert(behavior);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AreBehaviorsActivatedHelper::AreBehaviorsActivated() const
{
  DEV_ASSERT(!_behaviorSet.empty(), "AreBehaviorsActivatedHelper.AreBehaviorsActivated.NoBehaviors");
  for(const auto& ptr: _behaviorSet){
    if(ptr->IsActivated()){
      return true;
    }
  }
  return false;
}


} // namespace Cozmo
} // namespace Anki
