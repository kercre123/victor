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

  // if true, force the robot to drive to a different predock pose if it is already near one
  bool ignoreCurrentPredockPose = false;
  
  f32 approachAngle_rad = DEG_TO_RAD(0);
  bool useApproachAngle = false;
  
  // PLACE_RELATIVE parameters
  f32 placeRelOffsetX_mm = 0;
  f32 placeRelOffsetY_mm = 0;
};
  
struct PickupBlockParamaters{
  AnimationTrigger animBeforeDock = AnimationTrigger::Count;

  // If true, allow this helper to try to approach the cube from different faces if the pickup fails. This
  // defaults to false so that the helper will fail and allow the behavior to decide what to do next (e.g. try
  // again with another block).
  bool allowedToRetryFromDifferentPose = false;
};

struct PlaceRelObjectParameters{
  f32 placementOffsetX_mm = 0;
  f32 placementOffsetY_mm = 0;
  bool relativeCurrentMarker = true;
};

struct RollBlockParameters{
  DriveToAlignWithObjectAction::PreDockCallback preDockCallback = nullptr;
  AnimationTrigger sayNameAnimationTrigger = AnimationTrigger::Count;
  AnimationTrigger noNameAnimationTrigger = AnimationTrigger::Count;
  Radians maxTurnToFaceAngle = 0;
};
  
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperParameters_H__

