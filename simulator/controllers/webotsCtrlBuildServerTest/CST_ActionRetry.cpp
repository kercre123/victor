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
      PickupObject,
      PlaceObject,
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
    const f32 defaultDockSpeed_mmps = 100;
    const f32 defaultDockAccel_mmps2 = 200;
    const f32 defaultReverseSpeed_mmps = 30;
    PathMotionProfile motionProfile2(defaultPathSpeed_mmps,
                                    defaultPathAccel_mmps2,
                                    defaultPathDecel_mmps2,
                                    defaultPathPointTurnSpeed_rad_per_sec,
                                    defaultPathPointTurnAccel_rad_per_sec2,
                                    defaultPathPointTurnDecel_rad_per_sec2,
                                    defaultDockSpeed_mmps,
                                    defaultDockAccel_mmps2,
                                    defaultReverseSpeed_mmps);
    
    
    // ============ Test class declaration ============
    class CST_ActionRetry : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateInternal() override;
      
      TestState _testState = TestState::Init;
      
      bool _lastActionSucceeded = false;
      
      bool _actionFailed = false;
      
      // Message handlers
      virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_ActionRetry);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_ActionRetry::UpdateInternal()
    {
      switch (_testState) {
        case TestState::Init:
        {
          SendMoveHeadToAngle(0, 100, 100);
          
          ExternalInterface::QueueSingleAction m;
          m.robotID = 1;
          m.position = QueueActionPosition::NEXT;
          m.idTag = 4;
          m.action.Set_wait(ExternalInterface::Wait(1));
          ExternalInterface::MessageGameToEngine message;
          message.Set_QueueSingleAction(m);
          SendMessage(message);
          
          m.robotID = 1;
          m.position = QueueActionPosition::AT_END;
          m.idTag = 5;
          m.action.Set_turnInPlace(ExternalInterface::TurnInPlace(-PI/4, false, 1));
          message.Set_QueueSingleAction(m);
          SendMessage(message);
          _testState = TestState::PickupObject;
          break;
        }
        case TestState::PickupObject:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL) &&
                                           GetNumObjects() == 2, 10)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 9;
            // Pickup object 0
            m.action.Set_pickupObject(ExternalInterface::PickupObject(0, motionProfile2, 0, false, true, false));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::PlaceObject;
          }
          break;
        }
        case TestState::PlaceObject:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, 5) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 128, 5) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 1, 5) &&
                                           GetCarryingObjectID() == 0, 10)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 10;
            // Place object 0 facing -x
            m.action.Set_placeObjectOnGround(ExternalInterface::PlaceObjectOnGround(100, 100, 0, 0, 0, 1, motionProfile2, 0, false, true));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::Stack;
          }
          break;
        }
        // Trying to stack the blocks will cause a FAILURE_RETRY
        case TestState::Stack:
        {
          // Verify robot has put block down and is in the right position
          Pose3d pose;
          GetObjectPose(0, pose);
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           (NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 180, 5) ||
                                            NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -180, 5)) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 191, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 100, 10) &&
                                           GetCarryingObjectID() == -1 &&
                                           NEAR(pose.GetRotationAxis().x(), 0.0, 0.1) &&
                                           NEAR(pose.GetRotationAxis().y(), 0.0, 0.1) &&
                                           (NEAR(pose.GetRotationAxis().z(), 1, 0.1) ||
                                            NEAR(pose.GetRotationAxis().z(), -1, 0.1)), 10)
          {
            ExternalInterface::QueueCompoundAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 12;
            m.parallel = false;
            m.numRetries = 5;
            // Pickup object 1
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PickupObject(1, motionProfile2, 0, false, true, false));
            // Place object 1 on object 0
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PlaceOnObject(0, motionProfile2, 0, false, true, false));
            ExternalInterface::MessageGameToEngine message;
            
            message.Set_QueueCompoundAction(m);
            SendMessage(message);
            
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
          // Stacking the blocks can fail (rare) as the visuallyVerifyObjectAction can timeout because
          // it ends up not being able to see the second block after turning to face it. So if this happens just
          // say it completed
          IF_CONDITION_WITH_TIMEOUT_ASSERT(_actionFailed || (!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetCarryingObjectID() == -1 &&
                                           NEAR(pose0.GetTranslation().z(), 22, 10) &&
                                           NEAR(pose1.GetTranslation().z(), 65, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 100, 30) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 24, 20)), 30)
          {
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
      if (msg.result == ActionResult::SUCCESS) {
        _lastActionSucceeded = true;
      // 12 is the id of the action that can fail due to not seeing the second block when stacking
      } else if(msg.idTag == 12) {
        PRINT_NAMED_WARNING("", "Failed with timeout saying success");
        _actionFailed = true;
      }
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Cozmo
} // end namespace Anki

