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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
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

  virtual bool IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  
private:
  // loaded in from JSON
  float _delayBetweenChecks_s;
  
  float _nextCheckTime_s;
  Pose3d _initialRobotPose;
  mutable std::vector<ObjectID> _knownBlockIDs;
  int _knownBlockIndex;
  
  std::unique_ptr<ObservableObject> _ghostStackedObject;


  virtual void UpdateTargetBlocks(const Robot& robot) const;
  
  void TransitionToSetup(Robot& robot);
  void TransitionToFacingBlock(Robot& robot);
  void TransitionToCheckingAboveBlock(Robot& robot);
  void TransitionToReturnToSearch(Robot& robot);

  // find objectID in the given index. Returns unset ID if the index is not valid
  ObjectID GetKnownObjectID(int index) const;
  
  // find current known object by it's cached id
  const ObservableObject* GetKnownObject(const Robot& robot, int index) const;
  
};


}
}


#endif
