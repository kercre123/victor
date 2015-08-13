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

#ifndef __testController_H_
#define __testController_H_

#include <string>
#include <queue>

namespace Anki {
namespace Cozmo {
namespace WebotsKeyboardController {

enum class TestState {
  NotStarted,
  WaitingForInit,
  ReadyForNextCommand,
  ExecutingTestAnimation,
  TestDone
};

class TestController {
public:
  void AddTest(std::string&& testName);
  void Update();
  void AnimationCompleted(const std::string& animationName);
private:
  void ExecuteTestAnimation();
  TestState _testState = TestState::NotStarted;
  std::queue<std::string> _commands;
  size_t _totalCommands = 0;
  size_t _tickCounter = 0;
  size_t _waitCounter = 0;
};


} // end namespace WebotsKeyboardController
} // end namespace Cozmo
} // end namespace Anki









#endif //__testController_H_
