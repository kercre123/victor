/**
 * File: CST_DockActionInterrupts.cpp
 *
 * Author: Al Chaussee
 * Created: 3/9/17
 *
 * Description: This test tests that Engine does not crash when unobserving and re-observing a dock object
 *              while a DockAction is running. This crash was caused from caching object markers and then
 *              having the object the markers belonged to get deleted due to unobserving it. The cached marker
 *              became invalid and would cause Engine to assert/crash
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/robot.h"

#define SET_STATE(s) {                                          \
PRINT_NAMED_INFO("CST_DockActionInterrupts.TransitionTestState",      \
"%s", #s);                                   \
_testState = TestState::s;                                    \
}

namespace Anki {
namespace Cozmo {
    
enum class TestState
{
  Init,
  StartPickup,
  MoveAndObscureObject,
  SeeObject,
  TestDone
};

// ============ Test class declaration ============
class CST_DockActionInterrupts : public CozmoSimTestController
{
private:
  
  virtual s32 UpdateSimInternal() override;
  
  TestState _testState = TestState::Init;
  
  ObjectID _id;
  int _solidBoxId = -1;
  
  // Message handlers
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg) override;
  std::pair<u32, ActionResult> _lastActionResult = {0, ActionResult::ABORT};
  
  PathMotionProfile motionProfile = DEFAULT_PATH_MOTION_PROFILE;
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_DockActionInterrupts);

// =========== Test class implementation ===========
s32 CST_DockActionInterrupts::UpdateSimInternal()
{
  switch (_testState)
  {
    case TestState::Init:
    {
      MakeSynchronous();
      
      // We need to have waits before and after the VisuallyVerifyAction in TurnTowardsObject so we have
      // enough time to move the dock object and have it be unobserved
      SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::SetDebugConsoleVarMessage("InsertWaitsInTurnTowardsObjectVerify", "true")));
      SendMoveHeadToAngle(0, 100, 100);
      SET_STATE(StartPickup);
      break;
    }
    case TestState::StartPickup:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                       NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL) &&
                                       GetNumObjects() == 2, DEFAULT_TIMEOUT)
      {
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 10;
        m.numRetries = 3;
        // Pickup object with type LIGHTCUBE1, whatever its ID happens to be
        auto objectsWithType = GetAllObjectIDsByFamilyAndType(ObjectFamily::LightCube, ObjectType::Block_LIGHTCUBE1);
        CST_ASSERT(objectsWithType.size()==1, "Expecting 1 object of type LIGHTCUBE1");
        _id = objectsWithType.front();
        m.action.Set_pickupObject(ExternalInterface::PickupObject(_id, motionProfile, 0, false, false, false, false));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);

        SET_STATE(MoveAndObscureObject);
      }
      break;
    }
    case TestState::MoveAndObscureObject:
    {
      // Wait 1 second before moving LightCube1 and putting a box in front of it.
      // Engine should be in the middle of the first WaitAction right before it attempts to VisuallyVerify
      // LightCube1
      if(HasXSecondsPassedYet(1))
      {
        Pose3d p = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
        const Vec3f trans = p.GetTranslation();
        p.SetTranslation({trans.x() + 50, trans.y(), trans.z() + 10});
        SetLightCubePose(ObjectType::Block_LIGHTCUBE1, p);
        
        // Find the SolidBox in the world and put it in front of LightCube1
        webots::Field* rootChildren = GetSupervisor()->getRoot()->getField("children");
        int numRootChildren = rootChildren->getCount();
        for (int n = 0 ; n<numRootChildren; ++n) {
          webots::Node* nd = rootChildren->getMFNode(n);
          std::string nodeName = nd->getTypeName();
          
          if(nodeName.find("SolidBox") != std::string::npos)
          {
            _solidBoxId = n;
          
            webots::Field* transField = nd->getField("translation");
            
            const double translation[3] = {
              MM_TO_M(trans.x()),
              MM_TO_M(trans.y()),
              MM_TO_M(trans.z())
            };
            transField->setSFVec3f(translation);
            
            break;
          }
        }
        
        SET_STATE(SeeObject);
      }
      break;
    }
    case TestState::SeeObject:
    {
      // Wait 1 second before moving LightCube1 back to its original location and removing the box in front of it.
      // Engine should be in the middle of a VisuallyVerifyAction
      if(HasXSecondsPassedYet(1))
      {
        webots::Field* rootChildren = GetSupervisor()->getRoot()->getField("children");
        webots::Node* nd = rootChildren->getMFNode(_solidBoxId);
        std::string nodeName = nd->getTypeName();
        
        if(nodeName.find("SolidBox") != std::string::npos)
        {
          webots::Field* transField = nd->getField("translation");
          const double translation[3] = {10, 10, 10};
          transField->setSFVec3f(translation);
        }
        
        Pose3d p = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
        const Vec3f trans = p.GetTranslation();
        p.SetTranslation({trans.x() - 50, trans.y(), trans.z()});
        p.SetRotation(Radians(DEG_TO_RAD(90)), Y_AXIS_3D());
        SetLightCubePose(ObjectType::Block_LIGHTCUBE1, p);
        
        SET_STATE(TestDone);
      }
      break;
    }
    case TestState::TestDone:
    {
      // Unobserving and re-observing a cube during a DockAction used to cause engine to crash
      // so if the PickupAction has not completed in 10 seconds then assume Engine has crashed
      if(HasXSecondsPassedYet(10))
      {
        _result = RESULT_FAIL;
        CST_EXIT();
      }
    
      if(_lastActionResult.first == 10)
      {
        _result = (_lastActionResult.second == ActionResult::SUCCESS ? RESULT_OK : RESULT_FAIL);
        CST_EXIT();
      }
      break;
    }
  }
  return _result;
}

// ================ Message handler callbacks ==================
void CST_DockActionInterrupts::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction& msg)
{
  _lastActionResult = {msg.idTag, msg.result};
}
  
} // end namespace Cozmo
} // end namespace Anki

