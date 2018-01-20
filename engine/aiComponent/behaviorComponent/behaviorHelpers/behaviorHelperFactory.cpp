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
HelperHandle BehaviorHelperFactory::CreateDriveToHelper(ICozmoBehavior& behavior,
                                                        const ObjectID& targetID,
                                                        const DriveToParameters& parameters)
{
  IHelper* helper = new DriveToHelper(behavior, *this, targetID, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePickupBlockHelper(ICozmoBehavior& behavior,
                                                            const ObjectID& targetID,
                                                            const PickupBlockParamaters& parameters)
{
  IHelper* helper = new PickupBlockHelper(behavior, *this, targetID, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceBlockHelper(ICozmoBehavior& behavior)
{
  IHelper* helper = new PlaceBlockHelper(behavior, *this);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceRelObjectHelper(ICozmoBehavior& behavior,
                                                               const ObjectID& targetID,
                                                               const bool placingOnGround,
                                                               const PlaceRelObjectParameters& parameters)
{
  IHelper* helper = new PlaceRelObjectHelper(behavior, *this,
                                             targetID,
                                             placingOnGround,
                                             parameters);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateRollBlockHelper(ICozmoBehavior& behavior,
                                                          const ObjectID& targetID,
                                                          bool rollToUpright,
                                                          const RollBlockParameters& parameters)
{
  IHelper* helper = new RollBlockHelper(behavior, *this, targetID, rollToUpright, parameters);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateSearchForBlockHelper(ICozmoBehavior& behavior,
                                                               const SearchParameters& params)
{
  IHelper* helper = new SearchForBlockHelper(behavior, *this, params);
  return _helperComponent.AddHelperToComponent(helper);
}
  
} // namespace Cozmo
} // namespace Anki

