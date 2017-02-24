/**
 * File: placeBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles placing a carried block at a given pose
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceBlockHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceBlockHelper_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"


namespace Anki {
namespace Cozmo {

class PlaceBlockHelper : public IHelper{
public:
  PlaceBlockHelper(Robot& robot, IBehavior& behavior, BehaviorHelperFactory& helperFactory);
  virtual ~PlaceBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot) override;
  
  void RespondToTurnAction(ActionResult result, Robot& robot);
  void RespondToPlacedAction(ActionResult result, Robot& robot);
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceBlockHelper_H__

