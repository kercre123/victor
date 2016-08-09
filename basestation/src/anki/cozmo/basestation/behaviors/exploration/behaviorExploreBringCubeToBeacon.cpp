/**
 * File: behaviorExploreBringCubeToBeacon
 *
 * Author: Raul
 * Created: 04/14/16
 *
 * Description: Behavior to pick up and bring a cube to the current beacon.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "behaviorExploreBringCubeToBeacon.h"

//#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIBeacon.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

#include <array>

namespace Anki {
namespace Cozmo {

// padding between cubes when calculating destination positions inside a beacon
CONSOLE_VAR(float, kBebctb_PaddingBetweenCubes_mm, "BehaviorExploreBringCubeToBeacon", 10.0f);
// debug render for the behavior
CONSOLE_VAR(float, kBebctb_DebugRenderAll, "BehaviorExploreBringCubeToBeacon", true);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// internal helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LocationCalculator: given row and column can calculate a 3d pose in a beacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct LocationCalculator {
  LocationCalculator(const ObservableObject* pickedUpObject, const Vec3f& beaconCenter, const Rotation3d& directionality, float beaconRadius, const Robot& robot)
  : object(pickedUpObject), center(beaconCenter), rotation(directionality), radiusSQ(beaconRadius*beaconRadius), robotRef(robot), isFirstFind(true)
  {
    ASSERT_NAMED( nullptr != object, "BehaviorExploreBringCubeToBeacon.LocationCalculator.NullObjectWillCrash" );
  }
  
  const ObservableObject* object;
  const Vec3f& center;
  const Rotation3d& rotation;
  const float radiusSQ;
  const Robot& robotRef;
  mutable bool isFirstFind;

  // calculate location offset given current config
  float GetLocationOffset() const;
  
  // compute pose for row and col, and return true if it's free to drop a cube, false otherwise
  bool IsLocationFreeForObject(const int row, const int col, Pose3d& outPose) const;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float LocationCalculator::GetLocationOffset() const
{
  const float cubeLen = object->GetSize().x();
  ASSERT_NAMED( FLT_NEAR(object->GetSize().x(), object->GetSize().y()) , "LocationCalculator.GetLocationOffset");
  return cubeLen + kBebctb_PaddingBetweenCubes_mm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LocationCalculator::IsLocationFreeForObject(const int row, const int col, Pose3d& outPose) const
{
  // calculate 3D location
  const float locOffset = GetLocationOffset();
  
  Vec3f offset{ locOffset*row, locOffset*col, 0.0f};
  offset = rotation * offset;
  Vec3f candidateLoc = center + offset;
  
  // check if out of radius
  if ( offset.LengthSq() > radiusSQ )
  {
    // debug render
    if ( kBebctb_DebugRenderAll )
    {
      outPose = Pose3d(rotation, candidateLoc, &robotRef.GetPose().FindOrigin());
      Quad2f candidateQuad = object->GetBoundingQuadXY(outPose);
      robotRef.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorExploreBringCubeToBeacon.Locations",
        candidateQuad, 20.0f, NamedColors::BLACK, false);
    }
    return false;
  }

  bool isFree = true;
  
  // calculate if candidate pose is free for this object
  {
    // note: this part of the code is similar to the DriveToPlaceCarriedObjectAction free goal check, standarize?
    BlockWorldFilter ignoreSelfFilter;
    ignoreSelfFilter.AddIgnoreID( object->GetID() );
    
    // calculate quad at candidate destination
    outPose = Pose3d(rotation, candidateLoc, &robotRef.GetPose().FindOrigin());
    Quad2f candidateQuad = object->GetBoundingQuadXY(outPose);

    // TODO rsam: this only checks for other cubes, but not for unknown obstacles since we don't have collision sensor
    std::vector<const ObservableObject *> intersectingObjects;
    robotRef.GetBlockWorld().FindIntersectingObjects(candidateQuad, intersectingObjects, kBebctb_PaddingBetweenCubes_mm, ignoreSelfFilter);
    
    isFree = intersectingObjects.empty();

    // debug render
    if ( kBebctb_DebugRenderAll ) {
      robotRef.GetContext()->GetVizManager()->DrawQuadAsSegments("BehaviorExploreBringCubeToBeacon.Locations",
        candidateQuad, 20.0f, isFree ? (isFirstFind ? NamedColors::YELLOW : NamedColors::WHITE) : NamedColors::RED, false);
    }
    isFirstFind = isFirstFind && !isFree;
  }
  
  return isFree;
}

} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorExploreBringCubeToBeacon
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreBringCubeToBeacon::BehaviorExploreBringCubeToBeacon(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("BehaviorExploreBringCubeToBeacon");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreBringCubeToBeacon::~BehaviorExploreBringCubeToBeacon()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreBringCubeToBeacon::IsRunnableInternal(const Robot& robot) const
{
  // check if any known cubes are not currently in a valid beacon
  const AIWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
  const AIWhiteboard::BeaconList& beaconList = whiteboard.GetBeacons();
  
  #if ANKI_DEVELOPER_CODE
  {
    // we only want to receive the current beacons
    for ( const auto& beacon : beaconList ) {
      ASSERT_NAMED(&beacon.GetPose().FindOrigin() == robot.GetWorldOrigin(), "Whiteboard's beacons are dirty!");
    }
  }
  #endif

  _candidateObjects.clear();
  if ( !beaconList.empty() )
  {
    // if we are currently a cube, validate right away, regardless of whether it's inside or outisde the cube.
    // We would always want to drop it
    if ( robot.IsCarryingObject() )
    {
      const ObservableObject* const carryingObject = robot.GetBlockWorld().GetObjectByID( robot.GetCarryingObject() );
      if ( nullptr != carryingObject ) {
        _candidateObjects.emplace_back( carryingObject->GetID(), carryingObject->GetFamily() );
      } else {
        // this can block us, since we don't want to try to run. We think we are carrying an object but it does not
        // exist in the world?
        PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.IsRunnable.CarryingNullObject",
          "Could not get carrying object pointer");
      }
    }
    else
    {
      // ask for all cubes we know, and if any is not inside a beacon, then we want to bring that one to the closest beacon
      BlockWorldFilter filter;
      filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
      filter.SetFilterFcn([this,&beaconList](const ObservableObject* blockPtr) {
        if(!blockPtr->IsPoseStateUnknown())
        {
          bool isBlockInAnyBeacon = false;
          
          // check if the object is within any beacon
          for ( const auto& beacon : beaconList ) {
            isBlockInAnyBeacon = beacon.IsLocWithinBeacon(blockPtr->GetPose());
            if ( isBlockInAnyBeacon ) {
              break;
            }
          }
          
          // this block should be carried to a beacon
          if ( !isBlockInAnyBeacon ) {
            _candidateObjects.emplace_back( blockPtr->GetID(), blockPtr->GetFamily() );
          }
        }
        return false; // have to return true/false, even though not used
      });
      
      robot.GetBlockWorld().FilterObjects(filter);
    }
  }

  // do we have blocks that we want to bring to a beacon?
  bool hasBlocksOutOfBeacons = !_candidateObjects.empty();
  return hasBlocksOutOfBeacons;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreBringCubeToBeacon::InitInternal(Robot& robot)
{
  // clear variables from previous run
  _selectedObjectID.UnSet();
  _invalidCubesToStackOn.clear();
  
  // calculate starting state
  if ( robot.IsCarryingObject() )
  {
    // we are carrying an object
    // assert what we expect from IsRunnable cache
    ASSERT_NAMED( _candidateObjects.size() == 1 && _candidateObjects[0].id == robot.GetCarryingObject(),
      "BehaviorExploreBringCubeToBeacon.InitInternal.CarryingObjectNotCached" );
    
    // select it and pretend we just picked it up
    _selectedObjectID = _candidateObjects[0].id;
    TransitionToObjectPickedUp(robot);
  }
  else
  {
    // we are not currently carrying one, pick up one
    TransitionToPickUpObject(robot);
  }

  const Result ret = IsActing() ? RESULT_OK : RESULT_FAIL;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::StopInternal(Robot& robot)
{
  // clear render on exit just in case
  if ( kBebctb_DebugRenderAll ) {
    robot.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreBringCubeToBeacon.Locations");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TransitionToPickUpObject(Robot& robot)
{
  if ( !_candidateObjects.empty() )
  {
    // if _selectedObjectID is set this is a retry
    const bool isRetry = _selectedObjectID.IsSet();
    if ( !isRetry )
    {
      // pick closest block
      const Vec3f& robotLoc = robot.GetPose().GetTranslation();
      const BlockWorld& world = robot.GetBlockWorld();
      // TODO rsam/brad: consider using path distance here
      size_t bestIndex = 0;
      const ObservableObject* candidateObj = GetCandidate(world, bestIndex);
      float bestDistSQ = (nullptr != candidateObj) ? (candidateObj->GetPose().GetTranslation() - robotLoc).LengthSq() : FLT_MAX;
      for( size_t idx=1; idx<_candidateObjects.size(); ++idx)
      {
        candidateObj = GetCandidate(world, bestIndex);
        float candidateDistSQ = (nullptr != candidateObj) ? (candidateObj->GetPose().GetTranslation() - robotLoc).LengthSq() : FLT_MAX;
        if ( candidateDistSQ < bestDistSQ )
        {
          bestDistSQ = candidateDistSQ;
          bestIndex = idx;
        }
      }

      // did we find a candidate?
      const bool foundCandidate = FLT_LT(bestDistSQ, FLT_MAX);
      if ( foundCandidate ) {
        _selectedObjectID = _candidateObjects[bestIndex].id;
      } else {
        PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.InvalidCandidates", "Could not pick candidate");
        return;
      }
    }


    // fire action with proper callback
    DriveToPickupObjectAction* pickUpAction = new DriveToPickupObjectAction(robot, _selectedObjectID );
    RobotCompletedActionCallback onActionResult = [this, &robot, isRetry](const ExternalInterface::RobotCompletedAction& actionRet)
    {
      if ( actionRet.result == ActionResult::SUCCESS ) {
        // object was picked up
        TransitionToObjectPickedUp(robot);
      }
      else if ((actionRet.result == ActionResult::FAILURE_RETRY) || (actionRet.result == ActionResult::FAILURE_TIMEOUT))
      {
        // do we currently have the object in the lift?
        const bool isCarrying = (robot.IsCarryingObject() && robot.GetCarryingObject() == _selectedObjectID);
        if ( isCarrying )
        {
          // TODO rsam: set a max number of retries?
          // we were already carrying the object, try to find a new position for it
          TransitionToObjectPickedUp( robot );
        }
        else if ( !isRetry )
        {
          // something else failed, maybe we failed to align with the cube, try to pick up the cube again
          TransitionToPickUpObject( robot );
        }
      } else {
        PRINT_NAMED_INFO("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.DriveToPickUpFailed", "Unhandled result");
      }
    };
    StartActing(pickUpAction, onActionResult);
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.NoCandidates", "Can't run with no selected objects");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TransitionToObjectPickedUp(Robot& robot)
{
  // clear this render to not mistake previous frame debug with this
  if ( kBebctb_DebugRenderAll ) {
    robot.GetContext()->GetVizManager()->EraseSegments("BehaviorExploreBringCubeToBeacon.Locations");
  }

  // if we successfully picked up the object we wanted we can continue, otherwise freak out
  if ( robot.IsCarryingObject() && robot.GetCarryingObject() == _selectedObjectID )
  {
    const ObservableObject* const pickedUpObject = robot.GetBlockWorld().GetObjectByID( _selectedObjectID);
    if ( pickedUpObject == nullptr ) {
      PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.ObjectIsNull",
        "Could not obtain obj from ID '%d'", _selectedObjectID.GetValue());
        return;
    }
    
    // grab the selected beacon (there should be one)
    const AIWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
    const AIBeacon* selectedBeacon = whiteboard.GetActiveBeacon();
    if ( nullptr == selectedBeacon ) {
      ASSERT_NAMED( nullptr!= selectedBeacon, "BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.NullBeacon");
      return;
    }
    
    // 1) if there're other cubes and we can put this on top, do that
    const ObservableObject* bottomCube = FindFreeCubeToStackOn(pickedUpObject, selectedBeacon, robot);
    if ( nullptr != bottomCube )
    {
      // create action to stack
      DriveToPlaceOnObjectAction* stackAction = new DriveToPlaceOnObjectAction(robot, bottomCube->GetID());
      RobotCompletedActionCallback onPlaceActionResult = [this, &robot](const ExternalInterface::RobotCompletedAction& actionRet)
      {
        if (actionRet.result == ActionResult::FAILURE_RETRY)
        {
          // do we currently have the object in the lift?
          const bool isCarrying = (robot.IsCarryingObject() && robot.GetCarryingObject() == _selectedObjectID);
          if ( isCarrying )
          {
            // TODO rsam: set a max number of retries?
            // we were already carrying the object, try to find a new position for it
            TransitionToObjectPickedUp( robot );
          }
        } else {
          // PRINT_NAMED_INFO("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.DriveToPickUpFailed", "Unhandled result");
        }
      };
      StartActing( stackAction, onPlaceActionResult );
    }
    else
    {
      // 2) otherwise, find area as close to the center as possible (use memory map for this?)
      Pose3d dropPose;
      const bool foundPose = FindFreePoseInBeacon(pickedUpObject, selectedBeacon, robot, dropPose);
      if ( foundPose )
      {
        const bool checkFreeDestination = true;
        const float padding_mm = kBebctb_PaddingBetweenCubes_mm;
        // create action to drive to the drop location
        PlaceObjectOnGroundAtPoseAction* placeObjectAction = new PlaceObjectOnGroundAtPoseAction(robot, dropPose, false, false, checkFreeDestination, padding_mm);
        RobotCompletedActionCallback onPlaceActionResult = [this, &robot](const ExternalInterface::RobotCompletedAction& actionRet)
        {
          if (actionRet.result == ActionResult::FAILURE_RETRY)
          {
            // do we currently have the object in the lift?
            const bool isCarrying = (robot.IsCarryingObject() && robot.GetCarryingObject() == _selectedObjectID);
            if ( isCarrying )
            {
              // TODO rsam: set a max number of retries?
              // we were already carrying the object, try to find a new position for it
              TransitionToObjectPickedUp( robot );
            }
          } else {
            // PRINT_NAMED_INFO("BehaviorExploreBringCubeToBeacon.TransitionToPickUpObject.DriveToPickUpFailed", "Unhandled result");
          }
        };
        
        StartActing( placeObjectAction, onPlaceActionResult );
      }
      else
      {
        PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.NoFreePoses",
          "Could not decide where to drop the cube in the beacon");
      }
    }
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.TransitionToObjectPickedUp.NotPickedUp",
      "We did not pick up the cube. Retry?");
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorExploreBringCubeToBeacon::FindFreeCubeToStackOn(const ObservableObject* object,
  const AIBeacon* beacon, const Robot& robot) const
{
  // here we would check if stacking is (un)locked
  const bool canStackCubes = robot.GetProgressionUnlockComponent().IsUnlocked(UnlockId::StackTwoCubes);
  if ( !canStackCubes ) {
    return nullptr;
  }

  // ask for all cubes we know, and if any is not inside a beacon, then we want to bring that one to the closest beacon
  BlockWorldFilter filter;
  filter.SetAllowedFamilies({{ ObjectFamily::Block, ObjectFamily::LightCube }});
  
  // additional threshold so that we don't stack on top of a cube in the border. This prevents stacking on
  // a cube close to the border, which would cause the stacked cube to be out of the beacon
  const float kPrecisionOffset_mm = 10.0f; // this is just to account for errors when readjusting cube positions
  const float inwardThreshold_mm = object->GetSize().x() + kPrecisionOffset_mm;
  
  filter.SetFilterFcn([object,beacon,&robot,inwardThreshold_mm,this](const ObservableObject* blockPtr) {
    // if this is our block or state is not good, skip
    if ( blockPtr == object || !blockPtr->IsPoseStateKnown() ) {
      return false;
    }
    
    // if we don't want to stack on this cube, skip
    if ( _invalidCubesToStackOn.find(blockPtr->GetID()) != _invalidCubesToStackOn.end() )
    {
      return false;
    }
    
    ASSERT_NAMED( FLT_NEAR(object->GetSize().x(), object->GetSize().y()) ,
                 "BehaviorExploreBringCubeToBeacon.FindFreeCubeToStackOn.AssumedXYEqual");
    
    // check it this cube is in beacon, but also if it's actually closer than
    const bool isBlockInSelectedBeacon = beacon->IsLocWithinBeacon(blockPtr->GetPose(), inwardThreshold_mm);
    if ( isBlockInSelectedBeacon )
    {
      // check if we can stack on top of this object
      const bool canStackOnObject = robot.CanStackOnTopOfObject(*blockPtr);
      if ( canStackOnObject )
      {
        // TODO rsam: this stops at the first found, not closest or anything fancy
        return true;
      }
    }
    
    return false;
  });
                      
  return robot.GetBlockWorld().FindMatchingObject(filter);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreBringCubeToBeacon::FindFreePoseInBeacon(const ObservableObject* object,
  const AIBeacon* beacon, const Robot& robot, Pose3d& freePose)
{
  // store directionallity of beacon
  // TODO rsam: it would be cool so all cubes lined up together. Maybe we should check the closest cube to the selected
  // location, but that would require making more collision computations. A solution might be do the location selection
  // as a sphere, that way we can turn the cube around Z and the location would still be valid (at the same time
  // sphere checks are faster than cube checks)
  const Vec3f& kUpVector = Z_AXIS_3D();
  Rotation3d beaconDirectionality(0.0f, kUpVector);
  {
    // TODO rsam put this utility somewhere: create Rotation3d from vector in XY plane
    Vec3f beaconNormal = (beacon->GetPose().GetTranslation() - robot.GetPose().GetTranslation());
    beaconNormal.z() = 0.0f;
    float distance = beaconNormal.MakeUnitLength();
    
    float rotRads = 0.0f;
    if ( !NEAR_ZERO(distance) )
    {
      // calculate rotation transform for the beaconNormal
      const Vec3f& kFwdVector = X_AXIS_3D();
      const Vec3f& kRightVector = -Y_AXIS_3D();
    
      const float fwdAngleCos = DotProduct(beaconNormal, kFwdVector);
      const bool isPositiveAngle = (DotProduct(beaconNormal, kRightVector) >= 0.0f);
      rotRads = isPositiveAngle ? -std::acos(fwdAngleCos) : std::acos(fwdAngleCos);
      
      beaconDirectionality = Rotation3d( rotRads, kUpVector );
    }
  }
  
  const Vec3f& beaconCenter = beacon->GetPose().GetTranslation();
  LocationCalculator locCalc(object, beaconCenter, beaconDirectionality, beacon->GetRadius(), robot);

  const int kMaxRow = beacon->GetRadius() / locCalc.GetLocationOffset();
  const int kMaxCol = kMaxRow;
  
  bool foundLocation = false;
  
  // iterate negative rows first so that locations start at center and become closer every time
  for( int row=0; row>=-kMaxRow; --row)
  {
    foundLocation = locCalc.IsLocationFreeForObject(row, 0, freePose);
    if( foundLocation ) { break; }
    
    for( int col=1; col<=kMaxCol; ++col)
    {
      foundLocation = locCalc.IsLocationFreeForObject(row, -col, freePose);
      if( foundLocation ) { break; }
      foundLocation = locCalc.IsLocationFreeForObject(row,  col, freePose);
      if( foundLocation ) { break; }
    }
    
    // this is necessary because the inner breaks affect columns only
    if( foundLocation ) { break; }
  }
  
  // if first loop didn't find location yet, try second
  if ( !foundLocation ) {
    // positive order now (away from robot)
    for( int row=1; row<=kMaxRow; ++row)
    {
      locCalc.IsLocationFreeForObject(row, 0, freePose);
      if( foundLocation ) { break; }
      
      for( int col=1; col<=kMaxCol; ++col)
      {
        locCalc.IsLocationFreeForObject(row, -col, freePose);
        if( foundLocation ) { break; }
        
        locCalc.IsLocationFreeForObject(row,  col, freePose);
        if( foundLocation ) { break; }
      }
      
      // this is necessary because the inner breaks affect columns only
      if( foundLocation ) { break; }
    }
  }
  
  return foundLocation;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ObservableObject* BehaviorExploreBringCubeToBeacon::GetCandidate(const BlockWorld& world, size_t index) const
{
  const ObservableObject* ret = nullptr;
  if ( index < _candidateObjects.size() ) {
    ret = world.GetObjectByID(_candidateObjects[index].id, _candidateObjects[index].family);
  }
  return ret;
}

} // namespace Cozmo
} // namespace Anki
