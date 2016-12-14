/**
 * File: CST_DockActions.cpp
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
      RollObject,
      Stack,
      SeenNextBlock,
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
    PathMotionProfile motionProfile(defaultPathSpeed_mmps,
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
    class CST_DockActions : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::Init;
      
      bool _lastActionSucceeded = false;
      bool _lastObjectIdSet = false;
      int _lastObjectId = -1;
      int _block1 = -1;
      int _block2 = -1;
      bool _waitActionCompleted = false;
      
      // Message handlers
      virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
      virtual void HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg) override;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_DockActions);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_DockActions::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::Init:
        {
          MakeSynchronous();
          StartMovieConditional("DockActions");
          //TakeScreenshotsAtInterval("DockActions", 1.f);

          SendMoveHeadToAngle(0, 100, 100);
          _testState = TestState::PickupObject;
          break;
        }
        case TestState::PickupObject:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL) &&
                                           GetNumObjects() == 2, 20)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 9;
            // Pickup object 0
            m.action.Set_pickupObject(ExternalInterface::PickupObject(0, motionProfile, 0, false, true, false, true));
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
                                           NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 128, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 1, 10) &&
                                           GetCarryingObjectID() == 0, 20)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 10;
            // Place object 0 facing -x
            const bool checkDestinationFree = false;
            m.action.Set_placeObjectOnGround(ExternalInterface::PlaceObjectOnGround(100, 100, 0, 0, 0, 1, motionProfile, 0, false, true, checkDestinationFree));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::RollObject;
          }
          break;
        }
        case TestState::RollObject:
        {
          Pose3d pose;
          GetObjectPose(0, pose);
          // Verify robot has put block down and is in the right position
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           (NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 180, 10) ||
                                            NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), -180, 10)) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 191, 20) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), 100, 20) &&
                                           GetCarryingObjectID() == -1 &&
                                           NEAR(pose.GetRotationAxis().x(), 0.0f, 0.1f) &&
                                           NEAR(pose.GetRotationAxis().y(), 0.0f, 0.1f) &&
                                           (NEAR(pose.GetRotationAxis().z(), 1.f, 0.1f) ||
                                            NEAR(pose.GetRotationAxis().z(), -1.f, 0.1f)), 20)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 11;
            m.numRetries = 3;
            // Roll object 0
            m.action.Set_rollObject(ExternalInterface::RollObject(0, motionProfile, 0, false, false, true, false, true));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::Stack;
          }
          break;
        }
        case TestState::Stack:
        {
          // Verify robot has rolled the block
          Pose3d pose;
          GetObjectPose(0, pose);
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetCarryingObjectID() == -1 &&
                                           ((NEAR(pose.GetRotationAxis().x(), 0.707f, 0.1f) &&
                                             NEAR(pose.GetRotationAxis().z(), 0.707f, 0.1f)) ||
                                            (NEAR(pose.GetRotationAxis().x(), -0.707f, 0.1f) &&
                                             NEAR(pose.GetRotationAxis().z(), -0.707f, 0.1f))) &&
                                           NEAR(pose.GetRotationAxis().y(), 0.0f, 0.1f), 20)
          {
            std::vector<s32> objects = GetAllObjectIDs();
            std::sort(objects.begin(), objects.end());
          
            ExternalInterface::QueueCompoundAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 12;
            m.parallel = false;
            m.numRetries = 3;
            // Pickup object 1
            PRINT_NAMED_INFO("CST_DockActions.Stack", "Picking up block %i", objects.back());
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PickupObject(objects.back(), motionProfile, 0, false, true, false, true));
            _block1 = _lastObjectId;
            // Wait a few seconds to see the block behind the one we just picked up
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::Wait(2));
            ExternalInterface::MessageGameToEngine message;
            
            message.Set_QueueCompoundAction(m);
            SendMessage(message);

            _testState = TestState::SeenNextBlock;
          }
          break;
        }
        case TestState::SeenNextBlock:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetCarryingObjectID() == _block1 &&
                                           NEAR(GetLiftHeight_mm(), LIFT_HEIGHT_CARRY, 1) &&
                                           _waitActionCompleted,
                                           30)
          {
            // Sort all object IDs because we are placing this block on the last block seen which will have the
            // highest object ID
            std::vector<s32> objects = GetAllObjectIDs();
            std::sort(objects.begin(), objects.end());
          
            ExternalInterface::QueueCompoundAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NEXT;
            m.idTag = 13;
            m.parallel = false;
            m.numRetries = 3;
            // Place object 1 on object 2
            PRINT_NAMED_INFO("CST_DockActions.SeenNextBlock", "Placing carried block %i on block %i", GetCarryingObjectID(), objects.back());
            m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PlaceOnObject(objects.back(), motionProfile, 0, false, true, false, true));
            _block2 = objects.back();
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
          GetObjectPose(_block1, pose0);
          Pose3d pose1;
          GetObjectPose(_block2, pose1);
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetCarryingObjectID() == -1 &&
                                           NEAR(pose0.GetTranslation().z(), 65, 10) &&
                                           NEAR(pose1.GetTranslation().z(), 22, 10) &&
                                           NEAR(GetRobotPose().GetTranslation().x(), 180, 30) &&
                                           NEAR(GetRobotPose().GetTranslation().y(), -88, 20), 30)
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
    void CST_DockActions::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
    {
      if (msg.result == ActionResult::SUCCESS) {
        _lastActionSucceeded = true;
        
        if(msg.idTag == 12)
        {
          _waitActionCompleted = true;
        }
      }
      else if(msg.result == ActionResult::FAILURE_RETRY && msg.idTag == 12)
      {
        PRINT_NAMED_INFO("CST_DockActions", "Pickup block failed requeueing action");
        
        std::vector<s32> objects = GetAllObjectIDs();
        std::sort(objects.begin(), objects.end());
        
        ExternalInterface::QueueCompoundAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW_AND_CLEAR_REMAINING;
        m.idTag = 12;
        m.parallel = false;
        m.numRetries = 3;
        // Pickup object 1
        PRINT_NAMED_INFO("CST_DockActions.Stack", "Picking up block %i", objects.back());
        m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::PickupObject(objects.back(), motionProfile, 0, false, true, false, true));
        _block1 = _lastObjectId;
        // Wait a few seconds to see the block behind the one we just picked up
        m.actions.push_back((ExternalInterface::RobotActionUnion)ExternalInterface::Wait(2));
        ExternalInterface::MessageGameToEngine message;
        
        message.Set_QueueCompoundAction(m);
        SendMessage(message);
        
        _testState = TestState::SeenNextBlock;
      }
    }
    
    void CST_DockActions::HandleRobotObservedObject(ExternalInterface::RobotObservedObject const& msg)
    {
      if(msg.objectID > 0 && !_lastObjectIdSet)
      {
        _lastObjectId = msg.objectID;
        _lastObjectIdSet = true;
      }
    }
    
    // ================ End of message handler callbacks ==================
    
  } // end namespace Cozmo
} // end namespace Anki

