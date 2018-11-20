/**
 * File: behaviorReactToHand.h
 *
 * Author:  Andrew Stein
 * Created: 2018-11-14
 *
 * Description: Animation sequence with drive-to based on prox distance for reacting to a hand.
 *              Note that this is JUST the reaction sequence: it does not check for a hand and
 *              always wants to be activated.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Vector_Behaviors_Reactions_ReactToHand_H__
#define __Vector_Behaviors_Reactions_ReactToHand_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {

class BehaviorReactToHand : public ICozmoBehavior
{
public:
  ~BehaviorReactToHand();
  
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToHand(const Json::Value& config);
  
  virtual void OnBehaviorActivated() override;
  
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override { }
  
  virtual bool WantsToBeActivatedBehavior() const override { return true; }
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override { }
  
  bool IsRobotLifted() const;
  
  void TransitionToLifting();
  void TransitionToReaction();
  void TransitionToGetOut();
  
  Radians _pitchAngleStart_rad = 0.f;
};

}
}

#endif /* __Vector_Behaviors_Reactions_ReactToHand_H__ */
