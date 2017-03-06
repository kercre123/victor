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
  WaitForDeloc,
  DontStartBehavior,
  TurnBack,
  Rolling,
  PushBlockBackward,
  PushBlockToSide,
  WaitingForRollBlockDone,
  TestDone
};

static const char* kBehaviorName = "RollBlockOnSide";

class CST_RollBlockBehavior : public CozmoSimTestController {
private:
      
  virtual s32 UpdateSimInternal() override;

  // Message handlers:
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg) override;
  virtual void HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg) override;
  
  // State:
  TestState _testState = TestState::Init;

  // Parameters:
  const float _kRobotNearBlockThreshold_mm = 50.f;
  
  // Status flags:
  bool _startedBehavior = false;
  bool _stoppedBehavior = false;
 
  ActionResult _moveHeadToAngleResult = ActionResult::RUNNING;
  ActionResult _turnInPlaceResult = ActionResult::RUNNING;
  ActionResult _rollActionResult = ActionResult::RUNNING;

  // Timers:
  double _behaviorStartedTime = 0.0;
  double _pushedBlockTime = 0.0;
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
      
      _moveHeadToAngleResult = ActionResult::RUNNING;
      SendMoveHeadToAngle(0, 100, 100);
      SET_STATE(VerifyObject)
      break;
    }

      
    case TestState::VerifyObject:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            _moveHeadToAngleResult == ActionResult::SUCCESS,
                                            GetNumObjects() == 1,
                                            IsLocalizedToObject()) {
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace(DEG_TO_RAD(90.f), M_PI_F, 500.f);
        SET_STATE(TurnAway)
      }
      break;
    }

      
    case TestState::TurnAway:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_turnInPlaceResult == ActionResult::SUCCESS, 10) {
        // Make sure we are still localized (to an object) before sending deloc
        CST_ASSERT( IsLocalizedToObject(), "Should be localized to object before we deloc");
        SendForceDeloc();
        SET_STATE(WaitForDeloc);
      }
      break;
    }

      
    case TestState::WaitForDeloc:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsLocalizedToObject(), 2) {
        // once we are deloc'd, try to start the behavior (which shouldn't start)
        SendMessage(ExternalInterface::MessageGameToEngine(
                      ExternalInterface::ActivateBehaviorChooser(BehaviorChooserType::Selection)));
        SendMessage(ExternalInterface::MessageGameToEngine(
                      ExternalInterface::ExecuteBehaviorByName(kBehaviorName)));
        
        _behaviorStartedTime = GetSupervisor()->getTime();
        SET_STATE(DontStartBehavior);
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
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace(DEG_TO_RAD(-90.f), M_PI_F, 500.f);
        SET_STATE(TurnBack)
      }
      break;
    }

      
    case TestState::TurnBack:
    {
      // At some point (possibly before we stop moving) the behavior should become runnable and start on it's own.
      //  The behavior kicking in may cause the TurnInPlace to be CANCELLED.
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(10,
                                            (_turnInPlaceResult == ActionResult::SUCCESS || _turnInPlaceResult == ActionResult::CANCELLED),
                                            _startedBehavior) {
        // behavior is running, wait for it to finish
        SET_STATE(Rolling)
      }
      break;
    }

      
    case TestState::Rolling:
    {
      // Verify that the behavior has stopped and that the cube has been flipped.
      Pose3d pose = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(25,
                                            _stoppedBehavior,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            GetCarryingObjectID() == -1,
                                            pose.GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS) {
        // reset the flags keeping track of roll behavior (since it may restart)
        _startedBehavior = false;
        _stoppedBehavior = false;
        _rollActionResult = ActionResult::RUNNING;
        // Apply a force to flip the block back into its original orientation.
        //  The behavior will trigger again automatically once the robot sees it.
        SendApplyForce("cube", 15, -10, 0);
        SET_STATE(PushBlockBackward)
      }
      break;
    }
      
    
    case TestState::PushBlockBackward:
    {
      // Wait for robot to restart the behavior, get close to block, and
      //  stop driving, then give it a shove backward but still in view.
      Pose3d robotPose = GetRobotPoseActual();
      Pose3d cubePose = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThreshold_mm;
      
      // Still driving?
      const bool wheelsMoving = IsRobotStatus(RobotStatusFlag::ARE_WHEELS_MOVING);
      
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(25,
                                            _startedBehavior,
                                            nearBlock,
                                            !wheelsMoving) {
        // Push the block away so that the roll will fail.
        SendApplyForce("cube", 10, -5, 10);
        _pushedBlockTime = GetSupervisor()->getTime();
        SET_STATE(PushBlockToSide)
      }
      break;
    }
      
      
    case TestState::PushBlockToSide:
    {
      // Cozmo should fail to roll the block, move backward, and approach block to retry.
      
      // The roll behavior should never have stopped:
      CST_ASSERT(!_stoppedBehavior, "Roll behavior should not be finished yet! Still need to retry.")
      
      // Wait for robot to get close to block, then give it a shove to the side such that it is out of view.
      Pose3d robotPose = GetRobotPoseActual();
      Pose3d cubePose = GetLightCubePoseActual(ObjectType::Block_LIGHTCUBE1);
      const bool nearBlock = ComputeDistanceBetween(robotPose, cubePose) < _kRobotNearBlockThreshold_mm;
      
      // wait a bit for the block to move away after the previous push.
      const double timeToWait_s = 0.5;
      double currTime = GetSupervisor()->getTime();
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(15,
                                            currTime - _pushedBlockTime > timeToWait_s,
                                            nearBlock) {
        // Push the block to the side, out of Cozmo's view. He should end the
        //  roll behavior with a failed action.
        SendApplyForce("cube", 5, 20, 10);
        SET_STATE(WaitingForRollBlockDone);
      }
      break;
    }
      
      
    case TestState::WaitingForRollBlockDone:
    {
      // Wait for behavior to end and action to fail with "REACHED_MAX_NUM_RETRIES".
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            _stoppedBehavior,
                                            _rollActionResult == ActionResult::REACHED_MAX_NUM_RETRIES) {
        // Roll action failed as expected and behavior stopped.
        SET_STATE(TestDone)
      }
      break;
    }
      
      
    case TestState::TestDone:
    {
      StopMovie();
      CST_EXIT();
      break;
    }
  }
  return _result;
}

  
void CST_RollBlockBehavior::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg)
{
  PRINT_NAMED_INFO("CST_RollBlockBehavior.HandleRobotCompletedAction", "completed action %s, result %s",
                   EnumToString(msg.actionType),
                   EnumToString(msg.result));
  
  if(msg.actionType == RobotActionType::MOVE_HEAD_TO_ANGLE) {
    _moveHeadToAngleResult = msg.result;
  } else if(msg.actionType == RobotActionType::TURN_IN_PLACE) {
    _turnInPlaceResult = msg.result;
  } else if (msg.actionType == RobotActionType::ROLL_OBJECT_LOW) {
    _rollActionResult = msg.result;
  }
}
  
  
void CST_RollBlockBehavior::HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg)
{
  PRINT_NAMED_INFO("CST_RollBlockBehavior.transition", "%s -> %s",
                   msg.oldBehaviorName.c_str(),
                   msg.newBehaviorName.c_str());
  
  if(msg.oldBehaviorName == kBehaviorName) {
    _stoppedBehavior = true;
  }
  if(msg.newBehaviorName == kBehaviorName) {
    _startedBehavior = true;
  }
}

}
}
