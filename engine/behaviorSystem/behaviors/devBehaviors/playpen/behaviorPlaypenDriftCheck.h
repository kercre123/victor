/**
 * File: behaviorPlaypenDriftCheck.h
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description: Checks head and lift motor range, speaker works, mics work, and imu drift is minimal
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenDriftCheck : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenDriftCheck(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  void TransitionToPlayingSound(Robot& robot);
  void CheckDrift(Robot& robot);
  
  // Angle at the start of drift check
  Radians _startingRobotOrientation = 0;
  
  // Whether or not the sound animation has completed
  bool _soundComplete = false;
  
  // Whether or not the drift check is complete
  bool _driftCheckComplete = false;
  
  IMUTempDuration _imuTemp;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
