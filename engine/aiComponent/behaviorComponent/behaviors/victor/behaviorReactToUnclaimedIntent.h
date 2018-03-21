/**
 * File: behaviorReactToUnclaimedIntent.h
 *
 * Author: ross
 * Created: 2018 feb 21
 *
 * Description: WantsToActivate when the UserIntentComponent received an intent that nobody claimed
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUnclaimedIntent_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUnclaimedIntent_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BehaviorReactToUnclaimedIntent : public ICozmoBehavior
{
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToUnclaimedIntent( const Json::Value& config );


public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void GetBehaviorOperationModifiers( BehaviorOperationModifiers& modifiers ) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

protected:

  virtual void OnBehaviorActivated() override;
  
};

}
}

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorUnclaimedIntent_H__

