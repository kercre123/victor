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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
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
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorExpressNeeds(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  // don't resume, since it will run again anyway if it wants to
  virtual Result ResumeInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;

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

  AnimationTrigger _animTrigger;

  //////////
  // Members
  //////////

  float _lastTimeExpressed = 0.0f;

  ////////////
  // Functions
  ////////////
  
  float GetCooldownSec(const Robot& robot) const;
  
};


}
}

#endif
