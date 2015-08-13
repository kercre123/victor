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
#include <iterator>
#include "testController.h"
#include "anki/cozmo/basestation/data/dataPlatform.h"
#include "util/logging/logging.h"
#include "util/ptree/ptreeTools.h"
#include "webotsCtrlKeyboard.h"

namespace Anki {
namespace Cozmo {
namespace WebotsKeyboardController {

void TestController::SetTestFileName(const std::string& testFileName)
{
  _testFileName = testFileName;
}

void TestController::Update()
{
  ++_tickCounter;
  switch (_testState) {

    case TestState::NotStarted:
      Init();
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
      const Json::Value& nextTest = _commands.front();
      const std::string& testType = nextTest["type"].asString();
      PRINT_NAMED_INFO("TestController.Update", "executing command %s", testType.c_str());
      if (testType.compare("testAnim") == 0) {
        ExecuteTestAnimation(nextTest["params"]["animToPlay"].asString());
      } else {
        PRINT_NAMED_ERROR("TestController.Update", "unknown command %s", testType.c_str());
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

void TestController::Init()
{
  if (_dataPlatform != nullptr && !_testFileName.empty()) {

    Json::Value config;
    if (!_dataPlatform->readAsJson(Data::Scope::Resources, _testFileName, config)) {
      PRINT_NAMED_ERROR("TestController.loadConfig", "Failed to parse Json file %s", _testFileName.c_str());
    }
    const Json::Value& tests = config["tests"];
    for (const auto& jsonObj : tests) {
      _commands.push(jsonObj);
    }

    _testState = TestState::WaitingForInit;
    _waitCounter = _tickCounter;

  } else {
    _testState = TestState::TestDone;
  }
}

void TestController::AnimationCompleted(const std::string& animationName)
{
  if (_testState == TestState::ExecutingTestAnimation && animationName.compare(_currentAnim) == 0) {
    _testState = TestState::ReadyForNextCommand;
  }
}

void TestController::ExecuteTestAnimation(const std::string& animToPlay)
{
  _currentAnim = animToPlay;
  _testState = TestState::ExecutingTestAnimation;
  WebotsKeyboardController::SendAnimation(_currentAnim.c_str(), 1);
}

} // end namespace WebotsKeyboardController
} // end namespace Cozmo
} // end namespace Anki

