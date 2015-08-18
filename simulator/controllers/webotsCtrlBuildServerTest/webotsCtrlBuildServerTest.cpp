/**
* File: webotsCtrlBuildServerTest.cpp
*
* Author: damjan stulic
* Created: 8/12/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#include "webotsCtrlBuildServerTest.h"

#include <opencv2/imgproc/imgproc.hpp>
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "anki/cozmo/shared/ledTypes.h"
#include "anki/cozmo/shared/activeBlockTypes.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/vision/basestation/image.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/data/dataPlatform.h"
#include "clad/types/actionTypes.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/ptree/ptreeTools.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iterator>


namespace Anki {
namespace Cozmo {


  BuildServerTestController::BuildServerTestController(s32 step_time_ms) :
  UiGameController(step_time_ms)
  {
    
  }
  
  void BuildServerTestController::SetTestFileName(const std::string& testFileName)
  {
    _testFileName = testFileName;
  }

  s32 BuildServerTestController::UpdateInternal()
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
        PRINT_NAMED_INFO("TestController.Update.TestDone","Result: %d", _result);
        QuitWebots(_result);
        break;
    }
    
    
    return _result;
  }

  void BuildServerTestController::InitInternal()
  {
    _result = 0;
    
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
      _result = -1;
      _testState = TestState::TestDone;
    }
  }

  void BuildServerTestController::AnimationCompleted(const std::string& animationName)
  {
    if (_testState == TestState::ExecutingTestAnimation && animationName.compare(_currentAnim) == 0) {
      _testState = TestState::ReadyForNextCommand;
    }
  }

  void BuildServerTestController::ExecuteTestAnimation(const std::string& animToPlay)
  {
    _currentAnim = animToPlay;
    _testState = TestState::ExecutingTestAnimation;
    SendAnimation(_currentAnim.c_str(), 1);
  }

  
  
  
  
  // ================ Message handler callbacks ==================
  
  void BuildServerTestController::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
  {
    switch((RobotActionType)msg.actionType)
    {
      case RobotActionType::PICKUP_OBJECT_HIGH:
      case RobotActionType::PICKUP_OBJECT_LOW:
        break;
   
      case RobotActionType::PLACE_OBJECT_HIGH:
      case RobotActionType::PLACE_OBJECT_LOW:
        break;
   
      case RobotActionType::PLAY_ANIMATION:
        printf("Robot %d finished playing animation %s. [Tag=%d]\n",
               msg.robotID, msg.completionInfo.animName.c_str(), msg.idTag);
        AnimationCompleted(msg.completionInfo.animName);
        break;
   
      default:
        break;
    }
   
   }
   
   void BuildServerTestController::HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg)
   {
     _animationAvailableCount++;
     PRINT_NAMED_INFO("HandleAnimationAvailable", "Animation available: %s", msg.animName.c_str());
   }
   
   // ============== End of message handlers =================

  
  

} // end namespace Cozmo
} // end namespace Anki



// =======================================================================

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  Anki::Cozmo::BuildServerTestController buildServerTestCtrl(BS_TIME_STEP);
  
  Anki::Util::PrintfLoggerProvider loggerProvider;
  loggerProvider.SetMinLogLevel(0);
  Anki::Util::gLoggerProvider = &loggerProvider;
  
  // Get the last position of '/'
  std::string aux(argv[0]);
  #if defined(_WIN32) || defined(WIN32)
  size_t pos = aux.rfind('\\');
  #else
  size_t pos = aux.rfind('/');
  #endif
  // Get the path and the name
  std::string path = aux.substr(0,pos+1);
  //std::string name = aux.substr(pos+1);
  std::string resourcePath = path;
  std::string filesPath = path + "temp";
  std::string cachePath = path + "temp";
  std::string externalPath = path + "temp";
  Data::DataPlatform dataPlatform(filesPath, cachePath, externalPath, resourcePath);
   
  buildServerTestCtrl.SetDataPlatform(&dataPlatform);
  if (argc > 1) {
    buildServerTestCtrl.SetTestFileName(argv[1]);
  }

  buildServerTestCtrl.Init();
  
  while (buildServerTestCtrl.Update() == 0)
  {
  }
  
  Anki::Util::gLoggerProvider = nullptr;
  return 0;
}

