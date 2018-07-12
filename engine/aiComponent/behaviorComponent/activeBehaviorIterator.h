/**
 * File: activeBehaviorIterator.h
 *
 * Author: Brad Neuman
 * Created: 2018-05-09
 *
 * Description: Component which exposes ways to iterate over active behaviors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_ActiveBehaviorIterator_H__
#define __Engine_AiComponent_BehaviorComponent_ActiveBehaviorIterator_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class BehaviorSystemManager;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class ActiveBehaviorIterator : public IDependencyManagedComponent<BCComponentID>
                             , public Anki::Util::noncopyable
{
public:
  ActiveBehaviorIterator();

  virtual void GetInitDependencies( BCCompIDSet& dependencies ) const override {
    dependencies.insert(BCComponentID::BehaviorSystemManager);    
  }
  
  virtual void InitDependent( Robot* robot, const BCCompMap& dependentComps ) override;

  // Operand function to be called on every iteration of the Iterate function below. Boolean return value
  // indicates to the iterator whether to continue iterating to the next behavior in the stack.
  using CozmoBehaviorCallback = std::function<bool(const ICozmoBehavior& behavior)>;

  // call the operand function on every ICozmoBehavior that is currently active, starting at the specified
  // behavior (defaults to the base behavior) and moving towards behaviors that were delegated to by that
  // behavior, ending with the behavior currently in control, or when the callback returns false.
  void IterateActiveCozmoBehaviorsForward(CozmoBehaviorCallback operand, const IBehavior* startingBehavior = nullptr) const;

  // call the operand function on every ICozmoBehavior that is currently active, starting at the specified
  // behavior (defaults to the top behavior) and moving towards behaviors that delegated to that behavior,
  // ending with the base behavior, or when the callback returns false.
  void IterateActiveCozmoBehaviorsBackward(CozmoBehaviorCallback operand, const IBehavior* startingBehavior = nullptr) const;
  
  // Return the last tick when the behavior stack was updated (i.e. new behavior delegated to or one was
  // canceled)
  size_t GetLastTickBehaviorStackChanged() const;
  
private:
  const BehaviorSystemManager* _bsm = nullptr;
  
};

}
}

#endif
