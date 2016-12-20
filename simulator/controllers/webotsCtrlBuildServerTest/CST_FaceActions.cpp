/**
 * File: CST_FaceActions.cpp
 *
 * Author: Al Chaussee
 * Created: 2/29/16
 *
 * Description: See TestStates below
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
  namespace Cozmo {
    
    enum class TestState {
      TurnToFace,
      TurnAwayFromFace,
      TurnBackToFace,
      TestDone
    };
    
    // ============ Test class declaration ============
    class CST_FaceActions : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::TurnToFace;
      
      bool _lastActionSucceeded = false;
      
      TimeStamp_t _prevFaceSeenTime = 0;
      TimeStamp_t _faceSeenTime = 0;
      
      // Message handlers
      virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
      virtual void HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg) override;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_FaceActions);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_FaceActions::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::TurnToFace:
        {
          MakeSynchronous();
          SendMoveHeadToAngle(MAX_HEAD_ANGLE, 100, 100);
          
          ExternalInterface::QueueSingleAction m;
          m.robotID = 1;
          m.position = QueueActionPosition::AT_END;
          m.idTag = 2;
          m.action.Set_turnInPlace(ExternalInterface::TurnInPlace(-M_PI_F/2, DEG_TO_RAD(100), 0, false, 1));
          ExternalInterface::MessageGameToEngine message;
          message.Set_QueueSingleAction(m);
          SendMessage(message);
          _testState = TestState::TurnAwayFromFace;
          break;
        }
        case TestState::TurnAwayFromFace:
        {
          // Verify robot has turned and has seen the face
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotHeadAngle_rad(), MAX_HEAD_ANGLE, HEAD_ANGLE_TOL),
                                                NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -90, 10),
                                                _faceSeenTime != 0)
          {
            SendMoveHeadToAngle(0, 20, 20);
            
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::AT_END;
            m.idTag = 3;
            m.action.Set_turnInPlace(ExternalInterface::TurnInPlace(-M_PI_F/2, DEG_TO_RAD(100), 0, false, 1));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::TurnBackToFace;
          }
          break;
        }
        case TestState::TurnBackToFace:
        {
          // Verify robot has turned away from face
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                                (NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -180, 10) ||
                                                 NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 180, 10)))
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 10;
            // Turn towards the last face pose
            m.action.Set_turnTowardsLastFacePose(ExternalInterface::TurnTowardsLastFacePose(M_PI_F, 0, 0, 0, 0, 0, 0, false, AnimationTrigger::Count, AnimationTrigger::Count, 1));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::TestDone;
          }
          break;
        }
        case TestState::TestDone:
        {
          // Verify robot has turned back towards the face
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotHeadAngle_rad(), DEG_TO_RAD(42.5f), DEG_TO_RAD(5.f)),
                                                NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -90, 10),
                                                _prevFaceSeenTime < _faceSeenTime,
                                                _prevFaceSeenTime != 0)
          {
            CST_EXIT();
          }
          break;
        }
      }
      return _result;
    }
    
    
    // ================ Message handler callbacks ==================
    void CST_FaceActions::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
    {
      if (msg.result == ActionResult::SUCCESS) {
        _lastActionSucceeded = true;
      }
    }
    
    void CST_FaceActions::HandleRobotObservedFace(ExternalInterface::RobotObservedFace const& msg)
    {
      _prevFaceSeenTime = _faceSeenTime;
      _faceSeenTime = msg.timestamp;
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Cozmo
} // end namespace Anki

