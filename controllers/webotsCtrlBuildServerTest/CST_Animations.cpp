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

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include <stdio.h>
#include <string.h>


namespace Anki {
namespace Cozmo {
  
  enum class TestState {
    WaitingForInit,
    ReadyForNextCommand,
    ExecutingTestAnimation,
    TestDone
  };
  
  // ============ Test class declaration ============
  class CST_Animations : public CozmoSimTestController {
  public:
    CST_Animations();
    
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
  
  // Register class with factory
  REGISTER_COZMO_SIM_TEST_CLASS(CST_Animations);
  
  
  // =========== Test class implementation ===========
  
  CST_Animations::CST_Animations() :
  CozmoSimTestController()
  {
    _result = 0;
    _testState = TestState::WaitingForInit;
    _currentAnimIdx = -1;
  }
  

  s32 CST_Animations::UpdateInternal()
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
  
  void CST_Animations::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
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
   
  void CST_Animations::HandleAnimationAvailable(ExternalInterface::AnimationAvailable const& msg)
  {
    _availableAnims.push_back(msg.animName);
    PRINT_NAMED_INFO("HandleAnimationAvailable", "Animation available: %s", msg.animName.c_str());
  }
   
  // ============== End of message handlers =================

  

} // end namespace Cozmo
} // end namespace Anki

