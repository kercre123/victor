/**
 * File: behaviorHelperFactory.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/13/17
 *
 * Description: Factory for creating behaviorHelpers
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"

#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/driveToHelper.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/placeBlockHelper.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/placeRelObjectHelper.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/rollBlockHelper.h"
#include "engine/aiComponent/behaviorSystem/behaviorHelpers/searchForBlockHelper.h"

namespace Anki {
namespace Cozmo {

namespace{
  

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperFactory::BehaviorHelperFactory(BehaviorHelperComponent& component)
: _helperComponent(component)
{
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateDriveToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                        IBehavior& behavior,
                                                        const ObjectID& targetID,
                                                        const DriveToParameters& parameters)
{
  IHelper* helper = new DriveToHelper(behaviorExternalInterface, behavior, *this, targetID, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                            IBehavior& behavior,
                                                            const ObjectID& targetID,
                                                            const PickupBlockParamaters& parameters)
{
  IHelper* helper = new PickupBlockHelper(behaviorExternalInterface, behavior, *this, targetID, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                           IBehavior& behavior)
{
  IHelper* helper = new PlaceBlockHelper(behaviorExternalInterface, behavior, *this);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceRelObjectHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                               IBehavior& behavior,
                                                               const ObjectID& targetID,
                                                               const bool placingOnGround,
                                                               const PlaceRelObjectParameters& parameters)
{
  IHelper* helper = new PlaceRelObjectHelper(behaviorExternalInterface, behavior, *this,
                                             targetID,
                                             placingOnGround,
                                             parameters);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateRollBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                          IBehavior& behavior,
                                                          const ObjectID& targetID,
                                                          bool rollToUpright,
                                                          const RollBlockParameters& parameters)
{
  IHelper* helper = new RollBlockHelper(behaviorExternalInterface, behavior, *this, targetID, rollToUpright, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateSearchForBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                               IBehavior& behavior,
                                                               const SearchParameters& params)
{
  IHelper* helper = new SearchForBlockHelper(behaviorExternalInterface, behavior, *this, params);
  return _helperComponent.AddHelperToComponent(helper);
}
  
} // namespace Cozmo
} // namespace Anki

