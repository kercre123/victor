/**
 * File: behaviorExpressNeeds.h
 *
 * Author: Brad Neuman
 * Created: 2017-06-08
 *
 * Description: Play a one-shot animation to express needs states with a built-in cooldown based on needs
 *              levels
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_Behaviors_Freeplay_OneShots_BehaviorExpressNeeds_H__
#define __Cozmo_Basestation_BehaviorSystem_Behaviors_Freeplay_OneShots_BehaviorExpressNeeds_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/needsSystemTypes.h"

#include <memory>

namespace Anki {

namespace Util {
class GraphEvaluator2d;
}

namespace Cozmo {

class BehaviorExpressNeeds : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorExpressNeeds(const Json::Value& config);

  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual bool CarryingObjectHandledInternally() const override { return false; }

  virtual bool ShouldRunWhileOnCharger() const override { return _supportCharger; }

private:

  /////////////
  // Parameters
  /////////////
  
  NeedId _need;
  NeedBracketId _requiredBracket;

  // x = need level
  // y = cooldown in seconds
  std::unique_ptr<Util::GraphEvaluator2d> _cooldownEvaluator;

  std::vector<AnimationTrigger> _animTriggers;
  
  bool _shouldClearExpressedState;
  bool _caresAboutExpressedState;

  // defaults to false, but if set true, this will allow the behavior to work while the robot is sitting on
  // the charger. It will lock out the body track to avoid coming off the charger (if we're on one)
  bool _supportCharger;

  //////////
  // Members
  //////////

  float _lastTimeExpressed = 0.0f;

  ////////////
  // Functions
  ////////////
  
  float GetCooldownSec(BehaviorExternalInterface& behaviorExternalInterface) const;

  // internal helper to properly handle locking extra tracks if needed
  // TODO:(bn) this is code duplication from BehaviorPlayAnimSequence. See if we can combine
  u8 GetTracksToLock(BehaviorExternalInterface& behaviorExternalInterface) const; 
  
  
};


}
}

#endif
