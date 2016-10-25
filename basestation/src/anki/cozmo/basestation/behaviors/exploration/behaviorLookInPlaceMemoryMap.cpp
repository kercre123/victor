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
#include "behaviorLookInPlaceMemoryMap.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/groundPlaneROI.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/math/point_impl.h"

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
constexpr NavMemoryMapTypes::FullContentArray typesWeWantToVisit =
{
  {NavMemoryMapTypes::EContentType::Unknown               , true },
  {NavMemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {NavMemoryMapTypes::EContentType::ClearOfCliff          , false},
  {NavMemoryMapTypes::EContentType::ObstacleCube          , false},
  {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
  {NavMemoryMapTypes::EContentType::ObstacleCharger       , false},
  {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
  {NavMemoryMapTypes::EContentType::Cliff                 , false},
  {NavMemoryMapTypes::EContentType::InterestingEdge       , false},
  {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false}
};
static_assert(NavMemoryMapTypes::IsSequentialArray(typesWeWantToVisit),
  "This array does not define all types once and only once.");
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookInPlaceMemoryMap::BehaviorLookInPlaceMemoryMap(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _configParams{}
{
  SetDefaultName("BehaviorLookInPlaceMemoryMap");
  
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
bool BehaviorLookInPlaceMemoryMap::IsRunnableInternal(const Robot& robot) const
{
  // obviously this behavior needs memory map
  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  if ( nullptr == memoryMap ) {
    return false;
  }

  // Unfortunately we need to calculate borders in the memory map in order to check whether they are
  // inside our radius of action. Since I don't wanna break the const correctness of the Robot during IsRunnable,
  // I'm going to validate as long as we have not failed previously at this location, and always as long
  // as we have borders to process
  float distanceSQ;
  const float distThreshold = _configParams.distanceThresholdForLocations_mm;
  const float closeDistSQ = distThreshold*distThreshold;
  const Pose3d& currentPose = robot.GetPose();
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
  const std::string& debugName = GetName() + ".BehaviorLookInPlaceMemoryMap.LoadConfig";
  
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
    GetName().c_str(),
    lookInPlaceAnimTriggerStr.c_str());
  }
  
  // todo seeNewInterestingEdgesAnimTrigger; when we see interesting edges while scanning
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorLookInPlaceMemoryMap::InitInternal(Robot& robot)
{
  // PRINT_CH_INFO("Behaviors", (GetName() + ".InitInternal").c_str(), "Starting first iteration");
  
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
  
  _startingBodyFacing_rad = robot.GetPose().GetWithRespectToOrigin().GetRotationAngle<'Z'>();
  
  // restart all sectors
  _sectors.assign( _sectors.size(), SectorStatus::NeedsChecking);
  UpdateSectorRender(robot);
  
  // find the closest sector to visit starting at 0 (forward)
  FindAndVisitClosestVisitableSector(robot, 0, kSectorsPerLocation-1, 0);
  
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::StopInternal(Robot& robot)
{
  if ( kDrawDebugInfo ) {
    robot.GetContext()->GetVizManager()->EraseSegments("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo");
    robot.GetContext()->GetVizManager()->EraseSegments("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo.UpdateSectorRender");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorLookInPlaceMemoryMap::EvaluateRunningScoreInternal(const Robot& robot) const
{
  float baseScore = BaseClass::EvaluateRunningScoreInternal(robot);
  
  // if we have visited the priority count, apply the score reduction now
  if ( _visitedSectorCount >= _configParams.prioritySectorCount ) {
    baseScore -= _configParams.lowPriorityScoreReduction;
  }
  return baseScore;
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
void BehaviorLookInPlaceMemoryMap::FindAndVisitClosestVisitableSector(Robot& robot,
  const int16_t lastIndex, const int16_t nextLeft, const int16_t nextRight)
{
  // correct nextLeft if it went past 0 into negatives (it should wrap to kSectorsPerLocation-1)
  ASSERT_NAMED(nextLeft<kSectorsPerLocation, "BehaviorLookInPlaceMemoryMap.FindClosestSector.LeftIndexMovedRight");
  const int16_t leftIdx = (nextLeft >= 0) ? nextLeft : (kSectorsPerLocation-1);
  
  // correct nextRight if it went past kSectorsPerLocation-1 (it should wrap to 0)
  ASSERT_NAMED(nextRight>=0, "BehaviorLookInPlaceMemoryMap.FindClosestSector.RightIndexMovedLeft");
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
      CheckIfSectorNeedsVisit(robot, leftIdx);

      // if we need to visit left sector, do it now
      if ( NeedsVisit(leftIdx) )
      {
        VisitSector(robot, leftIdx, leftIdx-1, rightIdx);
        return; // and do not cascade down more
      }
      
      // we did not visit the left index, but it was closer, advance nextLeft
      FindAndVisitClosestVisitableSector(robot, lastIndex, leftIdx-1, rightIdx);
      return; // do not cascade down more
    }
    else
    {
      // check right sector
      CheckIfSectorNeedsVisit(robot, rightIdx);
      
      // if we need to visit the right sector, do it now
      if ( NeedsVisit(rightIdx) )
      {
        VisitSector(robot, rightIdx, leftIdx, rightIdx+1);
        return; // and do not cascade down more
      }
      
      // we did not visit the right index, but it was closer, advance nextRight
      FindAndVisitClosestVisitableSector(robot, lastIndex, leftIdx, rightIdx+1);
      return; // do not cascade down more
    }
  }
  else
  {
    const int16_t finalIdx = leftIdx; // or right, it's the same
    
    // check final sector
    CheckIfSectorNeedsVisit(robot, finalIdx);

    // if we need to visit the sector, do it now
    if ( NeedsVisit(finalIdx) )
    {
      VisitSector(robot, finalIdx, finalIdx, finalIdx);
      return; // do not cascade down more
    }
  }
  
  // if we reach this point it means that we covered all indices and we don't have anything else to visit
  #if ANKI_DEVELOPER_CODE
  {
    for( const auto& sectorStatus : _sectors ) {
      ASSERT_NAMED((sectorStatus==SectorStatus::Visited)||(sectorStatus==SectorStatus::No_NeedToVisit),
      "BehaviorLookInPlaceMemoryMap.FindClosestSector.AlgorithmFailure" );
    }
  }
  #endif
  
  // no more sectors to visit
  FinishedAllSectorsAtLocation(robot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::CheckIfSectorNeedsVisit(const Robot& robot, int16_t index)
{
  ASSERT_NAMED_EVENT(NeedsChecking(index),
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
  
  // from robot current pose towards
  const Point3f& robotLocation = robot.GetPose().GetWithRespectToOrigin().GetTranslation();
  const Point3f& from3D = robotLocation + sectorNormal * minDist;
  const Point3f& to3D   = robotLocation + sectorNormal * maxDist;
  
  // grab mem map
  const INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  ASSERT_NAMED( memoryMap, "BehaviorLookInPlaceMemoryMap.NeedMemoryMap" ); // checked before
  
  // ask the memory map by tracing the ray cast whether we want to visit the sector
  const bool hasCollision = memoryMap->HasCollisionRayWithTypes(from3D, to3D, typesWeWantToVisit);
  const bool needsVisit = hasCollision;
  _sectors[index] = needsVisit ? SectorStatus::Yes_NeedToVisit : SectorStatus::No_NeedToVisit;
  
  // log result
  PRINT_CH_INFO("Behaviors", (GetName()).c_str(), "Checked sector %d (at %.2fdeg from %.2f = abs %.2f) [%s]",
    index, relativeAngle_deg, _startingBodyFacing_rad.getDegrees(), centerAngle_deg,
    needsVisit ? "YES VISIT" : "NO VISIT" );

  // debug render
  if ( kDrawDebugInfo ) {
    const ColorRGBA& color = needsVisit ? NamedColors::YELLOW : NamedColors::DARKGRAY;
    robot.GetContext()->GetVizManager()->DrawSegment("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo",
        from3D, to3D, color, false, 15.0f);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::VisitSector(Robot& robot, const int16_t index, const int16_t nextLeft, const int16_t nextRight)
{
  auto runAfterAction = [this, &robot, index, nextRight, nextLeft](const ExternalInterface::RobotCompletedAction& actionRet)
  {
    // What should we do on failure? Should we bail? Currently we just pretend we visited the sector.
    // It's not the worst thing, unless we have actually become stuck, eval in the future
    PRINT_CH_INFO("Behaviors", (GetName()).c_str(), "Done visiting sector %d (priority=%s)",
      index, ( _visitedSectorCount < _configParams.prioritySectorCount ) ? "regular" : "low" ); // print priority before updating count
    
    // flag sector as finally visited
    _sectors[index] = SectorStatus::Visited;
    ++_visitedSectorCount;
  
    // The current index (we just visited) should match either both next indices (meaning we finished), or should
    // not match either, since next* should really point to new indices to explore
    ASSERT_NAMED((index != nextLeft)==(index != nextRight), "BehaviorLookInPlaceMemoryMap.VisitSector.MatchAllOrNone");
    // we should check next if either one is not the one we just did
    const bool shouldCheckNext = ( index != nextLeft || index != nextRight );
    if ( shouldCheckNext )
    {
      // continue checking next sector
      this->FindAndVisitClosestVisitableSector(robot, index, nextLeft, nextRight);
    }
    else
    {
      // this means we checked all sectors at this location
      this->FinishedAllSectorsAtLocation(robot);
    }
  };

  // calculate offset
  const float relativeAngle_deg = GetRelativeAngleOfSectorInDegrees(index);
  const float bodyTargetAngle_deg = _startingBodyFacing_rad.getDegrees() + relativeAngle_deg;

  // log visit
  PRINT_CH_INFO("Behaviors", (GetName()).c_str(), "Going to visit sector %d (at %.2fdeg from %.2f = abs %.2f)",
    index, relativeAngle_deg, _startingBodyFacing_rad.getDegrees(), bodyTargetAngle_deg );
  
  // create action and run
  CompoundActionSequential* fullVisitAction = new CompoundActionSequential(robot);
  
  // 1) turn to the sector
  IAction* turnAction1 = CreateBodyAndHeadTurnAction( robot,
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
    IAction* moveLiftDownAction = new MoveLiftToHeightAction(robot, MoveLiftToHeightAction::Preset::LOW_DOCK);
    fullVisitAction->AddAction( moveLiftDownAction );
  }
  
  // 3) play anim
  if ( _configParams.lookInPlaceAnimTrigger != AnimationTrigger::Count )
  {
    IAction* imExploringAction = new TriggerLiftSafeAnimationAction(robot, _configParams.lookInPlaceAnimTrigger);
    fullVisitAction->AddAction( imExploringAction );
  }
  
  // 4) explore different angle
  IAction* turnAction2 = CreateBodyAndHeadTurnAction( robot,
    -1.0f * _configParams.bodyAngleFromSectorCenterRangeMin_deg,
    -1.0f * _configParams.bodyAngleFromSectorCenterRangeMax_deg,
    bodyTargetAngle_deg,
    _configParams.t2_headAngleAbsRangeMin_deg,
    _configParams.t2_headAngleAbsRangeMax_deg,
    _configParams.bodyTurnSpeed_degPerSec,
    _configParams.headTurnSpeed_degPerSec );
  fullVisitAction->AddAction( turnAction2 );
  
  // visit by performing sequential actions
  StartActing( fullVisitAction, runAfterAction );
  
  // update sector render because it will have changed right before deciding to visit
  UpdateSectorRender(robot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::FinishedAllSectorsAtLocation(Robot& robot)
{
  PRINT_CH_INFO("Behaviors", (GetName()).c_str(), "Finished all sectors without interruption. Flagging location");
  
  // if we completed the loop ourselves, flag this location so that we don't try again soon
  if ( _configParams.maxPreviousLocationCount > 0 )
  {
    // if we have reached the max amount of locations, remove the front one
    if ( _previousFullLocations.size() >= _configParams.maxPreviousLocationCount ) {
      _previousFullLocations.pop_front();
    }
    // add this new one
    _previousFullLocations.emplace_back( robot.GetPose() );
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
IAction* BehaviorLookInPlaceMemoryMap::CreateBodyAndHeadTurnAction(Robot& robot,
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
  PanAndTiltAction* turnAction = new PanAndTiltAction(robot, bodyTargetAngleAbs_rad, headTargetAngleAbs_rad, true, true);
  turnAction->SetMaxPanSpeed ( DEG_TO_RAD(bodyTurnSpeed_degPerSec) );
  turnAction->SetMaxTiltSpeed( DEG_TO_RAD(headTurnSpeed_degPerSec) );

  return turnAction;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookInPlaceMemoryMap::UpdateSectorRender(Robot& robot)
{
  #if ANKI_DEV_CHEATS
  {
    if ( kDrawDebugInfo )
    {
      // clear previous
      const std::string debugId("BehaviorLookInPlaceMemoryMap.kDrawDebugInfo.UpdateSectorRender");
      robot.GetContext()->GetVizManager()->EraseSegments(debugId);
      
      const ColorRGBA& needCheckColor = NamedColors::LIGHTGRAY;
      const ColorRGBA& noVisitColor   = NamedColors::DARKGRAY;
      const ColorRGBA& yesVisitColor  = NamedColors::ORANGE;
      const ColorRGBA& visitedColor   = NamedColors::BLUE;
      
      // ring distances
      const float minDist = MinCircleDist();
      const float maxDist = MaxCircleDist();
      
      // render rings
      const Point3f& renderCenter = (robot.GetPose().GetWithRespectToOrigin().GetTranslation()) + Vec3f{0,0,15.0f}; // z offset to render above mem map
      robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments(debugId,
        renderCenter, minDist, needCheckColor, false, kSectorsPerLocation);
      robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments(debugId,
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
          const Radians sectorStart_rad  = _startingBodyFacing_rad.ToFloat() + DEG_TO_RAD_F32( sectorStart_deg );
          const Rotation3d startRotateAbsAroundUp(sectorStart_rad, kUpVector);
          const Vec3f& sectorStartLine = startRotateAbsAroundUp * kFwdVector;
          const Point3f& from3D = renderCenter + sectorStartLine * minDist;
          const Point3f& to3D   = renderCenter + sectorStartLine * maxDist;
          robot.GetContext()->GetVizManager()->DrawSegment(debugId, from3D, to3D, thisSectorColor, false);
        }

        {
          const float sectorEnd_deg = (idx * anglePerSector_deg) - angleOffsetToNotOverlap_deg;
          const Radians sectorEnd_rad = _startingBodyFacing_rad.ToFloat() + DEG_TO_RAD_F32( sectorEnd_deg );
          const Rotation3d endRotateAbsAroundUp  (sectorEnd_rad, kUpVector);
          const Vec3f& sectorEndLine = endRotateAbsAroundUp * kFwdVector;
          const Point3f& from3D = renderCenter + sectorEndLine * minDist;
          const Point3f& to3D   = renderCenter + sectorEndLine * maxDist;
          robot.GetContext()->GetVizManager()->DrawSegment(debugId, from3D, to3D, thisSectorColor, false);
          
        }
      }
    }
  }
  #endif
}


} // namespace Cozmo
} // namespace Anki
