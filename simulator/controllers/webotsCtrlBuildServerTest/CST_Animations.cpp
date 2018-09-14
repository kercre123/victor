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

#include "simulator/game/cozmoSimTestController.h"
#include <stdio.h>
#include <string.h>


namespace Anki {
namespace Vector {

namespace {
static const constexpr float kTimeToWaitForAnimations_s = 3.0f;
static const constexpr int kNumLoops = 1;
}
  
enum class TestState {
  Setup,
  SendingNextAnimation,
  ExecutingAnimation,
  TestDone
};

// ============ Test class declaration ============
class CST_Animations : public CozmoSimTestController {
  private:
    virtual s32 UpdateSimInternal() override;
    
    virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    virtual void HandleAnimationAborted(const ExternalInterface::AnimationAborted& msg) override;
    virtual void HandleAnimationAvailable(const ExternalInterface::AnimationAvailable& msg) override;
    
    TestState _testState = TestState::Setup;
    time_t _startedWaitingForAnimations_s = -1.f;

    std::vector<std::string> _allAnimations;
    int _animationIndex = 0;
    std::vector<std::string> _animationsFailed;
  
    std::string _lastAnimPlayed;
    double _lastAnimPlayedTime = 0;
  
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_Animations);


// =========== Test class implementation ===========

s32 CST_Animations::UpdateSimInternal()
{
  switch (_testState) {
    case TestState::Setup: 
    {
      if(_startedWaitingForAnimations_s < 0){
        // Setup Sound Banks for Tests
        // Handle Text to Speech audio events
        // Note: MessageGameToEngine uses the UnityAudioClient Connection to set audio state
        using namespace AudioMetaData;
        using namespace AudioEngine::Multiplexer;
        using namespace ExternalInterface;
        const auto stateId = static_cast<GameState::GenericState>(GameState::Robot_Vic_System_Tests::Unit_Test);
        MessageGameToEngine audioStateMsg;
        audioStateMsg.Set_PostAudioGameState(PostAudioGameState(GameState::StateGroupType::Robot_Vic_System_Tests,
                                                                stateId));
        SendMessage(audioStateMsg);
        
        time(&_startedWaitingForAnimations_s);
      }else{
        time_t currentTime;
        time(&currentTime);
        if(difftime(currentTime, _startedWaitingForAnimations_s) > kTimeToWaitForAnimations_s){
          // we've waited longe enough to have buildup the animation list - start the test
          SET_TEST_STATE(SendingNextAnimation);
        }
      }
    }

    case TestState::SendingNextAnimation:
    {
      _lastAnimPlayed = _allAnimations[_animationIndex];
      _lastAnimPlayedTime = GetSupervisor()->getTime();
      _animationIndex++;
      
      PRINT_NAMED_INFO("CST_Animations.PlayingAnimation", "%s", _lastAnimPlayed.c_str());
      SendAnimation(_lastAnimPlayed.c_str(), kNumLoops);
      SET_TEST_STATE(ExecutingAnimation);
      break;
    }
      
    case TestState::ExecutingAnimation:
    {
      double timeoutInSeconds = 99;
      // If no action complete message, this will timeout with assert.
      if (CONDITION_WITH_TIMEOUT_ASSERT(_lastAnimPlayed.empty(), _lastAnimPlayedTime, timeoutInSeconds)) {
        if(_animationIndex < _allAnimations.size()){
          SET_TEST_STATE(SendingNextAnimation);
        }else{
          SET_TEST_STATE(TestDone);
        }
      }
      break;
    }

    case TestState::TestDone:
    {
      PRINT_NAMED_INFO("Webots.AnimationPlayback.TestData", "##teamcity[buildStatisticValue key='%s' value='%lu']", "wbtsAnimationCount", _allAnimations.size());
      PRINT_NAMED_INFO("Webots.AnimationPlayback.TestData", "##teamcity[buildStatisticValue key='%s' value='%lu']", "wbtsAnimationFailures", _animationsFailed.size());
      if(!_animationsFailed.empty()){
        // Print all animations that failed and then crash the test
        for(const auto& anim: _animationsFailed){
          PRINT_NAMED_ERROR("CST_Animations.AnimationFailed", "Animation %s printed an error", anim.c_str());
        }
      }
      CST_EXIT();
      break;
    }
  }
  
  return _result;
}

// ================ Message handler callbacks ==================

void CST_Animations::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
{
  switch((RobotActionType)msg.actionType)
  {
    case RobotActionType::PLAY_ANIMATION:
    {
      printf("Robot finished playing animation %s. [Tag=%d]\n",
             msg.completionInfo.Get_animationCompleted().animationName.c_str(), msg.idTag);
      const bool isLastPlayedAnimation = msg.completionInfo.Get_animationCompleted().animationName.compare(_lastAnimPlayed) == 0;
      if (_testState == TestState::ExecutingAnimation && isLastPlayedAnimation){
        if(msg.result != ActionResult::SUCCESS){
          _animationsFailed.push_back(_lastAnimPlayed);
        }
        
        // Clear this to signal to state machine to play the next animation
        _lastAnimPlayed.clear();
      }
      break;
    }
    default:
      break;
  }
}

void CST_Animations::HandleAnimationAborted(const ExternalInterface::AnimationAborted& msg)
{
  PRINT_NAMED_ERROR("CST_Animations.HandleAnimationAborted",
                    "'%s' was aborted.",
                    _lastAnimPlayed.c_str());
  _animationsFailed.push_back(_lastAnimPlayed);
  _lastAnimPlayed.clear();
}

void CST_Animations::HandleAnimationAvailable(const ExternalInterface::AnimationAvailable& msg)
{
  _allAnimations.push_back(msg.animName);
}


// ============== End of message handlers =================

  

} // end namespace Vector
} // end namespace Anki

