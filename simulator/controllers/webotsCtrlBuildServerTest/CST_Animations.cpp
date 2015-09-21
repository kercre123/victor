/**
* File: CST_Animations.cpp
*
* Author: damjan stulic
* Created: 8/12/15
*
* Description: Checks that there at least a certain number of animations available and
*              and that a few of them can be played successfully.
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
    InitCheck,
    ReadyForNextCommand,
    ExecutingTestAnimation,
    TestDone
  };
  
  // ============ Test class declaration ============
  class CST_Animations : public CozmoSimTestController {
    
  private:
    const u32 MIN_NUM_ANIMS_REQUIRED = 10;
    const u32 NUM_ANIMS_TO_PLAY = 1;
    
    virtual s32 UpdateInternal() override;
    
    virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    
    TestState _testState = TestState::InitCheck;

    std::unordered_set<std::string>::iterator _currAnimIter;
    std::string _lastAnimPlayed = "";
    double _lastAnimPlayedTime = 0;
    u32 _numAnimsPlayed = 0;
  };
  
  // Register class with factory
  REGISTER_COZMO_SIM_TEST_CLASS(CST_Animations);
  
  
  // =========== Test class implementation ===========
  
  s32 CST_Animations::UpdateInternal()
  {
    switch (_testState) {

      case TestState::InitCheck:
       
        // Check that there were atleast a few animations defined
        // TODO: Should animFailedToLoad be an EngineToGame message? Or do we test this is EngineTestController (which doesn't exist yet) that can listen to events.
        CST_ASSERT (GetNumAvailableAnimations() >= MIN_NUM_ANIMS_REQUIRED,
                    GetNumAvailableAnimations() << " anims loaded but " << MIN_NUM_ANIMS_REQUIRED << " expected");

        _currAnimIter = GetAvailableAnimations().begin();
        
        _testState = TestState::ReadyForNextCommand;
        break;

      case TestState::ReadyForNextCommand:
      {
        CST_ASSERT(_currAnimIter != GetAvailableAnimations().end(), "Fewer animations available than expected");
        
        if (_numAnimsPlayed == NUM_ANIMS_TO_PLAY) {
          _testState = TestState::TestDone;
          PRINT_NAMED_INFO("TestController.Update", "all tests completed (Result = %d)", _result);
          break;
        }
        PRINT_NAMED_INFO("CST_Animations.PlayingAnim", "%d: %s", _numAnimsPlayed, _currAnimIter->c_str());
        SendAnimation(_currAnimIter->c_str(), 1);
        _lastAnimPlayed = *_currAnimIter;
        ++_currAnimIter;
        ++_numAnimsPlayed;
        _lastAnimPlayedTime = GetSupervisor()->getTime();
        _testState = TestState::ExecutingTestAnimation;
        
        break;
      }
        
      case TestState::ExecutingTestAnimation:
        // If no action complete message, this will timeout with assert.
        if (CONDITION_WITH_TIMEOUT_ASSERT(_lastAnimPlayed.empty(), _lastAnimPlayedTime, 10)) {
          _testState = TestState::ReadyForNextCommand;
        }
        break;

      case TestState::TestDone:
        CST_EXIT();
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
            (msg.completionInfo.animName.compare(_lastAnimPlayed) == 0)) {
          
          CST_EXPECT(msg.result == ActionResult::SUCCESS,
                     _lastAnimPlayed << " failed to play");
          
          // Clear this to signal to state machine to play the next animation
          _lastAnimPlayed.clear();
        }
        break;
   
      default:
        break;
    }
   
  }
   
  // ============== End of message handlers =================

  

} // end namespace Cozmo
} // end namespace Anki

