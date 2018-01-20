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

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"
#include "coretech/common/shared/types.h"

namespace Anki {
namespace Cozmo {

class PlaceRelObjectHelper : public IHelper{
public:
  PlaceRelObjectHelper(ICozmoBehavior& behavior,
                       BehaviorHelperFactory& helperFactory,
                       const ObjectID& targetID,
                       const bool placingOnGround = false,
                       const PlaceRelObjectParameters& parameters = {});
  virtual ~PlaceRelObjectHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates() const override;
  virtual HelperStatus InitBehaviorHelper() override;
  virtual HelperStatus UpdateWhileActiveInternal() override;
private:
  ObjectID _targetID;
  bool _placingOnGround;
  PlaceRelObjectParameters _params;
  u32 _tmpRetryCounter;
  
  void StartPlaceRelObject();
  void RespondToPlaceRelResult(ActionResult result);

  void MarkFailedToStackOrPlace();
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PlaceRelObjectHelper_H__

