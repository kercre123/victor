/**
 * File: behaviorPlaypenInitChecks.h
 *
 * Author: Al Chaussee
 * Created: 08/09/17
 *
 * Description: Quick check of initial robot state for playpen. Checks things like firmware version,
 *              battery voltage, cliff sensors, etc
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenInitChecks : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenInitChecks(const Json::Value& config);
  
protected:
  virtual void GetBehaviorOperationModifiersInternal(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }  
  virtual Result OnBehaviorActivatedInternal() override;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__
