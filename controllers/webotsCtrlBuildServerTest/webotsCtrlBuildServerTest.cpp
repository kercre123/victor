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
#include "clad/types/actionTypes.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/ptree/ptreeTools.h"
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iterator>

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"


namespace Anki {
namespace Cozmo {
  
  
  enum class TestState {
    WaitingForInit,
    ReadyForNextCommand,
    ExecutingTestAnimation,
    TestDone
  };
  
  class COZMO_SIM_TEST_CLASS : public CozmoSimTestController {
  public:
    COZMO_SIM_TEST_CLASS(s32 step_time_ms);
    
  private:
    const u32 MIN_NUM_ANIMS_REQUIRED = 10;
    const u32 NUM_ANIMS_TO_PLAY = 2;
    
    virtual s32 UpdateInternal() override;
    
    virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    virtual void HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg) override;
    
    TestState _testState = TestState::WaitingForInit;

    std::vector<std::string> _availableAnims;
    s32 _currentAnimIdx;
    
    int _result;
  };
  
  
  
  // ==================== Class implementation ====================
  
  COZMO_SIM_TEST_CLASS::COZMO_SIM_TEST_CLASS(s32 step_time_ms) :
  CozmoSimTestController(step_time_ms)
  {
    _result = 0;
    _testState = TestState::WaitingForInit;
    _currentAnimIdx = -1;
  }
  

  s32 COZMO_SIM_TEST_CLASS::UpdateInternal()
  {
    switch (_testState) {

      case TestState::WaitingForInit:
        if (GetSupervisor()->getTime() > 2) {   // TODO: Better way to wait for ready. Ready message to game?
          // Check that there were atleast a few animations defined
          // TODO: Should animFailedToLoad be an EngineToGame message? Or do we test this is EngineTestController (which doesn't exist yet) that can listen to events.
          
          if (_availableAnims.size() < MIN_NUM_ANIMS_REQUIRED) {
            PRINT_NAMED_INFO("BST.UpdateInternal.InsufficientAnimsLoaded", "%lu anims loaded", _availableAnims.size());
            _result = -1;
            _testState = TestState::TestDone;
          } else {
            _testState = TestState::ReadyForNextCommand;
          }
        }
        break;

      case TestState::ReadyForNextCommand:
      {
        if (_currentAnimIdx == NUM_ANIMS_TO_PLAY - 1) {
          _testState = TestState::TestDone;
          PRINT_NAMED_INFO("TestController.Update", "all tests completed (Result = %d)", _result);
          break;
        }
        ++_currentAnimIdx;
        SendAnimation(_availableAnims[_currentAnimIdx].c_str(), 1);
        _testState = TestState::ExecutingTestAnimation;
      }
        break;

      case TestState::ExecutingTestAnimation:
        break;

      case TestState::TestDone:
        //QuitWebots(_result);
        break;
    }
    
    
    return _result;
  }

  
  
  // ================ Message handler callbacks ==================
  
  void COZMO_SIM_TEST_CLASS::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
  {
    switch((RobotActionType)msg.actionType)
    {
      case RobotActionType::PLAY_ANIMATION:
        printf("Robot %d finished playing animation %s. [Tag=%d]\n",
               msg.robotID, msg.completionInfo.animName.c_str(), msg.idTag);
        if ((_testState == TestState::ExecutingTestAnimation) &&
            (_currentAnimIdx >= 0) &&
            (msg.completionInfo.animName.compare(_availableAnims[_currentAnimIdx]) == 0)) {
          if (msg.result == ActionResult::SUCCESS) {
            _testState = TestState::ReadyForNextCommand;
          } else {
            _result = -1;
            _testState = TestState::TestDone;
          }
        }
        break;
   
      default:
        break;
    }
   
  }
   
  void COZMO_SIM_TEST_CLASS::HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg)
  {
    _availableAnims.push_back(msg.animName);
    PRINT_NAMED_INFO("HandleAnimationAvailable", "Animation available: %s", msg.animName.c_str());
  }
   
  // ============== End of message handlers =================

  

} // end namespace Cozmo
} // end namespace Anki



// =======================================================================

RUN_BUILD_SERVER_TEST
