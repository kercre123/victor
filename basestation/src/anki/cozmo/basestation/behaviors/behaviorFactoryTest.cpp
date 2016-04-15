/**
 * File: behaviorFactoryTest.h
 *
 * Author: Kevin Yoon
 * Date:   03/18/2016
 *
 * Description: Functional test fixture behavior
 *
 *              Init conditions:
 *                - Cozmo is on starting charger 1 of the test fixture
 *
 *              Behavior:
 *                - Drive straight off charger until cliff detected
 *                - Backup to pose for camera calibration
 *                - Turn to take pictures of all calibration targets and calibrate
 *                - Go to pickup block
 *                - Place block back down
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

#define END_TEST_IN_HANDLER(ERRCODE) EndTest(robot, ERRCODE); return RESULT_OK;

namespace Anki {
namespace Cozmo {

  ////////////////////////////
  // Static consts
  ////////////////////////////
  
  // Rotation ambiguities for observed blocks.
  // We only care that the block is upright.
  const std::vector<RotationMatrix3d> BehaviorFactoryTest::_kBlockRotationAmbiguities({
    RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
    RotationMatrix3d({0,1,0,  1,0,0,  0,0,1})
  });


  
  BehaviorFactoryTest::BehaviorFactoryTest(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _cliffDetectPose(0, Z_AXIS_3D(), {50, 0, 0}, &robot.GetPose().FindOrigin())
  , _camCalibPose(0, Z_AXIS_3D(), {0, 0, 0}, &robot.GetPose().FindOrigin())
  , _prePickupPose( DEG_TO_RAD(90), Z_AXIS_3D(), {-50, 150, 0}, &robot.GetPose().FindOrigin())
  , _expectedLightCubePose(0, Z_AXIS_3D(), {-50, 300, 0}, &robot.GetPose().FindOrigin())
  , _expectedChargerPose(0, Z_AXIS_3D(), {-300, 200, 0}, &robot.GetPose().FindOrigin())
  , _currentState(FactoryTestState::RequestCalibrationImages)
  , _lastHandlerResult(RESULT_OK)
  , _testResult(FactoryTestResultCode::UNKNOWN)
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
      EngineToGameTag::ObjectMoved,
      EngineToGameTag::CameraCalibration
    }});

    SubscribeToTags({
      GameToEngineTag::ClearAllObjects
    });
  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorFactoryTest::IsRunnable(const Robot& robot) const
  {
    return _testResult == FactoryTestResultCode::UNKNOWN;
  }
  
  Result BehaviorFactoryTest::InitInternal(Robot& robot)
  {
    const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    Result lastResult = RESULT_OK;
    
    _testResult = FactoryTestResultCode::UNKNOWN;
    _actionCallbackMap.clear();
    _holdUntilTime = -1;
    _watchdogTriggerTime = currentTime_sec + _kWatchdogTimeout;
    
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

  void BehaviorFactoryTest::EndTest(Robot& robot, FactoryTestResultCode resCode)
  {
    // Send test result out and make this behavior stop running
    if (_testResult == FactoryTestResultCode::UNKNOWN) {
      _testResult = resCode;
      if (_testResult == FactoryTestResultCode::SUCCESS) {
        PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.TestComplete", "PASS");
      } else {
        PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.TestComplete",
                            "FAIL: %s (code %d, state %s)",
                            EnumToString(_testResult), (int)_testResult, GetStateName().c_str());
      }
      
      robot.Broadcast( ExternalInterface::MessageEngineToGame( ExternalInterface::FactoryTestResult(_testResult)));
    } else {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.TestAlreadyComplete",
                          "Existing result %s (new result %s)",
                          EnumToString(_testResult), EnumToString(resCode) );
    }
  }
  
  
  IBehavior::Status BehaviorFactoryTest::UpdateInternal(Robot& robot)
  {
    #define END_TEST(ERRCODE) EndTest(robot, ERRCODE); return Status::Failure;

    const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    // If test is complete
    if (_testResult != FactoryTestResultCode::UNKNOWN) {
      PRINT_NAMED_WARNING("BehaviorFactory.Update.TestAlreadyComplete", "Result %s (code %d)", EnumToString(_testResult), (int)_testResult);
      return Status::Complete;
    }
    
    // Check to see if we had any problems with any handlers
    if(_lastHandlerResult != RESULT_OK) {
      PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.HandlerFailure",
                        "Event handler failed, returning Status::FAILURE.");
      return Status::Failure;
    }
    
    UpdateStateName();
   
    // Check watchdog timer
    if (currentTime_sec > _watchdogTriggerTime) {
      END_TEST(FactoryTestResultCode::TEST_TIMED_OUT);
    }
    
    if (IsActing()) {
      return Status::Running;
    }
    
    auto gotoPoseCallback = [this,&robot](ActionResult ret){
      if (ret != ActionResult::SUCCESS) {
        EndTest(robot, FactoryTestResultCode::GOTO_POSE_ACTION_FAILED);
      } else {
        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.0f;
      }
      return true;
    };
    
    
    switch(_currentState)
    {
      case FactoryTestState::RequestCalibrationImages:
      {
        // Set calibration if not already set
        if (!robot.GetVisionComponent().IsCameraCalibrationSet()) {
          PRINT_NAMED_INFO("BehaviorFactoryTest.Update.SettingFakeCalib", "");
          Vision::CameraCalibration fakeCalib(240, 320,
                                              290, 290,
                                              160, 120);
          robot.GetVisionComponent().SetCameraCalibration(fakeCalib);
        }
        
        
        PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.RequestCalibrationImages", "TODO");
        SetCurrState(FactoryTestState::ChargerAndIMUCheck);
        break;
      }
      case FactoryTestState::ChargerAndIMUCheck:
      {
        // Check that robot is on charger
        if (!robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOnCharger", "");
          END_TEST(FactoryTestResultCode::CHARGER_UNDETECTED);
        }
       
        // Check for IMU drift
        if (_holdUntilTime < 0) {
          // Capture initial robot orientation and check if it changes over some period of time
          _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
          _holdUntilTime = currentTime_sec + _kIMUDriftDetectPeriod_sec;
        } else if (currentTime_sec > _holdUntilTime) {
          f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
          if(angleChange > _kIMUDriftAngleThreshDeg) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.IMUDrift",
                                "Angle change of %f deg detected in %f seconds",
                                angleChange, _kIMUDriftDetectPeriod_sec);
            END_TEST(FactoryTestResultCode::IMU_DRIFTING);
          }
          
          // Drive off charger
          DriveStraightAction *driveAction = new DriveStraightAction(robot, 250, 100);
          StartActing(robot, driveAction );
          SetCurrState(FactoryTestState::DriveToSlot);
        }
        break;
      }
      case FactoryTestState::DriveToSlot:
      {
        if (!robot.IsOnCliff()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingCliff", "");
          END_TEST(FactoryTestResultCode::CLIFF_UNDETECTED);
        }
        
        if (robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOffCharger", "");
          END_TEST(FactoryTestResultCode::STILL_ON_CHARGER);
        }
        
        // Set pose to expected
        // TODO: Create a function that's shared by LocalizeToObject and LocalizeToMat that does this?
        robot.SetNewPose(_cliffDetectPose);
        
        
        // Go to camera calibration pose
        StartActing(robot, new DriveToPoseAction(robot, _camCalibPose, _motionProfile) );
        SetCurrState(FactoryTestState::GotoCalibrationPose);
        break;
      }
        
      case FactoryTestState::GotoCalibrationPose:
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
          END_TEST(FactoryTestResultCode::NOT_IN_CALIBRATION_POSE);
        }
        
        
        // Check if all calibration images received from flash.
        // If so, go directly to ComputeCameraCalibration.
        // Otherwise, acquire images
        if (robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() >= _kMinNumberOfCalibrationImagesRequired) {
           SetCurrState(FactoryTestState::ComputeCameraCalibration);
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.InsufficientCalibrationImagesInFlash",
                              "Only %zu images found in flash. Taking pictures now.",
                              robot.GetVisionComponent().GetNumStoredCameraCalibrationImages());
          _camCalibPoseIndex = 0;
          robot.GetVisionComponent().ClearCalibrationImages();
          SetCurrState(FactoryTestState::TakeCalibrationImages);
        }
        break;
      }
        
      case FactoryTestState::TakeCalibrationImages:
      {
        // All calibration images acquired.
        // Start computing calibration.
        if (robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() >= _camCalibPanAndTiltAngles.size()) {
          SetCurrState(FactoryTestState::ComputeCameraCalibration);
          break;
        }

        // Move to fixed calibration pose and take image
        if (!robot.GetMoveComponent().IsMoving()) {
          if (robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() == _camCalibPoseIndex) {
            PanAndTiltAction *ptAction = new PanAndTiltAction(robot,
                                                              _camCalibPanAndTiltAngles[_camCalibPoseIndex].first,
                                                              _camCalibPanAndTiltAngles[_camCalibPoseIndex].second,
                                                              true,
                                                              true);
            ptAction->SetMaxPanSpeed(_motionProfile.pointTurnSpeed_rad_per_sec);
            ptAction->SetPanAccel(_motionProfile.pointTurnAccel_rad_per_sec2);
            StartActing(robot, ptAction);
            ++_camCalibPoseIndex;
          } else {
            robot.GetVisionComponent().StoreNextImageForCameraCalibration();
          }
        }
        break;
      }
        
      case FactoryTestState::ComputeCameraCalibration:
      {
        // Start calibration computation
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.StartingCalibration",
                         "%zu images", robot.GetVisionComponent().GetNumStoredCameraCalibrationImages());
        robot.GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, true);
        _calibrationReceived = false;
        _holdUntilTime = currentTime_sec + 30.f;
        SetCurrState(FactoryTestState::WaitForCameraCalibration);
      }
      case FactoryTestState::WaitForCameraCalibration:
      {
        if (_calibrationReceived) {
          // Goto pose where block is visible
          StartActing(robot, new DriveToPoseAction(robot, _prePickupPose, _motionProfile, false, false), gotoPoseCallback);
          SetCurrState(FactoryTestState::GotoPickupPose);
        } else if (currentTime_sec > _holdUntilTime) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CalibrationTimedout", "");
          END_TEST(FactoryTestResultCode::CALIBRATION_TIMED_OUT);
        }
        break;
      }
      case FactoryTestState::GotoPickupPose:
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
          END_TEST(FactoryTestResultCode::NOT_IN_PRE_PICKUP_POSE);
        }
        
        // Verify that block exists
        if (!_blockObjectID.IsSet()) {
          if (currentTime_sec > _holdUntilTime) {
            PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.ExpectingCubeToExist", "currTime %f", currentTime_sec);
            END_TEST(FactoryTestResultCode::CUBE_NOT_FOUND);
          }
          
          // Waiting for block to exist. Should be seeing it very soon!
          break;
        }

        // Verify that block is approximately where expected
        ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(_blockObjectID);
        Vec3f Tdiff;
        Radians angleDiff;
        if (!oObject->GetPose().IsSameAs_WithAmbiguity(_expectedLightCubePose,
                                                       _kBlockRotationAmbiguities,
                                                       oObject->GetSameDistanceTolerance(),
                                                       oObject->GetSameAngleTolerance()*0.5f, true,
                                                       Tdiff, angleDiff)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CubeNotWhereExpected",
                              "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f; tdiff: %f %f %f; angleDiff (deg): %f",
                              oObject->GetPose().GetTranslation().x(),
                              oObject->GetPose().GetTranslation().y(),
                              oObject->GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _expectedLightCubePose.GetTranslation().x(),
                              _expectedLightCubePose.GetTranslation().y(),
                              _expectedLightCubePose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              Tdiff.x(), Tdiff.y(), Tdiff.z(),
                              angleDiff.getDegrees());
          END_TEST(FactoryTestResultCode::CUBE_NOT_WHERE_EXPECTED);
        }
        
        _actualLightCubePose = oObject->GetPose();
        _attemptCounter = 0;
        SetCurrState(FactoryTestState::StartPickup);
        break;
      }
      case FactoryTestState::StartPickup:
      {
        auto pickupCallback = [this,&robot](ActionResult ret){
          if (ret == ActionResult::SUCCESS && robot.GetCarryingObject() == _blockObjectID) {
            PRINT_NAMED_INFO("BehaviorFactoryTest.pickupCallback.Success", "");
          } else if (_attemptCounter <= _kNumPickupRetries) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.pickupCallback.FailedRetrying", "");
            SetCurrState(FactoryTestState::StartPickup);
          } else {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.pickupCallback.Failed", "");
            EndTest(robot, FactoryTestResultCode::PICKUP_FAILED);
          }
          return true;
        };
       
        // Pickup block
        PRINT_NAMED_INFO("BehaviorFactory.Update.PickingUp", "Attempt %d", _attemptCounter);
        ++_attemptCounter;
        StartActing(robot,
                    new DriveToPickupObjectAction(robot, _blockObjectID, _motionProfile),
                    pickupCallback);
        SetCurrState(FactoryTestState::PickingUpBlock);
        break;
      }
      case FactoryTestState::PickingUpBlock:
      {
        auto placementCallback = [this,&robot](ActionResult ret){
          if (ret == ActionResult::SUCCESS && !robot.IsCarryingObject()) {
            PRINT_NAMED_INFO("BehaviorFactoryTest.placementCallback.Success", "");
          } else {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.placementCallback.Failed", "");
            EndTest(robot, FactoryTestResultCode::PLACEMENT_FAILED);
          }
          return true;
        };
        
        // Put block down
        StartActing(robot,
                    new PlaceObjectOnGroundAtPoseAction(robot, _actualLightCubePose, _motionProfile),
                    placementCallback);
        SetCurrState(FactoryTestState::PlacingBlock);
        break;
      }
      case FactoryTestState::PlacingBlock:
      {
        // Verify that block is where expected
        ObservableObject* oObject = robot.GetBlockWorld().GetObjectByID(_blockObjectID);
        Vec3f Tdiff;
        Radians angleDiff;
        if (!oObject->GetPose().IsSameAs_WithAmbiguity(_actualLightCubePose,
                                                       _kBlockRotationAmbiguities,
                                                       oObject->GetSameDistanceTolerance(),
                                                       oObject->GetSameAngleTolerance()*0.5f, true,
                                                       Tdiff, angleDiff)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CubeNotWhereExpectedAfterPlacement",
                              "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f",
                              oObject->GetPose().GetTranslation().x(),
                              oObject->GetPose().GetTranslation().y(),
                              oObject->GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _actualLightCubePose.GetTranslation().x(),
                              _actualLightCubePose.GetTranslation().y(),
                              _actualLightCubePose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          END_TEST(FactoryTestResultCode::CUBE_NOT_WHERE_EXPECTED);
        }
        
        // Look at charger
        StartActing(robot, new TurnTowardsPoseAction(robot, _expectedChargerPose, DEG_TO_RAD(180)), gotoPoseCallback );
        SetCurrState(FactoryTestState::DockToCharger);
        break;
      }
      case FactoryTestState::DockToCharger:
      {
        if (!_chargerObjectID.IsSet()) {
          if (currentTime_sec > _holdUntilTime) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectedChargerToExist", "");
            END_TEST(FactoryTestResultCode::CHARGER_NOT_FOUND);
          }
          break;
        }
        
        auto chargerCallback = [this,&robot](ActionResult ret){
          if (ret == ActionResult::SUCCESS && robot.IsOnCharger()) {
            EndTest(robot, FactoryTestResultCode::SUCCESS);
          } else {
            EndTest(robot, FactoryTestResultCode::CHARGER_DOCK_FAILED);
          }
          return true;
        };
        
        StartActing(robot,
                    new DriveToAndMountChargerAction(robot, _chargerObjectID, _motionProfile),
                    chargerCallback);
        break;
      }
      default:
        PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.UnknownState",
                          "Reached unknown state %d.", (u32)_currentState);
        return Status::Failure;
    }
    
    return Status::Running;
  }

  Result BehaviorFactoryTest::InterruptInternal(Robot& robot)
  {
    StopInternal(robot);
    return RESULT_OK;
  }
  
  void BehaviorFactoryTest::StopInternal(Robot& robot)
  {
    // Cancel all actions
    for (auto tag : _actionCallbackMap) {
      robot.GetActionList().Cancel(tag.first);
    }
    _actionCallbackMap.clear();
    
    if (_testResult == FactoryTestResultCode::UNKNOWN) {
      EndTest(robot, FactoryTestResultCode::TEST_CANCELLED);
    }
  }

  
  void BehaviorFactoryTest::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotCompletedAction:
        _lastHandlerResult = HandleActionCompleted(robot, event.GetData().Get_RobotCompletedAction());
        break;
        
      case EngineToGameTag::RobotObservedObject:
        _lastHandlerResult = HandleObservedObject(robot,
                                                  event.GetData().Get_RobotObservedObject());
        break;
        
      case EngineToGameTag::RobotDeletedObject:
        _lastHandlerResult = HandleDeletedObject(event.GetData().Get_RobotDeletedObject());
        break;
        
      case EngineToGameTag::ObjectMoved:
        _lastHandlerResult = HandleObjectMoved(robot, event.GetData().Get_ObjectMoved());
        break;
        
      case EngineToGameTag::CameraCalibration:
        _lastHandlerResult = HandleCameraCalibration(robot, event.GetData().Get_CameraCalibration());
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
  
  void BehaviorFactoryTest::SetCurrState(FactoryTestState s)
  {
    // Update watchdog
    if (s != _currentState) {
      _watchdogTriggerTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _kWatchdogTimeout;
    }
    
    _currentState = s;

    UpdateStateName();

    BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR, "BehaviorFactoryTest.SetState",
                           "set state to '%s'", GetStateName().c_str());
  }

  void BehaviorFactoryTest::UpdateStateName()
  {
    std::string name = EnumToString(_currentState);
        
    if( IsActing() ) {
      name += '*';
    }
    else {
      name += ' ';
    }

    SetStateName(name);
  }


  
  
  Result BehaviorFactoryTest::HandleActionCompleted(Robot& robot,
                                                  const ExternalInterface::RobotCompletedAction &msg)
  {
    Result lastResult = RESULT_OK;
    
    if(!IsRunning()) {
      // Ignore action completion messages while not running
      return lastResult;
    }
    
    if (_actionCallbackMap.count(msg.idTag) == 0) {
      BEHAVIOR_VERBOSE_PRINT(DEBUG_FACTORY_TEST_BEHAVIOR, "BehaviorFactoryTest.HandleActionCompleted.OtherAction",
                             "finished action id=%d type=%s but don't care",
                             msg.idTag, EnumToString(msg.actionType));
      return lastResult;
    }
    
    if (_actionCallbackMap.count(msg.idTag) != 0) {
      if (_actionCallbackMap[msg.idTag]) {
        _actionCallbackMap[msg.idTag](msg.result);
      }
      _actionCallbackMap.erase(msg.idTag);
    }
    
    return lastResult;
  } // HandleActionCompleted()



  Result BehaviorFactoryTest::HandleObservedObject(Robot& robot,
                                                   const ExternalInterface::RobotObservedObject &msg)
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
          END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
        }
        break;
        
      case ObjectType::Block_LIGHTCUBE1:
        if (!_blockObjectID.IsSet() || _blockObjectID == objectID) {
          _blockObjectID = objectID;
          return RESULT_OK;
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedBlock", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
        }
        break;
        
      case ObjectType::Charger_Basic:
        if (!_chargerObjectID.IsSet() || _chargerObjectID == objectID) {
          _chargerObjectID = objectID;
          return RESULT_OK;
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedCharger", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
          END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
        }
        break;
        
      default:
        PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedObjectType", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
        END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
    }
    
    return RESULT_OK;
  }

  Result BehaviorFactoryTest::HandleDeletedObject(const ExternalInterface::RobotDeletedObject &msg)
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
  
  Result BehaviorFactoryTest::HandleCameraCalibration(Anki::Cozmo::Robot &robot, const Anki::Cozmo::CameraCalibration &msg)
  {
    #define CHECK_OOR(value, min, max) (value < min || value > max)

    // Check if calibration values are sane
    if (CHECK_OOR(msg.focalLength_x, 250, 310) ||
        CHECK_OOR(msg.focalLength_y, 250, 310) ||
        CHECK_OOR(msg.center_x, 130, 190) ||
        CHECK_OOR(msg.center_y, 90, 150) ||
        msg.nrows != 240 ||
        msg.ncols != 320)
    {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleCameraCalibration.OOR",
                          "focalLength (%f, %f), center (%f, %f)",
                          msg.focalLength_x, msg.focalLength_y, msg.center_x, msg.center_y);
      END_TEST_IN_HANDLER(FactoryTestResultCode::CALIBRATION_VALUES_OOR);
    }

    Vision::CameraCalibration camCalib(msg.nrows, msg.ncols,
                                       msg.focalLength_x, msg.focalLength_y,
                                       msg.center_x, msg.center_y,
                                       msg.skew);
    
    // Set camera calibration
    PRINT_NAMED_INFO("BehaviorFactoryTest.HandleCameraCalibration.SettingNewCalibration", "");
    robot.GetVisionComponent().SetCameraCalibration(camCalib);
    
    // TODO: Save calibration to robot
    // ...

    
    robot.GetVisionComponent().ClearCalibrationImages();
    _calibrationReceived = true;
    return RESULT_OK;
  }

  void BehaviorFactoryTest::StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback)
  {
    assert(_actionCallbackMap.count(action->GetTag()) == 0);

    if (robot.GetActionList().QueueActionNow(action) == RESULT_OK) {
      _actionCallbackMap[action->GetTag()] = callback;
    } else {
      PRINT_NAMED_WARNING("BehaviorFactory.StartActing.QueueActionFailed", "Action type %s", EnumToString(action->GetType()));
      EndTest(robot, FactoryTestResultCode::QUEUE_ACTION_FAILED);
    }
  }


  
} // namespace Cozmo
} // namespace Anki
