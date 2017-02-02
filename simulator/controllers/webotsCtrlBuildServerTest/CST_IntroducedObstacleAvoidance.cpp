#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/common/basestation/math/rotation.h"

namespace Anki {
namespace Cozmo {
enum class TestState {
  Init,
  MoveCubeOutOfWay,
  WaitOneSecond,
  ExecuteStraightPath,
  IntroduceObstacle,
  VerifyDriveToPoseCompleted,
  VerifyObstacleAvoidance,
  Exit
};

class CST_IntroducedObstacleAvoidance : public CozmoSimTestController
{
public:
  CST_IntroducedObstacleAvoidance();

private:
  s32 UpdateSimInternal() override;

  TestState _testState = TestState::Init;
  s32 _result = 0;

  bool _observedLightCube = false;
  int _lightCubeId = -1;
  bool _driveToPoseSucceeded = false;

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
  PathMotionProfile motionProfile = PathMotionProfile(defaultPathSpeed_mmps,
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

  void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
  void HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg) override;
};

REGISTER_COZMO_SIM_TEST_CLASS(CST_IntroducedObstacleAvoidance);

CST_IntroducedObstacleAvoidance::CST_IntroducedObstacleAvoidance() {}

s32 CST_IntroducedObstacleAvoidance::UpdateSimInternal()
{
  const f32 kHeadLookupAngle_rad = DEG_TO_RAD(7);

  // At a Z height of 22mm, the cube sits flat on the floor since the origin of the cube is in the
  // middle of it.
  const int kCubeHeight_mm = 22;
  const Pose3d kCubeObstructingPose = {0, {0, 0, 1}, {300, 0, kCubeHeight_mm}};
  const Pose3d kRobotDestination = {0, {0, 0, 1}, {500, 0, 0}};

  switch (_testState) {
    case TestState::Init:
    {
      CozmoSimTestController::MakeSynchronous();
      SendMoveHeadToAngle(kHeadLookupAngle_rad, 100, 100);

      _testState = TestState::MoveCubeOutOfWay;
      break;
    }

    case TestState::MoveCubeOutOfWay:
    {
      const f32 kHeadAngleTolerance_rad = DEG_TO_RAD(1);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(NEAR(GetRobotHeadAngle_rad(), kHeadLookupAngle_rad,
                                            kHeadAngleTolerance_rad) &&
                                       _observedLightCube &&
                                       _lightCubeId != -1, 5) {
        // Move cube out of the way before path planning.
        _testState = TestState::WaitOneSecond;
        // Additional Z height to drop the cube and force a cube delocalization.
        SetLightCubePose(_lightCubeId, {0, {0, 0, 1}, {0, 500, kCubeHeight_mm + 10}});
      }
      break;
    }

    case TestState::WaitOneSecond:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(HasXSecondsPassedYet(1), 2){
        _testState = TestState::ExecuteStraightPath;
      }
      break;
    }

    case TestState::ExecuteStraightPath:
    {
      // Note that this does not drive cozmo to the ground truth pose of kRobotDestination, but
      // rather it will be relative to where it starts. That is, if cozmo starts at x=-200mm and the
      // translation in the pose is x=+500mm, he will travel to a ground truth position of +300mm
      // (500-200), and his robot estimated pose will be +500mm.
      SendExecutePathToPose(kRobotDestination, motionProfile, false);
      _testState = TestState::IntroduceObstacle;
      break;
    }

    case TestState::IntroduceObstacle:
    {
      // The time to wait is a somewhat arbitrary choice. It allows the robot to execute some part
      // of the original path but still leave enough room to introduce the obstacle.
      IF_CONDITION_WITH_TIMEOUT_ASSERT(HasXSecondsPassedYet(1.8), 3){
        // Put the cube in the way of the robot path.
        SetLightCubePose(_lightCubeId, kCubeObstructingPose);
        _driveToPoseSucceeded = false;  // reset var just before we check for it in the next stage just in case
        _testState = TestState::VerifyDriveToPoseCompleted;
      }
      break;
    }

    case TestState::VerifyDriveToPoseCompleted:
    {
      // Takes about 12 seconds for robot to complete the path, set timeout at 15 for some leeway.
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_driveToPoseSucceeded, 15){
        _testState = TestState::VerifyObstacleAvoidance;
      }
      break;
    }

    case TestState::VerifyObstacleAvoidance:
    {
      const Point3f kDistanceThreshold = {5, 5, 5};
      const Radians kAngleThreshold = DEG_TO_RAD(1);

      const Pose3d lightCubePoseActual = GetLightCubePoseActual(_lightCubeId);
      Pose3d cubeObstructingPose(kCubeObstructingPose);
      cubeObstructingPose.SetParent(&lightCubePoseActual);

      const Pose3d robotPoseActual = GetRobotPose();
      Pose3d robotDestination(kRobotDestination);
      robotDestination.SetParent(&robotPoseActual);

      CST_ASSERT(lightCubePoseActual.IsSameAs(cubeObstructingPose,
                                              kDistanceThreshold, kAngleThreshold),
                 "The cube was moved when it should have been avoided by the robot.")

      CST_ASSERT(robotPoseActual.IsSameAs(robotPoseActual, kDistanceThreshold, kAngleThreshold),
                 "The robot didn't reach its destination.");

      _testState = TestState::Exit;
      break;
    }

    case TestState::Exit:
    {
      CST_EXIT();
      break;
    }
  }

  return _result;
}

void CST_IntroducedObstacleAvoidance::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
{
  if (msg.actionType == RobotActionType::DRIVE_TO_POSE &&
        msg.result == ActionResult::SUCCESS){
    _driveToPoseSucceeded = true;
  }
}

void CST_IntroducedObstacleAvoidance::HandleRobotObservedObject(const ExternalInterface::RobotObservedObject& msg)
{
  if (!_observedLightCube){
    _lightCubeId = msg.objectID;
    _observedLightCube = true;
  }
}

}  // namespace Cozmo
}  // namespace Anki
