/**
 * File: CST_ActionRetry.cpp
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
      Init,
      WaitToSeeObjectOne,
      PickupObject,
      PlaceObject,
      PickupForStack,
      Stack,
      TestDone
    };
    
    // Motion profile for test
    const f32 defaultPathSpeed_mmps = 60;
    const f32 defaultPathAccel_mmps2 = 200;
    const f32 defaultPathDecel_mmps2 = 500;
    const f32 defaultPathPointTurnSpeed_rad_per_sec = 1.5;
    const f32 defaultPathPointTurnAccel_rad_per_sec2 = 100;
    const f32 defaultPathPointTurnDecel_rad_per_sec2 = 500;
    const f32 defaultDockSpeed_mmps = 60;
    const f32 defaultDockAccel_mmps2 = 200;
    const f32 defaultDockDecel_mmps2 = 100;
    const f32 defaultReverseSpeed_mmps = 30;
    PathMotionProfile motionProfile2(defaultPathSpeed_mmps,
                                    defaultPathAccel_mmps2,
                                    defaultPathDecel_mmps2,
                                    defaultPathPointTurnSpeed_rad_per_sec,
                                    defaultPathPointTurnAccel_rad_per_sec2,
                                    defaultPathPointTurnDecel_rad_per_sec2,
                                    defaultDockSpeed_mmps,
                                    defaultDockAccel_mmps2,
                                    defaultDockDecel_mmps2,
                                    defaultReverseSpeed_mmps,
                                    true);
    
    
    // ============ Test class declaration ============
    class CST_ActionRetry : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::Init;
      
      // Message handlers
      virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_ActionRetry);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_ActionRetry::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::Init:
        {
          MakeSynchronous();
          StartMovieConditional("ActionRetry");
          //TakeScreenshotsAtInterval("ActionRetry", 1.f);

          SendMoveHeadToAngle(0, 100, 100);
          
          ExternalInterface::QueueSingleAction m;
          m.robotID = 1;
          m.position = QueueActionPosition::NEXT;
          m.idTag = 4;
          m.action.Set_wait(ExternalInterface::Wait(1));
          ExternalInterface::MessageGameToEngine message;
          message.Set_QueueSingleAction(m);
          SendMessage(message);
          
          PRINT_NAMED_INFO("CST_ActionRetry", "Waiting to see object 1");
          _testState = TestState::WaitToSeeObjectOne;
          break;
        }
        case TestState::WaitToSeeObjectOne:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetNumObjects() == 1, DEFAULT_TIMEOUT)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::AT_END;
            m.idTag = 5;
            m.action.Set_turnInPlace(ExternalInterface::TurnInPlace(-M_PI_F/4, DEG_TO_RAD(100), 0, false, 1));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
          
            PRINT_NAMED_INFO("CST_ActionRetry", "Turning back to first block");
            _testState = TestState::PickupObject;
          }
          break;
        }
        case TestState::PickupObject:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL) &&
                                           GetNumObjects() == 2, DEFAULT_TIMEOUT)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 9;
            m.numRetries = 2;
            // Pickup object 0
            m.action.Set_pickupObject(ExternalInterface::PickupObject(0, motionProfile2, 0, false, true, false, true));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            
            PRINT_NAMED_INFO("CST_ActionRetry", "Picking up object 0");
            _testState = TestState::PlaceObject;
          }
          break;
        }
        case TestState::PlaceObject:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 128, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 1, 10) &&
                                           GetCarryingObjectID() == 0, DEFAULT_TIMEOUT)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 10;
            // Place object 0 facing -x
            const bool checkDestinationFree = false;
            m.action.Set_placeObjectOnGround(ExternalInterface::PlaceObjectOnGround(100, 100, 0, 0, 0, 1, motionProfile2, 0, false, true, checkDestinationFree));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            
            PRINT_NAMED_INFO("CST_ActionRetry", "Placing object 0 on ground");
            _testState = TestState::PickupForStack;
          }
          break;
        }
        // Trying to stack the blocks will cause a FAILURE_RETRY
        case TestState::PickupForStack:
        {
          // Verify robot has put block down and is in the right position
          Pose3d pose;
          GetObjectPose(0, pose);
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           (NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 180, 10) ||
                                            NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -180, 10)) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 191, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 100, 10) &&
                                           GetCarryingObjectID() == -1 &&
                                           NEAR(pose.GetRotationAxis().x(), 0.0f, 0.1f) &&
                                           NEAR(pose.GetRotationAxis().y(), 0.0f, 0.1f) &&
                                           (NEAR(pose.GetRotationAxis().z(), 1.f, 0.1f) ||
                                            NEAR(pose.GetRotationAxis().z(), -1.f, 0.1f)), DEFAULT_TIMEOUT)
          {
            ExternalInterface::QueueCompoundAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 12;
            m.parallel = false;
            m.numRetries = 5;
            // Pickup object 1
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PickupObject(1, motionProfile2, 0, false, true, false, true));
            ExternalInterface::MessageGameToEngine message;
            
            message.Set_QueueCompoundAction(m);
            SendMessage(message);
            
            PRINT_NAMED_INFO("CST_ActionRetry", "Picking up object 1");
            _testState = TestState::Stack;
          }
          break;
        }
        case TestState::Stack:
        {
          // Verify robot has picked up block to stack
          Pose3d pose;
          GetObjectPose(0, pose);
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                            GetCarryingObjectID() == 1, 30)
          {
            ExternalInterface::QueueCompoundAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 13;
            m.parallel = false;
            m.numRetries = 5;
            // Place object 1 on object 0
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PlaceOnObject(0, motionProfile2, 0, false, true, false, true));
            ExternalInterface::MessageGameToEngine message;
            
            message.Set_QueueCompoundAction(m);
            SendMessage(message);
            
            PRINT_NAMED_INFO("CST_ActionRetry", "Placing object 1 on object 0");
            _testState = TestState::TestDone;
          }
          break;
        }
        case TestState::TestDone:
        {
          // Verify robot has stacked the blocks
          Pose3d pose0;
          GetObjectPose(0, pose0);
          Pose3d pose1;
          GetObjectPose(1, pose1);
          // Note: I'm using robot's actual pose because depending on what the robot does the error in pose estimation can change dramatically
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetCarryingObjectID() == -1 &&
                                           NEAR(pose0.GetTranslation().z(), 22, 10) &&
                                           NEAR(pose1.GetTranslation().z(), 65, 10) &&
                                           NEAR(GetRobotPoseActual().GetTranslation().x(), 78, 20) &&
                                           NEAR(GetRobotPoseActual().GetTranslation().y(), 45, 20), 60)
          {
            StopMovie();
            CST_EXIT();
          }
          break;
        }
      }
      return _result;
    }
    
    
    // ================ Message handler callbacks ==================
    void CST_ActionRetry::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
    {
      if(msg.idTag == 13 && msg.result != ActionResult::SUCCESS) {
        PRINT_NAMED_INFO("CST_ActionRetry", "Stacking blocks failed queue now and clear remaining a new PlaceOnObject action");
        
        ExternalInterface::QueueCompoundAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW_AND_CLEAR_REMAINING;
        m.idTag = 13;
        m.parallel = false;
        m.numRetries = 5;
        // Place object 1 on object 0
        m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PlaceOnObject(0, motionProfile2, 0, false, true, false, true));
        ExternalInterface::MessageGameToEngine message;
        
        message.Set_QueueCompoundAction(m);
        SendMessage(message);
        
        _testState = TestState::TestDone;
      }
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Cozmo
} // end namespace Anki

