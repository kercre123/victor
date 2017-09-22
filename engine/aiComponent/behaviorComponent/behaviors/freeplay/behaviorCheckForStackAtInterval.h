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

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "anki/common/basestation/objectIDs.h"

#include <vector>

namespace Anki {
namespace Cozmo {


class BehaviorCheckForStackAtInterval : public IBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorCheckForStackAtInterval(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
  
private:
  // loaded in from JSON
  float _delayBetweenChecks_s;
  
  float _nextCheckTime_s;
  Pose3d _initialRobotPose;
  mutable std::vector<ObjectID> _knownBlockIDs;
  int _knownBlockIndex;
  
  std::unique_ptr<ObservableObject> _ghostStackedObject;


  virtual void UpdateTargetBlocks(const BehaviorExternalInterface& behaviorExternalInterface) const;
  
  void TransitionToSetup(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToFacingBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToCheckingAboveBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToReturnToSearch(BehaviorExternalInterface& behaviorExternalInterface);

  // find objectID in the given index. Returns unset ID if the index is not valid
  ObjectID GetKnownObjectID(int index) const;
  
  // find current known object by it's cached id
  const ObservableObject* GetKnownObject(const BehaviorExternalInterface& behaviorExternalInterface, int index) const;
  
};


}
}


#endif
