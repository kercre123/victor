/**
 * File: behaviorLookInPlaceMemoryMap
 *
 * Author: Raul
 * Created: 08/03/16
 *
 * Description: Look around in place by checking the memory map and deciding which areas we still need to 
 * discover.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorLookInPlaceMemoryMap.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/point_impl.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/navMap/mapComponent.h"
#include "engine/cozmoContext.h"
#include "engine/groundPlaneROI.h"
#include "engine/navMap/iNavMap.h"
#include "engine/viz/vizManager.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// kSectorsPerLocation: how many sectors we divide 360 into. Each sector implies 1 memoryMap raycast
// this could be in json, but don't wanna tweak much since it could have performance implications
//CONSOLE_VAR(uint8_t, kSectorsPerLocation, "BehaviorLookInPlaceMemoryMap", 8);
const uint8_t kSectorsPerLocation = 8;
// kDrawDebugInfo: Debug. If set to true the behavior renders debug privimitives
CONSOLE_VAR(bool, kDrawDebugInfo, "BehaviorLookInPlaceMemoryMap", false);

static const char* kConfigParamsKey = "params";

// This is the configuration of memory map types that this behavior checks against. If a type
// is set to true, it means that we want to check the sector with that type to see what's there.
constexpr MemoryMapTypes::FullContentArray typesWeWantToVisit =
{
  {MemoryMapTypes::EContentType::Unknown               , true },
  {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
  {MemoryMapTypes::EContentType::ObstacleObservable    , false},
  {MemoryMapTypes::EContentType::ObstacleCharger       , false},
  {MemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {MemoryMapTypes::EContentType::ObstacleProx          , false},
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
  {MemoryMapTypes::EContentType::Cliff                 , false},
  {MemoryMapTypes::EContentType::InterestingEdge       , false},
  {MemoryMapTypes::EContentType::NotInterestingEdge    , false}
};
static_assert(MemoryMapTypes::IsSequentialArray(typesWeWantToVisit),
  "This array does not define all types once and only once.");
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookInPlaceMemoryMap::BehaviorLookInPlaceMemoryMap(const Json::Value& config)
: ICozmoBehavior(config)
, _configParams{}
{  
  // set the proper size of sectors only once
  _sectors.resize( kSectorsPerLocation, SectorStatus::NeedsChecking );

  // parse all parameters now
  LoadConfig(config[kConfigParamsKey]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookInPlaceMemoryMap::~BehaviorLookInPlaceMemoryMap()
{  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kConfigParamsKey );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookInPlaceMemoryMap::WantsToBeActivatedBehavior() const
{
  if(!GetBEI().HasMapComponent()){
    return false;
  }
  
  // obviously this behavior needs memory map
  const INavMap* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  if ( nullptr == memoryMap ) {
    return false;
  }
  
  auto& robotInfo = GetBEI().GetRobotInfo();

  // Unfortunately we need to calculate borders in the memory map in order to check whether they are
  // inside our radius of action. Since I don't wanna break the const correctness of the Robot during WantsToBeActivated,
  // I'm going to validate as long as we have not failed previously at this location, and always as long
  // as we have borders to process
  float distanceSQ;
  const float distThreshold = _configParams.distanceThresholdForLocations_mm;
  const float closeDistSQ = distThreshold*distThreshold;
  const Pose3d& currentPose = robotInfo.GetPose();
  for( const auto& previousFullLocation : _previousFullLocations )
  {
    // try to grab distance between robot pose and previousFullLocation (if comparable)
    if ( ComputeDistanceSQBetween(previousFullLocation, currentPose, distanceSQ) )
    {
      // comparable, check distance
      const bool isLocationClose = FLT_LE(distanceSQ, closeDistSQ);
      if ( isLocationClose ) {
        return false;
      }
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = GetDebugLabel() + ".BehaviorLookInPlaceMemoryMap.LoadConfig";
  
  _configParams.bodyTurnSpeed_degPerSec = ParseFloat(config, "bodyTurnSpeed_degPerSec", debugName);
  _configParams.headTurnSpeed_degPerSec = ParseFloat(config, "headTurnSpeed_degPerSec", debugName);
  _configParams.bodyAngleFromSectorCenterRangeMin_deg = ParseFloat(config, "bodyAngleFromSectorCenterRangeMin_deg", debugName);
  _configParams.bodyAngleFromSectorCenterRangeMax_deg = ParseFloat(config, "bodyAngleFromSectorCenterRangeMax_deg", debugName);
  _configParams.t1_headAngleAbsRangeMin_deg = ParseFloat(config, "t1_headAngleAbsRangeMin_deg", debugName);
  _configParams.t1_headAngleAbsRangeMax_deg = ParseFloat(config, "t1_headAngleAbsRangeMax_deg", debugName);
  _configParams.t2_headAngleAbsRangeMin_deg = ParseFloat(config, "t2_headAngleAbsRangeMin_deg", debugName);
  _configParams.t2_headAngleAbsRangeMax_deg = ParseFloat(config, "t2_headAngleAbsRangeMax_deg", debugName);
  _configParams.prioritySectorCount = ParseUint8(config, "prioritySectorCount", debugName);
  _configParams.lowPriorityScoreReduction = ParseFloat(config, "lowPriorityScoreReduction", debugName);
  _configParams.distanceThresholdForLocations_mm = ParseFloat(config, "distanceThresholdForLocations_mm", debugName);
  _configParams.maxPreviousLocationCount = ParseUint8(config, "maxPreviousLocationCount", debugName);
  
  // anim triggers
  std::string lookInPlaceAnimTriggerStr = ParseString(config, "lookInPlaceAnimTrigger", debugName);
  _configParams.lookInPlaceAnimTrigger = lookInPlaceAnimTriggerStr.empty()  ? AnimationTrigger::Count : AnimationTriggerFromString(lookInPlaceAnimTriggerStr.c_str());
  if ( _configParams.lookInPlaceAnimTrigger == AnimationTrigger::Count )
  {
    PRINT_NAMED_ERROR("BehaviorLookInPlaceMemoryMap.VisitSector",
    "[%s] Invalid animation trigger '%s'",
    GetDebugLabel().c_str(),
    lookInPlaceAnimTriggerStr.c_str());
  }
  
  // todo seeNewInterestingEdgesAnimTrigger; when we see interesting edges while scanning
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::OnBehaviorActivated()
{
  // PRINT_CH_INFO("Behaviors", (GetDebugLabel() + ".InitInternal").c_str(), "Starting first iteration");
  
  // raul: there are several options here. After lengthy discussion with Andrew we decided simplifying was good, so
  // the best solutions we came up with were:
  // 1) Cast up to 8 raycasts and check for Unknown collision
  // 2) Similarly, divide 360 into 8 sectors, FindBorders and check whether any border falls within the sector
  // Both could potentially be fairly quick, so, since (1) is more intuitive and simple to implement, that's what
  // I'm going for.
  // Additionally, if this required more than one tick, a good idea would be to play a Thinking animation (even
  // with some humming), and use that time to compute stuff. This is good idea for any behavior that has high
  // computational cost.
  
  // Other ideas would be to cache which sectors we have done per location, so that a location we did
  // in the past only needs to raycast the sectors left, but that seems more error prone and complicated than
  // recasting if we get interrupted (after all, interruptions will probably cause other behaviors to kick in
  // and move us away from the current pose)
  
  _visitedSectorCount = 0; // note that doing this here will be reset if we Resume (should consider different ResumeInternal)
  
  auto& robotInfo = GetBEI().GetRobotInfo();
  _startingBodyFacing_rad = robotInfo.GetPose().GetWithRespectToRoot().GetRotationAngle<'Z'>();
  
  // restart all sectors
  _sectors.assign( _sectors.size(), SectorStatus::NeedsChecking);
  UpdateSectorRender();
  
  // find the closest sector to visit starting at 0 (forward)
  FindAndVisitClosestVisitableSector(0, kSectorsPerLocation-1, 0);
  
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::OnBehaviorDeactivated()
{
  if ( kDrawDebugInfo ) {
    auto& robotInfo = GetBEI().GetRobotInfo();
    
    robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo");
    robotInfo.GetContext()->GetVizManager()->EraseSegments("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo.UpdateSectorRender");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// normalDif = abs(a-b)
// circularDif = normalDif<=half ? normalDif : total-normalDif
inline int16_t CircularDifference(int16_t a, int16_t b) {
  const int16_t rangeSize = kSectorsPerLocation;
  const int16_t halfRange = rangeSize / 2; // cast is fine for even or odd
  const int16_t normalDif   = abs(a-b);
  const int16_t circularDif = (normalDif<=halfRange) ? normalDif : (rangeSize - normalDif);
  return circularDif;
}

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorLookInPlaceMemoryMap::MinCircleDist()
{
  return GroundPlaneROI::GetDist();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorLookInPlaceMemoryMap::MaxCircleDist()
{
  return MinCircleDist() + GroundPlaneROI::GetLength();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::FindAndVisitClosestVisitableSector(const int16_t lastIndex, 
                                                                      const int16_t nextLeft, 
                                                                      const int16_t nextRight)
{
  // correct nextLeft if it went past 0 into negatives (it should wrap to kSectorsPerLocation-1)
  DEV_ASSERT(nextLeft<kSectorsPerLocation, "BehaviorLookInPlaceMemoryMap.FindClosestSector.LeftIndexMovedRight");
  const int16_t leftIdx = (nextLeft >= 0) ? nextLeft : (kSectorsPerLocation-1);
  
  // correct nextRight if it went past kSectorsPerLocation-1 (it should wrap to 0)
  DEV_ASSERT(nextRight>=0, "BehaviorLookInPlaceMemoryMap.FindClosestSector.RightIndexMovedLeft");
  const int16_t rightIdx = (nextRight < kSectorsPerLocation) ? nextRight : 0;
  
  // check if we have reached the last sector from both sides
  if ( rightIdx != leftIdx )
  {
    // we haven't reached the last one, proceed to check closest one of the next ones
    const int16_t distanceL = CircularDifference(leftIdx , lastIndex);
    const int16_t distanceR = CircularDifference(rightIdx, lastIndex);
    const bool leftIsCloser = (distanceL < distanceR);
    if ( leftIsCloser )
    {
      // check left sector
      CheckIfSectorNeedsVisit(leftIdx);

      // if we need to visit left sector, do it now
      if ( NeedsVisit(leftIdx) )
      {
        VisitSector(leftIdx, leftIdx-1, rightIdx);
        return; // and do not cascade down more
      }
      
      // we did not visit the left index, but it was closer, advance nextLeft
      FindAndVisitClosestVisitableSector(lastIndex, leftIdx-1, rightIdx);
      return; // do not cascade down more
    }
    else
    {
      // check right sector
      CheckIfSectorNeedsVisit(rightIdx);
      
      // if we need to visit the right sector, do it now
      if ( NeedsVisit(rightIdx) )
      {
        VisitSector(rightIdx, leftIdx, rightIdx+1);
        return; // and do not cascade down more
      }
      
      // we did not visit the right index, but it was closer, advance nextRight
      FindAndVisitClosestVisitableSector(lastIndex, leftIdx, rightIdx+1);
      return; // do not cascade down more
    }
  }
  else
  {
    const int16_t finalIdx = leftIdx; // or right, it's the same
    
    // check final sector
    CheckIfSectorNeedsVisit(finalIdx);

    // if we need to visit the sector, do it now
    if ( NeedsVisit(finalIdx) )
    {
      VisitSector(finalIdx, finalIdx, finalIdx);
      return; // do not cascade down more
    }
  }
  
  // if we reach this point it means that we covered all indices and we don't have anything else to visit
  #if ANKI_DEVELOPER_CODE
  {
    for ( const auto& sectorStatus : _sectors ) {
      DEV_ASSERT((sectorStatus==SectorStatus::Visited)||(sectorStatus==SectorStatus::No_NeedToVisit),
                 "BehaviorLookInPlaceMemoryMap.FindClosestSector.AlgorithmFailure" );
    }
  }
  #endif
  
  // no more sectors to visit
  FinishedAllSectorsAtLocation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::CheckIfSectorNeedsVisit(int16_t index)
{
  DEV_ASSERT_MSG(NeedsChecking(index),
    "BehaviorLookInPlaceMemoryMap.CheckIfSectorNeedsVisit.MultipleChecksOnIndex",
    "Index %d already checked",
    index);

  // distance we are going to check
  const float minDist = MinCircleDist();
  const float maxDist = MaxCircleDist();
  
  // calculate offset for index
  const float relativeAngle_deg = GetRelativeAngleOfSectorInDegrees(index);
  const float centerAngle_deg = _startingBodyFacing_rad.getDegrees() + relativeAngle_deg;
  
  const Vec3f& kFwdVector = X_AXIS_3D();
  const Vec3f& kUpVector = Z_AXIS_3D();
  const Radians centerAngleAbs_rad = DEG_TO_RAD( centerAngle_deg );
  const Rotation3d rotateAbsAroundUp(centerAngleAbs_rad, kUpVector);
  
  const Vec3f& sectorNormal = rotateAbsAroundUp * kFwdVector;
  
  auto& robotInfo = GetBEI().GetRobotInfo();
  
  // from robot current pose towards
  const Point3f robotLocation = robotInfo.GetPose().GetWithRespectToRoot().GetTranslation();
  const Point3f& from3D = robotLocation + sectorNormal * minDist;
  const Point3f& to3D   = robotLocation + sectorNormal * maxDist;
  
  if(!GetBEI().HasMapComponent()){
    return;
  }

  // grab mem map
  const INavMap* memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT(memoryMap, "BehaviorLookInPlaceMemoryMap.NeedMemoryMap"); // checked before
  
  // ask the memory map by tracing the ray cast whether we want to visit the sector
  const bool hasCollision = memoryMap->HasCollisionRayWithTypes(from3D, to3D, typesWeWantToVisit);
  const bool needsVisit = hasCollision;
  _sectors[index] = needsVisit ? SectorStatus::Yes_NeedToVisit : SectorStatus::No_NeedToVisit;
  
  // log result
  PRINT_CH_INFO("Behaviors", (GetDebugLabel()).c_str(), "Checked sector %d (at %.2fdeg from %.2f = abs %.2f) [%s]",
    index, relativeAngle_deg, _startingBodyFacing_rad.getDegrees(), centerAngle_deg,
    needsVisit ? "YES VISIT" : "NO VISIT" );

  // debug render
  if ( kDrawDebugInfo ) {
    const ColorRGBA& color = needsVisit ? NamedColors::YELLOW : NamedColors::DARKGRAY;
    GetBEI().GetRobotInfo().GetContext()->GetVizManager()->
      DrawSegment("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo", from3D, to3D, color, false, 15.0f);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::VisitSector(const int16_t index, const int16_t nextLeft, const int16_t nextRight)
{
  auto runAfterAction = [this, index, nextRight, nextLeft](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    // What should we do on failure? Should we bail? Currently we just pretend we visited the sector.
    // It's not the worst thing, unless we have actually become stuck, eval in the future
    PRINT_CH_INFO("Behaviors", (GetDebugLabel()).c_str(), "Done visiting sector %d (priority=%s)",
      index, ( _visitedSectorCount < _configParams.prioritySectorCount ) ? "regular" : "low" ); // print priority before updating count
    
    // flag sector as finally visited
    _sectors[index] = SectorStatus::Visited;
    ++_visitedSectorCount;
  
    // The current index (we just visited) should match either both next indices (meaning we finished), or should
    // not match either, since next* should really point to new indices to explore
    DEV_ASSERT((index != nextLeft)==(index != nextRight), "BehaviorLookInPlaceMemoryMap.VisitSector.MatchAllOrNone");
    // we should check next if either one is not the one we just did
    const bool shouldCheckNext = ( index != nextLeft || index != nextRight );
    if ( shouldCheckNext )
    {
      // continue checking next sector
      this->FindAndVisitClosestVisitableSector(index, nextLeft, nextRight);
    }
    else
    {
      // this means we checked all sectors at this location
      this->FinishedAllSectorsAtLocation();
    }
  };

  // calculate offset
  const float relativeAngle_deg = GetRelativeAngleOfSectorInDegrees(index);
  const float bodyTargetAngle_deg = _startingBodyFacing_rad.getDegrees() + relativeAngle_deg;

  // log visit
  PRINT_CH_INFO("Behaviors", (GetDebugLabel()).c_str(), "Going to visit sector %d (at %.2fdeg from %.2f = abs %.2f)",
    index, relativeAngle_deg, _startingBodyFacing_rad.getDegrees(), bodyTargetAngle_deg );
  
  // create action and run
  CompoundActionSequential* fullVisitAction = new CompoundActionSequential();
  
  // 1) turn to the sector
  IAction* turnAction1 = CreateBodyAndHeadTurnAction(
    _configParams.bodyAngleFromSectorCenterRangeMin_deg,
    _configParams.bodyAngleFromSectorCenterRangeMax_deg,
    bodyTargetAngle_deg,
    _configParams.t1_headAngleAbsRangeMin_deg,
    _configParams.t1_headAngleAbsRangeMax_deg,
    _configParams.bodyTurnSpeed_degPerSec,
    _configParams.headTurnSpeed_degPerSec );
  fullVisitAction->AddAction( turnAction1 );
  
  // 2) make sure lift is down
  {
    IAction* moveLiftDownAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
    fullVisitAction->AddAction( moveLiftDownAction );
  }
  
  // 3) play anim
  if ( _configParams.lookInPlaceAnimTrigger != AnimationTrigger::Count )
  {
    IAction* imExploringAction = new TriggerLiftSafeAnimationAction(_configParams.lookInPlaceAnimTrigger);
    fullVisitAction->AddAction( imExploringAction );
  }
  
  // 4) explore different angle
  IAction* turnAction2 = CreateBodyAndHeadTurnAction(
    -1.0f * _configParams.bodyAngleFromSectorCenterRangeMin_deg,
    -1.0f * _configParams.bodyAngleFromSectorCenterRangeMax_deg,
    bodyTargetAngle_deg,
    _configParams.t2_headAngleAbsRangeMin_deg,
    _configParams.t2_headAngleAbsRangeMax_deg,
    _configParams.bodyTurnSpeed_degPerSec,
    _configParams.headTurnSpeed_degPerSec );
  fullVisitAction->AddAction( turnAction2 );
  
  // visit by performing sequential actions
  DelegateIfInControl( fullVisitAction, runAfterAction );
  
  // update sector render because it will have changed right before deciding to visit
  UpdateSectorRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::FinishedAllSectorsAtLocation()
{
  PRINT_CH_INFO("Behaviors", (GetDebugLabel()).c_str(), "Finished all sectors without interruption. Flagging location");
  
  // if we completed the loop ourselves, flag this location so that we don't try again soon
  if ( _configParams.maxPreviousLocationCount > 0 )
  {
    // if we have reached the max amount of locations, remove the front one
    if ( _previousFullLocations.size() >= _configParams.maxPreviousLocationCount ) {
      _previousFullLocations.pop_front();
    }

    auto& robotInfo = GetBEI().GetRobotInfo();
    
    // add this new one
    _previousFullLocations.emplace_back( robotInfo.GetPose() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorLookInPlaceMemoryMap::GetRelativeAngleOfSectorInDegrees(int16_t index) const
{
  const float anglePerSector_deg = 360.0f / kSectorsPerLocation;
  const float relativeCenterOfSector_deg = (index * anglePerSector_deg) - (0.5f * anglePerSector_deg); // because the center is total-half
  return relativeCenterOfSector_deg;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAction* BehaviorLookInPlaceMemoryMap::CreateBodyAndHeadTurnAction(
  float bodyRelativeMin_deg, float bodyRelativeMax_deg,
  float bodyAbsoluteTargetAngle_deg,
  float headAbsoluteMin_deg, float headAbsoluteMax_deg,
  float bodyTurnSpeed_degPerSec,
  float headTurnSpeed_degPerSec)
{
  // [min,max] range for random body angle turn
  const double bodyRelativeRandomAngle_deg = GetRNG().RandDblInRange(bodyRelativeMin_deg, bodyRelativeMax_deg);
  const double bodyTargetAngleAbs_deg = bodyAbsoluteTargetAngle_deg + bodyRelativeRandomAngle_deg;
  
  // [min,max] range for random head angle turn
  const double headTargetAngleAbs_deg = GetRNG().RandDblInRange(headAbsoluteMin_deg, headAbsoluteMax_deg);
  
  // create proper action for body & head turn
  const Radians bodyTargetAngleAbs_rad( DEG_TO_RAD(bodyTargetAngleAbs_deg) );
  const Radians headTargetAngleAbs_rad( DEG_TO_RAD(headTargetAngleAbs_deg) );
  PanAndTiltAction* turnAction = new PanAndTiltAction(bodyTargetAngleAbs_rad, headTargetAngleAbs_rad, true, true);
  turnAction->SetMaxPanSpeed ( DEG_TO_RAD(bodyTurnSpeed_degPerSec) );
  turnAction->SetMaxTiltSpeed( DEG_TO_RAD(headTurnSpeed_degPerSec) );

  return turnAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::UpdateSectorRender()
{
  #if ANKI_DEV_CHEATS
  {
    if ( kDrawDebugInfo )
    {
      auto& robotInfo = GetBEI().GetRobotInfo();

      // clear previous
      const std::string debugId("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo.UpdateSectorRender");
      robotInfo.GetContext()->GetVizManager()->EraseSegments(debugId);
      
      const ColorRGBA& needCheckColor = NamedColors::LIGHTGRAY;
      const ColorRGBA& noVisitColor   = NamedColors::DARKGRAY;
      const ColorRGBA& yesVisitColor  = NamedColors::ORANGE;
      const ColorRGBA& visitedColor   = NamedColors::BLUE;
      
      // ring distances
      const float minDist = MinCircleDist();
      const float maxDist = MaxCircleDist();
      
      // render rings
      const Point3f renderCenter = (robotInfo.GetPose().GetWithRespectToRoot().GetTranslation()) + Vec3f{0,0,15.0f}; // z offset to render above mem map
      robotInfo.GetContext()->GetVizManager()->DrawXYCircleAsSegments(debugId,
        renderCenter, minDist, needCheckColor, false, kSectorsPerLocation);
      robotInfo.GetContext()->GetVizManager()->DrawXYCircleAsSegments(debugId,
        renderCenter, maxDist, needCheckColor, false, kSectorsPerLocation);
      
      // for every sector, render segments at the end of the sector to delimit it
      const Vec3f& kFwdVector = X_AXIS_3D();
      const Vec3f& kUpVector = Z_AXIS_3D();
      for(int16_t idx=0; idx<kSectorsPerLocation; ++idx)
      {
        const float angleOffsetToNotOverlap_deg = 0.25f; // degrees that sectors will draw inwards to not overlap with neighbors
        const int16_t prevIdx = idx > 0 ? (idx-1) : (kSectorsPerLocation-1);
        const float anglePerSector_deg = 360.0f / kSectorsPerLocation;
        
        ColorRGBA thisSectorColor = needCheckColor;
        switch(_sectors[idx])
        {
          case SectorStatus::NeedsChecking:   { thisSectorColor = needCheckColor; break; }
          case SectorStatus::No_NeedToVisit:  { thisSectorColor = noVisitColor;   break; }
          case SectorStatus::Yes_NeedToVisit: { thisSectorColor = yesVisitColor;  break; }
          case SectorStatus::Visited:         { thisSectorColor = visitedColor;   break; }
        };
        
        {
          const float sectorStart_deg = (prevIdx * anglePerSector_deg) + angleOffsetToNotOverlap_deg;
          const Radians sectorStart_rad  = _startingBodyFacing_rad.ToFloat() + DEG_TO_RAD( sectorStart_deg );
          const Rotation3d startRotateAbsAroundUp(sectorStart_rad, kUpVector);
          const Vec3f& sectorStartLine = startRotateAbsAroundUp * kFwdVector;
          const Point3f& from3D = renderCenter + sectorStartLine * minDist;
          const Point3f& to3D   = renderCenter + sectorStartLine * maxDist;
          robotInfo.GetContext()->GetVizManager()->DrawSegment(debugId, from3D, to3D, thisSectorColor, false);
        }

        {
          const float sectorEnd_deg = (idx * anglePerSector_deg) - angleOffsetToNotOverlap_deg;
          const Radians sectorEnd_rad = _startingBodyFacing_rad.ToFloat() + DEG_TO_RAD( sectorEnd_deg );
          const Rotation3d endRotateAbsAroundUp  (sectorEnd_rad, kUpVector);
          const Vec3f& sectorEndLine = endRotateAbsAroundUp * kFwdVector;
          const Point3f& from3D = renderCenter + sectorEndLine * minDist;
          const Point3f& to3D   = renderCenter + sectorEndLine * maxDist;
          robotInfo.GetContext()->GetVizManager()->DrawSegment(debugId, from3D, to3D, thisSectorColor, false);
          
        }
      }
    }
  }
  #endif
}


} // namespace Cozmo
} // namespace Anki
