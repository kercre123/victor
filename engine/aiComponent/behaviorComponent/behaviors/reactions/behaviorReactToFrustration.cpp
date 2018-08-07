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

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToFrustration.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/pose.h"
#include "engine/actions/animActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorListenerInterfaces/iSubtaskListener.h"
#include "engine/drivingAnimationHandler.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "engine/moodSystem/moodManager.h"
#include "coretech/common/engine/utils/timer.h"
#include "util/math/math.h"

// TODO:(bn) this entire behavior could be generic for any type of emotion.... but that's too much effort

namespace Anki {
namespace Vector {

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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToFrustration::BehaviorReactToFrustration(const Json::Value& config)
: ICozmoBehavior(config)
{
  LoadJson(config);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::OnBehaviorActivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();

  // push driving animations in case we decide to drive
  robotInfo.GetDrivingAnimationHandler().PushDrivingAnimations(kFrustratedDrivingAnims, GetDebugLabel());
  
  if(_animToPlay != AnimationTrigger::Count) {
    TransitionToReaction();
    
  }
  else {
    PRINT_NAMED_WARNING("BehaviorReactToFrustration.NoReaction.Bug",
                        "We decided to run the reaction, but there is no valid one. this is a bug");
  }    
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::OnBehaviorDeactivated()
{
  auto& robotInfo = GetBEI().GetRobotInfo();
  robotInfo.GetDrivingAnimationHandler().RemoveDrivingAnimations(GetDebugLabel());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::TransitionToReaction()
{
  TriggerLiftSafeAnimationAction* action = new TriggerLiftSafeAnimationAction(_animToPlay);

  DelegateIfInControl(action, [this]() {
      AnimationComplete();
    });    
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::AnimationComplete()
{  
  // mark cooldown and update emotion. Note that if we get interrupted, this won't happen
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  if( !_finalEmotionEvent.empty() ) {
    if(GetBEI().HasMoodManager()){
      auto& moodManager = GetBEI().GetMoodManager();
      moodManager.TriggerEmotionEvent(_finalEmotionEvent, currTime_s);
    }
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

    auto& robotInfo = GetBEI().GetRobotInfo();

    // pick a pose by starting at the robot pose, then turning by randomAngle, then driving straight by
    // randomDriveMaxDist_mm (note that the real path may be different than this). This makes it look nicer
    // because the robot always turns away first, as if saying "screw this". Note that pose applies
    // translation and then rotation, so this is done as two different transformations
    
    Pose3d randomPoseRot( DEG_TO_RAD(randomAngleDeg), Z_AXIS_3D(),
                          {0.0f, 0.0f, 0.0f},
                          robotInfo.GetPose() );
    Pose3d randomPoseRotAndTrans( 0.f, Z_AXIS_3D(),
                                  {randomDist_mm, 0.0f, 0.0f},
                                  randomPoseRot );

    // TODO:(bn) motion profile?
    const bool kForceHeadDown = false;
    DriveToPoseAction* action = new DriveToPoseAction(randomPoseRotAndTrans.GetWithRespectToRoot(), kForceHeadDown);
    DelegateIfInControl(action); // finish behavior when we are done
  }
  BehaviorObjectiveAchieved(BehaviorObjective::ReactedToFrustration);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::LoadJson(const Json::Value& config)
{
  _minDistanceToDrive_mm = config.get(kRandomDriveMinDistKey_mm, 0).asFloat();
  _maxDistanceToDrive_mm = config.get(kRandomDriveMaxDistKey_mm, 0).asFloat();
  _randomDriveAngleMin_deg = config.get(kRandomDriveMinAngleKey_deg, 0).asFloat();
  _randomDriveAngleMax_deg = config.get(kRandomDriveMaxAngleKey_deg, 0).asFloat();
  JsonTools::GetCladEnumFromJSON(config, kAnimationKey, _animToPlay, "BehaviorReactToFrustration");
  _finalEmotionEvent = config.get(kEmotionEventKey, "").asString();
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kRandomDriveMinDistKey_mm,
    kRandomDriveMaxDistKey_mm,
    kRandomDriveMinAngleKey_deg,
    kRandomDriveMaxAngleKey_deg,
    kAnimationKey,
    kEmotionEventKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToFrustration::AddListener(ISubtaskListener* listener)
{
  _frustrationListeners.insert(listener);
}




}
}
