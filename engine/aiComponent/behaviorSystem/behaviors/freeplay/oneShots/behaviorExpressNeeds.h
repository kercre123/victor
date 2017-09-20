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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/needsSystemTypes.h"

#include <memory>

namespace Anki {

namespace Util {
class GraphEvaluator2d;
}

namespace Cozmo {

class Robot;

class BehaviorExpressNeeds : public IBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorExpressNeeds(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  // don't resume, since it will run again anyway if it wants to
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual bool CarryingObjectHandledInternally() const override { return false; }

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


  //////////
  // Members
  //////////

  float _lastTimeExpressed = 0.0f;

  ////////////
  // Functions
  ////////////
  
  float GetCooldownSec(BehaviorExternalInterface& behaviorExternalInterface) const;
  
};


}
}

#endif
