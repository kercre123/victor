/**
 * File: behaviorReactToFrustration.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-08-09
 *
 * Description: Behavior to react when the robot gets really frustrated (e.g. because he is failing actions)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToFrustration.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorListenerInterfaces/iSubtaskListener.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/math/math.h"

// TODO:(bn) this entire behavior could be generic for any type of emotion.... but that's too much effort

namespace Anki {
namespace Cozmo {

namespace {
static const DrivingAnimationHandler::DrivingAnimations kFrustratedDrivingAnims { AnimationTrigger::DriveStartAngry,
                                                                                  AnimationTrigger::DriveLoopAngry,
                                                                                  AnimationTrigger::DriveEndAngry };

static const char* kAnimationKey = "anim";
static const char* kEmotionEventKey = "finalEmotionEvent";
static const char* kRandomDriveMinDistKey_mm = "randomDriveMinDist_mm";
static const char* kRandomDriveMaxDistKey_mm = "randomDriveMaxDist_mm";
static const char* kRandomDriveMinAngleKey_deg = "randomDriveMinAngle_deg";
static const char* kRandomDriveMaxAngleKey_deg = "randomDriveMaxAngle_deg";


}

BehaviorReactToFrustration::BehaviorReactToFrustration(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  LoadJson(config);
}


Result BehaviorReactToFrustration::InitInternal(Robot& robot)
{
  // push driving animations in case we decide to drive
  robot.GetDrivingAnimationHandler().PushDrivingAnimations(kFrustratedDrivingAnims);
  
  if(_animToPlay != AnimationTrigger::Count) {
    TransitionToReaction(robot);
    return Result::RESULT_OK;
  }
  else {
    PRINT_NAMED_WARNING("BehaviorReactToFrustration.NoReaction.Bug",
                        "We decided to run the reaction, but there is no valid one. this is a bug");
    return Result::RESULT_FAIL;
  }    
}

void BehaviorReactToFrustration::StopInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PopDrivingAnimations();
}

void BehaviorReactToFrustration::TransitionToReaction(Robot& robot)
{

  TriggerLiftSafeAnimationAction* action = new TriggerLiftSafeAnimationAction(robot, _animToPlay);

  StartActing(action, [this](Robot& robot) {
      AnimationComplete(robot);
    });    
}

void BehaviorReactToFrustration::AnimationComplete(Robot& robot)
{  
  // mark cooldown and update emotion. Note that if we get interrupted, this won't happen
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( !_finalEmotionEvent.empty() ) {
    robot.GetMoodManager().TriggerEmotionEvent(_finalEmotionEvent, currTime_s);
  }

  for(auto listener: _frustrationListeners){
    listener->AnimationComplete();
  }

  // if we want to drive somewhere, do that AFTER the emotion update, so we don't get stuck in a loop if this
  // part gets interrupted
  if( FLT_GT(_maxDistanceToDrive_mm, 0.0f) ) {
    // pick a random pose
    // TODO:(bn) use memory map here
    float randomAngleDeg = GetRNG().RandDblInRange(_randomDriveAngleMin_deg,
                                                   _randomDriveAngleMax_deg);

    bool randomAngleNegative = GetRNG().RandDbl() < 0.5;
    if( randomAngleNegative ) {
      randomAngleDeg = -randomAngleDeg;
    }

    float randomDist_mm = GetRNG().RandDblInRange(_minDistanceToDrive_mm,
                                                  _maxDistanceToDrive_mm);

    // pick a pose by starting at the robot pose, then turning by randomAngle, then driving straight by
    // randomDriveMaxDist_mm (note that the real path may be different than this). This makes it look nicer
    // because the robot always turns away first, as if saying "screw this". Note that pose applies
    // translation and then rotation, so this is done as two different transformations
    
    Pose3d randomPoseRot( DEG_TO_RAD(randomAngleDeg), Z_AXIS_3D(),
                          {0.0f, 0.0f, 0.0f},
                          &robot.GetPose() );
    Pose3d randomPoseRotAndTrans( 0.f, Z_AXIS_3D(),
                                  {randomDist_mm, 0.0f, 0.0f},
                                  &randomPoseRot );

    // TODO:(bn) motion profile?
    const bool kForceHeadDown = false;
    DriveToPoseAction* action = new DriveToPoseAction(robot, randomPoseRotAndTrans.GetWithRespectToOrigin(), kForceHeadDown);
    StartActing(action); // finish behavior when we are done
  }
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedToFrustration);
}

  
void BehaviorReactToFrustration::LoadJson(const Json::Value& config)
{
  _minDistanceToDrive_mm = config.get(kRandomDriveMinDistKey_mm, 0).asFloat();
  _maxDistanceToDrive_mm = config.get(kRandomDriveMaxDistKey_mm, 0).asFloat();
  _randomDriveAngleMin_deg = config.get(kRandomDriveMinAngleKey_deg, 0).asFloat();
  _randomDriveAngleMax_deg = config.get(kRandomDriveMaxAngleKey_deg, 0).asFloat();
  _animToPlay = AnimationTriggerFromString(
                      config.get(kAnimationKey,
                                 AnimationTriggerToString(AnimationTrigger::Count)).asString().c_str());
  _finalEmotionEvent = config.get(kEmotionEventKey, "").asString();
  
}
  
void BehaviorReactToFrustration::AddListener(ISubtaskListener* listener)
{
  _frustrationListeners.insert(listener);
}




}
}
