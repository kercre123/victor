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
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"

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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::InstanceConfig::InstanceConfig()
{
  searchAnimTrigger = AnimationTrigger::Count;
  numSearches       = 0;
  minDrivingDist_mm = 0.f;
  maxDrivingDist_mm = 0.f;
  maxObservedAge_ms = 0;
  homeFilter        = std::make_unique<BlockWorldFilter>();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  JsonTools::GetCladEnumFromJSON(config, kSearchAnimKey, searchAnimTrigger, debugName);
  numSearches        = JsonTools::ParseUint8(config, kNumSearchesKey, debugName);
  minDrivingDist_mm  = JsonTools::ParseFloat(config, kMinDrivingDistKey, debugName);
  maxDrivingDist_mm  = JsonTools::ParseFloat(config, kMaxDrivingDistKey, debugName);
  maxObservedAge_ms  = Util::numeric_cast<TimeStamp_t>(1000.f * JsonTools::ParseFloat(config, kMaxObservedAgeSecKey, debugName));
  homeFilter         = std::make_unique<BlockWorldFilter>();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::DynamicVariables::DynamicVariables()
{
  numSearchesCompleted = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::BehaviorFindHome(const Json::Value& config)
: ICozmoBehavior(config)
{
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";
  _iConfig = InstanceConfig(config, debugName);
  
  // Set up block world filter for finding Home object
  _iConfig.homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  _iConfig.homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSearchAnimKey,
    kNumSearchesKey,
    kMinDrivingDistKey,
    kMaxDrivingDistKey,
    kMaxObservedAgeSecKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindHome::WantsToBeActivatedBehavior() const
{
  // Cannot be activated if we already know where a charger is and have seen it recently enough
  std::vector<const ObservableObject*> locatedHomes;
  GetBEI().GetBlockWorld().FindLocatedMatchingObjects(*_iConfig.homeFilter, locatedHomes);
  
  const bool hasAHome = !locatedHomes.empty();
  
  // If a max allowable observation age was given as a parameter,
  // then check if we have seen a charger recently
  bool recentlyObserved = true;
  if (_iConfig.maxObservedAge_ms != 0) {
    recentlyObserved = std::any_of(locatedHomes.begin(),
                                   locatedHomes.end(),
                                   [this](const ObservableObject* obj) {
                                     const auto nowTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
                                     return nowTimestamp < obj->GetLastObservedTime() + _iConfig.maxObservedAge_ms;
                                   });
  }
  
  // Not runnable if we have a charger in the world and have recently seen it.
  return !(hasAHome && recentlyObserved);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::OnBehaviorActivated()
{
  _dVars.numSearchesCompleted = 0;
  
  
  // If we're carrying a block, we must first put it down
  const auto& robotInfo = GetBEI().GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    // Put down the block. Even if it fails, we still just want
    // to move along in the behavior.
    DelegateIfInControl(new PlaceObjectOnGroundAction(),
                        &BehaviorFindHome::TransitionToSearchAnim);
  } else {
    TransitionToHeadStraight();
  }
}


void BehaviorFindHome::TransitionToHeadStraight()
{
  // Move head to look straight ahead in case charger is right in
  // front of us.
  DelegateIfInControl(new MoveHeadToAngleAction(0.f),
                      [this]() {
                        TransitionToSearchAnim();
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToSearchAnim()
{
  DelegateIfInControl(new TriggerAnimationAction(_iConfig.searchAnimTrigger),
                      [this]() {
                        ++_dVars.numSearchesCompleted;
                        TransitionToRandomDrive();
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToRandomDrive()
{
  // Have we already completed the required number of searches?
  if (_dVars.numSearchesCompleted >= _iConfig.numSearches) {
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetRandomDrivingPose(Pose3d& outPose)
{
  // TODO: use memory map! avoid obstacles!!
  
  const float turnAngle_rad = GetRNG().RandDblInRange(-M_PI, M_PI);
  const float distance_mm = GetRNG().RandDblInRange(_iConfig.minDrivingDist_mm, _iConfig.maxDrivingDist_mm);
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  outPose = robotInfo.GetPose();
  
  Radians newAngle = outPose.GetRotationAngle<'Z'>() + Radians(turnAngle_rad);
  outPose.SetRotation( outPose.GetRotation() * Rotation3d{ newAngle, Z_AXIS_3D() } );
  outPose.TranslateForward(distance_mm);
}


} // namespace Cozmo
} // namespace Anki
