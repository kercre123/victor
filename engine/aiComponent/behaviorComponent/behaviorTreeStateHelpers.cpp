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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AreBehaviorsActivatedHelper::AreBehaviorsActivatedHelper(const BehaviorContainer& bc, const std::set<BehaviorID>& behaviors)
{
  for(const auto& id: behaviors){
    _behaviorSet.push_back(bc.FindBehaviorByID(id));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AreBehaviorsActivatedHelper::AreBehaviorsActivated() const
{
  for(const auto& ptr: _behaviorSet){
    if(ptr->IsActivated()){
      return true;
    }
  }
  return false;
}


} // namespace Cozmo
} // namespace Anki
