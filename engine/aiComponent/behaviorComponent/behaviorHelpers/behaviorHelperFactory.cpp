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

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"

#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/driveToHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/pickupBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/placeBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/placeRelObjectHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/rollBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/searchForBlockHelper.h"

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
                                                        ICozmoBehavior& behavior,
                                                        const ObjectID& targetID,
                                                        const DriveToParameters& parameters)
{
  IHelper* helper = new DriveToHelper(behaviorExternalInterface, behavior, *this, targetID, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                            ICozmoBehavior& behavior,
                                                            const ObjectID& targetID,
                                                            const PickupBlockParamaters& parameters)
{
  IHelper* helper = new PickupBlockHelper(behaviorExternalInterface, behavior, *this, targetID, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                           ICozmoBehavior& behavior)
{
  IHelper* helper = new PlaceBlockHelper(behaviorExternalInterface, behavior, *this);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceRelObjectHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                               ICozmoBehavior& behavior,
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
                                                          ICozmoBehavior& behavior,
                                                          const ObjectID& targetID,
                                                          bool rollToUpright,
                                                          const RollBlockParameters& parameters)
{
  IHelper* helper = new RollBlockHelper(behaviorExternalInterface, behavior, *this, targetID, rollToUpright, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateSearchForBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                                               ICozmoBehavior& behavior,
                                                               const SearchParameters& params)
{
  IHelper* helper = new SearchForBlockHelper(behaviorExternalInterface, behavior, *this, params);
  return _helperComponent.AddHelperToComponent(helper);
}
  
} // namespace Cozmo
} // namespace Anki

