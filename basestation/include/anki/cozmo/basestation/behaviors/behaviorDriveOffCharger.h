/**
 * File: behaviorDriveOffCharger.h
 *
 * Author: Molly Jameson
 * Created: 2016-05-19
 *
 * Description: Behavior to drive to the edge off a charger and deal with the firmware cliff stop
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__
#define __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__

#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

class BehaviorDriveOffCharger : public IBehavior
{
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDriveOffCharger(Robot& robot, const Json::Value& config);

public:

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}

  // true so that we can handle edge cases where the robot is on the charger and not on his treads
  virtual bool ShouldRunWhileOffTreads() const override { return true;}

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  enum class DebugState {
    DrivingForward,
  };
  
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using BaseClass = IBehavior;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void TransitionToDrivingForward(Robot& robot);
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  float _distToDrive_mm = 0.0f;

};

}
}


#endif // __Cozmo_Basestation_Behaviors_BehaviorDriveOffCharger_H__
