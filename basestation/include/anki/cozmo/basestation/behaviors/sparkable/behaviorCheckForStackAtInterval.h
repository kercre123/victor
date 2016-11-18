/**
 * File: BehaviorCheckForStackAtInterval.h
 *
 * Author: Kevin M. Karol
 * Created: 2016-16-11
 *
 * Description: Check all known cubes at a set interval to see
 * if blocks have been stacked on top of them
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorCheckForStackAtInterval_H__
#define __Cozmo_Basestation_Behaviors_BehaviorCheckForStackAtInterval_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/objectIDs.h"

#include <vector>

namespace Anki {
namespace Cozmo {


class BehaviorCheckForStackAtInterval : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorCheckForStackAtInterval(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  
private:
  // loaded in from JSON
  float _delayBetweenChecks_s;
  
  float _nextCheckTime_s;
  Pose3d _initialRobotPose;
  mutable std::vector<const ObservableObject*> _knownBlocks;
  int _knownBlockIndex;
  

  virtual void UpdateTargetBlocks(const Robot& robot) const;
  
  void TransitionToSetup(Robot& robot);
  void TransitionToFacingBlock(Robot& robot);
  void TransitionToCheckingAboveBlock(Robot& robot);
  void TransitionToReturnToSearch(Robot& robot);

  std::unique_ptr<ObservableObject> _ghostStackedObject;
  
};


}
}


#endif
