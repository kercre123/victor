/**
 * File: behaviorPlaypenWaitToStart.h
 *
 * Author: Al Chaussee
 * Created: 10/12/17
 *
 * Description: Runs forever until the robot is on the charger and has been touched for some amount of time
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenWaitToStart : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenWaitToStart(const Json::Value& config);
  
protected:
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual BehaviorStatus PlaypenUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)           override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  TimeStamp_t _touchStartTime_ms = 0;

};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
