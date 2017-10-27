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

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenDriftCheck : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenDriftCheck(const Json::Value& config);
  
protected:
  virtual void InitBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual Result         OnBehaviorActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual BehaviorStatus InternalUpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void           OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }

  virtual void AlwaysHandle(const RobotToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  
  void TransitionToPlayingSound(BehaviorExternalInterface& behaviorExternalInterface);
  void CheckDrift(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Angle at the start of drift check
  Radians _startingRobotOrientation = 0;
  
  // Whether or not the sound animation has completed
  bool _soundComplete = false;
  
  // Whether or not the drift check is complete
  bool _driftCheckComplete = false;
  
  // Struct to hold imu temperature readings before and after drift check
  IMUTempDuration _imuTemp;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
