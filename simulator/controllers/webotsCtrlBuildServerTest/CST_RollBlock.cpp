/**
 * File: CST_RollBlock.cpp
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
      RollObject,
      TestDone
    };
    
    // ============ Test class declaration ============
    class CST_RollBlock : public CozmoSimTestController {
      
    private:
      
      virtual s32 UpdateSimInternal() override;
      
      TestState _testState = TestState::Init;
    };
    
    // Register class with factory
    REGISTER_COZMO_SIM_TEST_CLASS(CST_RollBlock);
    
    
    // =========== Test class implementation ===========
    
    s32 CST_RollBlock::UpdateSimInternal()
    {
      switch (_testState) {
        case TestState::Init:
        {
          MakeSynchronous();
          StartMovieConditional("RollBlock");
          // TakeScreenshotsAtInterval("RollBlock", 1.f);
          
          SendMoveHeadToAngle(0, 100, 100);
          _testState = TestState::RollObject;
          break;
        }
        case TestState::RollObject:
        {
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                                GetNumObjects() == 1)
          {
            ExternalInterface::QueueSingleAction m;
            m.robotID = 1;
            m.position = QueueActionPosition::NOW;
            m.idTag = 11;
            m.numRetries = 3;
            // Roll object 0
            m.action.Set_rollObject(ExternalInterface::RollObject(0, _defaultTestMotionProfile, 0, false, false, true, false, true, false));
            ExternalInterface::MessageGameToEngine message;
            message.Set_QueueSingleAction(m);
            SendMessage(message);
            _testState = TestState::TestDone;
          }
          break;
        }
        case TestState::TestDone:
        {
          // Verify robot has rolled the block
          Pose3d pose;
          GetObjectPose(0, pose);
          IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(25,
                                                !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                                GetCarryingObjectID() == -1,
                                                pose.GetRotationAngle().IsNear(-1.5f, 0.2f))
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

