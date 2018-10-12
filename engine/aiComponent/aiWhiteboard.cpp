/**
 * File: AIWhiteboard
 *
 * Author: Raul
 * Created: 03/25/16
 *
 * Description: Whiteboard for behaviors to share information that is only relevant to them.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/aiWhiteboard.h"

#include "coretech/common/engine/math/poseOriginList.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/activeObject.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/ankiEventUtil.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/sayNameProbabilityTable.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/rotation.h"
#include "coretech/common/engine/utils/timer.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS 0

namespace Anki {
namespace Vector {

namespace {

// all coordinates have to be this close from their counterpart to be considered the same observation (and thus override it)
CONSOLE_VAR(float, kBW_PossibleObjectClose_mm, "AIWhiteboard", 50.0f);
CONSOLE_VAR(float, kBW_PossibleObjectClose_rad, "AIWhiteboard", M_PI_F); // current objects flip due to distance, consider 360 since we don't care
// limit to how many pending possible objects we have stored
CONSOLE_VAR(uint32_t, kBW_MaxPossibleObjects, "AIWhiteboard", 10);
CONSOLE_VAR(float, kFlatPosisbleObjectTol_deg, "AIWhiteboard", 10.0f);
CONSOLE_VAR(float, kBW_MaxHeightForPossibleObject_mm, "AIWhiteboard", 30.0f);
// debug render
CONSOLE_VAR(bool, kBW_DebugRenderPossibleObjects, "AIWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderPossibleObjectsZ, "AIWhiteboard", 35.0f);
CONSOLE_VAR(bool, kBW_DebugRenderBeacons, "AIWhiteboard", true);
CONSOLE_VAR(float, kBW_DebugRenderBeaconZ, "AIWhiteboard", 35.0f);

CONSOLE_VAR(int, kMaxObservationAgeToEatCube_ms, "AIWhiteboard", 3000);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ObjectActionFailureToString(AIWhiteboard::ObjectActionFailure action)
{
  using ObjectActionFailure = AIWhiteboard::ObjectActionFailure;
  switch(action) {
    case ObjectActionFailure::PickUpObject : { return "PickUp";  }
    case ObjectActionFailure::StackOnObject: { return "StackOn"; }
    case ObjectActionFailure::PlaceObjectAt: { return "PlaceAt"; }
    case ObjectActionFailure::RollOrPopAWheelie: { return "RollOrPop";}
  };
  
  // should never get here, assert if it does (programmer error specifying action enum class)
  DEV_ASSERT(false, "AIWhiteboard.ObjectUseActionToString.InvalidAction");
  return "UNDEFINED_ERROR";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// return how many max entries of the given type we store. DO NOT return 0, at least there has to be 1
size_t GetObjectFailureListMaxSize(AIWhiteboard::ObjectActionFailure action)
{
  using ObjectActionFailure = AIWhiteboard::ObjectActionFailure;
  switch(action) {
    case ObjectActionFailure::PickUpObject :     { return 1; }
    case ObjectActionFailure::StackOnObject:     { return 1; }
    case ObjectActionFailure::PlaceObjectAt:     { return 10;} // this can affect behaviorExploreBringCubeToBeacon's kLocationFailureDist_mm (read note there)
    case ObjectActionFailure::RollOrPopAWheelie: { return 1; }
  };
  
  // should never get here, assert if it does (programmer error specifying action enum class)
  DEV_ASSERT(false, "AIWhiteboard.GetObjectFailureListMaxSize.InvalidAction");
  return 0;
}
  
} // namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIWhiteboard
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::AIWhiteboard(Robot& robot)
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::Whiteboard)
, _robot(robot)
, _gotOffChargerAtTime_sec(-1.0f)
, _returnedToTreadsAtTime_sec(-1.0f)
, _edgeInfoTime_sec(-1.0f)
, _edgeInfoClosestEdge_mm(-1.0f)
, _sayNameProbTable(new SayNameProbabilityTable(*_robot.GetContext()->GetRandom()))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::~AIWhiteboard()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::InitDependent(Vector::Robot* robot, const AICompMap& dependentComps)
{
  // register to possible object events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotObservedPossibleObject>();
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotOffTreadsStateChanged>();
  }
  else {
    PRINT_NAMED_WARNING("AIWhiteboard.Init", "Initialized whiteboard with no external interface. Will miss events.");
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdateDependent(const AICompMap& dependentComps)
{
  // TODO:(bn) cache this filter

  // Victor wants to eat cube #1 if it's known and upright
  
  BlockWorldFilter filter;
  filter.SetAllowedTypes( { ObjectType::Block_LIGHTCUBE1 } );
  filter.SetFilterFcn( [this](const ObservableObject* obj){
      return obj->IsPoseStateKnown() &&
        obj->GetPose().GetWithRespectToRoot().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS &&
        obj->GetLastObservedTime() + kMaxObservationAgeToEatCube_ms > _robot.GetLastImageTimeStamp();

    });

  const auto& blockWorld = _robot.GetBlockWorld();
  const ObservableObject* obj = blockWorld.FindLocatedObjectClosestTo(_robot.GetPose(), filter);
  if( obj != nullptr ) {
    _victor_cubeToEat = obj->GetID();
  }
  else {
    _victor_cubeToEat.UnSet();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OnRobotDelocalized()
{
  RemovePossibleObjectsFromZombieMaps();

  UpdatePossibleObjectRender();
  
  // TODO rsam we probably want to rematch beacons when robot relocalizes.
  ClearAllBeacons();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OnRobotRelocalized()
{
  // just need to update render, otherwise they render wrt old origin
  UpdatePossibleObjectRender();

  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OnRobotWakeUp()
{
  PRINT_CH_DEBUG("AIWhiteboard", "AIWhiteBoard.OnRobotWakeUp.ResetSayNameProbTable", "");
  _sayNameProbTable->Reset();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ProcessClearQuad(const Quad2f& quad)
{
  const Pose3d& worldOrigin = _robot.GetWorldOrigin();
  // remove all objects inside clear quads
  auto isObjectInsideQuad = [&quad, worldOrigin](const PossibleObject& possibleObj)
  {
    Pose3d relPose;
    if ( possibleObj.pose.GetWithRespectTo(worldOrigin, relPose) ) {
      return quad.Contains( relPose.GetTranslation() );
    } else {
      return false;
    }
  };
  const size_t countBefore = _possibleObjects.size();
  _possibleObjects.remove_if( isObjectInsideQuad );
  const size_t countAfter = _possibleObjects.size();
  
  // update render if we removed something
  if ( countBefore != countAfter ) {
    UpdatePossibleObjectRender();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::FinishedSearchForPossibleCubeAtPose(ObjectType objectType, const Pose3d& pose)
{
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.FinishedSearch",
                  "finished search, so removing possible object");
  }
  RemovePossibleObjectsMatching(objectType, pose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::FindUsableCubesOutOfBeacons(ObjectInfoList& outObjectList) const
{
  if(ANKI_DEVELOPER_CODE)
  {
    // all beacons should be in this world
    for ( const auto& beacon : _beacons ) {
      DEV_ASSERT(_robot.IsPoseInWorldOrigin( beacon.GetPose() ),
                 "AIWhiteboard.FindUsableCubesOutOfBeacons.DirtyBeacons");
    }
  }

  outObjectList.clear();
  if ( !_beacons.empty() )
  {
    // if the robot is currently carrying a cube, insert only that one right away, regardless of whether it's inside or
    // outisde the beacon, since we would always want to drop it. Any other cube would be unusable until we drop this one
    if ( _robot.GetCarryingComponent().IsCarryingObject() )
    {
      const ObservableObject* const carryingObject = _robot.GetBlockWorld().GetLocatedObjectByID( _robot.GetCarryingComponent().GetCarryingObject() );
      if ( nullptr != carryingObject ) {
        outObjectList.emplace_back( carryingObject->GetID(), carryingObject->GetFamily() );
      } else {
        // this can be blocking under certain scenarios, since we can think we are carrying a cube, but it is not in
        // the blockworld
        PRINT_NAMED_ERROR("AIWhiteboard.FindUsableCubesOutOfBeacons.NullCarryingObject",
                          "Could not get carrying object pointer (ID=%d)",
                          _robot.GetCarryingComponent().GetCarryingObject().GetValue());
      }
    }
    else
    {
      // ask for all cubes we know, and if any is not inside a beacon, add to the list
      BlockWorldFilter filter;
      filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
      filter.AddFilterFcn([this, &outObjectList](const ObservableObject* blockPtr) {
        // check if the robot can pick up this object
        const bool canPickUp = _robot.GetDockingComponent().CanPickUpObject(*blockPtr);
        if ( canPickUp )
        {
          bool isBlockInAnyBeacon = false;
          // check if the object is within any beacon
          for ( const auto& beacon : _beacons ) {
            isBlockInAnyBeacon = beacon.IsLocWithinBeacon(blockPtr->GetPose());
            if ( isBlockInAnyBeacon ) {
              break;
            }
          }
          
          // this block should be carried to a beacon
          if ( !isBlockInAnyBeacon ) {
            outObjectList.emplace_back( blockPtr->GetID(), blockPtr->GetFamily() );
          }
        }
        return false; // have to return true/false, even though not used
      });
      
      _robot.GetBlockWorld().FilterLocatedObjects(filter);
    }
  }

  // do we have any usable cubes out of beacons?
  bool hasBlocksOutOfBeacons = !outObjectList.empty();
  return hasBlocksOutOfBeacons;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::FindCubesInBeacon(const AIBeacon* beacon, ObjectInfoList& outObjectList) const
{
  #if ANKI_DEVELOPER_CODE
  {
    // passed beacon should be a beacon in our list, not null and in the current origin
    bool beaconIsValid = false;
    for ( const auto& beaconIt : _beacons ) {
      beaconIsValid = beaconIsValid || (beacon == &beaconIt);
    }
    DEV_ASSERT(beaconIsValid, "AIWhiteboard.FindCubesInBeacon.NotABeacon");
    DEV_ASSERT(beacon, "AIWhiteboard.FindCubesInBeacon.NullBeacon");
    DEV_ASSERT(_robot.IsPoseInWorldOrigin( beacon->GetPose() ),
               "AIWhiteboard.FindCubesInBeacon.BeaconNotInOrigin");
  }
  #endif
  
  outObjectList.clear();
  
  {
    // ask for all cubes we know about inside the beacon
    BlockWorldFilter filter;
    filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    filter.AddFilterFcn([this, &outObjectList, beacon](const ObservableObject* blockPtr)
    {
      if(!_robot.GetCarryingComponent().IsCarryingObject(blockPtr->GetID()) )
      {
        const bool isBlockInBeacon = beacon->IsLocWithinBeacon(blockPtr->GetPose());
        if ( isBlockInBeacon ) {
          outObjectList.emplace_back( blockPtr->GetID(), blockPtr->GetFamily() );
        }
      }
      return false; // have to return true/false, even though not used
    });
    
    _robot.GetBlockWorld().FilterLocatedObjects(filter);
  }
  
  return !outObjectList.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::AreAllCubesInBeacons() const
{
  #if ANKI_DEVELOPER_CODE
  {
    // all beacons should be in this world
    for ( const auto& beacon : _beacons ) {
      DEV_ASSERT(_robot.IsPoseInWorldOrigin( beacon.GetPose().FindRoot() ),
                 "AIWhiteboard.FindUsableCubesOutOfBeacons.DirtyBeacons");
    }
  }
  #endif

  bool allInBeacon = false;
  if ( !_beacons.empty() )
  {
    // robot can't be carrying an object, otherwise they are not in beacons
    if ( !_robot.GetCarryingComponent().IsCarryingObject() )
    {
      size_t locatedCubesInBeacon = 0;
      size_t allDefinedCubes = 0;
      
      // ask located cubes which ones are in a beacon. Note some cubes might be in the beacon already, but
      // if Cozmo doesn't know about them he won't count them before detecting them.
      {
        BlockWorldFilter filter;
        filter.SetAllowedFamilies({ ObjectFamily::LightCube });
        filter.AddFilterFcn([this, &locatedCubesInBeacon](const ObservableObject* blockPtr)
        {
          bool isBlockInAnyBeacon = false;
          if (blockPtr->IsPoseStateKnown()){
            // check if the object is within any beacon
            for ( const auto& beacon : _beacons ) {
              isBlockInAnyBeacon = beacon.IsLocWithinBeacon(blockPtr->GetPose());
              if ( isBlockInAnyBeacon ) {
                break;
              }
            }
          }
        
          // if the block is in a beacon, count as located
          if ( isBlockInAnyBeacon ) {
            ++locatedCubesInBeacon;
          }
          return false; // have to return true/false, even though not used
        });
        
        _robot.GetBlockWorld().FilterLocatedObjects(filter);
      }
      
      // Find out how many defined cubes we have and compare
      allDefinedCubes = _robot.GetBlockWorld().GetNumDefinedObjects(ObjectFamily::LightCube);
      
      allInBeacon = (allDefinedCubes == locatedCubesInBeacon);
    }
  }

  // return the result of the filter
  return allInBeacon;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetFailedToUse(const ObservableObject& object, ObjectActionFailure action)
{
  // PlaceObjectAt should provide location
  DEV_ASSERT(action != ObjectActionFailure::PlaceObjectAt, "AIWhiteboard.SetFailedToUse.PlaceObjectAtRequiresLocation");

  // use object current pose
  const Pose3d& objectPose = object.GetPose();
  SetFailedToUse(object, action, objectPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetFailedToUse(const ObservableObject& object, ObjectActionFailure action, const Pose3d& atLocation)
{
  // simply store failure's timestamp
  ObjectFailureTable& failureTableForAction = GetObjectFailureTable(action);
  FailureList& failureListForObj = failureTableForAction[object.GetID().GetValue()];
  
  // check if we need to remove an entry because we have reached the maximum
  const size_t maxItemsInList = GetObjectFailureListMaxSize(action);
  DEV_ASSERT(maxItemsInList > 0, "AIWhiteboard.SetFailedToUse.MaxIs0");
  if ( failureListForObj.size() >= maxItemsInList )
  {
    // can't have an empty list or a list bigger than the max (so it has to be equal to the max)
    DEV_ASSERT(!failureListForObj.empty(), "AIWhiteboard.SetFailedToUse.EmptyListReachedMax");
    DEV_ASSERT(failureListForObj.size() == maxItemsInList, "AIWhiteboard.SetFailedToUse.ListBeyondMax");
    
    // print the one we are removing
    PRINT_CH_INFO("AIWhiteboard", "SetFailedToUse",
      "Removed failure on ['%s'] for object '%d' atPose (%.2f,%.2f,%.2f) atTime (%fs) to add one (we are at the max=%zu)",
      ObjectActionFailureToString(action),
      object.GetID().GetValue(),
      failureListForObj.begin()->_pose.GetTranslation().x(),
      failureListForObj.begin()->_pose.GetTranslation().y(),
      failureListForObj.begin()->_pose.GetTranslation().z(),
      failureListForObj.begin()->_timestampSecs,
      maxItemsInList);
  
    // remove first
    failureListForObj.pop_front();
  }

  // always add
  {
    // insert at back (newest)
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    failureListForObj.emplace_back(atLocation, curTime);
    
    // log the new one
    PRINT_CH_INFO("AIWhiteboard", "SetFailedToUse",
      "Added failure on ['%s'] for object '%d' atPose (%.2f,%.2f,%.2f) atTime (%fs) ",
      ObjectActionFailureToString(action),
      object.GetID().GetValue(),
      atLocation.GetTranslation().x(),
      atLocation.GetTranslation().y(),
      atLocation.GetTranslation().z(),
      curTime);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectActionFailure action) const
{
  const bool ret = DidFailToUse(objectID, FailureReasonsContainer{action}, -1.f, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectActionFailure action, float recentSecs) const
{
  const bool ret = DidFailToUse(objectID, FailureReasonsContainer{action}, recentSecs, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectActionFailure action,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  const bool ret = DidFailToUse(objectID, FailureReasonsContainer{action}, -1.f, atPose, distThreshold_mm, angleThreshold);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, ObjectActionFailure action, float recentSecs,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  const bool ret = DidFailToUse(objectID, FailureReasonsContainer{action}, recentSecs, atPose, distThreshold_mm, angleThreshold);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::FailureReasonsContainer reasons) const
{
  const bool ret = DidFailToUse(objectID, reasons, -1.f, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::FailureReasonsContainer reasons, float recentSecs) const
{
  const bool ret = DidFailToUse(objectID, reasons, recentSecs, Pose3d(), -1.f, Radians());
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::FailureReasonsContainer reasons,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  const bool ret = DidFailToUse(objectID, reasons, -1.f, atPose, distThreshold_mm, angleThreshold);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::DidFailToUse(const int objectID, AIWhiteboard::FailureReasonsContainer reasons, float recentSecs,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  for( const auto& reason : reasons ) {
    const ObjectFailureTable& failureTable = GetObjectFailureTable(reason);
    const bool ret = FindMatchingEntry(failureTable, objectID, recentSecs, atPose, distThreshold_mm, angleThreshold);
    if( ret ) {
      return true;
    }
  }

  // not found for any passed in reason
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::FindMatchingEntry(const ObjectFailureTable& failureTable, const int objectID,
  float recentSecs,
  const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  // if we are looking for failures in any object
  if ( objectID == ANY_OBJECT )
  {
    // iterate all the queues for all objects
    for( const auto& pair : failureTable )
    {
      // iterate all entries in each queue
      const FailureList& failuresForObj = pair.second;
      for( const FailureInfo& failureInfo : failuresForObj)
      {
        // check if the entry matches the search
        const bool matchesSearch = EntryMatches(failureInfo, recentSecs, atPose, distThreshold_mm, angleThreshold);
        if ( matchesSearch ) {
          // it does, we found a failure we care about
          return true;
        }
      }
    }
    
    // none of the current entries for any object matched
    return false;
  }
  else
  {
    // find failures for the specific object
    const auto& failuresForObjIt = failureTable.find( objectID );
    if ( failuresForObjIt != failureTable.end() )
    {
      // iterate all entries in this queue
      const FailureList& failuresForObj = failuresForObjIt->second;
      for( const FailureInfo& failureInfo : failuresForObj)
      {
        // check if the entry matches the search
        const bool matchesSearch = EntryMatches(failureInfo, recentSecs, atPose, distThreshold_mm, angleThreshold);
        if ( matchesSearch ) {
          // it does, we found a failure we care about
          return true;
        }
      }
    }
    
    // none of the current entries for this object matched
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::EntryMatches(const FailureInfo& entry, float recentSecs, const Pose3d& atPose, float distThreshold_mm, const Radians& angleThreshold) const
{
  // if we specified a timestamp (ie: if we care about recent entries only)
  const bool checkTime = FLT_GE(recentSecs, 0.0f);
  if ( checkTime )
  {
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceFailureSecs = curTime - entry._timestampSecs;
    const bool entryIsTooOld = FLT_GE(timeSinceFailureSecs, recentSecs);
    if ( entryIsTooOld ) {
      // the entry is too old, we do not care about it
      return false;
    }
  }
  
  // passed time checks, check pose
  const bool checkLocation = FLT_GE(distThreshold_mm, 0.0f);
  if ( checkLocation )
  {
    const bool arePosesClose = entry._pose.IsSameAs(atPose, Point3f(distThreshold_mm), angleThreshold);
    if ( !arePosesClose ) {
      // the entry is too far, we do not care about it
      return false;
    }
  }
  
  // entry passed all checks, this entry does match the search
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AIWhiteboard::ObjectFailureTable& AIWhiteboard::GetObjectFailureTable(AIWhiteboard::ObjectActionFailure action) const
{
  switch(action) {
    case ObjectActionFailure::PickUpObject : { return _pickUpFailures;  }
    case ObjectActionFailure::StackOnObject: { return _stackOnFailures; }
    case ObjectActionFailure::PlaceObjectAt: { return _placeAtFailures; }
    case ObjectActionFailure::RollOrPopAWheelie: { return _rollOrPopFailures;}
  }

  // should never get here, assert if it does (programmer error specifying action enum class)
  {
    DEV_ASSERT(false, "AIWhiteboard.GetObjectFailureTable.InvalidAction");
    static ObjectFailureTable empty;
    return empty;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIWhiteboard::ObjectFailureTable& AIWhiteboard::GetObjectFailureTable(AIWhiteboard::ObjectActionFailure action)
{
  switch(action) {
    case ObjectActionFailure::PickUpObject : { return _pickUpFailures;  }
    case ObjectActionFailure::StackOnObject: { return _stackOnFailures; }
    case ObjectActionFailure::PlaceObjectAt: { return _placeAtFailures; }
    case ObjectActionFailure::RollOrPopAWheelie: { return _rollOrPopFailures;}
  }

  // should never get here, assert if it does (programmer error specifying action enum class)
  {
    DEV_ASSERT(false, "AIWhiteboard.GetObjectFailureTable.InvalidAction");
    static ObjectFailureTable empty;
    return empty;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::GetPossibleObjectsWRTOrigin(PossibleObjectVector& possibleObjects) const
{
  possibleObjects.clear();
  
  // iterate all possible objects
  const Pose3d& worldOrigin = _robot.GetWorldOrigin();
  for( const auto& possibleObject : _possibleObjects )
  {
    // if we can obtain a pose with respect to the current origin, store that output (relative pose and type)
    Pose3d poseWRTOrigin;
    if ( possibleObject.pose.GetWithRespectTo(worldOrigin, poseWRTOrigin) )
    {
      possibleObjects.emplace_back( poseWRTOrigin, possibleObject.type );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::AddBeacon( const Pose3d& beaconPos, const float radius )
{
  _beacons.emplace_back( beaconPos, radius );

  // update render
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ClearAllBeacons()
{
  _beacons.clear();
  
  // update render
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::FailedToFindLocationInBeacon(AIBeacon* beacon)
{
  // set time stamp in the beacon
  beacon->FailedToFindLocation();
  
  // update render
  UpdateBeaconRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const AIBeacon* AIWhiteboard::GetActiveBeacon() const
{
  if ( _beacons.empty() ) {
    return nullptr;
  }
  
  return &_beacons[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIBeacon* AIWhiteboard::GetActiveBeacon()
{
  if ( _beacons.empty() ) {
    return nullptr;
  }
  
  return &_beacons[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::RemovePossibleObjectsMatching(ObjectType objectType, const Pose3d& pose)
{
  // iterate all current possible objects
  auto possibleObjectIt = _possibleObjects.begin();
  while ( possibleObjectIt != _possibleObjects.end() )
  {
    // compare type (there shouldn't be many of the same type, but can happen if we see more than one possible object
    // for the same cube)
    if ( possibleObjectIt->type == objectType )
    {
      Pose3d relPose;
      if ( possibleObjectIt->pose.GetWithRespectTo(pose, relPose) )
      {
        // compare locations
        const float distSQ = kBW_PossibleObjectClose_mm*kBW_PossibleObjectClose_mm;
        const bool isClosePossibleObject = FLT_LE(relPose.GetTranslation().LengthSq(), distSQ );
        if ( isClosePossibleObject )
        {
          if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
            PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Remove",
                              "removing possible object that was at (%f, %f, %f), matching object type %d",
                              relPose.GetTranslation().x(),
                              relPose.GetTranslation().y(),
                              relPose.GetTranslation().z(),
                              objectType);
          }

          // they are close, remove old entry
          possibleObjectIt = _possibleObjects.erase( possibleObjectIt );
          // jump up to not increment iterator after erase
          continue;
        }
      }
    }

    // not erased, increment iter
    ++possibleObjectIt;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::RemovePossibleObjectsFromZombieMaps()
{
  PossibleObjectList::iterator iter = _possibleObjects.begin();
  while( iter != _possibleObjects.end() )
  {
    const PoseOriginID_t objOriginID = iter->pose.GetRootID();
    DEV_ASSERT_MSG(_robot.GetPoseOriginList().ContainsOriginID(objOriginID),
                   "AIWhiteboard.RemovePossibleObjectsFromZombieMaps.ObjectOriginNotInOriginList",
                   "ID:%d", objOriginID);
    const bool isZombie = _robot.GetBlockWorld().IsZombiePoseOrigin(objOriginID);
    if ( isZombie ) {
      if ( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
        PRINT_CH_INFO("AIWhiteboard", "RemovePossibleObjectsFromZombieMaps", "Deleted possible object because it was zombie");
      }
     iter = _possibleObjects.erase(iter);
    } else {
      ++iter;
    }
  }
  if ( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "RemovePossibleObjectsFromZombieMaps", "%zu possible objects not zombie", _possibleObjects.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotObservedObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg;
  
  Pose3d obsPose( msg.pose, _robot.GetPoseOriginList() );
  
  // iterate objects we previously had and remove them if we think they belong to this object
  RemovePossibleObjectsMatching(possibleObject.objectType, obsPose);

  // update render
  UpdatePossibleObjectRender();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotObservedPossibleObject& msg)
{
  const ExternalInterface::RobotObservedObject& possibleObject = msg.possibleObject;
  
  Pose3d obsPose( msg.possibleObject.pose, _robot.GetPoseOriginList() );
  
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "ObservedPossible", "robot observed a possible object");
  }

  ConsiderNewPossibleObject(possibleObject.objectType, obsPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void AIWhiteboard::HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged& msg)
{
  const bool onTreads = msg.treadsState == OffTreadsState::OnTreads;
  if ( onTreads ) {
    _returnedToTreadsAtTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ConsiderNewPossibleObject(ObjectType objectType, const Pose3d& obsPose)
{
  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Consider",
                      "considering pose (%f, %f, %f)",
                      obsPose.GetTranslation().x(),
                      obsPose.GetTranslation().y(),
                      obsPose.GetTranslation().z());
  }
  
  // only add relatively flat objects
  const RotationMatrix3d Rmat = obsPose.GetRotationMatrix();
  const bool isFlat = (Rmat.GetAngularDeviationFromParentAxis<'Z'>() < DEG_TO_RAD(kFlatPosisbleObjectTol_deg));

  if( ! isFlat ) {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.NotFlat",
                        "pose isn't flat, so not adding to list (angle %fdeg)",
                        Rmat.GetAngularDeviationFromParentAxis<'Z'>().getDegrees());
    }

    return;
  }

  Pose3d relPose;
  if( ! obsPose.GetWithRespectTo(_robot.GetPose(), relPose) ) {
    PRINT_NAMED_WARNING("AIWhiteboard.PossibleObject.NoRelPose",
                        "Couldnt get pose WRT to robot");
    return;
  }

  // can't really use size since this is just a pose
  // TODO:(bn) get the size based on obectType so this can be relative to the bottom of the object?
  if( relPose.GetTranslation().z() > kBW_MaxHeightForPossibleObject_mm ) {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.TooHigh",
                        "pose is too high, not considering. relativeZ = %f",
                        relPose.GetTranslation().z());
    }

    return;
  }

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.RemovingOldObjects",
                      "removing any old possible objects that are nearby");
  }

  // remove any objects that are similar to this
  RemovePossibleObjectsMatching(objectType, obsPose);
  
  // check with the world if this a possible marker for an object we have already recognized, because in that case we
  // are not interested in storing it again.
  
  Vec3f maxLocDist(kBW_PossibleObjectClose_mm);
  Radians maxLocAngle(kBW_PossibleObjectClose_rad);
  ObservableObject* prevObservedObject =
    _robot.GetBlockWorld().FindLocatedClosestMatchingObject( objectType, obsPose, maxLocDist, maxLocAngle);
  
  // found object nearby, no need
  if ( nullptr != prevObservedObject )
  {
    if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
      PRINT_CH_INFO("AIWhiteboard", "PossibleObject.NearbyObject",
                        "Already had a real(known) object nearby, not adding a new one");
    }

    return;
  }

  // if we are at the limit of stored objects, remove the front one
  if ( _possibleObjects.size() >= kBW_MaxPossibleObjects )
  {
    // we reached the limit, remove oldest entry
    DEV_ASSERT(!_possibleObjects.empty(), "AIWhiteboard.HandleMessage.ReachedLimitEmpty");
    _possibleObjects.pop_front();
    // PRINT_NAMED_INFO("AIWhiteboard.HandleMessage.BeyondObjectLimit", "Reached limit of pending objects, removing oldest one");
  }

  if( DEBUG_AI_WHITEBOARD_POSSIBLE_OBJECTS ) {
    PRINT_CH_INFO("AIWhiteboard", "PossibleObject.Add",
                      "added possible object. rot=%fdeg, z=%f",
                      Rmat.GetAngularDeviationFromParentAxis<'Z'>().getDegrees(),
                      relPose.GetTranslation().z());
  }

  
  // always add new entry
  _possibleObjects.emplace_back( obsPose, objectType );
  
  // update render
  UpdatePossibleObjectRender();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdatePossibleObjectRender()
{
  if ( kBW_DebugRenderPossibleObjects )
  {
    const Pose3d& worldOrigin = _robot.GetWorldOrigin();
    _robot.GetContext()->GetVizManager()->EraseSegments("AIWhiteboard.PossibleObjects");
    for ( auto& possibleObjectIt : _possibleObjects )
    {
      // this offset should not be applied pose
      const Vec3f& zRenderOffset = (Z_AXIS_3D() * kBW_DebugRenderPossibleObjectsZ);
      
      Pose3d thisPose;
      if ( possibleObjectIt.pose.GetWithRespectTo(worldOrigin, thisPose))
      {
        const float kLen = kBW_PossibleObjectClose_mm;
        Quad3f quad3({ kLen,  kLen, 0},
                     {-kLen,  kLen, 0},
                     { kLen, -kLen, 0},
                     {-kLen, -kLen, 0});
        thisPose.ApplyTo(quad3, quad3);
        quad3 += zRenderOffset;
        _robot.GetContext()->GetVizManager()->DrawQuadAsSegments("AIWhiteboard.PossibleObjects",
            quad3, NamedColors::ORANGE, false);

        Vec3f directionEndPoint = X_AXIS_3D() * kLen*0.5f;
        directionEndPoint = thisPose * directionEndPoint;
        _robot.GetContext()->GetVizManager()->DrawSegment("AIWhiteboard.PossibleObjects",
            thisPose.GetTranslation() + zRenderOffset, directionEndPoint + zRenderOffset, NamedColors::YELLOW, false);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::UpdateBeaconRender()
{
  // re-draw all beacons since they all use the same id
  if ( kBW_DebugRenderBeacons )
  {
    static const std::string renderId("AIWhiteboard.UpdateBeaconRender");
    _robot.GetContext()->GetVizManager()->EraseSegments(renderId);
  
    // iterate all beacons and render
    for( const auto& beacon : _beacons )
    {
      // currently we don't support beacons from older origins (rsam: I will soon)
      DEV_ASSERT(_robot.IsPoseInWorldOrigin( beacon.GetPose() ),
                 "AIWhiteboard.UpdateBeaconRender.BeaconFromOldOrigin");
      
      // note that since we don't know what timeout behaviors use, we can only say that it ever failed
      const ColorRGBA& color = NEAR_ZERO(beacon.GetLastTimeFailedToFindLocation()) ? NamedColors::DARKGREEN : NamedColors::ORANGE;
      
      Vec3f center = beacon.GetPose().GetWithRespectToRoot().GetTranslation();
      center.z() += kBW_DebugRenderBeaconZ;
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius(), color, false);
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius()-0.5f, color, false); // double line
      _robot.GetContext()->GetVizManager()->DrawXYCircleAsSegments("AIWhiteboard.UpdateBeaconRender",
          center, beacon.GetRadius()-1.0f, color, false); // triple line (jane's request)
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::OfferPostBehaviorSuggestion( const PostBehaviorSuggestions& suggestion )
{
  ANKI_VERIFY( suggestion != PostBehaviorSuggestions::Invalid,
               "AIWhiteboard.OfferPostBehaviorSuggestion.Invalid",
               "The 'Invalid' suggestion was offered" );
  _postBehaviorSuggestions[ suggestion ] = BaseStationTimer::getInstance()->GetTickCount();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIWhiteboard::GetPostBehaviorSuggestion( const PostBehaviorSuggestions& suggestion, size_t& tick ) const
{
  const auto it = _postBehaviorSuggestions.find( suggestion );
  const bool exists = it != _postBehaviorSuggestions.end();
  if( exists ) {
    tick = it->second;
  }
  return exists;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::ClearPostBehaviorSuggestions()
{
  _postBehaviorSuggestions.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIWhiteboard::SetLastEdgeInformation(const float closestEdgeDist_mm)
{
  _edgeInfoTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _edgeInfoClosestEdge_mm = closestEdgeDist_mm;
}


} // namespace Vector
} // namespace Anki
