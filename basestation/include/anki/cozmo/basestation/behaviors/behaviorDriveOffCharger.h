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
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {

class BehaviorDriveOffCharger : public IBehavior
{
protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorDriveOffCharger(Robot& robot, const Json::Value& config);

public:

  virtual bool IsRunnableInternal(const Robot& robot) const override;

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual Result ResumeInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  // Only start this behavior if we're on the charge contacts
  virtual float EvaluateScoreInternal(const Robot& robot) const override;

private:

  enum class State {
    DrivingForward,
  };

  State _state = State::DrivingForward;

  std::string _startDrivingAnimGroup = "ag_launch_startDriving";
  std::string _drivingLoopAnimGroup = "ag_launch_drivingLoop";
  std::string _stopDrivingAnimGroup = "ag_launch_endDriving";

  void TransitionToDrivingForward(Robot& robot);
  
  void SetState_internal(State state, const std::string& stateName);
};

}
}


#endif
