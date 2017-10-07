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
  PlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface, ICozmoBehavior& behavior, BehaviorHelperFactory& helperFactory);
  virtual ~PlaceBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual BehaviorStatus Init(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  void RespondToTurnAction(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface);
  void RespondToPlacedAction(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface);
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceBlockHelper_H__

