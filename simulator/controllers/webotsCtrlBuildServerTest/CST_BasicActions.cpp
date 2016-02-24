/**
 * File: CST_BasicActions.cpp
 *
 * Author: Al Chaussee
 * Created: 2/12/16
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
      MoveLiftUp,
      MoveLiftDown,
      MoveHeadUp,
      MoveHeadDown,
      DriveForwards,
      DriveBackwards,
      TurnLeft,
      TurnRight,
      PanAndTilt,
      FacePose,
      FaceObject,
      TestDone
    };
    
    // ============ Test class declaration ============
    class CST_BasicActions : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateInternal() override;
      
      TestState _testState = TestState::MoveLiftUp;
      
      bool _lastActionSucceeded = false;
      
      // Message handlers
      virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_BasicActions);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_BasicActions::UpdateInternal()
    {
      switch (_testState) {
        case TestState::MoveLiftUp:
        {
          SendMoveLiftToHeight(LIFT_HEIGHT_HIGHDOCK, 100, 100);
          _testState = TestState::MoveLiftDown;
          break;
        }
        case TestState::MoveLiftDown:
        {
          // Verify that lift is in up position
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetLiftHeight_mm(),
                                                LIFT_HEIGHT_HIGHDOCK,
                                                5), 3)
          {
            SendMoveLiftToHeight(LIFT_HEIGHT_LOWDOCK, 100, 100);
            _testState = TestState::MoveHeadUp;
          }
          break;
        }
        case TestState::MoveHeadUp:
        {
          // Verify that lift is in down position
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetLiftHeight_mm(),
                                                LIFT_HEIGHT_LOWDOCK,
                                                5), 3)
          {
            SendMoveHeadToAngle(MAX_HEAD_ANGLE, 100, 100);
            _testState = TestState::MoveHeadDown;
          }
          break;
        }
        case TestState::MoveHeadDown:
        {
          // Verify head is up
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotHeadAngle_rad(), MAX_HEAD_ANGLE, HEAD_ANGLE_TOL), 3)
          {
            SendMoveHeadToAngle(0, 100, 100);
            _testState = TestState::DriveForwards;
          }
          break;
        }
        case TestState::DriveForwards:
        {
          // Verify head is down
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 2;
            m.action.Set_driveStraight(ExternalInterface::DriveStraight(200, 50));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
          
            _testState = TestState::DriveBackwards;
          }
          break;
        }
        case TestState::DriveBackwards:
        {
          // Verify robot is 50 mm forwards
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 50, 10), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 3;
            m.action.Set_driveStraight(ExternalInterface::DriveStraight(-200, -50));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            
            _testState = TestState::TurnLeft;
          }
          break;
        }
        case TestState::TurnLeft:
        {
          // Verify robot is at starting point
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 0, 10) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, 10), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 4;
            m.action.Set_turnInPlace(ExternalInterface::TurnInPlace(PI/2, DEG_TO_RAD(100), 0, false, 1));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            
            _testState = TestState::TurnRight;
          }
          break;
        }
        case TestState::TurnRight:
        {
          // Verify robot turned to 90 degrees
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 90, 10), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 5;
            m.action.Set_turnInPlace(ExternalInterface::TurnInPlace(-PI/2, DEG_TO_RAD(100), 0, false, 1));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            
            _testState = TestState::PanAndTilt;
          }
          break;
        }
        case TestState::PanAndTilt:
        {
          // Verify robot turned back to 0 degrees
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, 10), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 6;
            m.action.Set_panAndTilt(ExternalInterface::PanAndTilt(PI, PI, true, true));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            
            _testState = TestState::FacePose;
          }
          break;
        }
        case TestState::FacePose:
        {
          // Verify robot turned 180 degrees and head is at right angle
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           (NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 180, 10) ||
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -180, 10)) &&
                                           NEAR(GetRobotHeadAngle_rad(), MAX_HEAD_ANGLE, HEAD_ANGLE_TOL), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 7;
            // Face the position (0,-100,0) wrt robot
            m.action.Set_facePose(ExternalInterface::FacePose(GetRobotPose().GetTranslation().x(),
                                                              GetRobotPose().GetTranslation().y() + -100,
                                                              NECK_JOINT_POSITION[2], PI, 0, 0, 0, 0, 0, 0, 1));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::FaceObject;
          }
          break;
        }
        case TestState::FaceObject:
        {
          // Verify robot is facing pose
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -90, 20) &&
                                           NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL), 5)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 8;
            // Face block 0
            m.action.Set_faceObject(ExternalInterface::FaceObject(0, PI, 0, 0, 0, 0, 0, 0, 1, true, false));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::TestDone;
          }
          break;
        }
        case TestState::TestDone:
        {
          // Verify robot is facing the object
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 0, 20), 5)
          {
            CST_EXIT();
          }
          break;
        }
      }
      return _result;
    }
    
    
    // ================ Message handler callbacks ==================
    void CST_BasicActions::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
    {
      if (msg.result == ActionResult::SUCCESS) {
        _lastActionSucceeded = true;
      }
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Cozmo
} // end namespace Anki

