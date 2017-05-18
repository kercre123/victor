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

  // Specify distance willing to turn to look at face before pickup, and whether
  // cozmo should say the user's name when looking at them.  Note: a turn of 0
  // means that cozmo won't look at the user regardless of if sayName is specified
  Radians maxTurnTowardsFaceAngle_rad = 0.f;
  bool sayNameBeforePickup = false;
    
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

  
enum class SearchIntensity {
  QuickSearch, // Perform a quick search just to the left and right of where cozmo is
  StandardSearch, // Performs a 180 degree search
  ExhaustiveSearch // performs a 360 degree search
};
  
struct SearchParameters{
  // Set a searchID for a block to complete search when located
  ObjectID searchingForID;
  // Number of blocks to acquire - only ID or # should be specified - not both
  uint32_t numberOfBlocksToLocate = 0;
  
  // Set the intensity of the search for the object
  SearchIntensity searchIntensity = SearchIntensity::StandardSearch;
};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperParameters_H__

