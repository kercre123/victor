/**
* File: placeRelObjectHelper.h
*
* Author: Kevin M. Karol
* Created: 2/21/17
*
* Description: Handles placing a carried block relative to another object
*
* Copyright: Anki, Inc. 2017
*
**/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceRelObjectHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceRelObjectHelper_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {

class PlaceRelObjectHelper : public IHelper{
public:
  PlaceRelObjectHelper(Robot& robot, IBehavior& behavior,
                       BehaviorHelperFactory& helperFactory,
                       const ObjectID& targetID,
                       const bool placingOnGround = false,
                       const PlaceRelObjectParameters& parameters = {});
  virtual ~PlaceRelObjectHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot) override;
private:
  ObjectID _targetID;
  bool _placingOnGround;
  PlaceRelObjectParameters _params;
  u32 _tmpRetryCounter;
  
  void StartPlaceRelObject(Robot& robot);
  void RespondToPlaceRelResult(ActionResult result, Robot& robot);

};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceRelObjectHelper_H__

