/**
 * File: behaviorPlaypenEndChecks.h
 *
 * Author: Al Chaussee
 * Created: 08/14/17
 *
 * Description: Checks any final things playpen is interested in like battery voltage and that we have heard
 *              from an active object
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenEndChecks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenEndChecks_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {
namespace ExternalInterface {
  struct ObjectAvailable;
}

class BehaviorPlaypenEndChecks : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPlaypenEndChecks(const Json::Value& config);
  
protected:
  virtual void OnBehaviorEnteredActivatableScope() override;
  
  virtual Result OnBehaviorActivatedInternal()   override;
  virtual void   OnBehaviorDeactivated()   override;
  
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  virtual void AlwaysHandleInScope(const GameToEngineEvent& event) override;
    
private:

  bool _heardFromLightCube = false;
  bool _wifiScanPassed = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenEndChecks_H__
