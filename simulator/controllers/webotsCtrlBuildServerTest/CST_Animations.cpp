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
    // const u32 NUM_ANIMS_TO_PLAY = 1;
    
    virtual s32 UpdateInternal() override;
    
    virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    virtual void HandleAnimationAborted(const ExternalInterface::AnimationAborted& msg) override;
    virtual void HandleAnimationAvailable(const ExternalInterface::AnimationAvailable& msg) override;
    virtual void HandleEndOfMessage(const ExternalInterface::EndOfMessage& msg) override;
    
    TestState _testState = TestState::InitCheck;

    std::string _lastAnimPlayed = "";
    double _lastAnimPlayedTime = 0;
    u32 _numAnimsPlayed = 0;
    bool _receivedAllAnimations = false;

    std::vector<std::string> _availableAnimations;
  };
  
  // Register class with factory
  REGISTER_COZMO_SIM_TEST_CLASS(CST_Animations);
  
  
  // =========== Test class implementation ===========
  
//  static const char* kTestAnimationName = "anim_speedTap_ask2play_01";

  s32 CST_Animations::UpdateInternal()
  {
    switch (_testState) {
      case TestState::InitCheck:
      {
        _animationToPlay = UiGameController::GetAnimationTestName();
        PRINT_NAMED_DEBUG("CST_Animations.InitCheck",
                          "Specified animation from .wbt world file: %s", _animationToPlay.c_str());
        // Sends a CLAD message to robot so that RobotAudioOutputSource is set to PlayOnRobot,
        // otherwise a lot of the audio bugs will not occur. See robotAudioClient.h on the robot
        // side for more information
        ExternalInterface::SetRobotAudioOutputSource m;
        m.source = ExternalInterface::RobotAudioOutputSourceCLAD::PlayOnRobot;
        ExternalInterface::MessageGameToEngine message;
        message.Set_SetRobotAudioOutputSource(m);
        SendMessage(message);



        _testState = TestState::ReadyForNextCommand;
        break;
      }
      case TestState::ReadyForNextCommand:
      {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(_receivedAllAnimations, 10){
          if (_numAnimsPlayed == _availableAnimations.size()) {
            _testState = TestState::TestDone;
            PRINT_NAMED_INFO("TestController.Update", "all tests completed (Result = %d)", _result);
            break;
          }
          // auto animToPlay = kTestAnimationName;
          auto animToPlay = _availableAnimations[_numAnimsPlayed];
          PRINT_NAMED_INFO("CST_Animations.PlayingAnim", "%d: %s", _numAnimsPlayed, animToPlay.c_str());
          SendAnimation(animToPlay.c_str(), 1);
          _lastAnimPlayed = animToPlay;
          ++_numAnimsPlayed;
          _lastAnimPlayedTime = GetSupervisor()->getTime();
          _testState = TestState::ExecutingTestAnimation;
        }
        
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
    _result = 255;
    CST_EXIT();
  }

  void CST_Animations::HandleAnimationAvailable(const ExternalInterface::AnimationAvailable& msg)
  {
    _availableAnimations.push_back(msg.animName);
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

