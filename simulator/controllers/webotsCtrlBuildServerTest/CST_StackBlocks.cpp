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

#include "simulator/game/cozmoSimTestController.h"
#include "coretech/common/engine/math/point_impl.h"
#include "engine/actions/basicActions.h"
#include "engine/robot.h"


namespace Anki {
  namespace Vector {
    
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
      s32  _topCube    = -1;
      s32  _bottomCube = -1;
      
      
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
          StartMovieConditional("StackBlocks");
          //TakeScreenshotsAtInterval("StackBlocks", 1.f);
          
          SendMoveHeadToAngle(0, 100, 100);
          SET_TEST_STATE(PickupObject);
          break;
        }
        case TestState::PickupObject:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                                GetNumObjectsInFamily(ObjectFamily::LightCube) == 1)
          {
            // Pickup first LightCube
            std::vector<s32> objIds = GetAllObjectIDsByFamily(ObjectFamily::LightCube);
            _topCube = objIds[0];
            
            SendPickupObject(_topCube, _defaultTestMotionProfile, true);
            SET_TEST_STATE(Stack);
          }
          break;
        }
        case TestState::Stack:
        {
          const auto objIds = GetAllObjectIDsByFamilyAndType(ObjectFamily::LightCube, ObjectType::Block_LIGHTCUBE2);
          const bool sawBottomCube = !objIds.empty();
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotPose().GetRotation().GetAngleAroundZaxis().getDegrees(), 0, ROBOT_ANGLE_TOL_DEG),
                                                NEAR(GetRobotPose().GetTranslation().x(), 36, ROBOT_POSITION_TOL_MM),
                                                NEAR(GetRobotPose().GetTranslation().y(), 0, ROBOT_POSITION_TOL_MM),
                                                sawBottomCube,
                                                GetCarryingObjectID() == _topCube)
          {
            // Place carrying object on second cube
            _bottomCube = objIds[0];
            SendPlaceOnObject(_bottomCube, _defaultTestMotionProfile, true);
            SET_TEST_STATE(TestDone);
          }
          break;
        }
        case TestState::TestDone:
        {
          // Verify robot has stacked the blocks
          Pose3d pose0;
          GetObjectPose(_topCube, pose0);
          Pose3d pose1;
          GetObjectPose(_bottomCube, pose1);
          
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
    
  } // end namespace Vector
} // end namespace Anki

