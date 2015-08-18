/**
* File: webotsCtrlBuildServerTest.h
*
* Author: damjan stulic
* Created: 8/12/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#ifndef __webotsCtrlBuildServerTest_H_
#define __webotsCtrlBuildServerTest_H_

#include <string>
#include <queue>
#include "json/json.h"
#include "anki/cozmo/simulator/game/uiGameController.h"

namespace Anki {
namespace Cozmo {
namespace Data{
class DataPlatform;
}


enum class TestState {
  NotStarted,
  WaitingForInit,
  ReadyForNextCommand,
  ExecutingTestAnimation,
  TestDone
};

  class BuildServerTestController : public UiGameController {
public:
  BuildServerTestController(s32 step_time_ms);
    
  void SetTestFileName(const std::string& testFileName);
  void SetDataPlatform(Data::DataPlatform* dataPlatform) {_dataPlatform = dataPlatform;}
  void AnimationCompleted(const std::string& animationName);
private:
  virtual void InitInternal() override;
  virtual s32 UpdateInternal() override;

  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
  virtual void HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg) override;
    
    
  void ExecuteTestAnimation(const std::string& animToPlay);
  Data::DataPlatform* _dataPlatform = nullptr;
  TestState _testState = TestState::NotStarted;
  std::string _testFileName;
  std::queue<Json::Value> _commands;
  size_t _tickCounter = 0;
  size_t _waitCounter = 0;
  std::string _currentAnim;
  u32 _animationAvailableCount;
    
  int _result;

};



} // end namespace Cozmo
} // end namespace Anki



#endif //__webotsCtrlBuildServerTest_H_
