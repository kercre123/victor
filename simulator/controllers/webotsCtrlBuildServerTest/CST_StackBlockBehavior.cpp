/**
 * File: CST_StackBlockBehavior.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-06-24
 *
 * Description: This is a test for the stack block behavior, not just the action
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/simulator/game/cozmoSimTestController.h"

#define SET_STATE(s) {                                                  \
    PRINT_NAMED_INFO("CST_StackBlockBehavior.TransitionTestState",      \
                     "%s", #s);                                         \
    _testState = TestState::s;                                          \
  }


namespace Anki {
namespace Cozmo {

enum class TestState {
  Init,
  WaitForCubeConnections,
  VerifyObject1,
  TurnAway,
  WaitForDeloc,
  VerifyObject2,
  DontStartBehavior,
  TurnBack,
  PickingUp,
  BehaviorShouldFail,
  TurnToObject2,
  Stacking,
  TestDone
};

static const char* kBehaviorName = "StackBlocks";

namespace {
  static const Pose3d kidnapCubePose(M_PI_F, Z_AXIS_3D(), {40.0, -150.0, 22.0});
  static const Pose3d hideCubePose(M_PI_F, Z_AXIS_3D(), {10000.0, 10000.0, 100.0});
}


class CST_StackBlockBehavior : public CozmoSimTestController {
private:
      
  virtual s32 UpdateSimInternal() override;

  virtual void HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg) override;
  virtual void HandleActiveObjectConnectionState(const ObjectConnectionState& msg) override;
  
  TestState _testState = TestState::Init;

  bool _startedMoving = false;
  int _startedBehavior = 0;
  int _stoppedBehavior = 0;

  int _cubeIdToMove = -1;

  double _behaviorCheckTime = 0.0;
  
  u32 _numObjectsConnected = 0;
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_StackBlockBehavior);


s32 CST_StackBlockBehavior::UpdateSimInternal()
{
  PrintPeriodicBlockDebug();

  switch (_testState) {
    case TestState::Init:
    {
      MakeSynchronous();
      DisableRandomPathSpeeds();
      StartMovieConditional("StackBlockBehavior");
      // TakeScreenshotsAtInterval("StackBlockBehavior", 1.f);
      
      // make sure rolling is unlocked
      UnlockId unlock = UnlockIdsFromString("StackTwoCubes");
      CST_ASSERT(unlock != UnlockId::Count, "couldn't get valid unlock id");
      SendMessage( ExternalInterface::MessageGameToEngine(
                     ExternalInterface::RequestSetUnlock(unlock, true)));

      // try to start the behavior now, it shouldn't start for a while
      SendMessage(ExternalInterface::MessageGameToEngine(
                    ExternalInterface::ActivateBehaviorChooser(BehaviorChooserType::Selection)));
      SendMessage(ExternalInterface::MessageGameToEngine(
                    ExternalInterface::ExecuteBehaviorByName(kBehaviorName)));

          
      SendMoveHeadToAngle(0, 100, 100);
      SET_STATE(WaitForCubeConnections);
      break;
    }

    case TestState::WaitForCubeConnections:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_numObjectsConnected == 2, 5)
      {
        SET_STATE(VerifyObject1);
      }
      break;
    }
      
    case TestState::VerifyObject1:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                            GetNumObjects() == 1,
                                            IsLocalizedToObject())
      {
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 11;
        m.numRetries = 1;
        uint8_t isAbsolute = 0; // relative turn
        m.action.Set_turnInPlace(ExternalInterface::TurnInPlace( DEG_TO_RAD(45), M_PI_F, 500.0f, isAbsolute, 1 ));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
        _startedMoving = false;
        SET_STATE(TurnAway);
      }
      break;
    }

    case TestState::TurnAway:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      if( !_startedMoving ) {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(IsRobotStatus(RobotStatusFlag::IS_MOVING), DEFAULT_TIMEOUT) {
          _startedMoving = true;
        }
      }
      else {      
        IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING), 10) {
          // Make sure we are still localized (to an object) before sending deloc
          CST_ASSERT( IsLocalizedToObject(), "Should be localized to object before we deloc");
          // deloc the robot so it doesn't see both cubes in the same frame
          SendForceDeloc();
          SET_STATE(WaitForDeloc);
        }
      }
      break;
    }

    case TestState::WaitForDeloc:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsLocalizedToObject(), 2) {

        // Turn the robot to see the second cube
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NEXT;
        m.idTag = 16;
        m.numRetries = 1;
        uint8_t isAbsolute = 0; // relative turn
        m.action.Set_turnInPlace(ExternalInterface::TurnInPlace( DEG_TO_RAD(30), M_PI_F, 500.0f, isAbsolute, 1 ));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
          
        _startedMoving = false;
        SET_STATE(VerifyObject2);
      }
      break;
    }
    
    case TestState::VerifyObject2:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      // NOTE: at some point in the future this might fail because GetNumObjects() might not return things not
      // in our current origin. In that case, the test will need to be updated
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                            GetNumObjects() == 2)
      {
        _behaviorCheckTime = GetSupervisor()->getTime();
        SET_STATE(DontStartBehavior);        
      }
      break;
    }

    case TestState::DontStartBehavior:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      const double timeToWait_s = 1.0;
      double currTime = GetSupervisor()->getTime();
      if( currTime - _behaviorCheckTime > timeToWait_s ) {
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 13;
        m.numRetries = 1;
        uint8_t isAbsolute = 0; // relative turn
        m.action.Set_turnInPlace(ExternalInterface::TurnInPlace( -DEG_TO_RAD(90), M_PI_F, 500.0f, isAbsolute, 1 ));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
        _startedMoving = false;
        SET_STATE(TurnBack);
      }
      break;
    }

    case TestState::TurnBack:
    {
      if( !_startedMoving ) {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(IsRobotStatus(RobotStatusFlag::IS_MOVING), DEFAULT_TIMEOUT) {
          _startedMoving = true;
        }
      }
      else {
        // at some point (possibly before we stop moving) the behavior should become runnable and start on it's own
        IF_CONDITION_WITH_TIMEOUT_ASSERT( _startedBehavior == 1, 10 ) {
          // behavior is running, wait for it to finish
          SET_STATE(PickingUp)
        }
      }
      break;
    }

    case TestState::PickingUp:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT( GetCarryingObjectID() >= 0, 20) {
        // move the cube we aren't holding
        _cubeIdToMove = GetCarryingObjectID() == 0 ? 1 : 0;

        // raise it off the ground so we trigger a moved event, and send it somewhere we know the robot won't see it
        SetLightCubePose(_cubeIdToMove, hideCubePose);
        SET_STATE(BehaviorShouldFail);
        // at this point, robot should be trying to do stacking action. It picked up the first cube, then will
        // go try to stack it, but should fail because the bottom cube isn't there
        _behaviorCheckTime = -1.0;
      }
      break;
    }

    case TestState::BehaviorShouldFail:
    {
      CST_ASSERT( _startedBehavior == 1, "Behavior started wrong number of times: "<<_startedBehavior );

      const double currTime_s = GetSupervisor()->getTime();

      if( _behaviorCheckTime < 0.0 ) {
        IF_CONDITION_WITH_TIMEOUT_ASSERT( _stoppedBehavior == 1, 30.0 ) {
          _behaviorCheckTime = currTime_s;
        }
      }
      else {
        static constexpr double timeToHold_s = 0.5f;
        if(currTime_s - _behaviorCheckTime >= timeToHold_s ) {

          CST_ASSERT(_cubeIdToMove >= 0, "Internal test error (this shouldn't happen)");
          
          // now send the cube to a place where the robot will be able to see / interact with it
          SetLightCubePose(_cubeIdToMove, kidnapCubePose);

          // first move the head down
          SendMoveHeadToAngle(0, 100, 100);
          
          // figure out angle to turn to
          float targetAngle = atan2(kidnapCubePose.GetTranslation().y() - GetRobotPoseActual().GetTranslation().y(),
                                    kidnapCubePose.GetTranslation().x() - GetRobotPoseActual().GetTranslation().x());
          
          float robotTrueAngle = GetRobotPoseActual().GetRotation().GetAngleAroundZaxis().ToFloat();
          float robotAssumedAngle = GetRobotPose().GetRotation().GetAngleAroundZaxis().ToFloat();
          float robotAngleError = robotTrueAngle - robotAssumedAngle;
          targetAngle -= robotAngleError;
          
          ExternalInterface::QueueSingleAction m;
          m.robotID = 1;
          // queue after the move head down
          m.position = QueueActionPosition::AT_END;
          m.idTag = 19;
          m.numRetries = 1;
          uint8_t isAbsolute = 1;
          m.action.Set_turnInPlace(ExternalInterface::TurnInPlace( targetAngle, M_PI_F, 500.0f, isAbsolute, 1 ));
          ExternalInterface::MessageGameToEngine message;
          message.Set_QueueSingleAction(m);
          SendMessage(message);
          _startedMoving = false;
          SET_STATE(TurnToObject2);
        }        
      }
      break;
    }

    case TestState::TurnToObject2:
    {
      // we already know about the object, so just wait a bit after we stop moving
      if( !_startedMoving ) {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(IsRobotStatus(RobotStatusFlag::IS_MOVING), DEFAULT_TIMEOUT) {
          _startedMoving = true;
        }
      }
      else  {
        IF_CONDITION_WITH_TIMEOUT_ASSERT( _startedBehavior == 2, 10 ) {
          SET_STATE(Stacking);
        }
      }
      break;
    }
    
    case TestState::Stacking:
    {
      CST_ASSERT( _startedBehavior == 2, "Behavior started wrong number of times: " << _startedBehavior );
      
      IF_CONDITION_WITH_TIMEOUT_ASSERT( _stoppedBehavior == 2, 40 ) {
        // behavior should eventually stop
        SET_STATE(TestDone)
      }
      break;
    }
    
    case TestState::TestDone:
    {
      // Verify robot has stacked the blocks
      // NOTE: actual poses are in meters
      Pose3d pose0 = GetLightCubePoseActual(0);
      Pose3d pose1 = GetLightCubePoseActual(1);

      const float stackingTolerance_mm = 40.0f;
      
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            GetCarryingObjectID() == -1,
                                            // (x,y) positions are nearly on top of each other
                                            NEAR(pose0.GetTranslation().x(), pose1.GetTranslation().x(), stackingTolerance_mm),
                                            NEAR(pose0.GetTranslation().y(), pose1.GetTranslation().y(), stackingTolerance_mm),
                                            // difference between z's is about a block height
                                            NEAR( ABS(pose1.GetTranslation().z() - pose0.GetTranslation().z()), 44.0f, 10.0f))
      {
        StopMovie();
        CST_EXIT();
      }
      break;
    }
  }
  return _result;
}

void CST_StackBlockBehavior::HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg)
{
  PRINT_NAMED_INFO("CST_StackBlockBehavior.transition", "%s -> %s",
                   msg.oldBehaviorName.c_str(),
                   msg.newBehaviorName.c_str());
  
  if(msg.oldBehaviorName == kBehaviorName) {
    _stoppedBehavior++;
  }
  if(msg.newBehaviorName == kBehaviorName) {
    _startedBehavior++;
  }
}
  
void CST_StackBlockBehavior::HandleActiveObjectConnectionState(const ObjectConnectionState& msg)
{
  if (msg.connected) {
    ++_numObjectsConnected;
  } else if (_numObjectsConnected > 0) {
    --_numObjectsConnected;
  }
}

}
}
