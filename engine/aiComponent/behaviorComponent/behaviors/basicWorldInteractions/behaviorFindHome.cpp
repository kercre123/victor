/**
 * File: behaviorFindHome.cpp
 *
 * Author: Matt Michini
 * Created: 1/31/18
 *
 * Description: Drive to a random nearby position, and play a 'searching' animation
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindHome.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/events/animationTriggerHelpers.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* kSearchAnimKey        = "searchAnimTrigger";
const char* kNumSearchesKey       = "numSearches";
const char* kMinDrivingDistKey    = "minDrivingDist_mm";
const char* kMaxDrivingDistKey    = "maxDrivingDist_mm";
const char* kMaxObservedAgeSecKey = "maxLastObservedAge_sec";
}
  
  
BehaviorFindHome::BehaviorFindHome(const Json::Value& config)
  : ICozmoBehavior(config)
  , _homeFilter(std::make_unique<BlockWorldFilter>())
{
  LoadConfig(config["params"]);
  
  // Set up block world filter for finding Home object
  _homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  _homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}
 

bool BehaviorFindHome::WantsToBeActivatedBehavior() const
{
  // Cannot be activated if we already know where a charger is and have seen it recently enough
  std::vector<const ObservableObject*> locatedHomes;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_homeFilter, locatedHomes);
  
  const bool hasAHome = !locatedHomes.empty();
  
  // If a max allowable observation age was given as a parameter,
  // then check if we have seen a charger recently
  bool recentlyObserved = true;
  if (_params.maxObservedAge_ms != 0) {
    recentlyObserved = std::any_of(locatedHomes.begin(),
                                   locatedHomes.end(),
                                   [this](const ObservableObject* obj) {
                                     const auto nowTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
                                     return nowTimestamp < obj->GetLastObservedTime() + _params.maxObservedAge_ms;
                                   });
  }
  
  // Not runnable if we have a charger in the world and have recently seen it.
  return !(hasAHome && recentlyObserved);
}


void BehaviorFindHome::OnBehaviorActivated()
{
  _numSearchesCompleted = 0;
  
  TransitionToSearchAnim();
}


void BehaviorFindHome::TransitionToSearchAnim()
{
  DelegateIfInControl(new TriggerAnimationAction(_params.searchAnimTrigger),
                      [this]() {
                        ++_numSearchesCompleted;
                        TransitionToRandomDrive();
                      });
}


void BehaviorFindHome::TransitionToRandomDrive()
{
  // Have we already completed the required number of searches?
  if (_numSearchesCompleted >= _params.numSearches) {
    return;
  }
  
  Pose3d randomPose;
  GetRandomDrivingPose(randomPose);
  
  const bool forceHeadDown = false;
  const Radians angleTol = M_PI_F;    // basically don't care about the angle of arrival
  auto driveToAction = new DriveToPoseAction(randomPose,
                                             forceHeadDown,
                                             DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                                             angleTol);
  DelegateIfInControl(driveToAction, &BehaviorFindHome::TransitionToSearchAnim);
}


void BehaviorFindHome::GetRandomDrivingPose(Pose3d& outPose)
{
  // TODO: use memory map! avoid obstacles!!
  
  const float turnAngle_rad = GetRNG().RandDblInRange(-M_PI, M_PI);
  const float distance_mm = GetRNG().RandDblInRange(_params.minDrivingDist_mm, _params.maxDrivingDist_mm);
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  outPose = robotInfo.GetPose();
  
  Radians newAngle = outPose.GetRotationAngle<'Z'>() + Radians(turnAngle_rad);
  outPose.SetRotation( outPose.GetRotation() * Rotation3d{ newAngle, Z_AXIS_3D() } );
  outPose.TranslateForward(distance_mm);
}


void BehaviorFindHome::LoadConfig(const Json::Value& config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  
  _params.searchAnimTrigger  = JsonTools::ParseAnimationTrigger(config, kSearchAnimKey, debugName);
  _params.numSearches        = JsonTools::ParseUint8(config, kNumSearchesKey, debugName);
  _params.minDrivingDist_mm  = JsonTools::ParseFloat(config, kMinDrivingDistKey, debugName);
  _params.maxDrivingDist_mm  = JsonTools::ParseFloat(config, kMaxDrivingDistKey, debugName);
  _params.maxObservedAge_ms  = Util::numeric_cast<TimeStamp_t>(1000.f * JsonTools::ParseFloat(config, kMaxObservedAgeSecKey, debugName));
}


} // namespace Cozmo
} // namespace Anki
