/**
 * File: CST_VariableHeightLow.cpp
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
      PickupLow,
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
    PathMotionProfile motionProfile5(defaultPathSpeed_mmps,
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
    class CST_VariableHeightLow : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::Init;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_VariableHeightLow);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_VariableHeightLow::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::Init:
        {
          MakeSynchronous();
          SetActualRobotPose(Pose3d(0, Z_AXIS_3D(), {0, 100, 0}));
          StartMovieConditional("VariableHeightLow");
          //TakeScreenshotsAtInterval("VariableHeightLow", 1.f);
          
          SendMoveHeadToAngle(0, 100, 100);
          _testState = TestState::PickupLow;
          break;
        }
        case TestState::PickupLow:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL) &&
                                           GetNumObjects() == 1, DEFAULT_TIMEOUT)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 1;
            m.numRetries = 3;
            // Pickup object with type LIGHTCUBE1, whatever its ID happens to be
            auto objectsWithType = GetAllObjectIDsByFamilyAndType(ObjectFamily::LightCube, ObjectType::Block_LIGHTCUBE1);
            CST_ASSERT(objectsWithType.size()==1, "Expecting 1 object of type LIGHTCUBE1");
            m.action.Set_pickupObject(ExternalInterface::PickupObject(objectsWithType.front(), motionProfile5, 0, false, true, false));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::TestDone;
          }
          break;
        }
        case TestState::TestDone:
        {
          IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                           GetCarryingObjectID() == 0, 20)
          {
            StopMovie();
            CST_EXIT();
          }
          break;
        }
      }
      return _result;
    }
  } // end namespace Cozmo
} // end namespace Anki

