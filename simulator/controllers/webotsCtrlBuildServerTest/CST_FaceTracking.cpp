/**
 * File: CST_FaceTracking
 *
 * Author: Wesley Yue
 * Created: 07/11/2016
 *
 * Description: Tests face tracking at various velocity and trajectories
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"

namespace Anki {
namespace Cozmo {
enum class TestState {
  Init,
  WaitToObserveFace,
  StopFace1,
  VerifyTranslationThenTranslateFaceIn3d,
  StopFace2,
  Exit
};

class CST_FaceTracking : public CozmoSimTestController
{
public:
  CST_FaceTracking();

private:
  s32 UpdateSimInternal() override;

  TestState _testState = TestState::Init;
  s32 _result = 0;

  Pose3d _facePose;
  bool _faceIsObserved = false;
  webots::Node* _face = nullptr;

  double _waitTimer = -1;

  void HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg) override;
};

REGISTER_COZMO_SIM_TEST_CLASS(CST_FaceTracking);

CST_FaceTracking::CST_FaceTracking() {}

s32 CST_FaceTracking::UpdateSimInternal()
{
  auto ZeroVelocityAfterXSeconds = [this](webots::Node* node, double xSeconds, TestState nextState){
    IF_CONDITION_WITH_TIMEOUT_ASSERT(HasXSecondsPassedYet(_waitTimer, xSeconds), xSeconds + 1){
      node->setVelocity((double[]){0, 0, 0, 0, 0, 0});
      _testState = nextState;
    }
  };

  const f32 headLookupAngle_rad = DEG_TO_RAD(15);
  const f32 headAngleTolerance_rad = DEG_TO_RAD(1);

  switch (_testState) {
    case TestState::Init:
    {
      CozmoSimTestController::MakeSynchronous();
      SendMoveHeadToAngle(headLookupAngle_rad, 100, 100);
      _face = GetNodeByDefName("Face_1");

      _testState = TestState::WaitToObserveFace;
      break;
    }

    case TestState::WaitToObserveFace:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(HasXSecondsPassedYet(_waitTimer, 1) &&
                                       _faceIsObserved &&
                                       NEAR(GetRobotHeadAngle_rad(), headLookupAngle_rad, headAngleTolerance_rad),
                                       5) {
        // Move in the y direction
        _face->setVelocity((double[]){0, 1, 0, 0, 0, 0});
        _testState = TestState::StopFace1;
      }
      break;
    }

    case TestState::StopFace1:
    {
      ZeroVelocityAfterXSeconds(_face, 0.5, TestState::VerifyTranslationThenTranslateFaceIn3d);
      break;
    }

    case TestState::VerifyTranslationThenTranslateFaceIn3d:
    {
      // (663, 259, 393) is the appx. position of the face after translating for 0.5 seconds.
      const Vec3f expectedFaceTranslation(663, 259, 393);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(
          NEAR(_facePose.GetTranslation().x(), expectedFaceTranslation.x(), 1) &&
          NEAR(_facePose.GetTranslation().y(), expectedFaceTranslation.y(), 1) &&
          NEAR(_facePose.GetTranslation().z(), expectedFaceTranslation.z(), 1),
          2) {
        // Moving at an faster but arbitrary speed
        _face->setVelocity((double[]){-1.5, -1.5, -1.1, 0, 0, 0});
        _testState = TestState::StopFace2;
      }
      break;
    }

    case TestState::StopFace2:
    {
      // 0.15 seconds is allows the face to move across the camera view but is not so long that the
      // face will go out of view
      ZeroVelocityAfterXSeconds(_face, 0.15, TestState::Exit);
      break;
    }

    case TestState::Exit:
    {
      // (332, -102, 136) is the appx. position of the face after translating for 0.15 seconds.
      const Vec3f expectedFaceTranslation(332, -102, 136);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(
          NEAR(_facePose.GetTranslation().x(), expectedFaceTranslation.x(), 1) &&
          NEAR(_facePose.GetTranslation().y(), expectedFaceTranslation.y(), 1) &&
          NEAR(_facePose.GetTranslation().z(), expectedFaceTranslation.z(), 1),
          2) {
        CST_EXIT();
      }
      break;
    }
  }

  return _result;
}

void CST_FaceTracking::HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg)
{
  _facePose = CreatePoseHelper(msg.pose);
  _faceIsObserved = true;
}

}  // namespace Cozmo
}  // namespace Anki
