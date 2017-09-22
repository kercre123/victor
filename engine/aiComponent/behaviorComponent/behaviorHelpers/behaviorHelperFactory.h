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
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class BehaviorHelperComponent;
class IBehavior;

class BehaviorHelperFactory{
public:
  BehaviorHelperFactory(BehaviorHelperComponent& component);
  
  HelperHandle CreateDriveToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                   IBehavior& behavior,
                                   const ObjectID& targetID,
                                   const DriveToParameters& parameters = {});
  
  HelperHandle CreatePickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                       IBehavior& behavior,
                                       const ObjectID& targetID,
                                       const PickupBlockParamaters& parameters = {});
  
  HelperHandle CreatePlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                      IBehavior& behavior);
  
  HelperHandle CreatePlaceRelObjectHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                          IBehavior& behavior,
                                          const ObjectID& targetID,
                                          const bool placingOnGround = false,
                                          const PlaceRelObjectParameters& parameters = {});
  
  
  HelperHandle CreateRollBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                     IBehavior& behavior,
                                     const ObjectID& targetID,
                                     bool rollToUpright = true,
                                     const RollBlockParameters& parameters = {});
  
  HelperHandle CreateSearchForBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                          IBehavior& behavior,
                                          const SearchParameters& params = {});
  
  
private:
  BehaviorHelperComponent& _helperComponent;
  
  
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperFactory_H__

