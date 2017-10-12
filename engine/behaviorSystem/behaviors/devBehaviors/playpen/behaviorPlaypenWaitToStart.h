/**
 * File: behaviorPlaypenWaitToStart.h
 *
 * Author: Al Chaussee
 * Created: 10/12/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenWaitToStart : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenWaitToStart(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  TimeStamp_t touchStartTime_ms = 0;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenWaitToStart_H__
