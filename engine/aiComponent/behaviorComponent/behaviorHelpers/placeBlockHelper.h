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

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"


namespace Anki {
namespace Cozmo {

class PlaceBlockHelper : public IHelper{
public:
  PlaceBlockHelper(ICozmoBehavior& behavior, BehaviorHelperFactory& helperFactory);
  virtual ~PlaceBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates() const override;
  virtual HelperStatus InitBehaviorHelper() override;
  virtual HelperStatus UpdateWhileActiveInternal() override;
  
  void RespondToTurnAction(ActionResult result);
  void RespondToPlacedAction(ActionResult result);
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceBlockHelper_H__

