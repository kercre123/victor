/**
 * File: CST_PickUpBlockThenSeeDropped.cpp
 *
 * Author: raul
 * Created: 2/16/17
 *
 * Description: Check that Cozmo properly detaches a cube from the lift when it observers the cube somewhere else. This
 *              should work regardless of move messages, since lifted cubes produce move messages when the robot moves,
 *              and we can potentially pick up unconnected blocks.
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
    
enum class TestState {
  Init,
  PickupObject,
  TeleportObject,
  MoveBack,
  TestDone
};
  
namespace {

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
PathMotionProfile motionProfile3(defaultPathSpeed_mmps,
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

const f32 ROBOT_POSITION_TOL_MM = 15;
const f32 ROBOT_ANGLE_TOL_DEG = 5;
const f32 BLOCK_Z_TOL_MM = 5;

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Declaration
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class CST_PickUpBlockThenSeeDropped : public CozmoSimTestController
{
private:
  
  virtual s32 UpdateSimInternal() override;
  
  TestState _testState = TestState::Init;
  
  bool _lastActionSucceeded = false;

  // causes the lifted cube to drop
  void DropCube();
  
  // Message handlers
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
};
  
// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_PickUpBlockThenSeeDropped);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Implemenation
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void CST_PickUpBlockThenSeeDropped::DropCube()
{
  // TODO Replace with new message KevinY created for me :D
  // rsam: Experimentally this causes the cube to drop by causing a DisengageGripper() call, but
  // we should have a proper message for that. I think Matt was working on that.
  f32 liftSpeedDegPerSec  = DEG_TO_RAD(120);
  f32 liftAccelDegPerSec2 = DEG_TO_RAD(600);
  f32 liftDurationSec = 0;
  SendMoveLiftToHeight(LIFT_HEIGHT_HIGHDOCK,liftSpeedDegPerSec,liftAccelDegPerSec2,liftDurationSec);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
s32 CST_PickUpBlockThenSeeDropped::UpdateSimInternal()
{
  switch (_testState) {
    case TestState::Init:
    {
      MakeSynchronous();
      StartMovieConditional("PickUpBlockThenSeeDropped");
      //TakeScreenshotsAtInterval("StackBlocks", 1.f);
      
      SendMoveHeadToAngle(0, 100, 100);
      _testState = TestState::PickupObject;
      break;
    }
    case TestState::PickupObject:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                            GetNumObjects() == 1)
      {
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 1;
        // Pickup object 0
        m.action.Set_pickupObject(ExternalInterface::PickupObject(0, motionProfile3, 0, false, true, false, true));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
        _testState = TestState::TeleportObject;
      }
      break;
    }
    case TestState::TeleportObject:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, ROBOT_ANGLE_TOL_DEG),
                                            NEAR(GetRobotPose().GetTranslation().x(), 36, ROBOT_POSITION_TOL_MM),
                                            NEAR(GetRobotPose().GetTranslation().y(), 0, ROBOT_POSITION_TOL_MM),
                                            GetCarryingObjectID() == 0)
      {
        DropCube();
        
        _testState = TestState::MoveBack;
      }
      break;
    }
    case TestState::MoveBack:
    {
      const float kCubeHalfHeight_mm = 44 * 0.5f; // I don't see a way in webots to get size of node, and other CST have this hardcoded
      const float cubeZ = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1).GetTranslation().z() - kCubeHalfHeight_mm;
      const float robotZ = GetRobotPose().GetTranslation().z();
    
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            GetCarryingObjectID() == 0,
                                            NEAR(cubeZ, robotZ, BLOCK_Z_TOL_MM)) // near Z (cube should be resting on floor)
      {
        // move back
        const float dist_mm = 50.0f;
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 3;
        m.action.Set_driveStraight(ExternalInterface::DriveStraight(200, -dist_mm, true));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
        
        _testState = TestState::TestDone;
      }
      break;
    }
    
    case TestState::TestDone:
    {
      // Verify we are not carrying the object anymore
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            GetCarryingObjectID() == -1) // should not be carrying it anymore
      {
        StopMovie();
        CST_EXIT();
      }
      break;
    }
  }
  return _result;
}


// ================ Message handler callbacks ==================
void CST_PickUpBlockThenSeeDropped::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
{
  if (msg.result == ActionResult::SUCCESS) {
    _lastActionSucceeded = true;
  }
}

// ================ End of message handler callbacks ==================
  
} // end namespace Cozmo
} // end namespace Anki

