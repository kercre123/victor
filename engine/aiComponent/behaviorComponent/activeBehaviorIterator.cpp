/**
 * File: activeBehaviorIterator.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-05-09
 *
 * Description: Component which exposes ways to iterate over active behaviors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/activeBehaviorIterator.h"

#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "lib/util/source/anki/util/entityComponent/dependencyManagedEntity.h"
#include "util/helpers/boundedWhile.h"


namespace Anki {
namespace Vector {

ActiveBehaviorIterator::ActiveBehaviorIterator()
  : IDependencyManagedComponent( this, BCComponentID::ActiveBehaviorIterator )
{
}

void ActiveBehaviorIterator::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  _bsm = dependentComps.GetComponentPtr<BehaviorSystemManager>();
}

void ActiveBehaviorIterator::IterateActiveCozmoBehaviorsForward(CozmoBehaviorCallback operand,
                                                                const IBehavior* startingBehavior) const
{
  const IBehavior* curr = startingBehavior;

  if( nullptr == curr ) {
    // start at the base behavior by default
    curr = _bsm->GetBaseBehavior();
  }

  bool continueIterating = true;
  BOUNDED_WHILE(1000, (curr != nullptr) && (continueIterating) ) {
    const ICozmoBehavior* cozmoBehaviorPtr = dynamic_cast<const ICozmoBehavior*>( curr );
    if( ANKI_VERIFY(cozmoBehaviorPtr != nullptr, "ActiveBehaviorIterator.NonCozmoBehaviorOnStack",
                    "Behavior cannot be dynamic casted to a cozmo behavior")) {
      continueIterating = operand(*cozmoBehaviorPtr);
    }
    
    curr = _bsm->GetBehaviorDelegatedTo(curr);
  }
}

void ActiveBehaviorIterator::IterateActiveCozmoBehaviorsBackward(CozmoBehaviorCallback operand,
                                                                 const IBehavior* startingBehavior) const
{
  const IBehavior* curr = startingBehavior;

  if( nullptr == curr ) {
    // start at the top behavior by default
    curr = _bsm->GetTopBehavior();
  }

  bool continueIterating = true;
  BOUNDED_WHILE(1000, (curr != nullptr) && (continueIterating) ) {
    const ICozmoBehavior* cozmoBehaviorPtr = dynamic_cast<const ICozmoBehavior*>( curr );
    if( ANKI_VERIFY(cozmoBehaviorPtr != nullptr, "ActiveBehaviorIterator.NonCozmoBehaviorOnStack",
                    "Behavior cannot be dynamic casted to a cozmo behavior")) {
      continueIterating = operand(*cozmoBehaviorPtr);
    }
    
    curr = _bsm->GetBehaviorDelegatedFrom(curr);
  }
}

size_t ActiveBehaviorIterator::GetLastTickBehaviorStackChanged() const
{
  return _bsm->GetLastTickBehaviorStackChanged();
}

}
}
