/**
 * File: behaviorVisitInterestingEdge
 *
 * Author: Raul
 * Created: 07/19/16
 *
 * Description: Behavior to visit interesting edges from the memory map. Some decision on whether we want to
 * visit any edges found there can be done by the behavior.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorVisitInterestingEdge.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/navMap/mapComponent.h"
#include "engine/cozmoContext.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "engine/groundPlaneROI.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/types/animationTrigger.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
namespace {
const char* const kObserveEdgeAnimTriggerKey = "observeEdgeAnimTrigger";
const char* const kEdgesNotFoundAnimTriggerKey = "edgesNotFoundAnimTrigger";
const char* const kSquintStartAnimTriggerKey = "squintStartAnimTrigger";
const char* const kSquintLoopAnimTriggerKey = "squintLoopAnimTrigger";
const char* const kSquintEndAnimTriggerKey = "squintEndAnimTrigger";
const char* const kAllowGoalsBehindOtherEdgesKey = "allowGoalsBehindOtherEdges";
const char* const kDistanceFromLookAtPointMinKey = "distanceFromLookAtPointMin_mm";
const char* const kDistanceFromLookAtPointMaxKey = "distanceFromLookAtPointMax_mm";
const char* const kDistanceInsideGoalToLookAtKey = "distanceInsideGoalToLookAt_mm";
const char* const kAdditionalClearanceInFrontKey = "additionalClearanceInFront_mm";
const char* const kAdditionalClearanceBehindKey = "additionalClearanceBehind_mm";
const char* const kVantagePointAngleOffsetPerTryKey = "vantagePointAngleOffsetPerTry_deg";
const char* const kVantagePointAngleOffsetTriesKey = "vantagePointAngleOffsetTries";
const char* const kForwardConeHalfWidthAtRobotKey = "forwardConeHalfWidthAtRobot_mm";
const char* const kForwardConeFarPlaneDistFromRobotKey = "forwardConeFarPlaneDistFromRobot_mm";
const char* const kForwardConeHalfWidthAtFarPlaneKey = "forwardConeHalfWidthAtFarPlane_mm";
const char* const kAccuracyDistanceFromBorderKey = "accuracyDistanceFromBorder_mm";
const char* const kObservationDistanceFromBorderKey = "observationDistanceFromBorder_mm";
const char* const kBorderApproachSpeedKey = "borderApproachSpeed_mmps";
}
  
// kVie_MoveActionRetries: should probably not be in json, since it's not subject to gameplay tweaks
CONSOLE_VAR(uint8_t, kVie_MoveActionRetries, "BehaviorVisitInterestingEdge", 3);
// kVieDrawDebugInfo: Debug. If set to true the behavior renders debug privimitives
CONSOLE_VAR(bool, kVieDrawDebugInfo, "BehaviorVisitInterestingEdge", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// This is the configuration of memory map types that would invalidate goals because we would
// need to cross an obstacle or edge to get there. If the obstacle is seen by the planner, we can likely path around it
constexpr MemoryMapTypes::FullContentArray typesThatInvalidateGoals =
{
  {MemoryMapTypes::EContentType::Unknown               , false},
  {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
  {MemoryMapTypes::EContentType::ObstacleObservable    , false},
  {MemoryMapTypes::EContentType::ObstacleCharger       , false}, // this could be ok, since we will walk around the charger
  {MemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {MemoryMapTypes::EContentType::ObstacleProx          , false}, // this could be ok, since we will walk around
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
  {MemoryMapTypes::EContentType::Cliff                 , true },
  {MemoryMapTypes::EContentType::InterestingEdge       , false}, // the goal itself is the closest one, so we can afford not to do this (which simplifies goal point)
  {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
};
static_assert(MemoryMapTypes::IsSequentialArray(typesThatInvalidateGoals),
  "This array does not define all types once and only once.");
  
// This is the configuration of memory map types that would invalidate vantage points because an obstacle would
// block the point or another edge would present a problem
constexpr MemoryMapTypes::FullContentArray typesThatInvalidateVantagePoints =
{
  {MemoryMapTypes::EContentType::Unknown               , false},
  {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
  {MemoryMapTypes::EContentType::ObstacleObservable    , true},
  {MemoryMapTypes::EContentType::ObstacleCharger       , true},
  {MemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {MemoryMapTypes::EContentType::ObstacleProx          , true},
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true},
  {MemoryMapTypes::EContentType::Cliff                 , true},
  {MemoryMapTypes::EContentType::InterestingEdge       , true},
  {MemoryMapTypes::EContentType::NotInterestingEdge    , true}
};
static_assert(MemoryMapTypes::IsSequentialArray(typesThatInvalidateVantagePoints),
  "This array does not define all types once and only once.");

// kMinUsefulRegionUnits: number of units in the memory map (eg: quads in a quad tree) that boundaries have to have
// in order for the region to be considered useful
static const uint32_t kMinUsefulRegionUnits = 4;

// kNumEdgeImagesToGetAccurateEdges: when Cozmo is focused on getting more edges, grab at least this number of
// images before analyzing what the borders mean
static const uint32_t kNumEdgeImagesToGetAccurateEdges = 5; // arbitrary

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorVisitInterestingEdge::BorderRegionScore
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::BorderRegionScore::BorderRegionScore(const BorderRegion* rPtr, const size_t idx, float dSQ)
: borderRegionPtr(rPtr)
, idxClosestSegmentInRegion(idx)
, distanceSQ(dSQ)
{
  DEV_ASSERT(borderRegionPtr, "BorderRegionScore.NullRegion");
  DEV_ASSERT(idxClosestSegmentInRegion<borderRegionPtr->segments.size(), "BorderRegionScore.InvalidIndex");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const MemoryMapTypes::BorderSegment& BehaviorVisitInterestingEdge::BorderRegionScore::GetSegment() const
{
  DEV_ASSERT(IsValid(), "BorderRegionScore.InvalidRegion"); // can't call if invalid!
  return borderRegionPtr->segments[idxClosestSegmentInRegion];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorVisitInterestingEdge
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::BehaviorVisitInterestingEdge(const Json::Value& config)
: ICozmoBehavior(config)
, _configParams{}
, _waitForImagesActionTag(ActionConstants::INVALID_TAG)
, _squintLoopAnimActionTag(ActionConstants::INVALID_TAG)
, _operatingState(EOperatingState::Invalid)
{  
  // load parameters from json
  LoadConfig(config);
  
  _requiredProcess = AIInformationAnalysis::EProcess::CalculateInterestingRegions;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorVisitInterestingEdge::~BehaviorVisitInterestingEdge()
{  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kObserveEdgeAnimTriggerKey,
    kEdgesNotFoundAnimTriggerKey,
    kSquintStartAnimTriggerKey,
    kSquintLoopAnimTriggerKey,
    kSquintEndAnimTriggerKey,
    kAllowGoalsBehindOtherEdgesKey,
    kDistanceFromLookAtPointMinKey,
    kDistanceFromLookAtPointMaxKey,
    kDistanceInsideGoalToLookAtKey,
    kAdditionalClearanceInFrontKey,
    kAdditionalClearanceBehindKey,
    kVantagePointAngleOffsetPerTryKey,
    kVantagePointAngleOffsetTriesKey,
    kForwardConeHalfWidthAtRobotKey,
    kForwardConeFarPlaneDistFromRobotKey,
    kForwardConeHalfWidthAtFarPlaneKey,
    kAccuracyDistanceFromBorderKey,
    kObservationDistanceFromBorderKey,
    kBorderApproachSpeedKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVisitInterestingEdge::WantsToBeActivatedBehavior() const
{
  ANKI_CPU_PROFILE("BehaviorVisitInterestingEdge::WantsToBeActivatedBehavior"); // we are doing some processing now, keep an eye
  
  const auto& robotInfo = GetBEI().GetRobotInfo();

  // clear debug render from previous runs
  robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo");
  
  // reset the computed info, since we use this to check whether we are activatable
  _cache.Reset();

  // pick a goal now
  BorderRegionScoreVector validRegions;
  PickGoals(validRegions);
  
  // if we have regions, process them to see if any is reachable
  if ( !validRegions.empty() )
  {
    // sort goals by distance now so that later we can iterate from front to back
    auto sortByDistance = [](const BorderRegionScore& goal1, const BorderRegionScore& goal2) {
      const bool isCloser = FLT_LT(goal1.distanceSQ, goal2.distanceSQ);
      return isCloser;
    };
    std::sort(validRegions.begin(), validRegions.end(), sortByDistance);
   
    // iterate the vector and find the first reachable goal with reachable vantage points
    for( const BorderRegionScore& regionScore : validRegions )
    {
      // the region was picked based on this segment, use it to calculate positions to visit the region
      const BorderRegionScore::BorderSegment& candidateSegment = regionScore.GetSegment();
      
      // instead of asking the planner if we can get there, check with here if we think it will be reachable
      const bool allowGoalsBehindOthers = _configParams.allowGoalsBehindOtherEdges;
      if ( !allowGoalsBehindOthers )
      {
        const Vec3f& goalPoint = candidateSegment.GetCenter();
        const bool isReachable = CheckGoalReachable(goalPoint);
        if ( !isReachable ) {
          // next
          continue;
        }
      }
      
      // the goal seems to be reachable, see if we fit in front of it
      VantagePointVector potentialVantagePoints;
      const Vec3f& insideGoalDir = (-candidateSegment.normal);
      const Vec3f& potentialLookAtPoint = candidateSegment.GetCenter() + (insideGoalDir * _configParams.distanceInsideGoalToLookAt_mm);

      // pick a vantage point from where to look at the goal. Those are the points we will feed the planner
      GenerateVantagePoints(regionScore, potentialLookAtPoint, potentialVantagePoints);
      
      // there are no available vantage points, not a good goal
      if ( potentialVantagePoints.empty() ) {
        // next
        continue;
      }
      
      // we found vantage points! set in the cache and stop, since this is the closest/best goal
      _cache.Set(std::move(potentialVantagePoints));
      break;
    }
  }

  // if we find a goal, we will set it in the cache, use that as return
  const bool foundGoal = _cache.IsSet();
  
  // clear debug render we may generate during WantsToBeActivated (comment out when debugging)
  if ( !foundGoal ){
    robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo");
  }

  return foundGoal;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::OnBehaviorActivated()
{
  // this is the sauce, it's required
  DEV_ASSERT(_cache.IsSet(), "BehaviorVisitInterestingEdge.InitInternal.CantTrustCache");
  
  // make sure we are not updating borders while running the behavior (useless)
  GetAIComp<AIInformationAnalyzer>().AddDisableRequest(AIInformationAnalysis::EProcess::CalculateInterestingRegions, GetDebugLabel());

  // reset operating state to pick the starting one
  _operatingState = EOperatingState::Invalid;

  // start moving to the vantage point we calculated
  TransitionToS1_MoveToVantagePoint(0);

  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::OnBehaviorDeactivated()
{
  // remove our request to disable the analysis process
  GetAIComp<AIInformationAnalyzer>().RemoveDisableRequest(AIInformationAnalysis::EProcess::CalculateInterestingRegions, GetDebugLabel());

  const auto& robotInfo = GetBEI().GetRobotInfo();
  // clear debug render
  robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo");
  
  // make sure if we were interrupted that this anim doesn't run anymore (since we queue in parallel)
  StopSquintLoop();
  
  // no need to receive notifications if not waiting for images, clear that. We could also stop the action, but it
  // has no effect on the robot, so keep it running but don't listen to it
  _waitForImagesActionTag = ActionConstants::INVALID_TAG;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
  
  // delegate update depending on state
  const EOperatingState operatingState = _operatingState; // cache value because it can change during this update
  switch(operatingState)
  {
    case EOperatingState::GatheringAccurateEdge:
      StateUpdate_GatheringAccurateEdge();
    break;
    
    case EOperatingState::Invalid:
      PRINT_NAMED_ERROR("BehaviorVisitInterestingEdge.UpdateInternal.InvalidState", "State is Invalid");
    break;
    
    case EOperatingState::MovingToVantagePoint:
    case EOperatingState::Observing:
    case EOperatingState::DoneVisiting:
      // these states don't need special update since actions run in their place
      // delegate on parent for return value
      if(!IsControlDelegated()){
        CancelSelf();
        return;
      }
      break;
  }  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorVisitInterestingEdge::CreateLowLiftAndLowHeadActions()
{
  CompoundActionParallel* allDownAction = new CompoundActionParallel();

  // lift
  {
    IAction* liftDownAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
    allDownAction->AddAction( liftDownAction );
  }
  
  // head
  {
    IAction* headDownAction = new MoveHeadToAngleAction(MoveHeadToAngleAction::Preset::GROUND_PLANE_VISIBLE);
    allDownAction->AddAction( headDownAction );
  }
  
  // also play get in to squint
  {
    IAction* squintInAnimAction = new TriggerLiftSafeAnimationAction(_configParams.squintStartAnimTrigger);
    allDownAction->AddAction( squintInAnimAction );
  }

  return allDownAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::StartWaitingForEdges()
{
  DEV_ASSERT(!IsWaitingForImages(), "BehaviorVisitInterestingEdge.StartWaitingForEdges.AlreadyWaiting");

  if(GetBEI().HasMapComponent()){
    // clear the borders in front of us, so that we can get new ones
    GetBEI().GetMapComponent().FlagGroundPlaneROIInterestingEdgesAsUncertain();

    // after we have removed edges we expect to capture
    // cache the current interesting edge area so that when we get new ones we know how much we have grown
    const INavMap* currentMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
    _interestingEdgesArea_m2 = currentMap->GetInterestingEdgeAreaM2();
    // create an action and store the tag so we know when it's done
    {
      WaitForImagesAction* waitForImgs = new WaitForImagesAction(kNumEdgeImagesToGetAccurateEdges, VisionMode::DetectingOverheadEdges);      
      const bool success = DelegateIfInControl(waitForImgs,
        [this](){
          _waitForImagesActionTag = ActionConstants::INVALID_TAG;
        });

      // if queued successfully
      if (success)
      {
        // grab the tag
        _waitForImagesActionTag = waitForImgs->GetTag();     
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::StopSquintLoop()
{
  // if looping, request to stop
  if ( IsPlayingSquintLoop() )
  {
    CancelDelegates(false);
    _squintLoopAnimActionTag = ActionConstants::INVALID_TAG;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// this should be in a Segment class or collisions, but we don't have either
// dist_Point_to_Segment(): get the distance of a point to a segment
//     Input:  a Point P and a Segment S (in any dimension)
//     Return: the shortest distance from P to S
float dist_Point_to_SegmentSQ( const Point3f& p, const Point3f& s0, const Point3f& s1)
{
  const Vec3f& segment = s1 - s0;
  const Vec3f& s0ToP = p - s0;

  // if the dot product is negative, s0 is the closest point to P, since P is behind
  float c1 = DotProduct(s0ToP,segment);
  if ( c1 <= 0.0f ) {
    return s0ToP.LengthSq();
  }

  // if c1 is greater than c2, it means that P is further away from s0 than s1 is, so s1 is the closes point to P
  float c2 = DotProduct(segment,segment);
  if ( c2 <= c1 ) {
    const Vec3f& s1ToP = p - s1;
    return s1ToP.LengthSq();
  }

  // the closest point is in the line
  double b = c1 / c2;
  const Point3f& Pb = s0 + segment * b;
  const Vec3f& pbToP = p - Pb;
  return pbToP.LengthSq();
};

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::PickGoals(BorderRegionScoreVector& validGoals) const
{
  
  // can't be running while picking the best goal, since we are not analyzing regions anymore
  DEV_ASSERT(!IsActivated(), "BehaviorVisitInterestingEdge.PickBestGoal.CantTrustAnalysisWhileRunning");
  
  validGoals.clear();

  // ask the information analyzer about the regions it has detected (should have been this frame)
  const INavMap::BorderRegionVector& interestingRegions = GetAIComp<AIInformationAnalyzer>().GetDetectedInterestingRegions();
  
  // process them and see if we can pick one
  if ( !interestingRegions.empty() && GetBEI().HasMapComponent() )
  {
    const auto& robotInfo = GetBEI().GetRobotInfo();
    const Vec3f& robotLoc = robotInfo.GetPose().GetWithRespectToRoot().GetTranslation();

    // define what a small region is in order to discard them as noise
    const float memMapPrecision_mm = GetBEI().GetMapComponent().GetCurrentMemoryMap()->GetContentPrecisionMM();
    const float memMapPrecision_m  = MM_TO_M(memMapPrecision_mm);
    const float kMinRegionArea_m2 = kMinUsefulRegionUnits*(memMapPrecision_m*memMapPrecision_m);
  
    // iterate all regions
    for ( const MemoryMapTypes::BorderRegion& region : interestingRegions )
    {
      // if the region is too small, ignore it
      if ( FLT_LE(region.area_m2, kMinRegionArea_m2) ) {
        // debug render
        RenderDiscardedRegion(region);
        // continue to next region
        continue;
      }

      float closestSegmentDistSQ = FLT_MAX;
      size_t closestSegmentIdx = 0;
    
      // iterate all segments to calculate if this region is the best/closest valid one
      for( size_t idx=0; idx<region.segments.size(); ++idx )
      {
        const MemoryMapTypes::BorderSegment& candidateSegment = region.segments[idx];
        
        // if the segment is too small, ignore it
        const float kMinSegmentLenSQ = 2*(memMapPrecision_mm*memMapPrecision_mm); // 2 because aaquads can yield hypotenuse (minSQ=c^2+c^2)
        const float segmentLenSQ = (candidateSegment.from - candidateSegment.to).LengthSq();
        if ( FLT_LE(segmentLenSQ, kMinSegmentLenSQ) ) {
          continue;
        }
      
        // compare segment to best so far
        const float curDistSQ = dist_Point_to_SegmentSQ(robotLoc, candidateSegment.from, candidateSegment.to);
        const bool isBetter = FLT_LT(curDistSQ, closestSegmentDistSQ);
        if ( isBetter ) {
          // this is the new best
          closestSegmentDistSQ = curDistSQ;
          closestSegmentIdx = idx;
        }
      }
      
      // if it has at least one valid segment, the distance will be set, add as valid goal at that point
      const bool hasValidSegments = !FLT_NEAR(closestSegmentDistSQ, FLT_MAX);
      if ( hasValidSegments ) {
        // insert this goal
        validGoals.emplace_back(&region, closestSegmentIdx, closestSegmentDistSQ);
        // debug render
        RenderAcceptedRegion(region);
      }
      else
      {
        // debug render
        RenderDiscardedRegion(region);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorVisitInterestingEdge::CheckGoalReachable(const Vec3f& goalPosition) const
{
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  INavMap* memoryMap = nullptr;
  if(GetBEI().HasMapComponent()){
    memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  }
  DEV_ASSERT(nullptr != memoryMap, "BehaviorVisitInterestingEdge.CheckGoalReachable.NeedMemoryMap");
  
  const Vec3f& fromRobot = robotInfo.GetPose().GetWithRespectToRoot().GetTranslation();

  const Vec3f& toGoal    = goalPosition; // assumed wrt origin
  
  // unforunately the goal (border point) can be inside InterestingEdge; this happens for diagonal edges.
  // Fortunately, we currently only pick the closest goal, which means that if we cross an interesting edge
  // it has to be the one belonging to the goal itself. This allows setting that type to false, and proceed
  // to raycast towards the goal, rather than having to calculate the proper offset from the goal towards the
  // actual corner in the quad
  static_assert( typesThatInvalidateGoals[Util::EnumToUnderlying(MemoryMapTypes::EContentType::InterestingEdge)].Value() == false,
    "toGoal is inside an InterestingEdge. This type needs to be false for current implementation");
  const bool hasCollision = memoryMap->HasCollisionWithTypes({{Point2f{fromRobot}, Point2f{toGoal}}}, typesThatInvalidateGoals);
  
  // debug render this ray
  if ( kVieDrawDebugInfo )
  {
    const ColorRGBA& color = hasCollision ? Anki::NamedColors::RED : Anki::NamedColors::GREEN;
    robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
      fromRobot, toGoal, color, false, 20.0f);
  }
  
  // check result of collision test
  const bool isGoalReachable = !hasCollision;
  return isGoalReachable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::GenerateVantagePoints(const BorderRegionScore& goal, const Vec3f& lookAtPoint, VantagePointVector& outVantagePoints) const
{
  const Vec3f& kFwdVector = X_AXIS_3D();
  const Vec3f& kRightVector = -Y_AXIS_3D();
  const Vec3f& kUpVector = Z_AXIS_3D();

  const auto& robotInfo = GetBEI().GetRobotInfo();
  const Pose3d& worldOrigin = robotInfo.GetWorldOrigin();

  outVantagePoints.clear();
  {
    uint16_t totalRayTries = _configParams.vantagePointAngleOffsetTries * 2; // *2 because we do +-angle per try
    uint16_t rayIndex = 0;
    while ( rayIndex <= totalRayTries )
    {
      // calculate rotation offset
      const uint16_t offsetIndex = (rayIndex+1) / 2; // rayIndex(0) = offset(0), rayIndex(1,2) = offset(1), rayIndex(3,4) = offset(2), ...
      const float offsetSign = ((rayIndex%2) == 0) ? 1.0f : -1.0f; // change sign every rayIndex
      const float rotationOffset_deg = (offsetIndex * _configParams.vantagePointAngleOffsetPerTry_deg) * offsetSign;
      DEV_ASSERT((rayIndex==0) || !FLT_NEAR(rotationOffset_deg, 0.0f),
                 "BehaviorVisitInterestingEdge.GenerateVantagePoints.BadRayOffset");
      
      Vec3f normalFromLookAtTowardsVantage = goal.GetSegment().normal;
      // rotate by the offset of this try
      const bool hasRotation = (rayIndex != 0); // we assert that rayIndex!=0 has a valid offset_def
      if ( hasRotation ) {
        Radians rotationOffset_rad = DEG_TO_RAD(rotationOffset_deg);
        Rotation3d rotationToTry(rotationOffset_rad, kUpVector);
        normalFromLookAtTowardsVantage = rotationToTry * normalFromLookAtTowardsVantage;
      }

      // randomize distance for this ray
      const float distanceFromLookAtToVantage = Util::numeric_cast<float>( GetRNG().RandDblInRange(
        _configParams.distanceFromLookAtPointMin_mm, _configParams.distanceFromLookAtPointMax_mm));
      const Point3f& vantagePointPos = lookAtPoint + (normalFromLookAtTowardsVantage * distanceFromLookAtToVantage);
      
      // check for collisions in the memory map from the goal, not from the lookAt point, since the lookAt
      // point is inside the border
      bool isValidVantagePoint = true;
      {
        // Implementation Note: it is possible that the border point be inside the InterestingEdge we want to visit (this
        // happens for diagonal borders). Casting the ray from there will indeed collide with that InterestingEdge itself.
        // The most accurate solution would be to ask the memory map for the intersection point of that quad with the ray
        // we are trying to cast, and use that intersection point (with a little margin outwards) as the toPoint for the
        // actual check. However that requires knowledge of how the memory map is divided into quads.
        // Fortunately we have a minimum distance away from the goal, and we would have to account for the front of the
        // robot to not step on edges, so we can cast the point from the front of the robot with a little offset
        // to give it wiggle room in case it needs to turn.
        
        // Alternatively, note that the the Border information provides From and To points, as well as the normal.
        // With that information, it should be trivial to calculate the corner of the quad (since From and To will have
        // different X and Y coordinates for diagonals. This however assumes the underlying structure, and hence I would
        // rather not use that extra knowledge here, unless necessary. (Maybe provide that point in INavMemoryMap API)
      
        // Note on clearance distances: since we generate the vantage point randomly and we check
        // for clearance after the random distance, there's a chance that we could have picked a closer or further distance
        // and not fail the check, but that the random distance fails. Not readjusting smartly instead of randomly
        // shouldn't be a big deal in the general case, and it simplifies logic here.
      
        // vars to calculate the collision ray
        const float robotFront = ROBOT_BOUNDING_X_FRONT + ROBOT_BOUNDING_X_LIFT; // consider lift too
        const float clearDistanceInFront = _configParams.additionalClearanceInFront_mm + robotFront;
        const float robotBack  = ROBOT_BOUNDING_X - ROBOT_BOUNDING_X_FRONT;
        const float clearDistanceBehind  = _configParams.additionalClearanceBehind_mm  + robotBack;
        const Vec3f& toPoint   = vantagePointPos - (normalFromLookAtTowardsVantage * clearDistanceInFront);
        const Vec3f& fromPoint = vantagePointPos + (normalFromLookAtTowardsVantage * clearDistanceBehind );
        const INavMap* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
        assert(memoryMap); // otherwise we are not even activatable
        
        // the vantage point is valid if there's no collision with the invalid types (would block the view or the pose)
        const bool hasCollision = memoryMap->HasCollisionWithTypes({{Point2f{fromPoint}, Point2f{toPoint}}}, typesThatInvalidateVantagePoints);
        isValidVantagePoint = !hasCollision;
        
        // debug render this ray
        if ( kVieDrawDebugInfo )
        {
          const float kUpLine_mm = 10.0f;
          const ColorRGBA& color = isValidVantagePoint ? Anki::NamedColors::GREEN : Anki::NamedColors::RED;
          robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
            fromPoint, toPoint, color, false, 15.0f);
          robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
            vantagePointPos-Vec3f{0,0,kUpLine_mm}, vantagePointPos+Vec3f{0,0,kUpLine_mm}, color, false, 15.0f);
          robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
            fromPoint-Vec3f{0,0,kUpLine_mm}, fromPoint+Vec3f{0,0,kUpLine_mm}, color, false, 15.0f);
          robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
            toPoint-Vec3f{0,0,kUpLine_mm}, toPoint+Vec3f{0,0,kUpLine_mm}, color, false, 15.0f);
        }
      }
      
      // if this vantage point is valid, add to the vector
      if ( isValidVantagePoint )
      {
        // generate a pose that looks at the LookAt point inside the border
        const Vec3f& vantagePointDir = -normalFromLookAtTowardsVantage;
        const float fwdAngleCos = DotProduct(vantagePointDir, kFwdVector);
        const bool isPositiveAngle = (DotProduct(vantagePointDir, kRightVector) >= 0.0f);
        float rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);

        // add pose to vector
        outVantagePoints.emplace_back(rotRads, kUpVector, vantagePointPos, worldOrigin);
        
        // we only need one vantage point, do not check more (optimization, because we could give the planner several)
        break;
      }
      ++rayIndex;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS1_MoveToVantagePoint(uint8_t attemptsDone)
{
  DEV_ASSERT(_operatingState == EOperatingState::Invalid || _operatingState == EOperatingState::MovingToVantagePoint,
   "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.StateShouldNotBeSetOrShouldBeRetry" );

  // change operating state
  _operatingState = EOperatingState::MovingToVantagePoint;
  SetDebugStateName("ToS1_MoveToVantagePoint");
  PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".S1").c_str(), "Moving to vantage point");
  
  // there have to be vantage points. If it's impossible to generate vantage points from the memory map,
  // we should change those borders/quads to "not visitable" to prevent failing multiple times
  DEV_ASSERT(!_cache._vantagePoints.empty(),
    "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.NoVantagePoints");
  
  // create compound action to force lift to be on low dock (just in case) and then move
  CompoundActionSequential* moveAction = new CompoundActionSequential();

  // 1) move to the vantage point
  {
    // request the action
    const bool kForceHeadDown = true;
    DriveToPoseAction* driveToPoseAction = new DriveToPoseAction( _cache._vantagePoints, kForceHeadDown );
    moveAction->AddAction( driveToPoseAction );
  }
  
  // 2) make sure lift and head are down AFTER we reach the vantage point, since moving might move head up
  {
    IActionRunner* liftAndHeadDownActions = CreateLowLiftAndLowHeadActions();
    moveAction->AddAction( liftAndHeadDownActions );
  }
  
  RobotCompletedActionCallback onActionResult = [this, attemptsDone](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    ActionResultCategory resCat = IActionRunner::GetActionResultCategory(actionRet.result);
    if ( resCat == ActionResultCategory::SUCCESS )
    {
      // we got there, gather accaurate border information
      TransitionToS2_GatherAccurateEdge();
    }
    else if (resCat == ActionResultCategory::RETRY)
    {
      // retry as long as we haven't run out of tries
      if ( attemptsDone < kVie_MoveActionRetries ) {
        PRINT_CH_INFO("Behaviors", "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.ActionFailedRetry",
          "Trying again (%d)", attemptsDone+1);
        TransitionToS1_MoveToVantagePoint(attemptsDone+1);
      } else {
        PRINT_CH_INFO("Behaviors", "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.ActionFailedRetry",
          "Attempted to retry (%d) times. Bailing", attemptsDone);
        // TODO this should add a whiteboard pose failure so that we don't try to get there again
      }
    } else if(resCat == ActionResultCategory::ABORT) {
      PRINT_CH_INFO("Behaviors", "BehaviorVisitInterestingEdge.TransitionToS1_MoveToVantagePoint.ActionFailed", "Unhandled result");
      // TODO this should add a whiteboard pose failure so that we don't try to get there again
    }
  };
  
  // start moving, and react to action results
  DelegateIfInControl(moveAction, onActionResult);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS2_GatherAccurateEdge()
{
  // change operating state
  _operatingState = EOperatingState::GatheringAccurateEdge;
  SetDebugStateName("S2_GatherBorderPrecision");
  PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".S2").c_str(), "At vantage point, trying to grab more accurate borders");
  
  // start squint loop
  DEV_ASSERT(!IsPlayingSquintLoop(), "BehaviorVisitInterestingEdge.TransitionToS2_GatherAccurateEdge.AlreadySquintLooping");
  IAction* squintLoopAnimAction = new TriggerLiftSafeAnimationAction(_configParams.squintLoopAnimTrigger, 0); // loop forever
  DelegateIfInControl(squintLoopAnimAction);
  _squintLoopAnimActionTag = squintLoopAnimAction->GetTag();

  // wait for new edges
  StartWaitingForEdges();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::TransitionToS3_ObserveFromClose()
{
  DEV_ASSERT(_operatingState == EOperatingState::GatheringAccurateEdge,
             "BehaviorVisitInterestingEdge.TransitionToS3_ObserveFromClose.InvalidState");

  // change operating state
  _operatingState = EOperatingState::Observing;
  SetDebugStateName("S3_ObserveFromClose");

  // we know the distance to the closest border, so we can get as close as we want before playing the anim
  const float robotLen = (ROBOT_BOUNDING_X_FRONT + ROBOT_BOUNDING_X_LIFT);
  const float lastEdgeDistance_mm = GetAIComp<AIWhiteboard>().GetLastEdgeClosestDistance();
  DEV_ASSERT(!std::isnan(lastEdgeDistance_mm), "BehaviorVisitInterestingEdge.TransitionToS3_ObserveFromClose.NaNEdgeDist");
  const float distanceToMoveForward_mm = lastEdgeDistance_mm - robotLen - _configParams.observationDistanceFromBorder_mm;
  
  PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".S3").c_str(), "Observing edges from close distance (moving closer %.2fmm)", distanceToMoveForward_mm);

  // This should be done when the move action finishes, but it's not big deal and that way I can have the
  // compound action be easier. It just basically means we clear from observationPose-distanceToMoveForward_mm.
  // could also pass in that offset to FlagVisitedQuadAsNotInteresting, but again, this seems to be ok in case
  // we leave a small border behind
  
  // ask blockworld to flag the interesting edges in front of Cozmo as noninteresting anymore
  const float halfWidthAtRobot_mm = _configParams.forwardConeHalfWidthAtRobot_mm;
  const float farPlaneDistFromRobot_mm = _configParams.forwardConeFarPlaneDistFromRobot_mm;
  const float halfWidthAtFarPlane_mm = _configParams.forwardConeHalfWidthAtFarPlane_mm;
  FlagVisitedQuadAsNotInteresting(GetBEI(), halfWidthAtRobot_mm, farPlaneDistFromRobot_mm, halfWidthAtFarPlane_mm);
  
  CompoundActionSequential* observationActions = new CompoundActionSequential();

  // 1) move closer if we have to
  if ( distanceToMoveForward_mm > 0.0f )
  {
    const float speed_mmps = _configParams.borderApproachSpeed_mmps;
    DriveStraightAction* driveCloser = new DriveStraightAction(distanceToMoveForward_mm, speed_mmps, false);
    observationActions->AddAction( driveCloser );
  }

  // 2) despite we stop the squint loop, it looks better to do this after moving. Alternatively we could do 1 and 2
  // in parallel, and then observe
  {
    IAction* squintOutAnimAction = new TriggerLiftSafeAnimationAction(_configParams.squintEndAnimTrigger);
    observationActions->AddAction( squintOutAnimAction );
  }

  // 3) play "i'm observing stuff" animation
  {
    IAction* observeInPlaceAction = new TriggerLiftSafeAnimationAction(_configParams.observeEdgeAnimTrigger);
    observationActions->AddAction( observeInPlaceAction );
  }
  
  // stop the squint loop and the movement, and start the new actions, which includes squint out
  CancelDelegates();
  StopSquintLoop();
  DelegateIfInControl(observationActions);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::FlagVisitedQuadAsNotInteresting(
  BehaviorExternalInterface& bei,
  float halfWidthAtRobot_mm,
  float farPlaneDistFromRobot_mm,
  float halfWidthAtFarPlane_mm)
{
  auto& robotInfo = bei.GetRobotInfo();
  
  const Pose3d& robotPoseWrtOrigin = robotInfo.GetPose().GetWithRespectToRoot();
  
  // bottom corners of the quad are based on the robot pose
  const Point3f& cornerBL = robotPoseWrtOrigin * Vec3f{ 0.f, +halfWidthAtRobot_mm, 0.f};
  const Point3f& cornerBR = robotPoseWrtOrigin * Vec3f{ 0.f, -halfWidthAtRobot_mm, 0.f};
  
  // top corners of the quad are based on the far plane
  const Point3f& cornerTL = robotPoseWrtOrigin * Vec3f{ +farPlaneDistFromRobot_mm, +halfWidthAtFarPlane_mm, 0.f};
  const Point3f& cornerTR = robotPoseWrtOrigin * Vec3f{ +farPlaneDistFromRobot_mm, -halfWidthAtFarPlane_mm, 0.f};

  if(bei.HasMapComponent()){
    const Quad2f robotToFarPlaneQuad{ cornerTL, cornerBL, cornerTR, cornerBR };
    bei.GetMapComponent().FlagQuadAsNotInterestingEdges(robotToFarPlaneQuad);
    // render the quad we are flagging as not interesting anymore
    if ( kVieDrawDebugInfo ) {
      robotInfo.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        robotToFarPlaneQuad, 32.0f, Anki::NamedColors::BLUE, true);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::FlagQuadAroundGoalAsNotInteresting(
  BehaviorExternalInterface& bei, 
  const Vec3f& goalPoint, 
  const Vec3f& goalNormal, 
  float halfQuadSideSize_mm)
{
  DEV_ASSERT(FLT_NEAR(goalNormal.z(), 0.0f),
             "BehaviorVisitInterestingEdge.FlagQuadAroundGoalAsNotInteresting.MemoryMapIs2DAtTheMoment");
  const Vec3f rightNormal{goalNormal.y(), -goalNormal.x(), goalNormal.z()}; // 2d perpendicular to the right
  const Vec3f forwardDir = goalNormal  * halfQuadSideSize_mm;
  const Vec3f rightDir   = rightNormal * halfQuadSideSize_mm;
  
  // corners of the quad are centered around goalPoint
  const Point3f& cornerBL = goalPoint - forwardDir - rightDir;
  const Point3f& cornerBR = goalPoint - forwardDir + rightDir;
  const Point3f& cornerTL = goalPoint + forwardDir - rightDir;
  const Point3f& cornerTR = goalPoint + forwardDir + rightDir;

  if(bei.HasMapComponent()){
    const Quad2f quadAroundGoal{ cornerTL, cornerBL, cornerTR, cornerBR };
    bei.GetMapComponent().FlagQuadAsNotInterestingEdges(quadAroundGoal);
    // render the quad we are flagging as not interesting beacuse
    if ( kVieDrawDebugInfo ) {
      auto& robotInfo = bei.GetRobotInfo();
      robotInfo.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        quadAroundGoal, 32.0f, Anki::NamedColors::BLUE, true);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::StateUpdate_GatheringAccurateEdge()
{
  // if we are waiting for images we don't want to analyze them yet
  const bool isWaitingForImages = IsWaitingForImages();
  if ( isWaitingForImages )
  {
    // TODO: timeout here if it takes too long (both in time or distance driven straight)
    // This would mean that we have not been able to process images for edge detection. If we were not moving, we should
    // stop by timeout, since we are stopped not moving and vision is not processing edges for some reason. If we are
    // moving stop by distance, since we may be ramming into stuff like a snowplow
    
    // even if not moving wait to receive edges
    return;
  }
  
  // no need to receive notifications if not waiting for images
  _waitForImagesActionTag = ActionConstants::INVALID_TAG;
  
  // check distance to closest detected edge
  const float lastEdgeDistance_mm = GetAIComp<AIWhiteboard>().GetLastEdgeClosestDistance();
  const bool detectedEdges = !std::isnan(lastEdgeDistance_mm);
  if ( detectedEdges && GetBEI().HasMapComponent() )
  { 
    // are the new edges big enough? otherwise this is probably a reflection, noise, or something whose border
    // changes drastically depending on where we look from
    const INavMap* currentMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
    const double newArea_m2 = currentMap->GetInterestingEdgeAreaM2();
    const double changeInArea_m2 = newArea_m2 - _interestingEdgesArea_m2;
    const float memMapPrecision_mm = currentMap->GetContentPrecisionMM();

    const float memMapPrecision_m  = MM_TO_M(memMapPrecision_mm);
    const float kMinRegionArea_m2 = kMinUsefulRegionUnits*(memMapPrecision_m*memMapPrecision_m);
    if ( changeInArea_m2 < kMinRegionArea_m2 )
    {
      // at the moment just gather information, but let close/far checks do the same it would do for a big region.
      // eventually we might want to review this and stop searching for borders (play NotFoundAnim), or flag the
      // memory map as "possibleBorderNoise"
      // Todo: make a decision about how to handle this (waiting to see what people think)
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".GatheringAccurateEdge.RegionTooSmall").c_str(),
        "Detected edges, but the region is too small (changed from %.8f to %.8f = %.8f, required %.8f at least). Is this a reflection or noise?",
        _interestingEdgesArea_m2,
        newArea_m2,
        changeInArea_m2,
        kMinRegionArea_m2
      );
    }
  
    const float kCloseToEdgeDist_mm = GroundPlaneROI::GetDist() + _configParams.accuracyDistanceFromBorder_mm;
    const bool isCloseToEdge = FLT_LE(lastEdgeDistance_mm, kCloseToEdgeDist_mm);
    // we have new edges
    if ( isCloseToEdge )
    {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".GatheringAccurateEdge.Close").c_str(), "Got a close edge, observe from here");
      
      // we can observe from here
      TransitionToS3_ObserveFromClose();
    }
    else
    {
      PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".GatheringAccurateEdge.Far").c_str(), "Got a far edge, continuing forward fetch");
      
      // not close enough, keep moving forward
      const bool isControlDelegated = IsControlDelegated();
      if ( !isControlDelegated )
      {
        // distance is not important since we will find close or far, or stop because there are no edges in front
        // the speed should be sufficiently slow that we get images before running over stuff
        const float distance_mm = 200.0f;
        const float speed_mmps = _configParams.borderApproachSpeed_mmps;
        IAction* driveFwd = new DriveStraightAction(distance_mm, speed_mmps, false);
        DelegateIfInControl( driveFwd );
      }

      // wait for new edges
      StartWaitingForEdges();
    }
  }
  else
  {
    // there are no borders in front of us, we can't see it anymore or it was never here
    PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".GatheringAccurateEdge.Done").c_str(), "Processed edges and did not find any.");
    
    // stop moving
    CancelDelegates();
    
    // action for final animations
    CompoundActionSequential* noEdgesFoundAnims = new CompoundActionSequential();
    
    // play get out of squint
    {
      IAction* squintOutAnimAction = new TriggerLiftSafeAnimationAction(_configParams.squintEndAnimTrigger);
      noEdgesFoundAnims->AddAction( squintOutAnimAction );
    }
 
    // then play "where the hell is the object that should be here? I can't see it" anim
    {
      IAction* noEdgeHereAnimAction = new TriggerLiftSafeAnimationAction(_configParams.edgesNotFoundAnimTrigger);
      noEdgesFoundAnims->AddAction( noEdgeHereAnimAction );
    }
    
    // stop the squint loop and start the new actions, which includes squint out
    StopSquintLoop();
    DelegateIfInControl(noEdgesFoundAnims);

    // done visiting (still playing anims)
    _operatingState = EOperatingState::DoneVisiting;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::RenderDiscardedRegion(const MemoryMapTypes::BorderRegion& region) const
{
  #if ANKI_DEV_CHEATS
  {
    if ( kVieDrawDebugInfo )
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      for( size_t idx=0; idx<region.segments.size(); ++idx )
      {
        const MemoryMapTypes::BorderSegment& candidateSegment = region.segments[idx];
        robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
          candidateSegment.from, candidateSegment.to, Anki::NamedColors::RED, false, 35.0f);
      }
    }
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::RenderAcceptedRegion(const MemoryMapTypes::BorderRegion& region) const
{
  #if ANKI_DEV_CHEATS
  {
    if ( kVieDrawDebugInfo )
    {
      auto& robotInfo = GetBEI().GetRobotInfo();

      for( size_t idx=0; idx<region.segments.size(); ++idx )
      {
        const MemoryMapTypes::BorderSegment& candidateSegment = region.segments[idx];
        robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
          candidateSegment.from, candidateSegment.to, Anki::NamedColors::YELLOW, false, 35.0f);
      }
    }
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::RenderChosenGoal(const BorderRegionScore& bestGoal) const
{
  #if ANKI_DEV_CHEATS
  {
    if ( kVieDrawDebugInfo && bestGoal.IsValid() )
    {
      auto& robotInfo = GetBEI().GetRobotInfo();
      
      const MemoryMapTypes::BorderSegment& b = bestGoal.GetSegment();
      robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        b.from, b.to, Anki::NamedColors::CYAN, false, 38.0f);
      Vec3f centerLine = (b.from + b.to)*0.5f;
      robotInfo.GetContext()->GetVizManager()->DrawSegment("BehaviorVisitInterestingEdge.kVieDrawDebugInfo",
        centerLine, centerLine+b.normal*15.0f, Anki::NamedColors::CYAN, false, 38.0f);
    }
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorVisitInterestingEdge::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = GetDebugLabel() + ".BehaviorVisitInterestingEdge.LoadConfig";

  // anim triggers
  {
    const std::string& triggerName = ParseString(config, kObserveEdgeAnimTriggerKey, debugName);
    _configParams.observeEdgeAnimTrigger = triggerName.empty() ? AnimationTrigger::Count :
      AnimationTriggerFromString(triggerName.c_str());
  } {
    const std::string& triggerName = ParseString(config, kEdgesNotFoundAnimTriggerKey, debugName);
    _configParams.edgesNotFoundAnimTrigger = triggerName.empty() ? AnimationTrigger::Count :
      AnimationTriggerFromString(triggerName.c_str());
  } {
    const std::string& triggerName = ParseString(config, kSquintStartAnimTriggerKey, debugName);
    _configParams.squintStartAnimTrigger = triggerName.empty() ? AnimationTrigger::Count :
      AnimationTriggerFromString(triggerName.c_str());
  } {
    const std::string& triggerName = ParseString(config, kSquintLoopAnimTriggerKey, debugName);
    _configParams.squintLoopAnimTrigger = triggerName.empty() ? AnimationTrigger::Count :
      AnimationTriggerFromString(triggerName.c_str());
  } {
    const std::string& triggerName = ParseString(config, kSquintEndAnimTriggerKey, debugName);
    _configParams.squintEndAnimTrigger = triggerName.empty() ? AnimationTrigger::Count :
      AnimationTriggerFromString(triggerName.c_str());
  }
  // gameplay params
  _configParams.allowGoalsBehindOtherEdges = ParseBool(config, kAllowGoalsBehindOtherEdgesKey, debugName);
  _configParams.distanceFromLookAtPointMin_mm = ParseFloat(config, kDistanceFromLookAtPointMinKey, debugName);
  _configParams.distanceFromLookAtPointMax_mm = ParseFloat(config, kDistanceFromLookAtPointMaxKey, debugName);
  _configParams.distanceInsideGoalToLookAt_mm = ParseFloat(config, kDistanceInsideGoalToLookAtKey, debugName);
  _configParams.additionalClearanceInFront_mm = ParseFloat(config, kAdditionalClearanceInFrontKey, debugName);
  _configParams.additionalClearanceBehind_mm  = ParseFloat(config, kAdditionalClearanceBehindKey, debugName);
  _configParams.vantagePointAngleOffsetPerTry_deg = ParseFloat(config, kVantagePointAngleOffsetPerTryKey, debugName);
  _configParams.vantagePointAngleOffsetTries = ParseUint8(config, kVantagePointAngleOffsetTriesKey, debugName);
  _configParams.forwardConeHalfWidthAtRobot_mm = ParseFloat(config, kForwardConeHalfWidthAtRobotKey, debugName);
  _configParams.forwardConeFarPlaneDistFromRobot_mm = ParseFloat(config, kForwardConeFarPlaneDistFromRobotKey, debugName);
  _configParams.forwardConeHalfWidthAtFarPlane_mm = ParseFloat(config, kForwardConeHalfWidthAtFarPlaneKey, debugName);
  _configParams.accuracyDistanceFromBorder_mm = ParseFloat(config, kAccuracyDistanceFromBorderKey, debugName);
  _configParams.observationDistanceFromBorder_mm = ParseFloat(config, kObservationDistanceFromBorderKey, debugName);
  _configParams.borderApproachSpeed_mmps = ParseFloat(config, kBorderApproachSpeedKey, debugName);
  
  // validate
  DEV_ASSERT(_configParams.observeEdgeAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidObserveEdgeAnimTrigger");
  DEV_ASSERT(_configParams.edgesNotFoundAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidEdgesNotFoundAnimTrigger");
  DEV_ASSERT(_configParams.squintStartAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidSquintStart");
  DEV_ASSERT(_configParams.squintLoopAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidSquintLoop");
  DEV_ASSERT(_configParams.squintEndAnimTrigger != AnimationTrigger::Count,
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidSquintEnd");
  DEV_ASSERT(FLT_LE(_configParams.distanceFromLookAtPointMin_mm,_configParams.distanceFromLookAtPointMax_mm),
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidDistanceFromGoalRange");
  DEV_ASSERT((_configParams.vantagePointAngleOffsetTries == 0) || FLT_GT(_configParams.vantagePointAngleOffsetPerTry_deg,0.0f),
    "BehaviorVisitInterestingEdge.LoadConfig.InvalidOffsetConfiguration");
}

} // namespace Cozmo
} // namespace Anki
