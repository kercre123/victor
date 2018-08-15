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
#include "engine/moodSystem/moodManager.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/utils/robotPointSamplerHelper.h"
#include "engine/vision/imageSaver.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/console/consoleInterface.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"
#include "util/random/randomIndexSampler.h"
#include "util/random/rejectionSamplerHelper.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"


namespace Anki {
namespace Vector {

namespace {
  const char* kSearchTurnAnimKey                 = "searchTurnAnimTrigger";
  const char* kSearchTurnWaitingForImagesAnimKey = "searchTurnWaitingForImagesAnimTrigger";
  const char* kSearchTurnEndAnimKey              = "searchTurnEndAnimTrigger";
  const char* kMinSearchAngleSweepKey            = "minSearchAngleSweep_deg";
  const char* kMaxSearchTurnsKey                 = "maxSearchTurns";
  const char* kRecentSearchWindowKey             = "recentSearchWindow_sec";
  const char* kMaxNumRecentSearchesKey           = "maxNumRecentSearches";
  const char* kMinDrivingDistKey                 = "minDrivingDist_mm";
  const char* kMaxDrivingDistKey                 = "maxDrivingDist_mm";
  const char* kUseExposureCyclingKey             = "useExposureCycling";
  const char* kNumImagesToWaitForKey             = "numImagesToWaitFor";

  const float kMinCliffPenaltyDist_mm = 100.0f;
  const float kMaxCliffPenaltyDist_mm = 600.0f;
  
  // Enable for debug, to save images during WaitForImageAction
  CONSOLE_VAR(bool, kFindHome_SaveImages, "Behaviors.FindHome", false);
  
  // When generating new locations from which to search for the charger, new
  // locations should be at least this far from previously-searched locations.
  const float kMinDistFromPreviousSearch_mm = 200.f;
  
  constexpr MemoryMapTypes::FullContentArray kTypesToBlockSampling =
  {
    {MemoryMapTypes::EContentType::Unknown               , false},
    {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
    {MemoryMapTypes::EContentType::ClearOfCliff          , false},
    {MemoryMapTypes::EContentType::ObstacleObservable    , true },
    {MemoryMapTypes::EContentType::ObstacleProx          , true },
    {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
    {MemoryMapTypes::EContentType::Cliff                 , true },
    {MemoryMapTypes::EContentType::InterestingEdge       , true },
    {MemoryMapTypes::EContentType::NotInterestingEdge    , true }
  };
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  JsonTools::GetCladEnumFromJSON(config, kSearchTurnAnimKey, searchTurnAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kSearchTurnWaitingForImagesAnimKey, waitForImagesAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kSearchTurnEndAnimKey, searchTurnEndAnimTrigger, debugName);
  minSearchAngleSweep_deg = JsonTools::ParseFloat(config, kMinSearchAngleSweepKey, debugName);
  maxSearchTurns          = JsonTools::ParseUint8(config, kMaxSearchTurnsKey, debugName);
  recentSearchWindow_sec  = JsonTools::ParseFloat(config, kRecentSearchWindowKey, debugName);
  maxNumRecentSearches    = JsonTools::ParseUint8(config, kMaxNumRecentSearchesKey, debugName);
  minDrivingDist_mm       = JsonTools::ParseFloat(config, kMinDrivingDistKey, debugName);
  maxDrivingDist_mm       = JsonTools::ParseFloat(config, kMaxDrivingDistKey, debugName);
  homeFilter              = std::make_unique<BlockWorldFilter>();
  searchSpacePointEvaluator = std::make_unique<Util::RejectionSamplerHelper<Point2f>>();
  searchSpacePolyEvaluator  = std::make_unique<Util::RejectionSamplerHelper<Poly2f>>();
  
  // Set up block world filter for finding charger object
  homeFilter->AddAllowedFamily(ObjectFamily::Charger);
  homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::BehaviorFindHome(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
  SubscribeToTags({
    EngineToGameTag::RobotOffTreadsStateChanged,
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::~BehaviorFindHome()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers,        EVisionUpdateFrequency::High });
  if(_iConfig.useExposureCycling)
  {
    modifiers.visionModesForActiveScope->insert({ VisionMode::CyclingExposure,         EVisionUpdateFrequency::High });
    modifiers.visionModesForActiveScope->insert({ VisionMode::MeteringFromChargerOnly, EVisionUpdateFrequency::High });
  }
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSearchTurnAnimKey,
    kSearchTurnWaitingForImagesAnimKey,
    kSearchTurnEndAnimKey,
    kMinSearchAngleSweepKey,
    kMaxSearchTurnsKey,
    kRecentSearchWindowKey,
    kMaxNumRecentSearchesKey,
    kMinDrivingDistKey,
    kMaxDrivingDistKey,
    kUseExposureCyclingKey,
    kNumImagesToWaitForKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
} 

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindHome::WantsToBeActivatedBehavior() const
{
  // WantsToBeActivated conditions defined in the behavior config
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::InitBehavior()
{
  using namespace RobotPointSamplerHelper;
  
  // Set up "in range of previous searches" condition
  _iConfig.condHandleNearPrevSearch = _iConfig.searchSpacePointEvaluator->AddCondition(
    std::make_shared<RejectIfInRange>(0.f, kMinDistFromPreviousSearch_mm)
  );
  
  // Set up "would cross cliff" condition
  _iConfig.condHandleCliffs = _iConfig.searchSpacePointEvaluator->AddCondition(
    std::make_shared<RejectIfWouldCrossCliff>(kMinCliffPenaltyDist_mm)
  );
  _iConfig.condHandleCliffs->SetAcceptanceInterpolant(kMaxCliffPenaltyDist_mm, GetRNG());
  
  // Set up nav map condition
  _iConfig.condHandleCollisions = _iConfig.searchSpacePolyEvaluator->AddCondition(
    std::make_shared<RejectIfCollidesWithMemoryMap>(kTypesToBlockSampling)
  );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::AlwaysHandleInScope(const EngineToGameEvent& event)
{
  switch(event.GetData().GetTag()) {
    case EngineToGameTag::RobotOffTreadsStateChanged:
    {
      const auto currTreadState = event.GetData().Get_RobotOffTreadsStateChanged().treadsState;
      if (currTreadState != OffTreadsState::OnTreads) {
        // If we've gotten off of our treads, our "visited locations" will no longer be valid
        _dVars.persistent.searchedLocations.clear();
      }
    }
    break;
    default:
      break;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::OnBehaviorActivated()
{
  {
    const auto persistent = std::move(_dVars.persistent);
    _dVars = DynamicVariables();
    _dVars.persistent = std::move(persistent);
  }
    
  // If we're carrying a block, we must first put it down
  const auto& robotInfo = GetBEI().GetRobotInfo();
  if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
    // Put down the block. Even if it fails, we still just want
    // to move along in the behavior.
    DelegateIfInControl(new PlaceObjectOnGroundAction(),
                        &BehaviorFindHome::TransitionToStartSearch);
  } else {
    TransitionToHeadStraight();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToHeadStraight()
{
  // Move head to look straight ahead in case charger is right in
  // front of us.
  DelegateIfInControl(new MoveHeadToAngleAction(0.f),
                      [this]() {
                        TransitionToStartSearch();
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToStartSearch()
{
  // Make sure we haven't already searched this location recently
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const Point2f robotPosition{ robotPose.GetTranslation() };
  _iConfig.condHandleNearPrevSearch->SetOtherPositions(GetRecentlySearchedLocations());
  const bool nearPreviousSearch = !_iConfig.condHandleNearPrevSearch->operator()(robotPosition);
  if (nearPreviousSearch) {
    PRINT_CH_INFO("Behaviors", "BehaviorFindHome.TransitionToStartSearch.TooCloseToPreviousSearch",
                  "We are too close to a previously-searched location, so driving to a new search pose");
    TransitionToRandomDrive();
  } else {
    TransitionToSearchTurn();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToSearchTurn()
{
  // Have we turned enough to sweep the required angle?
  // Or have we exceeded the max number of turns?
  const bool sweptEnough = (_dVars.angleSwept_deg > _iConfig.minSearchAngleSweep_deg);
  const bool exceededMaxTurns = (_dVars.numTurnsCompleted >= _iConfig.maxSearchTurns);
  
  if (sweptEnough || exceededMaxTurns) {
    PRINT_CH_INFO("Behaviors", "BehaviorFindHome.TransitionToSearchTurn.CompletedSearch",
                  "We have completed a full search. Played %d turn animations (max is %d), and swept an angle of %.2f deg (min required sweep angle %.2f deg)",
                  _dVars.numTurnsCompleted,
                  _iConfig.maxSearchTurns,
                  _dVars.angleSwept_deg,
                  _iConfig.minSearchAngleSweep_deg);
    ++_dVars.numSearchesCompleted;
    // Record this as a 'searched' location
    const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    _dVars.persistent.searchedLocations[now_sec] = Point2f{robotPose.GetTranslation()};
    TransitionToRandomDrive();
  } else {
    // In high stimulation, just play the search turn animations, but not the
    // "waiting for images" or "search turn end" animations
    const auto currMood = GetBEI().GetMoodManager().GetSimpleMood();
    const bool isHighStim = (currMood == SimpleMoodType::HighStim);

    auto* action = new CompoundActionSequential();
    
    // Always do the "search turn"
    action->AddAction(new TriggerAnimationAction(_iConfig.searchTurnAnimTrigger));
    
    WaitForImagesAction* waitForImagesAction = new WaitForImagesAction(_iConfig.numImagesToWaitFor,
                                                                       VisionMode::DetectingMarkers);
    if(kFindHome_SaveImages)
    {
      const std::string path = GetBEI().GetRobotInfo().GetDataPlatform()->pathToResource(Util::Data::Scope::Cache,
                                                                                         "findHomeImages");
      ImageSaverParams params(path,
                              ImageSaverParams::Mode::Stream,
                              -1);  // Quality: save PNGs
      
      waitForImagesAction->SetSaveParams(params);
    }
    
    if (isHighStim) {
      action->AddAction(waitForImagesAction);
    } else {
      // In non-high-stim, play a "waiting for images" animation in parallel with the wait for
      // images action, and play a "search turn end" animation after the "wait for images" anim.
      auto* loopAndWaitForImagesAction = new CompoundActionParallel();
      loopAndWaitForImagesAction->AddAction(new TriggerAnimationAction(_iConfig.waitForImagesAnimTrigger));
      loopAndWaitForImagesAction->AddAction(waitForImagesAction);
      action->AddAction(loopAndWaitForImagesAction);
      action->AddAction(new TriggerAnimationAction(_iConfig.searchTurnEndAnimTrigger));
    }
    
    // Keep track of the pose before the robot starts turning
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    const auto startHeading = robotPose.GetRotation().GetAngleAroundZaxis();
    
    DelegateIfInControl(action,
                        [this,startHeading]() {
                          ++_dVars.numTurnsCompleted;
                          const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
                          const auto endHeading = robotPose.GetRotation().GetAngleAroundZaxis();
                          const auto angleSwept_deg = (endHeading - startHeading).getDegrees();
                          DEV_ASSERT(angleSwept_deg < 0.f, "BehaviorFindHome.TransitionToSearchTurn.ShouldTurnClockwise");
                          if (std::abs(angleSwept_deg) > 75.f) {
                            PRINT_NAMED_WARNING("BehaviorFindHome.TransitionToSearchTurn.TurnedTooMuch",
                                                "The last turn swept an angle of %.2f deg - may have missed seeing the charger due to the camera's limited FOV",
                                                std::abs(angleSwept_deg));
                          }
                          _dVars.angleSwept_deg += std::abs(angleSwept_deg);
                          TransitionToSearchTurn();
                        });
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToRandomDrive()
{
  // Clear out dVars related to search turns, since we are going to drive
  // to a new place and begin the turns again.
  _dVars.angleSwept_deg = 0.f;
  _dVars.numTurnsCompleted = 0;
  
  // Have we searched too many times recently? If so, we should exit (which will allow the next behavior to run, which
  // is likely "RequestHome"). It's possible that this behavior is continually being interrupted for some reason, and
  // if that's the case we don't want to just keep searching endlessly, so we need a persistent stopping condition.
  const auto& recentlySearchedLocations = GetRecentlySearchedLocations();
  if (recentlySearchedLocations.size() >= _iConfig.maxNumRecentSearches) {
    PRINT_NAMED_WARNING("BehaviorFindHome.TransitionToRandomDrive.TooManyRecentSearches",
                        "We have performed too many (%zu) searches in the past %.1f seconds - ending behavior",
                        recentlySearchedLocations.size(), _iConfig.recentSearchWindow_sec);
    // Clear our recent searches so we can start fresh next time
    _dVars.persistent.searchedLocations.clear();
    CancelSelf();
    return;
  }
  
  std::vector<Pose3d> potentialSearchPoses;
  GenerateSearchPoses(potentialSearchPoses);
  
  const bool forceHeadDown = false;
  const Radians angleTol = M_PI_F;    // basically don't care about the angle of arrival
  auto driveToAction = new DriveToPoseAction(potentialSearchPoses,
                                             forceHeadDown,
                                             DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                                             angleTol);
  DelegateIfInControl(driveToAction, &BehaviorFindHome::TransitionToSearchTurn);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GenerateSearchPoses(std::vector<Pose3d>& outPoses)
{
  outPoses.clear();
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  const auto& robotPose = robotInfo.GetPose();
  const Point2f robotPosition{ robotPose.GetTranslation() };
  
  // If we have a charger in the map, first try to drive to a position in front of it to
  // begin a search. Only do this if we haven't recently visited the charger's location.
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const auto* charger = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.homeFilter);
  if ((charger != nullptr) &&
      (now_sec > _dVars.persistent.lastVisitedOldChargerTime + _iConfig.recentSearchWindow_sec)) {
    Pose3d inFrontOfCharger(0.f, Z_AXIS_3D(), {-150.f,0.f,0.f});
    inFrontOfCharger.SetParent(charger->GetPose());
    outPoses.push_back(std::move(inFrontOfCharger));
    _dVars.persistent.lastVisitedOldChargerTime = now_sec;
    return;
  }
  
  const int kNumPositionsForSearch = 4; // How many search positions to generate
  const int kNumAnglesAtPose = 6;       // How many poses at each position to generate
  outPoses.reserve(kNumPositionsForSearch * kNumAnglesAtPose);
  
  const auto memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT( nullptr != memoryMap, "BehaviorFindHome.GenerateSearchPoses.NeedMemoryMap" );
  
  // Update cliff positions
  _iConfig.condHandleCliffs->SetRobotPosition(robotPosition);
  _iConfig.condHandleCliffs->UpdateCliffs(memoryMap);
  
  _iConfig.condHandleNearPrevSearch->SetOtherPositions(GetRecentlySearchedLocations());
  
  // Update memory map
  _iConfig.condHandleCollisions->SetMemoryMap(memoryMap);
  
  int numAcceptedPoses = 0;
  
  const unsigned int kMaxNumSampleSteps = 50;
  for( unsigned int cnt=0; cnt<kMaxNumSampleSteps; ++cnt ) {
    // Sample points centered around current robot position
    const float r1 = _iConfig.minDrivingDist_mm;
    const float r2 = _iConfig.maxDrivingDist_mm;
    Point2f sampledPos = robotPosition;
    {
      const Point2f pt = RobotPointSamplerHelper::SamplePointInAnnulus(GetRNG(), r1, r2 );
      sampledPos.x() += pt.x();
      sampledPos.y() += pt.y();
    }
    
    const bool acceptedPoint = _iConfig.searchSpacePointEvaluator->Evaluate(GetRNG(), sampledPos );
    if( !acceptedPoint ) {
      continue;
    }
    
    // choose a random angle about the Z axis to create a full test pose
    const float angle = M_TWO_PI_F * static_cast<float>( GetRNG().RandDbl() );
    Pose2d testPose( Radians{angle}, sampledPos );
    
    // Get the robot's would-be bounding quad at the test pose
    const auto& quad = robotInfo.GetBoundingQuadXY( Pose3d{testPose} );
    Poly2f footprint;
    footprint.ImportQuad2d(quad);
    
    const bool acceptedFootprint = _iConfig.searchSpacePolyEvaluator->Evaluate( GetRNG(), footprint );
    if( !acceptedFootprint ) {
      continue;
    }
    
    // if we get here, accept!!
    ++numAcceptedPoses;
    
    // Convert to full 3D pose in robot world origin:
    Pose3d testPose3d{ testPose };
    testPose3d.SetParent(robotInfo.GetWorldOrigin());
    
    // Generate poses with the same position but different orientations. This should
    // be pretty cheap compared to a pose at a different location.
    for (int i=0; i<kNumAnglesAtPose; ++i ) {
      const float poseAngle = i * M_TWO_PI_F / kNumAnglesAtPose;
      testPose3d.SetRotation( Radians{poseAngle}, Z_AXIS_3D() );
      outPoses.push_back( testPose3d );
    }
    
    if( numAcceptedPoses >= kNumPositionsForSearch ) {
      PRINT_CH_INFO("Behaviors", "BehaviorFindHome.GenerateSearchPoses.Completed",
                    "Met required sampling of %d points, cnt=%d", numAcceptedPoses, cnt);
      break;
    }
  }

  if (outPoses.empty()) {
    PRINT_NAMED_WARNING("BehaviorFindHome.GenerateSearchPoses.NoPosesFound",
                        "No poses that satisfy the sampling requirements were found - using naive random sampling method.");
    outPoses.emplace_back();
    GetRandomDrivingPose(outPoses.back());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetRandomDrivingPose(Pose3d& outPose)
{
  const float turnAngle_rad = GetRNG().RandDblInRange(-M_PI, M_PI);
  const float distance_mm = GetRNG().RandDblInRange(_iConfig.minDrivingDist_mm, _iConfig.maxDrivingDist_mm);
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  outPose = robotInfo.GetPose();
  
  Radians newAngle = outPose.GetRotationAngle<'Z'>() + Radians(turnAngle_rad);
  outPose.SetRotation( outPose.GetRotation() * Rotation3d{ newAngle, Z_AXIS_3D() } );
  outPose.TranslateForward(distance_mm);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<Point2f> BehaviorFindHome::GetRecentlySearchedLocations()
{
  // Update previous search positions. Cull to recent window and transform into a vector.
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto& prevSearchMap = _dVars.persistent.searchedLocations;
  prevSearchMap.erase(prevSearchMap.begin(),
                      prevSearchMap.lower_bound(now_sec - _iConfig.recentSearchWindow_sec));
  std::vector<Point2f> prevSearchPositions;
  prevSearchPositions.reserve(prevSearchMap.size());
  std::transform(prevSearchMap.begin(), prevSearchMap.end(), std::back_inserter(prevSearchPositions),
                 [](const std::pair<float, Point2f>& mapEntry) { return mapEntry.second; });
  return prevSearchPositions;
}

} // namespace Vector
} // namespace Anki
