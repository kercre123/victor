/**
* File: CST_RobotKidnapping.cpp
*
* Author: Andrew Stein
* Created: 10/20/15
*
* Description: Tests robot's ability to re-localize itself and rejigger world 
*              origins when being delocalized and then re-seeing existing light cubes.
*
* Copyright: Anki, inc. 2015
*
*/

#include "anki/cozmo/simulator/game/cozmoSimTestController.h"


namespace Anki {
namespace Cozmo {
  
  enum class TestState {
    InitialLocalization, // Localize to Object A
    Kidnap,              // Delocalize robot and move to new position
    LocalizeToObjectB,   // See and localize to new Object B
    ReSeeObjectA,        // Drive to re-see Object A, to force re-jiggering of origins
    TestDone
  };
  
  // ============ Test class declaration ============
  class CST_RobotKidnapping : public CozmoSimTestController
  {
  public:
    
    CST_RobotKidnapping();
    
  private:
    
    const Pose3d  _kidnappedPose;
    const f32     _poseDistThresh_mm = 20.f;
    const Radians _poseAngleThresh;
    
    virtual s32 UpdateInternal() override;
    
    TestState _testState = TestState::InitialLocalization;
    
    ExternalInterface::RobotState _robotState;
    
    ObjectID _objectID_A;
    ObjectID _objectID_B;
    
    // Message handlers
    virtual void HandleRobotStateUpdate(ExternalInterface::RobotState const& msg) override;
    
  };
  
  // Register class with factory
  REGISTER_COZMO_SIM_TEST_CLASS(CST_RobotKidnapping);
  
  
  // =========== Test class implementation ===========
  CST_RobotKidnapping::CST_RobotKidnapping()
  : _kidnappedPose(-M_PI_2, Z_AXIS_3D(), {150.f, -150.f, 0})
  , _poseAngleThresh(DEG_TO_RAD(3.f))
  {
    
  }
  
  s32 CST_RobotKidnapping::UpdateInternal()
  {
    switch (_testState) {
        
      case TestState::InitialLocalization:
      {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(_objectID_A.IsSet(), 3)
        {
          CST_ASSERT(GetRobotPose().IsSameAs(GetRobotPoseActual(), _poseDistThresh_mm, _poseAngleThresh),
                     "Initial localization failed.");
          
          // Kidnap the robot (move actual robot and just tell it to delocalize
          // as if it has been picked up -- but it doesn't know where it actually
          // is anymore)
          SetActualRobotPose(_kidnappedPose);
          
          SendMessage(ExternalInterface::MessageGameToEngine(ExternalInterface::ForceDelocalizeRobot(_robotState.robotID)));
          
          _testState = TestState::Kidnap;
        }
        break;
      }
        
      case TestState::Kidnap:
      {
        // Wait until we see that the robot has gotten the delocalization message
        IF_CONDITION_WITH_TIMEOUT_ASSERT(_robotState.localizedToObjectID < 0, 2)
        {
          // Once kidnapping occurs, tell robot to turn to see the other object
          SendTurnInPlace(DEG_TO_RAD(90));
          
          _testState = TestState::LocalizeToObjectB;
        }
        break;
      }
        
      case TestState::LocalizeToObjectB:
      {
        // Wait until we see and localize to the other object
        IF_CONDITION_WITH_TIMEOUT_ASSERT(_objectID_B.IsSet(), 6)
        {
          const Pose3d checkPose = _kidnappedPose * GetRobotPose();
          CST_ASSERT(checkPose.IsSameAs(GetRobotPoseActual(), _poseDistThresh_mm, _poseAngleThresh),
                     "Localization to second object failed.");
          
          // Turn back to see object A
          SendTurnInPlace(DEG_TO_RAD(90));
          
          _testState = TestState::ReSeeObjectA;
        }
        break;
      }
        
      case TestState::ReSeeObjectA:
      {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(_robotState.localizedToObjectID == _objectID_A, 3)
        {
          CST_ASSERT(GetRobotPose().IsSameAs(GetRobotPoseActual(), _poseDistThresh_mm, _poseAngleThresh),
                     "Localization after re-seeing first object failed.");
          
          Pose3d poseA, poseB;
          CST_ASSERT(RESULT_OK == GetObjectPose(_objectID_A, poseA),
                     "Failed to get first object's pose.");
          CST_ASSERT(RESULT_OK == GetObjectPose(_objectID_B, poseB),
                     "Failed to get second object's pose.");
          
          const Pose3d poseA_actual(0, Z_AXIS_3D(), {150.f, 0.f, 22.f});
          const Pose3d poseB_actual(0, Z_AXIS_3D(), {300.f, -150.f, 22.f});
          CST_ASSERT(poseA.IsSameAs(poseA_actual, _poseDistThresh_mm, _poseAngleThresh),
                     "First object's pose incorrect after re-localization.");
          
          CST_ASSERT(poseB.IsSameAs(poseB_actual, _poseDistThresh_mm, _poseAngleThresh),
                     "Second object's pose incorrect after re-localization.");
          
          _testState = TestState::TestDone;
        }
        break;
      }
      
      case TestState::TestDone:
      {
        
        CST_EXIT();
        break;
      }
    }
    
    return _result;
  }
  
  // ================ Message handler callbacks ==================
  
  void CST_RobotKidnapping::HandleRobotStateUpdate(const ExternalInterface::RobotState &msg)
  {
    _robotState = msg;
    if(_testState == TestState::InitialLocalization)
    {
      _objectID_A = _robotState.localizedToObjectID;
    }
    else if(_testState == TestState::LocalizeToObjectB)
    {
      _objectID_B = _robotState.localizedToObjectID;
    }
  }  // ================ End of message handler callbacks ==================

} // end namespace Cozmo
} // end namespace Anki

