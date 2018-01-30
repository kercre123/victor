/**
 * File: behaviorHelperFactory.h
 *
 * Author: Kevin M. Karol
 * Created: 2/13/17
 *
 * Description: Factory for creating behaviorHelpers
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperFactory_H__

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/helperHandle.h"

#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperParameters.h"
#include "coretech/common/engine/objectIDs.h"

namespace Anki {
namespace Cozmo {

class BehaviorHelperComponent;
class ICozmoBehavior;

class BehaviorHelperFactory{
public:
  BehaviorHelperFactory(BehaviorHelperComponent& component);
  
  HelperHandle CreateDriveToHelper(ICozmoBehavior& behavior,
                                   const ObjectID& targetID,
                                   const DriveToParameters& parameters = {});
  
  HelperHandle CreatePickupBlockHelper(ICozmoBehavior& behavior,
                                       const ObjectID& targetID,
                                       const PickupBlockParamaters& parameters = {});
  
  HelperHandle CreatePlaceBlockHelper(ICozmoBehavior& behavior);
  
  HelperHandle CreatePlaceRelObjectHelper(ICozmoBehavior& behavior,
                                          const ObjectID& targetID,
                                          const bool placingOnGround = false,
                                          const PlaceRelObjectParameters& parameters = {});
  
  HelperHandle CreateRollBlockHelper(ICozmoBehavior& behavior,
                                     const ObjectID& targetID,
                                     bool rollToUpright = true,
                                     const RollBlockParameters& parameters = {});
  
  HelperHandle CreateSearchForBlockHelper(ICozmoBehavior& behavior,
                                          const SearchParameters& params = {});
  
  
private:
  BehaviorHelperComponent& _helperComponent;
  
  
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperFactory_H__

