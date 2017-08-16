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

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenEndChecks : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenEndChecks(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot) override;
  
private:

  bool _heardFromLightCube = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenEndChecks_H__
