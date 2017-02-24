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

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/driveToHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeRelObjectHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/rollBlockHelper.h"


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
HelperHandle BehaviorHelperFactory::CreateDriveToHelper(Robot& robot,
                                                        IBehavior& behavior,
                                                        const ObjectID& targetID,
                                                        const PreActionPose::ActionType& actionType)
{
  IHelper* helper = new DriveToHelper(robot, behavior, *this, targetID, actionType);
  return _helperComponent.AddHelperToComponent(helper);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePickupBlockHelper(Robot& robot,
                                                            IBehavior& behavior,
                                                            const ObjectID& targetID,
                                                            AnimationTrigger animBeforeDock)
{
  IHelper* helper = new PickupBlockHelper(robot, behavior, *this, targetID, animBeforeDock);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceBlockHelper(Robot& robot,
                                                           IBehavior& behavior)
{
  IHelper* helper = new PlaceBlockHelper(robot, behavior, *this);
  return _helperComponent.AddHelperToComponent(helper);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreatePlaceRelObjectHelper(Robot& robot,
                                                               IBehavior& behavior,
                                                               const ObjectID& targetID,
                                                               const bool placingOnGround,
                                                               const f32 placementOffsetX_mm,
                                                               const f32 placementOffsetY_mm,
                                                               const bool relativeCurrentMarker)
{
  IHelper* helper = new PlaceRelObjectHelper(robot, behavior, *this,
                                             targetID,
                                             placingOnGround,
                                             placementOffsetX_mm,
                                             placementOffsetY_mm,
                                             relativeCurrentMarker);
  return _helperComponent.AddHelperToComponent(helper);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperFactory::CreateRollBlockHelper(Robot& robot,
                                                          IBehavior& behavior,
                                                          const ObjectID& targetID,
                                                          bool rollToUpright,
                                                          DriveToAlignWithObjectAction::PreDockCallback preDockCallback)
{
  IHelper* helper = new RollBlockHelper(robot, behavior, *this, targetID, rollToUpright, preDockCallback);
  return _helperComponent.AddHelperToComponent(helper);
}

  
} // namespace Cozmo
} // namespace Anki

