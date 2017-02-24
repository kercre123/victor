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

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/helperHandle.h"

#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

class Robot;
class IBehavior;
class BehaviorHelperComponent;
  
class BehaviorHelperFactory{
public:
  BehaviorHelperFactory(BehaviorHelperComponent& component);
  
  HelperHandle CreateDriveToHelper(Robot& robot,
                                   IBehavior& behavior,
                                   const ObjectID& targetID,
                                   const PreActionPose::ActionType& actionType);
  
  HelperHandle CreatePickupBlockHelper(Robot& robot,
                                       IBehavior& behavior,
                                       const ObjectID& targetID,
                                       AnimationTrigger animBeforeDock);
  
  HelperHandle CreatePlaceBlockHelper(Robot& robot,
                                      IBehavior& behavior);
  
  HelperHandle CreatePlaceRelObjectHelper(Robot& robot,
                                          IBehavior& behavior,
                                          const ObjectID& targetID,
                                          const bool placingOnGround,
                                          const f32 placementOffsetX_mm,
                                          const f32 placementOffsetY_mm,
                                          const bool relativeCurrentMarker);
  
  
  HelperHandle CreateRollBlockHelper(Robot& robot,
                                     IBehavior& behavior,
                                     const ObjectID& targetID,
                                     bool rollToUpright,
                                     DriveToAlignWithObjectAction::PreDockCallback preDockCallback = nullptr);
  
private:
  BehaviorHelperComponent& _helperComponent;
  
  
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperFactory_H__

