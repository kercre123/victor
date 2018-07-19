/**
 * File: CST_PickupFromStack.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-06-24
 *
 * Description: This is a test for the stack block behavior, not just the action
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/robot.h"
#include "simulator/game/cozmoSimTestController.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {

enum class TestState {
  Init,                         // Init everything
  WaitForCubeConnections,       // Ensure 2 blocks are connected
  AttemptPickupHigh,            // Attempt to pick up the high block
  PickupHighShouldFail,         // Should fail since block was pushed away at the last second
  AttemptPickupLowOutOfView,    // Attempt to pick up the low block
  PickupLowOutOfViewShouldFail, // Should fail since block was pushed away at the last second
  MoveHeadUp,                   //
  WaitForCubeConfirmation2,     // Ensure cozmo's seen the cube in front of him
  AttemptPickupLowInView,       // Attempt to pick up the low block again and push the block away at last second.
  TeleportBlockInView,          // Teleport the block back into view of the robot.
  PickupLowInViewShouldFail,    // Should fail since robot sees the block in front of it and not on its lift.
  RemoveCube,                   // Pick up the cube again and make it vanish from the world after successful pickup
  PlaceObjectShouldFail,        // Placing should fail since the cube is no longer on the lift.
  TestDone
};

namespace {
  static const Pose3d kidnapCubePose(M_PI_F, Z_AXIS_3D(), {40.0, -150.0, 22.0});
  static const Pose3d hideCubePose(M_PI_F, Z_AXIS_3D(), {10000.0, 10000.0, 100.0});
}


class CST_PickupFromStack : public CozmoSimTestController {
private:
      
  virtual s32 UpdateSimInternal() override;
  
  virtual void HandleActiveObjectConnectionState(const ExternalInterface::ObjectConnectionState& msg) override;
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg) override;
  
  // Helpers:
  void SendPickupObjectByType(const ObjectType& objType);
  
  // State:
  TestState _testState = TestState::Init;

  // Parameters:
  const float _kRobotNearBlockThreshold_mm = 65.f;
  const float _kRobotNearBlockThresholdHigh_mm = 100.f;
  ObjectType  _baseCube    = ObjectType::Block_LIGHTCUBE1;
  std::string _baseCubeStr = "cube0";
  ObjectType  _topCube     = ObjectType::Block_LIGHTCUBE2;
  std::string _topCubeStr  = "cube1";
  
  ActionResult _moveHeadToAngleResult = ActionResult::RUNNING;
  ActionResult _turnInPlaceResult = ActionResult::RUNNING;
  ActionResult _pickupObjectResult = ActionResult::RUNNING;
  ActionResult _placeObjectResult = ActionResult::RUNNING;

  
  Pose3d _robotPoseWhenBlockShoved;
  u32 _numObjectsConnected = 0;
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_PickupFromStack);


s32 CST_PickupFromStack::UpdateSimInternal()
{
  PrintPeriodicBlockDebug();

  switch (_testState) {
    case TestState::Init:
    {
      DisableRandomPathSpeeds();
      StartMovieConditional("StackBlockBehavior_AttemptCube");
      // TakeScreenshotsAtInterval("StackBlockBehavior", 1.f);
          
      SendMoveHeadToAngle(0, 100, 100);
      
      // Cancel the stack behavior:
      SendMessage(ExternalInterface::MessageGameToEngine(
                     ExternalInterface::ExecuteBehaviorByID(
                       BehaviorTypesWrapper::BehaviorIDToString(BEHAVIOR_ID(Wait)), -1)));

      // Give one tick for world to load in
      SET_TEST_STATE(WaitForCubeConnections);
      break;
    }
      
    case TestState::WaitForCubeConnections:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT((_numObjectsConnected == 2)
                                       && (GetAllObjectIDsByFamily(ObjectFamily::LightCube).size() == 2), 15)
      {
        SET_TEST_STATE(AttemptPickupHigh);
        // Now attempt to pick up the top block:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(_topCube);
      }
      break;
    }

      
    case TestState::AttemptPickupHigh:
    {
      const Pose3d robotPose = GetRobotPoseActual();
      const Pose3d cubePose = GetLightCubePoseActual(_topCube);
      const float distBetween = ComputeDistanceBetween(robotPose, cubePose);
      const bool nearBlock = distBetween < _kRobotNearBlockThresholdHigh_mm;
      IF_CONDITION_WITH_TIMEOUT_ASSERT(nearBlock, 15) {
        // Push the block out of view so that the pickup will fail.
        SendApplyForce(_topCubeStr, 20, 4000, 5);
        SET_TEST_STATE(PickupHighShouldFail)
      }
      
      break;
    }
      
      
    case TestState::PickupHighShouldFail:
    {
      // Keep applying a force ocassionally to keep the block 'moving', so that the pickup will
      //  fail with PICKUP_OBJECT_UNEXPECTEDLY_MOVING.
      static int cnt = 1;
      if ((++cnt % 5) == 0) {
        SendApplyForce(_topCubeStr, -5, 0, 0);
      }
      
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_MOVING, 15) {
        // Pick up object 1:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(_baseCube);
        SET_TEST_STATE(AttemptPickupLowOutOfView)
      }
      
      break;
    }
      
      
    case TestState::AttemptPickupLowOutOfView:
    {
      Pose3d robotPose = GetRobotPoseActual();
      Pose3d cubePose = GetLightCubePoseActual(_baseCube);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThreshold_mm;
      IF_CONDITION_WITH_TIMEOUT_ASSERT(nearBlock, 15) {
        // Transport the block out of view:
        Pose3d cubePose = GetRobotPoseActual();
        Vec3f T = cubePose.GetTranslation();
        cubePose.SetTranslation(Vec3f(T.x(), T.y() + 200.f, 25));
        SetLightCubePose(_baseCube, cubePose);
        SET_TEST_STATE(PickupLowOutOfViewShouldFail)
      }
      
      break;
    }
    
      
    case TestState::PickupLowOutOfViewShouldFail:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING, DEFAULT_TIMEOUT) {
        // move cube in front of robot and look at it:
        Pose3d cubePose = GetRobotPoseActual();
        Vec3f T = cubePose.GetTranslation();
        cubePose.SetTranslation(Vec3f(T.x() + 120.f, T.y(), 25));
        SetLightCubePose(_baseCube, cubePose);
        // move head up to see the cube:
        _moveHeadToAngleResult = ActionResult::RUNNING;
        SendMoveHeadToAngle(0, 100, 100);
        SET_TEST_STATE(MoveHeadUp)
      }
      
      break;
    }
      
      
    case TestState::MoveHeadUp:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_moveHeadToAngleResult == ActionResult::SUCCESS, 5) {
        SET_TEST_STATE(WaitForCubeConfirmation2)
      }
      break;
    }
      
      
    case TestState::WaitForCubeConfirmation2:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(GetAllObjectIDsByFamily(ObjectFamily::LightCube).size() == 1, 5){
        // pick up object 1:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(_baseCube);
        SET_TEST_STATE(AttemptPickupLowInView);
      }
      break;
    }
      
      
    case TestState::AttemptPickupLowInView:
    {
      Pose3d robotPose = GetRobotPoseActual();
      Pose3d cubePose = GetLightCubePoseActual(_baseCube);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThreshold_mm;
      IF_CONDITION_WITH_TIMEOUT_ASSERT(nearBlock, 15) {
        // Push the block out of view:
        SendApplyForce(_baseCubeStr, 0, -40, 10);
        // Keep track of where the robot was when the cube was first pushed.
        _robotPoseWhenBlockShoved = GetRobotPoseActual();
        SET_TEST_STATE(TeleportBlockInView)
      }
      
      break;
    }
      
      
    case TestState::TeleportBlockInView:
    {
      // Keep applying a force ocassionally to keep the block moving,
      //  to trick Cozmo into thinking it's moving as expected.
      static int cnt = 0;
      if ((++cnt % 5) == 0) {
        SendApplyForce(_baseCubeStr, -5, 0, 0);
      }
      
      Pose3d robotPoseNow = GetRobotPoseActual();
      const float distChange = ComputeDistanceBetween(_robotPoseWhenBlockShoved, robotPoseNow);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(distChange > 30.f, DEFAULT_TIMEOUT) {
        // Teleport the cube to just in front of the robot:
        Pose3d cubePose = GetRobotPoseActual();
        Vec3f T = cubePose.GetTranslation();
        cubePose.SetTranslation(Vec3f(T.x() + 160.f, T.y(), 22));
        SetLightCubePose(_baseCube, cubePose);
        SET_TEST_STATE(PickupLowInViewShouldFail)
      }
      
      break;
    }

      
    case TestState::PickupLowInViewShouldFail:
    {
      // Cozmo should see the cube in front of him and realize he's not carrying it.
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::NOT_CARRYING_OBJECT_RETRY, DEFAULT_TIMEOUT) {
        // Pick up the cube again:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(_baseCube);
        
        SET_TEST_STATE(RemoveCube)
      }
      
      break;
    }

      
    case TestState::RemoveCube:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::SUCCESS, DEFAULT_TIMEOUT) {
        CST_ASSERT(GetCarryingObjectID() != -1, "robot should be carrying object at this point");
        
        // Remove the light cube from the world (as it someone has taken it off the lift)
        CST_ASSERT(RemoveLightCubeByType(_baseCube), "LightCube removal failed");
        
        // Attempt to place the now-removed block:
        Pose3d placePose = GetRobotPoseActual();
        Vec3f T = placePose.GetTranslation();
        placePose.SetTranslation(Vec3f(T.x(), T.y() - 100.f, 22));
        
        _placeObjectResult = ActionResult::RUNNING;
        SendPlaceObjectOnGroundSequence(placePose, _defaultTestMotionProfile);
        
        SET_TEST_STATE(PlaceObjectShouldFail)
      }
      
      break;
    }

      
    case TestState::PlaceObjectShouldFail:
    {
      // rsam: if the robot is faster flagging the object as unknown before verify action can finish it
      // will delete the object from BlockWorld, which makes the action fail with BAD_OBJECT rather
      // than NOT_CARRYING_OBJECT_ABORT. If the robot is not fast enough, it would fail in visual observation
      const bool actionFailed = (_placeObjectResult == ActionResult::BAD_OBJECT) ||
                                (_placeObjectResult == ActionResult::VISUAL_OBSERVATION_FAILED);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(actionFailed, 20)
      {
        PRINT_NAMED_INFO("CST_PickupFromStack.TestInfo",
                         "Finished PlaceObjectShouldFail with code '%s'",
                         EnumToString(_placeObjectResult));
        SET_TEST_STATE(TestDone)
      }
      
      break;
    }
    
      
    case TestState::TestDone:
    {
      // Since we deleted a node earlier, we have to save the world
      //  to prevent a modal "Save Changes?" dialog from preventing
      //  Webots from quitting. No biggie since it will just overwrite
      //  __generated__.wbt.
      GetSupervisor()->saveWorld();
      
      StopMovie();
      CST_EXIT();

      break;
    }
  }
  return _result;
}


void CST_PickupFromStack::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg)
{
  PRINT_NAMED_INFO("CST_PickupFromStack.HandleRobotCompletedAction", "completed action %s, result %s", EnumToString(msg.actionType), EnumToString(msg.result));
  
  if(msg.actionType == RobotActionType::MOVE_HEAD_TO_ANGLE) {
    _moveHeadToAngleResult = msg.result;
  } else if(msg.actionType == RobotActionType::TURN_IN_PLACE) {
    _turnInPlaceResult = msg.result;
  } else if (msg.actionType == RobotActionType::PICKUP_OBJECT_HIGH
             || msg.actionType == RobotActionType::PICKUP_OBJECT_LOW
             || msg.actionType == RobotActionType::COMPOUND) {
    _pickupObjectResult = msg.result;
  } else if (msg.actionType == RobotActionType::PLACE_OBJECT_HIGH
             || msg.actionType == RobotActionType::PLACE_OBJECT_LOW) {
    _placeObjectResult = msg.result;
  }
}
  
  
void CST_PickupFromStack::HandleActiveObjectConnectionState(const ExternalInterface::ObjectConnectionState& msg)
{
  if (msg.connected) {
    ++_numObjectsConnected;
  } else if (_numObjectsConnected > 0) {
    --_numObjectsConnected;
  }
}
  
void CST_PickupFromStack::SendPickupObjectByType(const ObjectType& objType)
{
  PRINT_NAMED_INFO("CST_PickupFromStack.SendPickupObjectByType", "Attempting to SendPickupObject for Object Type %s", EnumToString(objType));
  
  std::vector<s32> objIds = GetAllObjectIDsByFamilyAndType(ObjectFamily::LightCube, objType);
  
  // Should have found exactly one object ID for this object type:
  CST_ASSERT(objIds.size() == 1, "Did not find exactly one object ID for given type!");
  
  SendPickupObject(objIds[0], _defaultTestMotionProfile, true);
}
  

}
}
