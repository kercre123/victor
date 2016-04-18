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
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorWhiteboard.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

#include <array>

namespace Anki {
namespace Cozmo {

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
bool BehaviorExploreBringCubeToBeacon::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  // check if any known cubes are not currently in a valid beacon
  const BehaviorWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
  const BehaviorWhiteboard::BeaconList& beaconList = whiteboard.GetBeacons();
  
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
    // ask for all cubes we know, and if any is not inside a beacon, then we want to bring that one to the closest beacon
    static const std::vector<ObjectFamily> familyList = { ObjectFamily::Block, ObjectFamily::LightCube };
    for(const auto& family : familyList)
    {
      const auto& blocksByType = robot.GetBlockWorld().GetExistingObjectsByFamily(family);
      for (const auto& blocksOfSameType : blocksByType)
      {
        for(const auto& block : blocksOfSameType.second)
        {
          bool isBlockInBeacon = false;
          const ObservableObject* blockPtr = block.second;
          
          // check if the object is within any beacon
          for ( const auto& beacon : beaconList ) {
            isBlockInBeacon = beacon.IsLocWithinBeacon(blockPtr->GetPose().GetTranslation());
            if ( isBlockInBeacon ) {
              break;
            }
          }
          
          // this block should be carried to a beacon
          if ( !isBlockInBeacon ) {
            _candidateObjects.push_back( blockPtr );
          }
        }
      }
    }
  }

  // do we have blocks that we want to bring to a beacon?
  bool hasBlocksOutOfBeacons = !_candidateObjects.empty();
  return hasBlocksOutOfBeacons;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreBringCubeToBeacon::InitInternal(Robot& robot, double currentTime_sec)
{
  // starting state
  TransitionToPickUpObject(robot);

  const Result ret = IsActing() ? RESULT_OK : RESULT_FAIL;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TransitionToPickUpObject(Robot& robot)
{
  if ( !_candidateObjects.empty() )
  {
    const Vec3f& robotLoc = robot.GetPose().GetTranslation();

    // pick closest block
    // TODO rsam/brad: consider using path distance here
    size_t bestIndex = 0;
    float bestDistSQ = (_candidateObjects[bestIndex]->GetPose().GetTranslation() - robotLoc).LengthSq();
    for( size_t idx=1; idx<_candidateObjects.size(); ++idx)
    {
      float candidateDistSQ = (_candidateObjects[idx]->GetPose().GetTranslation() - robotLoc).LengthSq();
      if ( candidateDistSQ < bestDistSQ )
      {
        bestDistSQ = candidateDistSQ;
        bestIndex = idx;
      }
    }

    // pick up the cube
    _selectedObjectID = _candidateObjects[bestIndex]->GetID();
    DriveToPickupObjectAction* pickUpAction = new DriveToPickupObjectAction(robot, _selectedObjectID );
    StartActing(pickUpAction, &BehaviorExploreBringCubeToBeacon::TransitionToObjectPickedUp );
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorExploreBringCubeToBeacon.InitInternal.NoSelectedObjects", "Can't run with no selected objects");
  }
}

namespace {
//  void ComputeNextSpiralPoint(const int x, const int y, int& nextX, int& nextY)
//  {
//    if(abs(x) <= abs(y) && (x != -y || x >= 0))
//    {
//      nextX = x + ((y <= 0) ? 1 : -1);
//    } else {
//      nextY = y + ((x >= 0) ? 1 : -1);
//    }
//  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreBringCubeToBeacon::TransitionToObjectPickedUp(Robot& robot)
{
  // if we successfully picked up the object we wanted we can continue, otherwise freak out
  if ( robot.IsCarryingObject() && robot.GetCarryingObject() == _selectedObjectID )
  {
  
    // pick where to place it
    // 1) if there're other cubes and we can put this on top, do that
    
    
    
    // 2) otherwise, find area as close to the center as possible (use memory map for this?)
    
  
  
  
  }

}


} // namespace Cozmo
} // namespace Anki
