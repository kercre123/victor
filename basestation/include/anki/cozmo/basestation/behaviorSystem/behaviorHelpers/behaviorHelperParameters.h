/**
 * File: behaviorHelperParameters.h
 *
 * Author: Kevin M. Karol
 * Created: 2/28/17
 *
 * Description: Defines the optional parameters that can be passed in to helpers.
 * The helper system will always check/set these values on actions, 
 * so ensure that all parameters have default values.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperParameters_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperParameters_H__

#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/preActionPose.h"
#include "clad/types/animationTrigger.h"


namespace Anki {
namespace Cozmo {

struct DriveToParameters{
  PreActionPose::ActionType actionType = PreActionPose::ActionType::NONE;
  
  // PLACE_RELATIVE parameters
  f32 placeRelOffsetX_mm = 0;
  f32 placeRelOffsetY_mm = 0;
};
  
struct PickupBlockParamaters{
  AnimationTrigger animBeforeDock = AnimationTrigger::Count;
};

struct PlaceRelObjectParameters{
  f32 placementOffsetX_mm = 0;
  f32 placementOffsetY_mm = 0;
  bool relativeCurrentMarker = true;
};

struct RollBlockParameters{
  DriveToAlignWithObjectAction::PreDockCallback preDockCallback = nullptr;
};


  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperParameters_H__

