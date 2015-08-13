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
#include "json/json.h"

namespace Anki {
namespace Cozmo {
namespace Data{
class DataPlatform;
}
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
  void SetTestFileName(const std::string& testFileName);
  void SetDataPlatform(Data::DataPlatform* dataPlatform) {_dataPlatform = dataPlatform;}
  void Update();
  void AnimationCompleted(const std::string& animationName);
private:
  void Init();
  void ExecuteTestAnimation(const std::string& animToPlay);
  Data::DataPlatform* _dataPlatform = nullptr;
  TestState _testState = TestState::NotStarted;
  std::string _testFileName;
  std::queue<Json::Value> _commands;
  size_t _tickCounter = 0;
  size_t _waitCounter = 0;
  std::string _currentAnim;
};


} // end namespace WebotsKeyboardController
} // end namespace Cozmo
} // end namespace Anki









#endif //__testController_H_
