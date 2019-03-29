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
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/components/visionComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/charger.h"
#include "engine/components/carryingComponent.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/utils/robotPointSamplerHelper.h"
#include "engine/vision/imageSaver.h"
#include "engine/vision/visionProcessingResult.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"
#include "util/random/randomIndexSampler.h"
#include "util/random/rejectionSamplerHelper.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

namespace {
  const char* kSearchTurnAnimKey                 = "searchTurnAnimTrigger";
  const char* kPostSearchAnimTrigger             = "postSearchAnimTrigger";
  const char* kMinSearchAngleSweepKey            = "minSearchAngleSweep_deg";
  const char* kMaxSearchTurnsKey                 = "maxSearchTurns";
  const char* kRecentSearchWindowKey             = "recentSearchWindow_sec";
  const char* kMaxNumRecentSearchesKey           = "maxNumRecentSearches";
  const char* kNumSearchesBeforePlayingPostSearchAnim = "numSearchesBeforePlayingPostSearchAnim";
  const char* kMinDrivingDistKey                 = "minDrivingDist_mm";
  const char* kMaxDrivingDistKey                 = "maxDrivingDist_mm";

  const float kMinCliffPenaltyDist_mm = 100.0f;
  const float kMaxCliffPenaltyDist_mm = 600.0f;
  
  // When generating new locations from which to search for the charger, new
  // locations should be at least this far from previously-searched locations.
  const float kMinDistFromPreviousSearch_mm = 200.f;

  // Max allowed age of charger object to be considered recently seen
  const RobotTimeStamp_t kMaxAgeForChargerSeenRecently_ms = 5000;
  
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

  const float kIncidenceForObservation_rad = DEG_TO_RAD(75.f);
  const float kNumRandomPosesForObservation = 10;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::InstanceConfig::InstanceConfig(const Json::Value& config, const std::string& debugName)
{
  searchTurnAnimTrigger = AnimationTrigger::Count;
  postSearchAnimTrigger = AnimationTrigger::Count;
  
  JsonTools::GetCladEnumFromJSON(config, kSearchTurnAnimKey, searchTurnAnimTrigger, debugName);
  JsonTools::GetCladEnumFromJSON(config, kPostSearchAnimTrigger, postSearchAnimTrigger, debugName);
  minSearchAngleSweep_deg = JsonTools::ParseFloat(config, kMinSearchAngleSweepKey, debugName);
  maxSearchTurns          = JsonTools::ParseUint8(config, kMaxSearchTurnsKey, debugName);
  recentSearchWindow_sec  = JsonTools::ParseFloat(config, kRecentSearchWindowKey, debugName);
  numSearchesBeforePlayingPostSearchAnim = JsonTools::ParseInt32(config, kNumSearchesBeforePlayingPostSearchAnim, debugName);
  maxNumRecentSearches    = JsonTools::ParseUint8(config, kMaxNumRecentSearchesKey, debugName);
  minDrivingDist_mm       = JsonTools::ParseFloat(config, kMinDrivingDistKey, debugName);
  maxDrivingDist_mm       = JsonTools::ParseFloat(config, kMaxDrivingDistKey, debugName);
  homeFilter              = std::make_unique<BlockWorldFilter>();
  searchSpacePointEvaluator = std::make_unique<Util::RejectionSamplerHelper<Point2f>>();
  searchSpacePolyEvaluator  = std::make_unique<Util::RejectionSamplerHelper<Poly2f>>();
  
  // Set up block world filter for finding charger object
  homeFilter->AddAllowedType(ObjectType::Charger_Basic);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::BehaviorFindHome(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig(config, "Behavior" + GetDebugLabel() + ".LoadConfig")
{
  SubscribeToTags({{
    EngineToGameTag::RobotOffTreadsStateChanged,
    EngineToGameTag::RobotProcessedImage,
  }});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindHome::~BehaviorFindHome()
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert({ VisionMode::Markers, EVisionUpdateFrequency::High });
  
  modifiers.wantsToBeActivatedWhenOnCharger = false;
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSearchTurnAnimKey,
    kPostSearchAnimTrigger,
    kMinSearchAngleSweepKey,
    kMaxSearchTurnsKey,
    kRecentSearchWindowKey,
    kMaxNumRecentSearchesKey,
    kNumSearchesBeforePlayingPostSearchAnim,
    kMinDrivingDistKey,
    kMaxDrivingDistKey,
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

  _iConfig.observeChargerBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID(BEHAVIOR_ID(RobustChargerObservation));
  DEV_ASSERT(_iConfig.observeChargerBehavior != nullptr, "BehaviorFindHome.InitBehavior.NullObserveChargerBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  if( _iConfig.observeChargerBehavior ) {
    delegates.insert( _iConfig.observeChargerBehavior.get() );
  }
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
      break;
    }
    case EngineToGameTag::RobotProcessedImage:
    {
      // NOTE: This should be removed as per VIC-13815
      // It is a duplicated set of stats that are tracked for the purpose of
      //  analytics for robot's charger finding capability
      if(IsActivated() && GetBEI().HasVisionComponent()) {
        _dVars.numFramesOfMarkers++;
        if(GetBEI().GetVisionComponent().GetLastImageQuality() == Vision::ImageQuality::TooDark) {
          _dVars.numFramesOfImageTooDark++;
        }
      }
      break;
    }
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
    TransitionToStartSearch();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::OnBehaviorDeactivated()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const ObservableObject* charger = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.homeFilter);

  // NOTE: The charger is considered not seen, if it was deleted from Blockworld (i.e. the charger object is null).
  // This can happen under certain situations when getting a "negative-observation" (not seeing it when it was expected).
  bool chargerSeen = false;
  if(charger != nullptr) {
    RobotTimeStamp_t chrgObsTime = charger->GetLastObservedTime();
    RobotTimeStamp_t now = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
    chargerSeen = (now-chrgObsTime) < kMaxAgeForChargerSeenRecently_ms;
  }

  DASMSG(find_home_result, "find_home.result", "Whether the FindHome behavior succeeded in locating the object");
  DASMSG_SET(i1, chargerSeen, "Success/failure on locating the charger. 1=success 0=failuire");
  DASMSG_SET(i2, _dVars.numFramesOfMarkers, "Count of total number of processed image frames searching for Markers");
  DASMSG_SET(i3, _dVars.numFramesOfImageTooDark, "Count of number of processed image frames (searching for Markers) that are TooDark");
  DASMSG_SEND();
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
    LOG_INFO("BehaviorFindHome.TransitionToStartSearch.TooCloseToPreviousSearch",
             "We are too close to a previously-searched location, so driving to a new search pose");
    TransitionToRandomDrive();
  } else {
    TransitionToLookInPlace();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindHome::TransitionToLookInPlace()
{
  if(ANKI_VERIFY(_iConfig.observeChargerBehavior->WantsToBeActivated(), 
                 "BehaviorFindHome.TransitionToLookInPlace.ObserveChargerBehavior.NotActivatable", "" )) {
    DelegateIfInControl(_iConfig.observeChargerBehavior.get(), &BehaviorFindHome::TransitionToSearchTurn);
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
    LOG_INFO("BehaviorFindHome.TransitionToSearchTurn.CompletedSearch",
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
    // If appropriate, play the "post search" animation before transferring to random drive
    const bool shouldPlayPostSearchAnim = (_iConfig.numSearchesBeforePlayingPostSearchAnim >= 0) &&
                                          (_dVars.numSearchesCompleted > _iConfig.numSearchesBeforePlayingPostSearchAnim);
    if (shouldPlayPostSearchAnim) {
      DelegateIfInControl(new TriggerAnimationAction(_iConfig.postSearchAnimTrigger),
                          [this]() {
                            TransitionToRandomDrive();
                          });
    } else {
      TransitionToRandomDrive();
    }
  } else {
    // Queue the "search turn" animation
    auto* action = new TriggerAnimationAction(_iConfig.searchTurnAnimTrigger);
    
    // Keep track of the pose before the robot starts turning
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    const auto startHeading = robotPose.GetRotation().GetAngleAroundZaxis();
    
    DelegateIfInControl(action,
                        [this,startHeading]() {
                          ++_dVars.numTurnsCompleted;
                          const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
                          const auto endHeading = robotPose.GetRotation().GetAngleAroundZaxis();
                          const auto angleSwept_deg = (endHeading - startHeading).getDegrees();
                          if (!Util::InRange(std::abs(angleSwept_deg), 30.f, 75.f)) {
                            // We turned an unexpected amount - may have missed seeing the charger due to the camera's limited FOV
                            DASMSG(find_home_invalid_turn, "behavior.find_home.invalid_turn_angle", "Invalid turn angle during a search-for-charger turn");
                            DASMSG_SET(i1, (int) std::round(angleSwept_deg), "The actual angle swept during this turn (degrees)");
                            DASMSG_SET(i2, _dVars.numTurnsCompleted, "Number of search turns completed at this location");
                            DASMSG_SEND_WARNING();
                          }
                          _dVars.angleSwept_deg += std::abs(angleSwept_deg);
                          TransitionToLookInPlace();
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
    LOG_WARNING("BehaviorFindHome.TransitionToRandomDrive.TooManyRecentSearches",
                        "We have performed too many (%zu) searches in the past %.1f seconds - ending behavior",
                        recentlySearchedLocations.size(), _iConfig.recentSearchWindow_sec);
    // Clear our recent searches so we can start fresh next time
    _dVars.persistent.searchedLocations.clear();
    CancelSelf();
    return;
  }
  
  std::vector<Pose3d> potentialSearchPoses;
  GenerateSearchPoses(potentialSearchPoses);
  
  const Radians angleTol = M_PI_F;    // basically don't care about the angle of arrival
  auto driveToAction = new DriveToPoseAction(potentialSearchPoses);
  driveToAction->SetGoalThresholds(DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM, angleTol);
  DelegateIfInControl(driveToAction, &BehaviorFindHome::TransitionToLookInPlace);
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
  const auto* charger = dynamic_cast<const Charger*>(GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.homeFilter));
  if ((charger != nullptr) &&
      (now_sec > _dVars.persistent.lastVisitedOldChargerTime + _iConfig.recentSearchWindow_sec)) {
    outPoses = charger->GenerateObservationPoses( GetRNG(),
                                                  kNumRandomPosesForObservation,
                                                  kIncidenceForObservation_rad);
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
      LOG_INFO("BehaviorFindHome.GenerateSearchPoses.Completed",
               "Met required sampling of %d points, cnt=%d", numAcceptedPoses, cnt);
      break;
    }
  }

  if (outPoses.empty()) {
    LOG_WARNING("BehaviorFindHome.GenerateSearchPoses.NoPosesFound",
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
