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

class BehaviorPlaypenEndChecks : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenEndChecks(const Json::Value& config);
  
protected:
  virtual void InitBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  
  virtual void AlwaysHandle(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:

  bool _heardFromLightCube = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenEndChecks_H__
