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
    Setup,
    ReadyForNextCommand,
    ExecutingTestAnimation,
    WaitUntilEndOfMessage,
    TestDone
  };
  
  // ============ Test class declaration ============
  class CST_Animations : public CozmoSimTestController {
    
  private:
    virtual s32 UpdateSimInternal() override;
    
    virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    virtual void HandleAnimationAborted(const ExternalInterface::AnimationAborted& msg) override;
    virtual void HandleEndOfMessage(const ExternalInterface::EndOfMessage& msg) override;
    
    TestState _testState = TestState::InitCheck;

    std::string _lastAnimPlayed = "";
    double _lastAnimPlayedTime = 0;
    bool _receivedAllAnimations = false;
    std::string _animationToPlay = "";
  };
  
  // Register class with factory
  REGISTER_COZMO_SIM_TEST_CLASS(CST_Animations);
  
  
  // =========== Test class implementation ===========

  s32 CST_Animations::UpdateSimInternal()
  {
    switch (_testState) {
      case TestState::InitCheck:
      {
        _animationToPlay = UiGameController::GetAnimationTestName();
        PRINT_NAMED_DEBUG("CST_Animations.InitCheck",
                          "Specified animation from .wbt world file: %s", _animationToPlay.c_str());

        // If the sepcified animation is empty, wait until all the availalble animations have been
        // broadcasted by the robot and exit. Useful to fetch and log available animations by
        // running this test once without specifying animation to test.
        if (_animationToPlay == "%ANIMATION_TEST_NAME%") {
          _testState = TestState::WaitUntilEndOfMessage;
        } else {
          _testState = TestState::Setup;
        }

        break;
      }

      case TestState::Setup:
      {
        // Sends a CLAD message to robot so that RobotAudioOutputSource is set to PlayOnRobot,
        // otherwise a lot of the audio bugs will not occur. See robotAudioClient.h on the robot
        // side for more information
        ExternalInterface::SetRobotAudioOutputSource m;
        m.source = ExternalInterface::RobotAudioOutputSourceCLAD::PlayOnRobot;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetRobotAudioOutputSource(m);
        SendMessage(message);

        _testState = TestState::ReadyForNextCommand;
      }

      case TestState::ReadyForNextCommand:
      {
        PRINT_NAMED_INFO("CST_Animations.PlayingAnimation", "%s", _animationToPlay.c_str());
        u32 numLoops = 1;
        SendAnimation(_animationToPlay.c_str(), numLoops);
        _lastAnimPlayed = _animationToPlay;
        _lastAnimPlayedTime = GetSupervisor()->getTime();
        _testState = TestState::ExecutingTestAnimation;

        break;
      }
        
      case TestState::ExecutingTestAnimation:
      {
        double timeoutInSeconds = 99;
        // If no action complete message, this will timeout with assert.
        if (CONDITION_WITH_TIMEOUT_ASSERT(_lastAnimPlayed.empty(), _lastAnimPlayedTime, timeoutInSeconds)) {
          _testState = TestState::TestDone;
        }
        break;
      }

      case TestState::WaitUntilEndOfMessage:
      {
        double timeoutInSeconds = 10;
        IF_CONDITION_WITH_TIMEOUT_ASSERT(_receivedAllAnimations, timeoutInSeconds){
          _testState = TestState::TestDone;
        }
      }

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
               msg.robotID, msg.completionInfo.Get_animationCompleted().animationName.c_str(), msg.idTag);
        if ((_testState == TestState::ExecutingTestAnimation) &&
            (msg.completionInfo.Get_animationCompleted().animationName.compare(_lastAnimPlayed) == 0)) {
          
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

  void CST_Animations::HandleAnimationAborted(const ExternalInterface::AnimationAborted& msg)
  {
    PRINT_NAMED_WARNING("CST_Animations.HandleAnimationAborted",
                      "'%s' was aborted.",
                      _animationToPlay.c_str());
    _testState = TestState::TestDone;
  }

  void CST_Animations::HandleEndOfMessage(const ExternalInterface::EndOfMessage& msg)
  {
    if (msg.messageType == ExternalInterface::MessageType::AnimationAvailable) {
      PRINT_NAMED_INFO("CST_Animations.HandleEndOfMessage", "Received `EndOfMessage`; messageType: %s", ExternalInterface::MessageTypeToString(msg.messageType));
      _receivedAllAnimations = true;
    }
  }
  // ============== End of message handlers =================

  

} // end namespace Cozmo
} // end namespace Anki

