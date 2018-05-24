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

#include "engine/robot.h"
#include "simulator/game/cozmoSimTestController.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"

namespace Anki {
namespace Cozmo {

enum class TestState {
  Init,                         // Init everything
  WaitForCubeConnections,       // Ensure 2 blocks are connected
  VerifyObject1,                // Should see 1 block
  TurnAway,                     // Turn away from the block and force delocalize
  WaitForDeloc,                 //
  VerifyObject2,                // Turn to see the second block
  DontStartBehavior,            // Stack behavior shouldn't start yet (still delocalized)
  TurnBack,                     // Turn back toward first block
  PickingUp,                    // Stack behavior should begin
  BehaviorShouldFail,           // Stack behavior should fail (since second block was kidnapped and moved across workspace)
  TurnToObject2,                // Turn toward the second block's new location
  Stacking,                     // Stacking should occur
  VerifyStack,                  // Make sure stack was created
  TestDone
};

static const BehaviorID kBehaviorID = BEHAVIOR_ID(StackBlocks);

namespace {
  static const Pose3d kidnapCubePose(M_PI_F, Z_AXIS_3D(), {40.0, -150.0, 22.0});
  static const Pose3d hideCubePose(M_PI_F, Z_AXIS_3D(), {10000.0, 10000.0, 100.0});
}


class CST_StackBlockBehavior : public CozmoSimTestController {
private:
      
  virtual s32 UpdateSimInternal() override;
  
  virtual void HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg) override;
  virtual void HandleActiveObjectConnectionState(const ExternalInterface::ObjectConnectionState& msg) override;
  virtual void HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg) override;
  
  // State:
  TestState _testState = TestState::Init;

  // Parameters:
  ObjectType  _baseCube    = ObjectType::InvalidObject;
  std::string _baseCubeStr = "";
  int         _topCubeID  = -1;
  ObjectType  _topCube     = ObjectType::InvalidObject;
  std::string _topCubeStr  = "";
  
  // Status flags:
  int _startedBehavior = 0;
  int _stoppedBehavior = 0;
  
  ActionResult _moveHeadToAngleResult = ActionResult::RUNNING;
  ActionResult _turnInPlaceResult = ActionResult::RUNNING;
  ActionResult _pickupObjectResult = ActionResult::RUNNING;
  ActionResult _placeObjectResult = ActionResult::RUNNING;

  
  Pose3d _robotPoseWhenBlockShoved;
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
      DisableRandomPathSpeeds();
      StartMovieConditional("StackBlockBehavior");
      // TakeScreenshotsAtInterval("StackBlockBehavior", 1.f);
      
      // make sure stacking is unlocked
      UnlockId unlock = UnlockIdFromString("StackTwoCubes");
      CST_ASSERT(unlock != UnlockId::Count, "couldn't get valid unlock id");
      SendMessage( ExternalInterface::MessageGameToEngine(
                     ExternalInterface::RequestSetUnlock(unlock, true)));

      // try to start the behavior now, it shouldn't start for a while
      SendMessage(ExternalInterface::MessageGameToEngine(
                    ExternalInterface::ExecuteBehaviorByID(
                      BehaviorTypesWrapper::BehaviorIDToString(kBehaviorID), -1)));
          
      SendMoveHeadToAngle(0, 100, 100);
      SET_TEST_STATE(WaitForCubeConnections);
      break;
    }

      
    case TestState::WaitForCubeConnections:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_numObjectsConnected == 2, 5)
      {
        SET_TEST_STATE(VerifyObject1);
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
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace(DEG_TO_RAD(45.f), M_PI_F, 500.f);
        SET_TEST_STATE(TurnAway);
      }
      break;
    }

      
    case TestState::TurnAway:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      IF_CONDITION_WITH_TIMEOUT_ASSERT(_turnInPlaceResult == ActionResult::SUCCESS, 10) {
        // Make sure we are still localized (to an object) before sending deloc
        CST_ASSERT( IsLocalizedToObject(), "Should be localized to object before we deloc");
        // deloc the robot so it doesn't see both cubes in the same frame
        SendForceDeloc();
        SET_TEST_STATE(WaitForDeloc);
      }
      break;
    }

      
    case TestState::WaitForDeloc:
    {
      IF_CONDITION_WITH_TIMEOUT_ASSERT(!IsLocalizedToObject(), 2) {
        // Turn the robot to see the second cube
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace(DEG_TO_RAD(30.f), M_PI_F, 500.0f);

        SET_TEST_STATE(VerifyObject2);
      }
      break;
    }
      
    
    case TestState::VerifyObject2:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      // NOTE: at some point in the future this might fail because GetNumObjects() might not return things not
      // in our current origin. In that case, the test will need to be updated
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(DEFAULT_TIMEOUT,
                                            _turnInPlaceResult == ActionResult::SUCCESS,
                                            NEAR(GetRobotHeadAngle_rad(), 0, HEAD_ANGLE_TOL),
                                            GetNumObjects() == 1)
      {
        _behaviorCheckTime = GetSupervisor()->getTime();
        SET_TEST_STATE(DontStartBehavior);        
      }
      break;
    }

      
    case TestState::DontStartBehavior:
    {
      CST_ASSERT( _startedBehavior == 0, "Behavior shouldnt start because we delocalized (should only have one block)" );

      const double timeToWait_s = 1.0;
      double currTime = GetSupervisor()->getTime();
      if( currTime - _behaviorCheckTime > timeToWait_s ) {
        _turnInPlaceResult = ActionResult::RUNNING;
        SendTurnInPlace( DEG_TO_RAD(-90.f), M_PI_F, 500.f );
        SET_TEST_STATE(TurnBack);
      }
      break;
    }

      
    case TestState::TurnBack:
    {
      // at some point (possibly before we stop moving) the behavior should become activatable and start on it's own
      IF_CONDITION_WITH_TIMEOUT_ASSERT(_startedBehavior == 1, 10) {
        // behavior is running, wait for it to finish
        _pickupObjectResult = ActionResult::RUNNING;
        SET_TEST_STATE(PickingUp)
      }
      
      break;
    }

      
    case TestState::PickingUp:
    {
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            GetCarryingObjectID() >= 0,
                                            _pickupObjectResult == ActionResult::SUCCESS) {
        // move the cube we aren't holding
        ObjectType carryingCube;
        CST_ASSERT(GetObjectType(GetCarryingObjectID(), carryingCube) == Result::RESULT_OK,
                   "Could not get object type for cube being carried");
        _baseCube = (carryingCube == ObjectType::Block_LIGHTCUBE1) ?
                      ObjectType::Block_LIGHTCUBE2 :
                      ObjectType::Block_LIGHTCUBE1;
        _topCube = carryingCube;
        
        _baseCubeStr = (_baseCube == ObjectType::Block_LIGHTCUBE1) ? "cube0" : "cube1";
        _topCubeStr  = (_topCube == ObjectType::Block_LIGHTCUBE1)  ? "cube0" : "cube1";
        
        _topCubeID  = GetCarryingObjectID();


        // raise it off the ground so we trigger a moved event, and send it somewhere we know the robot won't see it
        SetLightCubePose(_baseCube, hideCubePose);
        SET_TEST_STATE(BehaviorShouldFail);
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
          
          // now send the cube to a place where the robot will be able to see / interact with it
          SetLightCubePose(_baseCube, kidnapCubePose);

          // first move the head down
          SendMoveHeadToAngle(0, 100, 100);
          
          // figure out angle to turn to
          float targetAngle = atan2(kidnapCubePose.GetTranslation().y() - GetRobotPoseActual().GetTranslation().y(),
                                    kidnapCubePose.GetTranslation().x() - GetRobotPoseActual().GetTranslation().x());
          
          float robotTrueAngle = GetRobotPoseActual().GetRotation().GetAngleAroundZaxis().ToFloat();
          float robotAssumedAngle = GetRobotPose().GetRotation().GetAngleAroundZaxis().ToFloat();
          float robotAngleError = robotTrueAngle - robotAssumedAngle;
          targetAngle -= robotAngleError;

          // queue turn in place after the move head down
          _turnInPlaceResult = ActionResult::RUNNING;
          SendTurnInPlace(targetAngle, M_PI_F, 500.0f, POINT_TURN_ANGLE_TOL, true, QueueActionPosition::AT_END); // Absolute turn, put at end of action queue.
          
          SET_TEST_STATE(TurnToObject2);
        }        
      }
      break;
    }

      
    case TestState::TurnToObject2:
    {
      // Behavior should start right after seeing block (and so TurnInPlace may be cancelled)
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(10,
                                            (_turnInPlaceResult == ActionResult::SUCCESS || _turnInPlaceResult == ActionResult::CANCELLED_WHILE_RUNNING),
                                            _startedBehavior == 2) {
        SET_TEST_STATE(Stacking);
      }
      break;
    }
      
    
    case TestState::Stacking:
    {
      CST_ASSERT( _startedBehavior == 2, "Behavior started wrong number of times: " << _startedBehavior );
      
      IF_CONDITION_WITH_TIMEOUT_ASSERT( _stoppedBehavior == 2, 40 ) {
        // behavior should eventually stop
        SET_TEST_STATE(VerifyStack)
      }
      break;
    }
    
      
    case TestState::VerifyStack:
    {
      // Verify robot has stacked the blocks
      // NOTE: actual poses are in meters
      Pose3d pose1 = GetLightCubePoseActual(_baseCube);
      Pose3d pose2 = GetLightCubePoseActual(_topCube);

      const float stackingTolerance_mm = 40.0f;
      
      IF_ALL_CONDITIONS_WITH_TIMEOUT_ASSERT(20,
                                            !IsRobotStatus(RobotStatusFlag::IS_MOVING),
                                            GetCarryingObjectID() == -1,
                                            // (x,y) positions are nearly on top of each other
                                            NEAR(pose1.GetTranslation().x(), pose2.GetTranslation().x(), stackingTolerance_mm),
                                            NEAR(pose1.GetTranslation().y(), pose2.GetTranslation().y(), stackingTolerance_mm),
                                            // difference between z's is about a block height
                                            NEAR( ABS(pose2.GetTranslation().z() - pose1.GetTranslation().z()), 44.0f, 10.0f)) {
        // Cancel the stack behavior:
        SendMessage(ExternalInterface::MessageGameToEngine(
                       ExternalInterface::ExecuteBehaviorByID(
                         BehaviorTypesWrapper::BehaviorIDToString(BEHAVIOR_ID(Wait)), -1)));
        SET_TEST_STATE(TestDone)
      }
      break;
    }
      
      
    case TestState::TestDone:
    {
      // Since we deleted a node earlier, we have to save the world
      //  to prevent a modal "Save Changes?" dialog from preventing
      //  Webots from quitting. No biggie since it will just overwrite
      //  __generated__.wbt.
      GetSupervisor()->saveWorld();
      
      StopMovie();
      CST_EXIT();

      break;
    }
  }
  return _result;
}


void CST_StackBlockBehavior::HandleRobotCompletedAction(const ExternalInterface::RobotCompletedAction &msg)
{
  PRINT_NAMED_INFO("CST_StackBlockBehavior.HandleRobotCompletedAction", "completed action %s, result %s", EnumToString(msg.actionType), EnumToString(msg.result));
  
  if(msg.actionType == RobotActionType::MOVE_HEAD_TO_ANGLE) {
    _moveHeadToAngleResult = msg.result;
  } else if(msg.actionType == RobotActionType::TURN_IN_PLACE) {
    _turnInPlaceResult = msg.result;
  } else if (msg.actionType == RobotActionType::PICKUP_OBJECT_HIGH
             || msg.actionType == RobotActionType::PICKUP_OBJECT_LOW
             || msg.actionType == RobotActionType::COMPOUND) {
    _pickupObjectResult = msg.result;
  } else if (msg.actionType == RobotActionType::PLACE_OBJECT_HIGH
             || msg.actionType == RobotActionType::PLACE_OBJECT_LOW) {
    _placeObjectResult = msg.result;
  }
}
  
void CST_StackBlockBehavior::HandleBehaviorTransition(const ExternalInterface::BehaviorTransition& msg)
{
  /**PRINT_NAMED_INFO("CST_StackBlockBehavior.transition", "%s -> %s",
                   BehaviorIDToString(msg.oldBehaviorID),
                   BehaviorIDToString(msg.newBehaviorID));
  
  if(msg.oldBehaviorID == kBehaviorID) {
    _stoppedBehavior++;
  }
  if(msg.newBehaviorID == kBehaviorID) {
    _startedBehavior++;
  }**/
}
  
void CST_StackBlockBehavior::HandleActiveObjectConnectionState(const ExternalInterface::ObjectConnectionState& msg)
{
  if (msg.connected) {
    ++_numObjectsConnected;
  } else if (_numObjectsConnected > 0) {
    --_numObjectsConnected;
  }
}
  

}
}
