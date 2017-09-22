/**
 * File: BehaviorEarnedSparks
 *
 * Author: Paul Terry
 * Created: 07/18/2017
 *
 * Description: Simple behavior for Cozmo playing an animaton when he has earned sparks
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorEarnedSparks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorEarnedSparks_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {

class BehaviorEarnedSparks : public IBehavior
{

private:

  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorEarnedSparks(const Json::Value& config);

public:

  virtual ~BehaviorEarnedSparks();

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true; }

protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

};


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEarnedSparks_H__
