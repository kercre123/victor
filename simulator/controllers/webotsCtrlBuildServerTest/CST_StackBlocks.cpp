/**
 * File: CST_StackBlocks.cpp
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
      Stack,
      TestDone
    };
    
    namespace {
    
    const f32 ROBOT_POSITION_TOL_MM = 15;
    const f32 ROBOT_ANGLE_TOL_DEG = 5;
    const f32 BLOCK_HEIGHT_TOL_MM = 10;
    
    };
    
    // ============ Test class declaration ============
    class CST_StackBlocks : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::Init;
      
      bool _lastActionSucceeded = false;
      
      // Message handlers
      virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_StackBlocks);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_StackBlocks::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::Init:
        {
          MakeSynchronous();
          StartMovieConditional("StackBlocks");
          //TakeScreenshotsAtInterval("StackBlocks", 1.f);
          
          SendMoveHeadToAngle(0, 100, 100);
          _testState = TestState::PickupObject;
          break;
        }
        case TestState::PickupObject:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                                GetNumObjects() == 1)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 1;
            // Pickup object 0
            m.action.Set_pickupObject(ExternalInterface::PickupObject(0, _defaultTestMotionProfile, 0, false, true, false, true));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::Stack;
          }
          break;
        }
        case TestState::Stack:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, ROBOT_ANGLE_TOL_DEG),
                                                NEAR(GetRobotPose().GetTranslation().x(), 36, ROBOT_POSITION_TOL_MM),
                                                NEAR(GetRobotPose().GetTranslation().y(), 0, ROBOT_POSITION_TOL_MM),
                                                GetCarryingObjectID() == 0)
          {
            ExternalInterface::QueueCompoundAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 2;
            m.parallel = false;
            m.numRetries = 3;
            // Wait a few seconds to see the block behind the one we just picked up
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::Wait(1));
            // Place object 0 on object 1
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PlaceOnObject(1, _defaultTestMotionProfile, 0, false, true, false, true));
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
          
          PRINT_NAMED_INFO("BAASDF", "BlockZ: %f %f, Robot (xy) %f %f",
                           pose0.GetTranslation().z(),
                           pose1.GetTranslation().z(),
                           GetRobotPose().GetTranslation().x(),
                           GetRobotPose().GetTranslation().y());
          
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                GetCarryingObjectID() == -1,
                                                NEAR(pose0.GetTranslation().z(), 65, BLOCK_HEIGHT_TOL_MM),
                                                NEAR(pose1.GetTranslation().z(), 22, BLOCK_HEIGHT_TOL_MM),
                                                NEAR(GetRobotPose().GetTranslation().x(), 105, ROBOT_POSITION_TOL_MM),
                                                NEAR(GetRobotPose().GetTranslation().y(), 0, ROBOT_POSITION_TOL_MM))
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
    void CST_StackBlocks::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
    {
      if (msg.result == ActionResult::SUCCESS) {
        _lastActionSucceeded = true;
      }
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Cozmo
} // end namespace Anki

