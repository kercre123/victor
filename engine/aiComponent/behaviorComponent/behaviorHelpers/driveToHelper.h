/**
 * File: driveToHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/21/17
 *
 * Description: Handles driving to objects and searching for them if they aren't
 * visually verified
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_DriveToHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_DriveToHelper_H__

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"
#include "engine/preActionPose.h"
#include "coretech/vision/engine/camera.h"

namespace Anki {
namespace Cozmo {

class DriveToHelper : public IHelper{
public:
  DriveToHelper(BehaviorExternalInterface& behaviorExternalInterface, ICozmoBehavior& behavior,
                BehaviorHelperFactory& helperFactory,
                const ObjectID& targetID,
                const DriveToParameters& params = {});
  virtual ~DriveToHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual HelperStatus InitBehaviorHelper(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual HelperStatus UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  ObjectID _targetID;
  DriveToParameters _params;
  
  u32 _tmpRetryCounter;
  Pose3d _initialRobotPose;

  void DriveToPreActionPose(BehaviorExternalInterface& behaviorExternalInterface);
  void RespondToDriveResult(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface);
    
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_DriveToHelper_H__

