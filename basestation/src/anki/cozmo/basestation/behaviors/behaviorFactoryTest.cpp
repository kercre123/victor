/**
 * File: behaviorFactoryTest.h
 *
 * Author: Kevin Yoon
 * Date:   03/18/2016
 *
 * Description: Functional test fixture behavior
 *
 *              Init conditions:
 *                - Cozmo is on starting charger of the test fixture
 *
 *              Behavior:
 *
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviors/behaviorFactoryTest.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#define DEBUG_FACTORY_TEST_BEHAVIOR 1

#define END_TEST_IN_HANDLER(ERRCODE) EndTest(ERRCODE); return RESULT_OK;

namespace Anki {
namespace Cozmo {
  
  BehaviorFactoryTest::BehaviorFactoryTest(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _cliffDetectPose(0, Z_AXIS_3D(), {50, 0, 0}, &robot.GetPose().FindOrigin())
  , _camCalibPose(0, Z_AXIS_3D(), {0, 0, 0}, &robot.GetPose().FindOrigin())
  , _prePickupPose( DEG_TO_RAD(90), Z_AXIS_3D(), {0, 100, 0}, &robot.GetPose().FindOrigin())
  , _expectedLightCubePose(0, Z_AXIS_3D(), {0, 300, 0}, &robot.GetPose().FindOrigin())
  , _currentState(State::StartOnCharger)
  , _lastHandlerResult(RESULT_OK)
  , _testResult(FactoryTestErrorCode::UNKNOWN)
  {
    SetDefaultName("FactoryTest");

    // start with defaults
    _motionProfile = DEFAULT_PATH_MOTION_PROFILE;

    _motionProfile.speed_mmps = 100.0f;
    _motionProfile.accel_mmps2 = 200.0f;
    _motionProfile.decel_mmps2 = 200.0f;
    _motionProfile.pointTurnSpeed_rad_per_sec = MAX_BODY_ROTATION_SPEED_RAD_PER_SEC;
    _motionProfile.pointTurnAccel_rad_per_sec2 = MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2;
    _motionProfile.pointTurnDecel_rad_per_sec2 = MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2;
    _motionProfile.dockSpeed_mmps = 80.0f; // slow it down a bit for reliability
    _motionProfile.reverseSpeed_mmps = 80.0f;

    _camCalibPanAndTiltAngles = {{0,               0},
                                 {0,               DEG_TO_RAD(20)},
                                 {DEG_TO_RAD(-90), 0},
                                 {DEG_TO_RAD(-40), 0},
                                 {DEG_TO_RAD( 40), 0},
    };
    
    
    
    SubscribeToTags({{
      EngineToGameTag::RobotCompletedAction,
      EngineToGameTag::RobotObservedObject,
      EngineToGameTag::RobotDeletedObject,
      EngineToGameTag::ObjectMoved
    }});

    SubscribeToTags({
      GameToEngineTag::ClearAllObjects
    });
  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorFactoryTest::IsRunnable(const Robot& robot, double currentTime_sec) const
  {
    return _testResult == FactoryTestErrorCode::UNKNOWN;
  }
  
  Result BehaviorFactoryTest::InitInternal(Robot& robot, double currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    _interrupted = false;
    
    robot.GetActionList().Cancel();
    
    // Go to the appropriate state
    InitState(robot);
    
    return lastResult;
  } // Init()

  
  
  void BehaviorFactoryTest::InitState(const Robot& robot)
  {
    // Move robot motors to expected positions
    // ...
  }

  void BehaviorFactoryTest::EndTest(FactoryTestErrorCode errCode)
  {
    // Send test result out and make this behavior stop running
    _testResult = errCode;
    if (_testResult == FactoryTestErrorCode::SUCCESS) {
      PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.TestComplete", "PASS");
    } else {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.TestComplete", "FAIL (code %d)", _testResult);
    }
  }
  
  
  IBehavior::Status BehaviorFactoryTest::UpdateInternal(Robot& robot, double currentTime_sec)
  {
    #define END_TEST(ERRCODE) EndTest(ERRCODE); return Status::Failure;
    
    // Check to see if we had any problems with any handlers
    if(_lastHandlerResult != RESULT_OK) {
      PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.HandlerFailure",
                        "Event handler failed, returning Status::FAILURE.");
      return Status::Failure;
    }
    
    if(_interrupted) {
      return Status::Complete;
    }

    UpdateStateName();
   
    if (_isActing) {
      return Status::Running;
    }
    
    switch(_currentState)
    {
      case State::StartOnCharger:
      {
        // Check that robot is on charger
        if (!robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOnCharger", "");
          END_TEST(FactoryTestErrorCode::CHARGER_UNDETECTED);
        }
        
        DriveStraightAction *driveAction = new DriveStraightAction(robot, 200, 100);
        StartActing(robot, driveAction );
        _currentState = State::DriveToSlot;
        break;
      }
        
      case State::DriveToSlot:
      {
        if (!robot.IsOnCliff()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingCliff", "");
          END_TEST(FactoryTestErrorCode::CLIFF_UNDETECTED);
        }
        
        if (robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOffCharger", "");
          END_TEST(FactoryTestErrorCode::STILL_ON_CHARGER);
        }
        
        // Set pose to expected
        // TODO: Create a function that's shared by LocalizeToObject and LocalizeToMat that does this?
        robot.SetNewPose(_cliffDetectPose);
        
        
        // Go to camera calibration pose
        StartActing(robot, new DriveToPoseAction(robot, _camCalibPose, _motionProfile) );
        _currentState = State::GotoCalibrationPose;
        break;
      }
        
      case State::GotoCalibrationPose:
      {
        // Check that robot is in correct pose
        if (!robot.GetPose().IsSameAs(_camCalibPose, _kRobotPoseSamenessDistThresh_mm, _kRbotPoseSamenessAngleThresh_rad)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingInCalibPose",
                              "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f",
                              robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y(),
                              robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _camCalibPose.GetTranslation().x(),
                              _camCalibPose.GetTranslation().y(),
                              _camCalibPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          END_TEST(FactoryTestErrorCode::NOT_IN_CALIBRATION_POSE);
        }
        
        _camCalibPoseIndex = 0;
        _currentState = State::TakeCalibrationImages;
        break;
      }
        
      case State::TakeCalibrationImages:
      {
        if (_camCalibPoseIndex >= _camCalibPanAndTiltAngles.size()) {
          _currentState = State::ComputeCameraCalibration;
          break;
        }

        if (!robot.GetMoveComponent().IsMoving()) {
          
          // TODO: Take and save picture. In another state?
          // ...
          
          // PanAndTilt to next pose for viewing calibration pattern
          PanAndTiltAction *ptAction = new PanAndTiltAction(robot,
                                                            _camCalibPanAndTiltAngles[_camCalibPoseIndex].first,
                                                            _camCalibPanAndTiltAngles[_camCalibPoseIndex].second,
                                                            true,
                                                            true);
          ptAction->SetMaxPanSpeed(_motionProfile.pointTurnSpeed_rad_per_sec);
          ptAction->SetPanAccel(_motionProfile.pointTurnAccel_rad_per_sec2);
          StartActing(robot, ptAction);
          ++_camCalibPoseIndex;
        }
        break;
      }
        
      case State::ComputeCameraCalibration:
      {
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.Calibrating", "TODO...");
        // TODO: Do calibration...
        
        
        StartActing(robot, new DriveToPoseAction(robot, _prePickupPose, _motionProfile, false, false));
        _currentState = State::GotoPickupPose;
        break;
      }
        
      case State::GotoPickupPose:
      {
        // Verify that robot is where expected
        if (!robot.GetPose().IsSameAs(_prePickupPose, _kRobotPoseSamenessDistThresh_mm, _kRbotPoseSamenessAngleThresh_rad)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingInPrePickupPose",
                              "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f",
                              robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y(),
                              robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _prePickupPose.GetTranslation().x(),
                              _prePickupPose.GetTranslation().y(),
                              _prePickupPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          END_TEST(FactoryTestErrorCode::NOT_IN_PRE_PICKUP_POSE);
        }
        
        // Verify that block is approximately where expected
        if (!_blockObjectID.IsSet()) {
          PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.ExpectingCubeToExist", "");
          END_TEST(FactoryTestErrorCode::CUBE_NOT_FOUND);
        }

        ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(_blockObjectID);
        Vec3f Tdiff;
        Radians angleDiff;
        if (!oObject->GetPose().IsSameAs_WithAmbiguity(_expectedLightCubePose,
                                                       oObject->GetRotationAmbiguities(),
                                                       oObject->GetSameDistanceTolerance()*0.5f,
                                                       oObject->GetSameAngleTolerance(), true,
                                                       Tdiff, angleDiff)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CubeNotWhereExpected",
                              "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f",
                              oObject->GetPose().GetTranslation().x(),
                              oObject->GetPose().GetTranslation().y(),
                              oObject->GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _expectedLightCubePose.GetTranslation().x(),
                              _expectedLightCubePose.GetTranslation().y(),
                              _expectedLightCubePose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          END_TEST(FactoryTestErrorCode::CUBE_NOT_WHERE_EXPECTED);
        }
        
        // Dock to block
        StartActing(robot, new DriveToPickupObjectAction(robot,
                                                         _blockObjectID,
                                                         _motionProfile));
        _currentState = State::PickingUpBlock;
        break;
      }
      case State::PickingUpBlock:
      {
        // Verify that block is being carried
        if (robot.GetCarryingObject() != _blockObjectID) {
          PRINT_NAMED_WARNING("BehaviorFactory.Update.ExpectedToBeCarryingBlock", "");
          END_TEST(FactoryTestErrorCode::PICKUP_FAILED);
        }
        
        // Put block down
        StartActing(robot, new DriveToPlaceCarriedObjectAction(robot,
                                                               _expectedLightCubePose,
                                                               true,
                                                               _motionProfile,
                                                               false));
        _currentState = State::PlacingBlock;
        break;
      }
      case State::PlacingBlock:
      {
        // Verify that block is where expected
        ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(_blockObjectID);
        Vec3f Tdiff;
        Radians angleDiff;
        if (!oObject->GetPose().IsSameAs_WithAmbiguity(_expectedLightCubePose,
                                                       oObject->GetRotationAmbiguities(),
                                                       oObject->GetSameDistanceTolerance()*0.5f,
                                                       oObject->GetSameAngleTolerance(), true,
                                                       Tdiff, angleDiff)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CubeNotWhereExpectedAfterPlacement",
                              "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f",
                              oObject->GetPose().GetTranslation().x(),
                              oObject->GetPose().GetTranslation().y(),
                              oObject->GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _expectedLightCubePose.GetTranslation().x(),
                              _expectedLightCubePose.GetTranslation().y(),
                              _expectedLightCubePose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          END_TEST(FactoryTestErrorCode::CUBE_NOT_WHERE_EXPECTED);
        }
        
        // Look at charger
        
        break;
      }
        
      default:
        PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.UnknownState",
                          "Reached unknown state %d.", _currentState);
        return Status::Failure;
    }
    
    return Status::Running;
  }

  Result BehaviorFactoryTest::InterruptInternal(Robot& robot, double currentTime_sec)
  {
    _interrupted = true;
    
    return RESULT_OK;
  }
  
  void BehaviorFactoryTest::StopInternal(Robot& robot, double currentTime_sec)
  {
    if(_lastActionTag != static_cast<u32>(ActionConstants::INVALID_TAG)) {
      // Make sure we don't stay in tracking when we leave this action
      // TODO: this will cancel any action we were doing. Cancel all tracking actions?
      robot.GetActionList().Cancel(_lastActionTag);
    }
  }
  

  
  void BehaviorFactoryTest::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotCompletedAction:
        _lastHandlerResult = HandleActionCompleted(robot, event.GetData().Get_RobotCompletedAction(),
                                                  event.GetCurrentTime());
        break;
        
      case EngineToGameTag::RobotObservedObject:
        _lastHandlerResult = HandleObservedObject(robot,
                                                  event.GetData().Get_RobotObservedObject(),
                                                  event.GetCurrentTime());
        break;
        
      case EngineToGameTag::RobotDeletedObject:
        _lastHandlerResult = HandleDeletedObject(event.GetData().Get_RobotDeletedObject(),
                                                 event.GetCurrentTime());
        break;
        
      case EngineToGameTag::ObjectMoved:
        _lastHandlerResult = HandleObjectMoved(robot, event.GetData().Get_ObjectMoved());
        break;
        
        
      default:
        PRINT_NAMED_ERROR("BehaviorFactoryTest.HandleWhileRunning.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
        _lastHandlerResult = RESULT_FAIL;
        break;
    }
  }
    
  
  
#pragma mark -
#pragma mark BlockPlay-Specific Methods
  
  void BehaviorFactoryTest::SetCurrState(State s)
  {
    _currentState = s;

    UpdateStateName();

    BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR, "BehaviorFactoryTest.SetState",
                           "set state to '%s'", GetStateName().c_str());
  }

  void BehaviorFactoryTest::UpdateStateName()
  {
    std::string name;
    switch(_currentState)
    {
      case State::StartOnCharger:
        name = "START_ON_CHARGER";
        break;
      case State::DriveToSlot:
        name = "DRIVE_TO_SLOT";
        break;
      case State::GotoCalibrationPose:
        name = "GOTO_CALIB_POSE";
        break;
      case State::TakeCalibrationImages:
        name = "TAKE_CALIB_IMAGES";
        break;
      case State::ComputeCameraCalibration:
        name = "CALIBRATING";
        break;
      case State::GotoPickupPose:
        name = "GOTO_PICKUP_POSE";
        break;
      case State::PickingUpBlock:
        name = "PICKING";
        break;
      case State::PlacingBlock:
        name = "PLACING";
        break;
      default:
        PRINT_NAMED_WARNING("BehaviorFactoryTest.SetCurrState.InvalidState", "");
        break;
    }

    if( _isActing ) {
      name += '*';
    }
    else {
      name += ' ';
    }

    SetStateName(name);
  }


  
  
  Result BehaviorFactoryTest::HandleActionCompleted(Robot& robot,
                                                  const ExternalInterface::RobotCompletedAction &msg,
                                                  double currentTime_sec)
  {
    Result lastResult = RESULT_OK;
    
    if(!IsRunning()) {
      // Ignore action completion messages while not running
      return lastResult;
    }
    
    
    if (msg.idTag != _lastActionTag) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR, "BehaviorFactoryTest.HandleActionCompleted.OtherAction",
                             "finished action id=%d, but only care about id %d",
                             msg.idTag,
                             _lastActionTag);
      return lastResult;
    }

    if( _actionResultCallback ) {
      if( _actionResultCallback(msg.result) ) {
        return lastResult;
      }
    }
    
    switch(_currentState)
    {

      case State::DriveToSlot:
      case State::GotoCalibrationPose:
      case State::TakeCalibrationImages:
      case State::ComputeCameraCalibration:
      case State::GotoPickupPose:
        _isActing = false;
        break;

      case State::PickingUpBlock:
      {
        ++_attemptCounter;
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PICKUP_OBJECT_LOW:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                     "BehaviorFactoryTest.HandleActionCompleted.PickupSuccessful",
                                     "");


              
              // We're done picking up the block.
              //SetCurrState(State::TrackingFace);
              _isActing = false;
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              
              PRINT_NAMED_ERROR("BehaviorFactoryTest.PickingUp.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");
              
              // We failed to pick up or place the last block, try again, and check if we need to roll or not
              //SetCurrState(State::InspectingBlock);
              //_holdUntilTime = currentTime_sec + _timetoInspectBlock;
              _isActing = false;
              break;

            default:
              // Simply ignore other action completions?
              break;
              
          } // switch(actionType)
        } else if( msg.result == ActionResult::FAILURE_RETRY ) {

          // // This isn't foolproof, but use lift height to check if this failure occurred during pickup
          // // verification.
          // if (robot.GetLiftHeight() > LIFT_HEIGHT_HIGHDOCK) {
          //   PlayAnimation(robot, "Demo_OCD_PickUp_Fail");
          // } // else {
          //   // don't make sad emote if we just drove to the wrong position
          //   _isActing = false;
          //   // }

          BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                 "BehaviorFactoryTest.HandleActionCompleted.PickupFailure",
                                 "failed pickup with %s, trying again",
                                 EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));

          switch(msg.completionInfo.Get_objectInteractionCompleted().result)
          {
            case ObjectInteractionResult::INCOMPLETE:
            case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE: {
              _isActing = false;
              break;
            }

            default: {
              // Simultaneously lower lift and play failure animation
              MoveLiftToHeightAction* lowerLift = new MoveLiftToHeightAction(robot, 
              MoveLiftToHeightAction::Preset::LOW_DOCK);
              lowerLift->SetDuration(0.25); // Lower it fast: frustrated
              StartActing(robot, new CompoundActionParallel(robot, {
//                new PlayAnimationAction(robot, "ID_rollBlock_fail_01"),
                lowerLift
              }));
            }
          }

//          SetCurrState(State::InspectingBlock);
//          _holdUntilTime = currentTime_sec + _timetoInspectBlock;


        } else {
            BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                   "BehaviorFactoryTest.HandleActionCompleted.PickupFailure",
                                   "failed pickup with %s, searching",
                                   EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));
//            _missingBlockFoundState = State::InspectingBlock;
//            SetCurrState(State::SearchingForMissingBlock);
            _isActing = false;
        }
        
        break;
      } // case PickingUpBlock
        
      case State::PlacingBlock:
      {
        ++_attemptCounter;
        if(msg.result == ActionResult::SUCCESS) {
          switch(msg.actionType) {
              
            case RobotActionType::PLACE_OBJECT_HIGH:
              BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                     "BehaviorFactoryTest.HandleActionCompleted.PlacementSuccessful",
                                     "Placed object %d",
                                     _blockObjectID.GetValue());
              
//              SetCurrState();
              _attemptCounter = 0;
              break;
              
            case RobotActionType::PICK_AND_PLACE_INCOMPLETE:
              PRINT_NAMED_ERROR("BehaviorFactoryTest.Placing.PICK_AND_PLACE_INCOMPLETE", "THIS ACTUALLY HAPPENS?");
              
              // We failed to place the last block, try again?
              SetCurrState(State::PlacingBlock);
              _isActing = false;
              break;
              
            default:
              // Simply ignore other action completions?
              break;
          }
        } else {

          switch(msg.completionInfo.Get_objectInteractionCompleted().result)
          {
            case ObjectInteractionResult::INCOMPLETE:
            case ObjectInteractionResult::DID_NOT_REACH_PREACTION_POSE:
            {
              // TODO:(bn) "soft fail" sound here?
              BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                     "BehaviorFactoryTest.HandleActionCompleted.PlacementFailure",
                                     "pre-dock fail, trying again");
              SetCurrState(State::PlacingBlock);
              _isActing = false;
              break;
            }

            case ObjectInteractionResult::VISUAL_VERIFICATION_FAILED:
            {
              BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                     "BehaviorFactoryTest.HandleActionCompleted.PlacementFailure",
                                     "failed to visually verify before placement docking, searching for cube");

              // search for the block, but hold a bit first in case we see it
//              _missingBlockFoundState = State::PlacingBlock;
              _holdUntilTime = currentTime_sec + 0.75f;
//              SetCurrState(State::SearchingForMissingBlock);
              _isActing = false;
              break;
            }
              
            default:
            {
              BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR,
                                     "BehaviorFactoryTest.HandleActionCompleted.PlacementFailure",
                                     "dock fail with %s, backing up to try again",
                                     EnumToString(msg.completionInfo.Get_objectInteractionCompleted().result));
              
              _blockObjectID.UnSet();

/*
              const float failureBackupDist = 70.0f;
              const float failureBackupSpeed = 80.0f;
              
              // back up and drop the block, then re-init to start over
              StartActing(robot,
                          new CompoundActionSequential(robot, {
                              new DriveStraightAction(robot, -failureBackupDist, -failureBackupSpeed),
                              new PlaceObjectOnGroundAction(robot)}),
                              [this,&robot](ActionResult ret){
                                _isActing = false;
                                InitState(robot);
                                return true;
                              }
                          );
 */
              break;
            }
          } // switch(objectInteractionResult)
          
        }
        break;
      } // case PlacingBlock

      default:
        PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleActionCompleted.UnexpectedAction", "Action type %d completed during %s", msg.actionType, GetStateName().c_str());
        break;
    } // switch(_currentState)

    return lastResult;
  } // HandleActionCompleted()



  Result BehaviorFactoryTest::HandleObservedObject(Robot& robot,
                                                   const ExternalInterface::RobotObservedObject &msg,
                                                   double currentTime_sec)
  {

    ObjectID objectID = msg.objectID;
    const ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(objectID);
    
    // Check if this is the markerless object for the cliff
    if (oObject->GetFamily() == ObjectFamily::MarkerlessObject) {
    }
    
    
    switch(oObject->GetType()) {
      case ObjectType::ProxObstacle:
        if (!_cliffObjectID.IsSet() || _cliffObjectID == objectID) {
          _cliffObjectID = objectID;
          return RESULT_OK;
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedProxObstacle", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          END_TEST_IN_HANDLER(FactoryTestErrorCode::UNEXPECTED_OBSERVED_OBJECT);
        }
        break;
        
      case ObjectType::Block_LIGHTCUBE1:
        if (!_blockObjectID.IsSet() || _blockObjectID == objectID) {
          _blockObjectID = objectID;
          return RESULT_OK;
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedBlock", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          END_TEST_IN_HANDLER(FactoryTestErrorCode::UNEXPECTED_OBSERVED_OBJECT);
        }
        break;
        
      case ObjectType::Charger_Basic:
        if (!_chargerObjectID.IsSet() || _chargerObjectID == objectID) {
          _chargerObjectID = objectID;
          return RESULT_OK;
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedCharger", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          END_TEST_IN_HANDLER(FactoryTestErrorCode::UNEXPECTED_OBSERVED_OBJECT);
        }
        break;
        
      default:
        PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedObjectType", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
        END_TEST_IN_HANDLER(FactoryTestErrorCode::UNEXPECTED_OBSERVED_OBJECT);
    }
    
    return RESULT_OK;
  }

  Result BehaviorFactoryTest::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg, double currentTime_sec)
  {
    // remove the object if we knew about it
    ObjectID objectID;
    objectID = msg.objectID;
    
// ...
    return RESULT_OK;
  }
  
  

  Result BehaviorFactoryTest::HandleObjectMoved(const Robot& robot, const ObjectMoved &msg)
  {
    ObjectID objectID;
    objectID = msg.objectID;

    
    // Check if tapped when docking?
    // ...
    
    return RESULT_OK;
  }  

  void BehaviorFactoryTest::StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback)
  {
    _lastActionTag = action->GetTag();
    _actionResultCallback = callback;
    robot.GetActionList().QueueActionNow(action);
    _isActing = true;
  }


  
} // namespace Cozmo
} // namespace Anki
