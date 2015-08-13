/**
* File: testController
*
* Author: damjan stulic
* Created: 8/12/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/
#include "testController.h"
#include "util/logging/logging.h"
#include "webotsCtrlKeyboard.h"
namespace Anki {
namespace Cozmo {
namespace WebotsKeyboardController {

void TestController::AddTest(std::string&& testName)
{
  _commands.emplace(testName);
}

void TestController::Update()
{
  ++_tickCounter;
  switch (_testState) {

    case TestState::NotStarted:
      _totalCommands = (uint32_t)_commands.size();
      if (_totalCommands > 0) {
        _testState = TestState::WaitingForInit;
        _waitCounter = _tickCounter;
      } else {
        _testState = TestState::TestDone;
      }
      break;

    case TestState::WaitingForInit:
      if (_tickCounter - _waitCounter > 10) {
        _testState = TestState::ReadyForNextCommand;
      }
      break;

    case TestState::ReadyForNextCommand:
    {
      if (_commands.size() == 0) {
        _testState = TestState::TestDone;
        PRINT_NAMED_INFO("TestController.Update", "all tests completed");
        break;
      }
      const std::string& commandToExecute = _commands.front();
      PRINT_NAMED_INFO("TestController.Update", "executing command %s", commandToExecute.c_str());
      if (commandToExecute.compare("testAnim") == 0) {
        ExecuteTestAnimation();
      } else {
        PRINT_NAMED_ERROR("TestController.Update", "unknown command %s", commandToExecute.c_str());
      }
      _commands.pop();
    }
      break;

    case TestState::ExecutingTestAnimation:
      break;

    case TestState::TestDone:
      break;
  }
}

void TestController::AnimationCompleted(const std::string& animationName)
{
  if (_testState == TestState::ExecutingTestAnimation && animationName.compare("Hello") == 0) {
    _testState = TestState::ReadyForNextCommand;
  }
}

void TestController::ExecuteTestAnimation()
{
  _testState = TestState::ExecutingTestAnimation;
  WebotsKeyboardController::SendAnimation("Hello", 1);
}

} // end namespace WebotsKeyboardController
} // end namespace Cozmo
} // end namespace Anki

