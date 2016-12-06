/**
 * File: CST_RollBlockBehavior.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-06-24
 *
 * Description: This is a test for the roll block behavior, not just the action
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/simulator/game/cozmoSimTestController.h"

#define SET_STATE(s) {                                                  \
    PRINT_NAMED_INFO("CST_RollBlockBehavior.TransitionTestState",       \
                     "%s", #s);                                         \
    _testState = TestState::s;                                          \
  }


namespace Anki {
namespace Cozmo {

enum class TestState {
  Init,
  VerifyObject,
  TurnAway,
  DontStartBehavior,
  TurnBack,
  Rolling,
  TestDone
};

static const char* kBehaviorName = "RollBlockOnSide";

class CST_RollBlockBehavior : public CozmoSimTestController {
private:
      
  virtual s32 UpdateSimInternal() override;

  virtual void HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg) override;
  
  TestState _testState = TestState::Init;

  bool _startedMoving = false;
  bool _startedBehavior = false;
  bool _stoppedBehavior = false;

  double _behaviorStartedTime = 0.0;
};

// Register class with factory
REGISTER_COZMO_SIM_TEST_CLASS(CST_RollBlockBehavior);


s32 CST_RollBlockBehavior::UpdateSimInternal()
{
  PrintPeriodicBlockDebug();
  
  switch (_testState) {
    case TestState::Init:
    {
      MakeSynchronous();
      DisableRandomPathSpeeds();
      StartMovieConditional("RollBlockBehavior");
      // TakeScreenshotsAtInterval("RollBlockBehavior", 1.f);
      
      // make sure rolling is unlocked
      UnlockId unlock = UnlockIdsFromString("RollCube");
      CST_ASSERT(unlock != UnlockId::Count, "couldn't get valid unlock id");
      SendMessage( ExternalInterface::MessageGameToEngine(
                     ExternalInterface::RequestSetUnlock(unlock, true)));
          
      SendMoveHeadToAngle(0, 100, 100);
      SET_STATE(VerifyObject)
      break;
    }

    case TestState::VerifyObject:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                       NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL) &&
                                       GetNumObjects() == 1, DEFAULT_TIMEOUT)
      {
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 11;
        m.numRetries = 1;
        uint8_t isAbsolute = 0; // relative turn
        m.action.Set_turnInPlace(ExternalInterface::TurnInPlace( DEG_TO_RAD(90), PI_F, 500.0f, isAbsolute, 1 ));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
        _startedMoving = false;
        SET_STATE(TurnAway)
      }
      break;
    }

    case TestState::TurnAway:
    {
      if( !_startedMoving ) {
        IF_CONDITION_WITH_TIMEOUT_ASSERT(IsRobotStatus(RobotStatusFlag::IS_MOVING), DEFAULT_TIMEOUT) {
          _startedMoving = true;
        }
      }
      else {      
        IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING), 10) {
          // deloc the robot, then try to start the behavior. Behavior shouldn't start
          ExternalInterface::ForceDelocalizeRobot delocMsg;
          delocMsg.robotID = 1;
          SendMessage(ExternalInterface::MessageGameToEngine(std::move(delocMsg)));

          SendMessage(ExternalInterface::MessageGameToEngine(
                        ExternalInterface::ActivateBehaviorChooser(BehaviorChooserType::Selection)));
          SendMessage(ExternalInterface::MessageGameToEngine(
                        ExternalInterface::ExecuteBehaviorByName(kBehaviorName)));

          _behaviorStartedTime = GetSupervisor()->getTime();
          SET_STATE(DontStartBehavior)
        }
      }
      break;
    }

    case TestState::DontStartBehavior:
    {

      CST_ASSERT( !_startedBehavior, "Behavior shouldnt start because we delocalized" );
      
      const double timeToWait_s = 2.0;
      double currTime = GetSupervisor()->getTime();
      if( currTime - _behaviorStartedTime > timeToWait_s ) {
        // turn back
        ExternalInterface::QueueSingleAction m;
        m.robotID = 1;
        m.position = QueueActionPosition::NOW;
        m.idTag = 17;
        m.numRetries = 1;
        uint8_t isAbsolute = 0; // relative turn
        m.action.Set_turnInPlace(ExternalInterface::TurnInPlace( -DEG_TO_RAD(90), PI_F, 500.0f, isAbsolute, 1 ));
        ExternalInterface::MessageGameToEngine message;
        message.Set_QueueSingleAction(m);
        SendMessage(message);
        _startedMoving = false;
        SET_STATE(TurnBack)
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
        IF_CONDITION_WITH_TIMEOUT_ASSERT( _startedBehavior, 10 ) {
          // behavior is running, wait for it to finish
          SET_STATE(Rolling)
        }
      }
      break;
    }

    case TestState::Rolling:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT( _stoppedBehavior, 20 ) {
        // behavior should eventually stop
        SET_STATE(TestDone)
      }
      break;
    }
    
    case TestState::TestDone:
    {
      // Verify robot has rolled the block
      Pose3d pose = GetLightCubePoseActual(0);
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsRobotStatus(RobotStatusFlag::IS_MOVING) &&
                                       GetCarryingObjectID() == -1 &&
                                       pose.GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS, 2)
      {
        StopMovie();
        CST_EXIT();
      }
      break;
    }
  }
  return _result;
}

void CST_RollBlockBehavior::HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg)
{
  PRINT_NAMED_INFO("CST_RollBlockBehavior.transition", "%s -> %s",
                   msg.oldBehavior.c_str(),
                   msg.newBehavior.c_str());
  
  if(msg.oldBehavior == kBehaviorName) {
    _stoppedBehavior = true;
  }
  if(msg.newBehavior == kBehaviorName) {
    _startedBehavior = true;
  }
}

}
}
