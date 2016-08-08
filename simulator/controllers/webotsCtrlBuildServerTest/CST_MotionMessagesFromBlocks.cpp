#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include <string>

namespace Anki {
namespace Cozmo {

enum class TestState {
  Init,
  TapCube,
  CheckForTappedMessage,
  CheckForStoppedMessage,
  MoveCube,
  CheckForMovedMessage,
  CheckForStoppedMessage1,
  Exit
};

class CST_MotionMessagesFromBlocks : public CozmoSimTestController
{
public:
  CST_MotionMessagesFromBlocks();

private:
  s32 UpdateSimInternal() override;

  void HandleActiveObjectTapped(const ObjectTapped& msg) override;
  void HandleActiveObjectStoppedMoving(const ObjectStoppedMoving& msg) override;
  void HandleActiveObjectMoved(const ObjectMoved& msg) override;

  TestState _testState = TestState::Init;
  const Pose3d _cubePose1 = {0, Vec3f(0.f, 0.f, 1.f), Vec3f(200.f, 50.f, 22.1f)};
  bool _wasTapped = false;
  bool _wasStopped = false;
  bool _wasMoved = false;
};

REGISTER_COZMO_SIM_TEST_CLASS(CST_MotionMessagesFromBlocks);

CST_MotionMessagesFromBlocks::CST_MotionMessagesFromBlocks() {}

s32 CST_MotionMessagesFromBlocks::UpdateSimInternal()
{
  switch(_testState) {
    case TestState::Init:
    {
      CozmoSimTestController::MakeSynchronous();
      SendEnableBlockTapFilter(false);

      _testState = TestState::TapCube;
      break;
    }

    case TestState::TapCube:
    {
      _wasTapped = false;
      _wasStopped = false;
      _wasMoved = false;
      UiGameController::SendApplyForce("cube", 0, 0, 6);
      _testState = TestState::CheckForTappedMessage;
      break;
    }

    case TestState::CheckForTappedMessage:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_wasTapped, 5) {
        _testState = TestState::CheckForStoppedMessage;
      }

      break;
    }

    case TestState::CheckForStoppedMessage:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_wasStopped, 5) {
        _testState = TestState::MoveCube;
      }
      break;
    }

    case TestState::MoveCube:
    {
      _wasTapped = false;
      _wasStopped = false;
      _wasMoved = false;
      UiGameController::SendApplyForce("cube", 0, 0, 6);
      _testState = TestState::CheckForMovedMessage;
      break;
    }

    case TestState::CheckForMovedMessage:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_wasMoved, 5) {
        _testState = TestState::CheckForStoppedMessage1;
      }
      break;
    }

    case TestState::CheckForStoppedMessage1:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_wasStopped, 5) {
        _testState = TestState::Exit;
      }
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

void CST_MotionMessagesFromBlocks::HandleActiveObjectTapped(const ObjectTapped& msg)
{
  _wasTapped = true;
}

void CST_MotionMessagesFromBlocks::HandleActiveObjectStoppedMoving(const ObjectStoppedMoving& msg)
{
  _wasStopped = true;
}

void CST_MotionMessagesFromBlocks::HandleActiveObjectMoved(const ObjectMoved& msg)
{
  _wasMoved = true;
}

}  // namespace Cozmo
}  // namespace Anki
