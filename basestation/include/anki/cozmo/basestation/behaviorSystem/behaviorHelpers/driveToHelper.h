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

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/cozmo/basestation/preActionPose.h"

namespace Anki {
namespace Cozmo {
  


class DriveToHelper : public IHelper{
public:
  DriveToHelper(Robot& robot, IBehavior& behavior,
                BehaviorHelperFactory& helperFactory,
                const ObjectID& targetID,
                const DriveToParameters& params = {});
  virtual ~DriveToHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot) override;
  
private:
  ObjectID _targetID;
  DriveToParameters _params;
  
  u32 _searchLevel;
  TimeStamp_t _lastSearchRun_ts;
  u32 _tmpRetryCounter;
  
  void DriveToPreActionPose(Robot& robot);
  void RespondToDriveResult(ActionResult result, Robot& robot);
  void SearchForBlock(ActionResult result, Robot& robot);
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_DriveToHelper_H__

