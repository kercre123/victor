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
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenInitChecks(const Json::Value& config);
  
protected:
  
  virtual Result OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__
