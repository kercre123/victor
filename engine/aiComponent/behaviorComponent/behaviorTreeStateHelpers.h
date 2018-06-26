/**
* File: behaviorTreeStateHelpers.h
*
* Author: Kevin M. Karol
* Created: 6/25/18
*
* Description: Functions that provide insight into the state of the behavior tree
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BehaviorComponent_BehaviorTreeStateHelpers_H__
#define __Engine_AiComponent_BehaviorComponent_BehaviorTreeStateHelpers_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// forward declaration
class BehaviorContainer;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class AreBehaviorsActivatedHelper
{
public:
  AreBehaviorsActivatedHelper(const BehaviorContainer& bc, const std::set<BehaviorID>& behaviors);

  bool AreBehaviorsActivated() const;

protected:
  std::vector<ICozmoBehaviorPtr> _behaviorSet;


};

} // end namespace Cozmo
} // end namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_BehaviorTreeStateHelpers_H__
