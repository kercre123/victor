/**
 * File: CST_AnimationFirmwareMessaging.cpp
 *
 * Author: Kevin M. Karol
 * Created: 9/22/16
 *
 * Description: Plays an animation that moves the robot to ensure our messaging is stable
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "simulator/game/cozmoSimTestController.h"
#include "coretech/common/engine/math/point_impl.h"
#include "engine/actions/basicActions.h"
#include "engine/robot.h"


namespace Anki {
  namespace Vector {
    
    enum class TestState {
      PlayAnimation,
      CheckWorldState
    };
    
    // ============ Test class declaration ============
    class CST_AnimationFirmwareMessaging : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::PlayAnimation;
      
      const char* kAnimationName = "anim_qa_firmwaremessaging_01";
      
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_AnimationFirmwareMessaging);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_AnimationFirmwareMessaging::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::PlayAnimation:
        {
          StartMovieConditional("AnimationFirmwareMessaging");

          ExternalInterface::PlayAnimation playAnimation;
          playAnimation.animationName = kAnimationName;

          SendMessage(ExternalInterface::MessageGameToEngine(
                          ExternalInterface::PlayAnimation(playAnimation)));
          
          SET_TEST_STATE(CheckWorldState);
          break;
        }
        case TestState::CheckWorldState:
        {
          float angle = GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees();
          float x = GetRobotPose().GetTranslation().x();
          float y = GetRobotPose().GetTranslation().y();
          float z = GetRobotPose().GetTranslation().z();
          float headAngle = GetRobotHeadAngle_rad();
          float liftAngle = GetLiftHeight_mm();
          
          const float animationAngle = 80.5;
          const float animationX = -5.5;
          const float animationY = 69;
          const float animationZ = 0;
          const float animationHead = 0.42f;
          const float animationLift = 91.7f;
          
          const bool nearAngle = NEAR(angle, animationAngle, 1);
          const bool nearX = NEAR(x, animationX, 1);
          const bool nearY = NEAR(y, animationY, 1);
          const bool nearZ = NEAR(z, animationZ, 1);
          const bool nearHead = NEAR(headAngle, animationHead, 1);
          const bool nearLift = NEAR(liftAngle, animationLift, 1);
          
          // Verify that lift is in up position
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                nearAngle,
                                                nearX,
                                                nearY,
                                                nearZ,
                                                nearHead,
                                                nearLift);
          {
            CST_EXIT();
          }
          break;
        }
      }
      return _result;
    }
    
  } // end namespace Vector
} // end namespace Anki

