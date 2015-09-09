/**
* File: CST_PickAndPlace.cpp
*
* Author: kevin yoon
* Created: 8/20/15
*
* Description: See TestStates below
*
* Copyright: Anki, inc. 2015
*
*/

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"


namespace Anki {
namespace Cozmo {
  
  enum class TestState {
    MoveHead,      // Lift head to look straight
    VerifyBlocks,  // Verify that expected blocks exist
    PickupBlock,   // Pickup closest block
    PlaceBlock,    // Place on next closest block
    TestDone
  };
  
  // ============ Test class declaration ============
  class CST_PickAndPlace : public CozmoSimTestController {
    
  private:
    
    const u32 NUM_BLOCKS_EXPECTED_ON_START = 6;
    
    virtual s32 UpdateInternal() override;
    
    TestState _testState = TestState::MoveHead;
    
    s32 _pickupBlockID = -1;
    s32 _placeBlockID = -1;
    bool _observedNewObject = false;
    
    // Message handlers
    virtual void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg) override;
    
    // Helper functions
    s32 GetClosestObjectID(const std::vector<s32>& objectIDs);
    
  };
  
  // Register class with factory
  REGISTER_COZMO_SIM_TEST_CLASS(CST_PickAndPlace);
  
  
  // =========== Test class implementation ===========
  
  s32 CST_PickAndPlace::UpdateInternal()
  {
    switch (_testState) {

      case TestState::MoveHead:
      {
        SendMoveHeadToAngle(0, 100, 100);
        _testState = TestState::VerifyBlocks;
        break;
      }
      case TestState::VerifyBlocks:
      {
        
        // Verify that head is in position
        IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(IS_MOVING) && NEAR(GetRobotHeadAngle_rad(), 0, 0.01),
                                         2) {

          // Verify that the expected number of blocks was observed
          IF_CONDITION_WITH_TIMEOUT_ASSERT(GetNumObjects() == NUM_BLOCKS_EXPECTED_ON_START,
                                           2) {
            
          // Get closest block and try to pick it up
          _pickupBlockID = GetClosestObjectID( GetAllObjectIDs() );
          CST_ASSERT(_pickupBlockID >= 0, "Failed to find closest object to robot");
          
          PRINT_NAMED_INFO("CST_PickAndPlace.VerifyBlocks.PickingUpBlock", "%d", _pickupBlockID);
          SendPickAndPlaceObject(_pickupBlockID, true);
          
          _observedNewObject = false;
          _testState = TestState::PickupBlock;
            
          }
        }
        break;
      }
      case TestState::PickupBlock:
      {
        // While it was picking the block up, it should have noticed another block appear
        // (i.e. the active block that was hiding behind it)
        IF_CONDITION_WITH_TIMEOUT_ASSERT(IsRobotStatus(IS_CARRYING_BLOCK) && _observedNewObject,
                                         10) {
          
          // Make list of known blocks minus the one that's being carried
          std::vector<s32> blockIDs = GetAllObjectIDs();
          auto pickupBlockIter = std::find(blockIDs.begin(), blockIDs.end(), _pickupBlockID);
          
          CST_ASSERT(pickupBlockIter != blockIDs.end(), "Pickup block disappeared");
          blockIDs.erase(pickupBlockIter);
          
          // Get closest block and try to pick it up
          _placeBlockID = GetClosestObjectID( blockIDs );
          CST_ASSERT(_placeBlockID >= 0, "Failed to find closest object to robot");
          
          PRINT_NAMED_INFO("CST_PickAndPlace.PickupBlock.PlacingBlock", "%d", _placeBlockID);
          SendPickAndPlaceObject(_placeBlockID, true);
          _testState = TestState::PlaceBlock;
        }
        break;
      }
      case TestState::PlaceBlock:
      {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(IS_CARRYING_BLOCK),
                                         15) {
          _testState = TestState::TestDone;
        }
        break;
      }
      case TestState::TestDone:
      {
        
        CST_EXIT();
        break;
      }
    }
    
    return _result;
  }
  
  s32 CST_PickAndPlace::GetClosestObjectID(const std::vector<s32>& objectIDs)
  {
    Pose3d robotPose = GetRobotPose();
    
    s32 closestObjectID = -1;
    f32 closestDist = 1000000;
    
    Pose3d objPose;
    for (auto obj=objectIDs.begin(); obj != objectIDs.end(); ++obj) {
      GetObjectPose(*obj, objPose);
      f32 dist = ComputeDistanceBetween(objPose, robotPose);
      if (dist < closestDist) {
        closestDist = dist;
        closestObjectID = *obj;
      }
    }
    
    return closestObjectID;
  }

  
  // ================ Message handler callbacks ==================
  void CST_PickAndPlace::HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg)
  {
    static u32 lastObjectCount = 0;
    if (GetNumObjects() != lastObjectCount) {
      if (GetNumObjects() > lastObjectCount) {
        _observedNewObject = true;
      }
      lastObjectCount = GetNumObjects();
    }
  }
  
  // ================ End of message handler callbacks ==================

} // end namespace Cozmo
} // end namespace Anki

