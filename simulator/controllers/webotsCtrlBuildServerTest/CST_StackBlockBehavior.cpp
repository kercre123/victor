/**
 * File: CST_StackBlockBehavior.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-06-24
 *
 * Description: This is a test for the stack block behavior, not just the action
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/simulator/game/cozmoSimTestController.h"

#define SET_STATE(s) {                                                  \
    PRINT_NAMED_INFO("CST_StackBlockBehavior.TransitionTestState",      \
                     "%s", #s);                                         \
    _testState = TestState::s;                                          \
  }


namespace Anki {
namespace Cozmo {

enum class TestState {
  Init,                         // Init everything
  WaitForCubeConnections,       // Ensure 2 blocks are connected
  VerifyObject1,                // Should see 1 block
  TurnAway,                     // Turn away from the block and force delocalize
  WaitForDeloc,                 //
  VerifyObject2,                // Turn to see the second block
  DontStartBehavior,            // Stack behavior shouldn't start yet (still delocalized)
  TurnBack,                     // Turn back toward first block
  PickingUp,                    // Stack behavior should begin
  BehaviorShouldFail,           // Stack behavior should fail (since second block was kidnapped and moved across workspace)
  TurnToObject2,                // Turn toward the second block's new location
  Stacking,                     // Stacking should occur
  VerifyStack,                  // Make sure stack was created
  AttemptPickupHigh,            // Attempt to pick up the high block
  PickupHighShouldFail,         // Should fail since block was pushed away at the last second
  AttemptPickupLowOutOfView,    // Attempt to pick up the low block
  PickupLowOutOfViewShouldFail, // Should fail since block was pushed away at the last second
  MoveHeadUp,                   //
  AttemptPickupLowInView,       // Attempt to pick up the low block again and push the block away at last second.
  TeleportBlockInView,          // Teleport the block back into view of the robot.
  PickupLowInViewShouldFail,    // Should fail since robot sees the block in front of it and not on its lift.
  RemoveCube,                   // Pick up the cube again and make it vanish from the world after successful pickup
  PlaceObjectShouldFail,        // Placing should fail since the cube is no longer on the lift.
  TestDone
};
  
// Motion profile for test
const f32 defaultPathSpeed_mmps = 60;
const f32 defaultPathAccel_mmps2 = 200;
const f32 defaultPathDecel_mmps2 = 500;
const f32 defaultPathPointTurnSpeed_rad_per_sec = 1.5;
const f32 defaultPathPointTurnAccel_rad_per_sec2 = 100;
const f32 defaultPathPointTurnDecel_rad_per_sec2 = 500;
const f32 defaultDockSpeed_mmps = 60;
const f32 defaultDockAccel_mmps2 = 200;
const f32 defaultDockDecel_mmps2 = 100;
const f32 defaultReverseSpeed_mmps = 30;
PathMotionProfile motionProfile (defaultPathSpeed_mmps,
                     defaultPathAccel_mmps2,
                     defaultPathDecel_mmps2,
                     defaultPathPointTurnSpeed_rad_per_sec,
                     defaultPathPointTurnAccel_rad_per_sec2,
                     defaultPathPointTurnDecel_rad_per_sec2,
                     defaultDockSpeed_mmps,
                     defaultDockAccel_mmps2,
                     defaultDockDecel_mmps2,
                     defaultReverseSpeed_mmps,
                     true);

static const char* kBehaviorName = "StackBlocks";

namespace {
  static const Pose3d kidnapCubePose(M_PI_F, Z_AXIS_3D(), {40.0, -150.0, 22.0});
  static const Pose3d hideCubePose(M_PI_F, Z_AXIS_3D(), {10000.0, 10000.0, 100.0});
}


class CST_StackBlockBehavior : public CozmoSimTestController {
private:
      
  virtual s32 UpdateSimInternal() override;
  
  virtual void HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg) override;
  virtual void HandleActiveObjectConnectionState(const ObjectConnectionState& msg) override;
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg) override;
  
  // Helpers:
  void SendPickupObjectByType(const ObjectType& objType);
  
  // State:
  TestState _testState = TestState::Init;

  // Parameters:
  const float _kRobotNearBlockThreshold_mm = 65.f;
  const float _kRobotNearBlockThresholdHigh_mm = 95.f;
  ObjectType _cubeToMove = ObjectType::InvalidObject;
  
  // Status flags:
  int _startedBehavior = 0;
  int _stoppedBehavior = 0;
  
  ActionResult _moveHeadToAngleResult = ActionResult::RUNNING;
  ActionResult _turnInPlaceResult = ActionResult::RUNNING;
  ActionResult _pickupObjectResult = ActionResult::RUNNING;
  ActionResult _placeObjectResult = ActionResult::RUNNING;

  
  Pose3d _robotPoseWhenBlockShoved;
  double _behaviorCheckTime = 0.0;
  u32 _numObjectsConnected = 0;
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_StackBlockBehavior);


s32 CST_StackBlockBehavior::UpdateSimInternal()
{
  PrintPeriodicBlockDebug();

  switch (_testState) {
    case TestState::Init:
    {
      MakeSynchronous();
      DisableRandomPathSpeeds();
      StartMovieConditional("StackBlockBehavior");
      // TakeScreenshotsAtInterval("StackBlockBehavior", 1.f);
      
      // make sure stacking is unlocked
      UnlockId unlock = UnlockIdFromString("StackTwoCubes");
      CST_ASSERT(unlock != UnlockId::Count, "couldn't get valid unlock id");
      SendMessage( ExternalInterface::MessageGameToEngine(
                     ExternalInterface::RequestSetUnlock(unlock, true)));

      // try to start the behavior now, it shouldn't start for a while
      SendMessage(ExternalInterface::MessageGameToEngine(
                    ExternalInterface::ActivateBehaviorChooser(BehaviorChooserType::Selection)));
      SendMessage(ExternalInterface::MessageGameToEngine(
                    ExternalInterface::ExecuteBehaviorByName(kBehaviorName)));

          
      SendMoveHeadToAngle(0, 100, 100);
      SET_STATE(WaitForCubeConnections);
      break;
    }

      
    case TestState::WaitForCubeConnections:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_numObjectsConnected == 2, 5)
      {
        SET_STATE(VerifyObject1);
      }
      break;
    }
      
      
    case TestState::VerifyObject1:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                            GetNumObjects() == 1,
                                            IsLocalizedToObject())
      {
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace(DEG_TO_RAD(45.f), M_PI_F, 500.f);
        SET_STATE(TurnAway);
      }
      break;
    }

      
    case TestState::TurnAway:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      IF_CONDITION_WITH_TIMEOUT_ASSERT(_turnInPlaceResult == ActionResult::SUCCESS, 10) {
        // Make sure we are still localized (to an object) before sending deloc
        CST_ASSERT( IsLocalizedToObject(), "Should be localized to object before we deloc");
        // deloc the robot so it doesn't see both cubes in the same frame
        SendForceDeloc();
        SET_STATE(WaitForDeloc);
      }
      break;
    }

      
    case TestState::WaitForDeloc:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsLocalizedToObject(), 2) {
        // Turn the robot to see the second cube
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace(DEG_TO_RAD(30.f), M_PI_F, 500.0f);

        SET_STATE(VerifyObject2);
      }
      break;
    }
      
    
    case TestState::VerifyObject2:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      // NOTE: at some point in the future this might fail because GetNumObjects() might not return things not
      // in our current origin. In that case, the test will need to be updated
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            _turnInPlaceResult == ActionResult::SUCCESS,
                                            NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                            GetNumObjects() == 1)
      {
        _behaviorCheckTime = GetSupervisor()->getTime();
        SET_STATE(DontStartBehavior);        
      }
      break;
    }

      
    case TestState::DontStartBehavior:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      const double timeToWait_s = 1.0;
      double currTime = GetSupervisor()->getTime();
      if( currTime - _behaviorCheckTime > timeToWait_s ) {
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace( DEG_TO_RAD(-90.f), M_PI_F, 500.f );
        SET_STATE(TurnBack);
      }
      break;
    }

      
    case TestState::TurnBack:
    {
      // at some point (possibly before we stop moving) the behavior should become runnable and start on it's own
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_startedBehavior == 1, 10) {
        // behavior is running, wait for it to finish
        _pickupObjectResult = ActionResult::RUNNING;
        SET_STATE(PickingUp)
      }
      
      break;
    }

      
    case TestState::PickingUp:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            GetCarryingObjectID() >= 0,
                                            _pickupObjectResult == ActionResult::SUCCESS) {
        // move the cube we aren't holding
        ObjectType carryingCube;
        CST_ASSERT(GetObjectType(GetCarryingObjectID(), carryingCube) == Result::RESULT_OK,
                   "Could not get object type for cube being carried");
        _cubeToMove = (carryingCube == ObjectType::Block_LIGHTCUBE1) ?
                      ObjectType::Block_LIGHTCUBE2 :
                      ObjectType::Block_LIGHTCUBE1;

        // raise it off the ground so we trigger a moved event, and send it somewhere we know the robot won't see it
        SetLightCubePose(_cubeToMove, hideCubePose);
        SET_STATE(BehaviorShouldFail);
        // at this point, robot should be trying to do stacking action. It picked up the first cube, then will
        // go try to stack it, but should fail because the bottom cube isn't there
        _behaviorCheckTime = -1.0;
      }
      break;
    }

      
    case TestState::BehaviorShouldFail:
    {
      CST_ASSERT( _startedBehavior == 1, "Behavior started wrong number of times: "<<_startedBehavior );

      const double currTime_s = GetSupervisor()->getTime();

      if( _behaviorCheckTime < 0.0 ) {
        IF_CONDITION_WITH_TIMEOUT_ASSERT( _stoppedBehavior == 1, 30.0 ) {
          _behaviorCheckTime = currTime_s;
        }
      }
      else {
        static constexpr double timeToHold_s = 0.5f;
        if(currTime_s - _behaviorCheckTime >= timeToHold_s ) {
          
          // now send the cube to a place where the robot will be able to see / interact with it
          SetLightCubePose(_cubeToMove, kidnapCubePose);

          // first move the head down
          SendMoveHeadToAngle(0, 100, 100);
          
          // figure out angle to turn to
          float targetAngle = atan2(kidnapCubePose.GetTranslation().y() - GetRobotPoseActual().GetTranslation().y(),
                                    kidnapCubePose.GetTranslation().x() - GetRobotPoseActual().GetTranslation().x());
          
          float robotTrueAngle = GetRobotPoseActual().GetRotation().GetAngleAroundZaxis().ToFloat();
          float robotAssumedAngle = GetRobotPose().GetRotation().GetAngleAroundZaxis().ToFloat();
          float robotAngleError = robotTrueAngle - robotAssumedAngle;
          targetAngle -= robotAngleError;

          // queue turn in place after the move head down
          _turnInPlaceResult = ActionResult::RUNNING;
          SendTurnInPlace(targetAngle, M_PI_F, 500.0f, POINT_TURN_ANGLE_TOL, true, QueueActionPosition::AT_END); // Absolute turn, put at end of action queue.
          
          SET_STATE(TurnToObject2);
        }        
      }
      break;
    }

      
    case TestState::TurnToObject2:
    {
      // Behavior should start right after seeing block (and so TurnInPlace may be cancelled)
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(10,
                                            (_turnInPlaceResult == ActionResult::SUCCESS || _turnInPlaceResult == ActionResult::CANCELLED_WHILE_RUNNING),
                                            _startedBehavior == 2) {
        SET_STATE(Stacking);
      }
      break;
    }
      
    
    case TestState::Stacking:
    {
      CST_ASSERT( _startedBehavior == 2, "Behavior started wrong number of times: " << _startedBehavior );
      
      IF_CONDITION_WITH_TIMEOUT_ASSERT( _stoppedBehavior == 2, 40 ) {
        // behavior should eventually stop
        SET_STATE(VerifyStack)
      }
      break;
    }
    
      
    case TestState::VerifyStack:
    {
      // Verify robot has stacked the blocks
      // NOTE: actual poses are in meters
      Pose3d pose1 = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
      Pose3d pose2 = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE2);

      const float stackingTolerance_mm = 40.0f;
      
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            GetCarryingObjectID() == -1,
                                            // (x,y) positions are nearly on top of each other
                                            NEAR(pose1.GetTranslation().x(), pose2.GetTranslation().x(), stackingTolerance_mm),
                                            NEAR(pose1.GetTranslation().y(), pose2.GetTranslation().y(), stackingTolerance_mm),
                                            // difference between z's is about a block height
                                            NEAR( ABS(pose2.GetTranslation().z() - pose1.GetTranslation().z()), 44.0f, 10.0f)) {
        // Cancel the stack behavior:
        SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::ExecuteBehaviorByName("NoneBehavior")));
        // Now attempt to pick up the top block:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(ObjectType::Block_LIGHTCUBE1);
        SET_STATE(AttemptPickupHigh)
      }
      break;
    }
      
      
    case TestState::AttemptPickupHigh:
    {
      const Pose3d robotPose = GetRobotPoseActual();
      const Pose3d cubePose = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThresholdHigh_mm;
      IF_CONDITION_WITH_TIMEOUT_ASSERT(nearBlock, 15) {
        // Push the block out of view so that the pickup will fail.
        SendApplyForce("cube0", -20, -5, 5);
        SET_STATE(PickupHighShouldFail)
      }
      
      break;
    }
      
      
    case TestState::PickupHighShouldFail:
    {
      // Keep applying a force ocassionally to keep the block 'moving', so that the pickup will
      //  fail with PICKUP_OBJECT_UNEXPECTEDLY_MOVING.
      static int cnt = 1;
      if ((++cnt % 5) == 0) {
        SendApplyForce("cube0", -5, 0, 0);
      }
      
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_MOVING, 15) {
        // Pick up object 1:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(ObjectType::Block_LIGHTCUBE2);
        SET_STATE(AttemptPickupLowOutOfView)
      }
      
      break;
    }
      
      
    case TestState::AttemptPickupLowOutOfView:
    {
      Pose3d robotPose = GetRobotPoseActual();
      Pose3d cubePose = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE2);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThreshold_mm;
      IF_CONDITION_WITH_TIMEOUT_ASSERT(nearBlock, 15) {
        // Push the block out of view:
        SendApplyForce("cube1", 20, -7, 10);
        SET_STATE(PickupLowOutOfViewShouldFail)
      }
      
      break;
    }
    
      
    case TestState::PickupLowOutOfViewShouldFail:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING, DEFAULT_TIMEOUT) {
        // move cube in front of robot and look at it:
        Pose3d cubePose = GetRobotPoseActual();
        Vec3f T = cubePose.GetTranslation();
        cubePose.SetTranslation(Vec3f(T.x(), T.y() - 100.f, 25));
        SetLightCubePose(ObjectType::Block_LIGHTCUBE2, cubePose);
        // move head up to see the cube:
        _moveHeadToAngleResult = ActionResult::RUNNING;
        SendMoveHeadToAngle(0, 100, 100);
        SET_STATE(MoveHeadUp)
      }
      
      break;
    }
      
      
    case TestState::MoveHeadUp:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_moveHeadToAngleResult == ActionResult::SUCCESS, 5) {
        // pick up object 1:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(ObjectType::Block_LIGHTCUBE2);
        SET_STATE(AttemptPickupLowInView)
      }
      
      break;
    }
      
      
    case TestState::AttemptPickupLowInView:
    {
      Pose3d robotPose = GetRobotPoseActual();
      Pose3d cubePose = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE2);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThreshold_mm;
      IF_CONDITION_WITH_TIMEOUT_ASSERT(nearBlock, 15) {
        // Push the block out of view:
        SendApplyForce("cube1", 20, -7, 10);
        // Keep track of where the robot was when the cube was first pushed.
        _robotPoseWhenBlockShoved = GetRobotPoseActual();
        SET_STATE(TeleportBlockInView)
      }
      
      break;
    }
      
      
    case TestState::TeleportBlockInView:
    {
      // Keep applying a force ocassionally to keep the block moving,
      //  to trick Cozmo into thinking it's moving as expected.
      static int cnt = 0;
      if ((++cnt % 5) == 0) {
        SendApplyForce("cube1", -5, 0, 0);
      }
      
      Pose3d robotPoseNow = GetRobotPoseActual();
        
      IF_CONDITION_WITH_TIMEOUT_ASSERT(ComputeDistanceBetween(_robotPoseWhenBlockShoved, robotPoseNow) > 35.f, DEFAULT_TIMEOUT) {
        // Teleport the cube to just in front of the robot:
        Pose3d cubePose = GetRobotPoseActual();
        Vec3f T = cubePose.GetTranslation();
        cubePose.SetTranslation(Vec3f(T.x(), T.y() - 100.f, 22));
        SetLightCubePose(ObjectType::Block_LIGHTCUBE2, cubePose);
        SET_STATE(PickupLowInViewShouldFail)
      }
      
      break;
    }

      
    case TestState::PickupLowInViewShouldFail:
    {
      // Cozmo should see the cube in front of him and realize he's not carrying it.
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::NOT_CARRYING_OBJECT_RETRY, DEFAULT_TIMEOUT) {
        // Pick up the cube again:
        _pickupObjectResult = ActionResult::RUNNING;
        SendPickupObjectByType(ObjectType::Block_LIGHTCUBE2);
        
        SET_STATE(RemoveCube)
      }
      
      break;
    }

      
    case TestState::RemoveCube:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_pickupObjectResult == ActionResult::SUCCESS, DEFAULT_TIMEOUT) {
        CST_ASSERT(GetCarryingObjectID() == 1, "robot should be carrying object at this point");
        
        // Remove the light cube from the world (as it someone has taken it off the lift)
        CST_ASSERT(RemoveLightCubeByType(ObjectType::Block_LIGHTCUBE2), "LightCube removal failed");
        
        // Attempt to place the now-removed block:
        Pose3d placePose = GetRobotPoseActual();
        Vec3f T = placePose.GetTranslation();
        placePose.SetTranslation(Vec3f(T.x(), T.y() - 100.f, 22));
        
        _placeObjectResult = ActionResult::RUNNING;
        SendPlaceObjectOnGroundSequence(placePose, motionProfile);
        
        SET_STATE(PlaceObjectShouldFail)
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
      IF_CONDITION_WITH_TIMEOUT_ASSERT(actionFailed, 10)
      {
        PRINT_NAMED_INFO("CST_StackBlockBehavior.TestInfo",
                         "Finished PlaceObjectShouldFail with code '%s'",
                         EnumToString(_placeObjectResult));
        SET_STATE(TestDone)
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


void CST_StackBlockBehavior::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg)
{
  PRINT_NAMED_INFO("CST_StackBlockBehavior.HandleRobotCompletedAction", "completed action %s, result %s", EnumToString(msg.actionType), EnumToString(msg.result));
  
  if(msg.actionType == RobotActionType::MOVE_HEAD_TO_ANGLE) {
    _moveHeadToAngleResult = msg.result;
  } else if(msg.actionType == RobotActionType::TURN_IN_PLACE) {
    _turnInPlaceResult = msg.result;
  } else if (msg.actionType == RobotActionType::PICKUP_OBJECT_HIGH
             || msg.actionType == RobotActionType::PICKUP_OBJECT_LOW) {
    _pickupObjectResult = msg.result;
  } else if (msg.actionType == RobotActionType::PLACE_OBJECT_HIGH
             || msg.actionType == RobotActionType::PLACE_OBJECT_LOW) {
    _placeObjectResult = msg.result;
  }
}
  
void CST_StackBlockBehavior::HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg)
{
  PRINT_NAMED_INFO("CST_StackBlockBehavior.transition", "%s -> %s",
                   msg.oldBehaviorName.c_str(),
                   msg.newBehaviorName.c_str());
  
  if(msg.oldBehaviorName == kBehaviorName) {
    _stoppedBehavior++;
  }
  if(msg.newBehaviorName == kBehaviorName) {
    _startedBehavior++;
  }
}
  
void CST_StackBlockBehavior::HandleActiveObjectConnectionState(const ObjectConnectionState& msg)
{
  if (msg.connected) {
    ++_numObjectsConnected;
  } else if (_numObjectsConnected > 0) {
    --_numObjectsConnected;
  }
}
  
void CST_StackBlockBehavior::SendPickupObjectByType(const ObjectType& objType)
{
  PRINT_NAMED_INFO("CST_StackBlockBehavior.SendPickupObjectByType", "Attempting to SendPickupObject for Object Type %s", EnumToString(objType));
  
  std::vector<s32> objIds = GetAllObjectIDsByFamilyAndType(ObjectFamily::LightCube, objType);
  
  // Should have found exactly one object ID for this object type:
  CST_ASSERT(objIds.size() == 1, "Did not find exactly one object ID for given type!");
  
  SendPickupObject(objIds[0], motionProfile, true);
}
  

}
}
