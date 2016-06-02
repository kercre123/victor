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
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/cozmo/shared/cozmoConfig.h"


#define TEST_CHARGER_CONNECT 0

// Whether or not to end the test before the actual block pickup
#define SKIP_BLOCK_PICKUP 0

// Set to 1 if you want the test to actually be able to write
// new camera calibration, calibration images, and test results to flash.
#define ENABLE_NVSTORAGE_WRITES 1

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

  static const Rectangle<s32> firstCalibImageROI(55, 0, 210, 90);
  
  BehaviorFactoryTest::BehaviorFactoryTest(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(FactoryTestState::InitRobot)
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
    _motionProfile.isCustom = true;

    /*
    _camCalibPanAndTiltAngles = {{0,               0},
                                 {0,               DEG_TO_RAD(20)},
                                 {DEG_TO_RAD(-90), 0},
                                 {DEG_TO_RAD(-40), 0},
                                 {DEG_TO_RAD( 45), 0},
     */
    
     // Fixture targets were smaller and lower than spec
     _camCalibPanAndTiltAngles = {{0,               0},
                                 {0,               DEG_TO_RAD(25)},
                                 {DEG_TO_RAD(-90), DEG_TO_RAD(-8)},
                                 {DEG_TO_RAD(-40), DEG_TO_RAD(-8)},
                                 {DEG_TO_RAD( 45), DEG_TO_RAD(-8)},
    };
    
    
    SubscribeToTags({{
      EngineToGameTag::RobotCompletedAction,
      EngineToGameTag::RobotObservedObject,
      EngineToGameTag::RobotDeletedObject,
      EngineToGameTag::ObjectMoved,
      EngineToGameTag::CameraCalibration,
      EngineToGameTag::RobotStopped,
      EngineToGameTag::RobotPickedUp,
      EngineToGameTag::MotorCalibration,
      EngineToGameTag::ObjectAvailable,
      EngineToGameTag::ObjectConnectionState
    }});

  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorFactoryTest::IsRunnableInternal(const Robot& robot) const
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
    _toolCodeImagesStored = false;
    
    robot.GetActionList().Cancel();
 
    // Set known poses
    _cliffDetectPose = Pose3d(0, Z_AXIS_3D(), {50, 0, 0}, &robot.GetPose().FindOrigin());
    _camCalibPose = Pose3d(0, Z_AXIS_3D(), {0, 0, 0}, &robot.GetPose().FindOrigin());
    _prePickupPose = Pose3d( DEG_TO_RAD(90), Z_AXIS_3D(), {-50, 100, 0}, &robot.GetPose().FindOrigin());
    _expectedLightCubePose = Pose3d(0, Z_AXIS_3D(), {-50, 250, 0}, &robot.GetPose().FindOrigin());
    _expectedChargerPose = Pose3d(0, Z_AXIS_3D(), {-300, 200, 0}, &robot.GetPose().FindOrigin());

    // Mute volume
    auto audioClient = robot.GetRobotAudioClient();
    if (audioClient) {
      audioClient->SetRobotVolume(0);
    }
    
    // Set blind docking mode
    ExternalInterface::SetDebugConsoleVarMessage dockMethodMsg;
    dockMethodMsg.varName = "PickupDockingMethod";
    dockMethodMsg.tryValue = "0";  // Blind docking
    robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetDebugConsoleVarMessage>(std::move(dockMethodMsg));

    // Disable driving animations
    ExternalInterface::SetDebugConsoleVarMessage driveAnimsMsg;
    driveAnimsMsg.varName = "EnableDrivingAnimations";
    driveAnimsMsg.tryValue = "false";
    robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::SetDebugConsoleVarMessage>(std::move(driveAnimsMsg));
    
    // Disable reactionary behaviors
    robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::EnableReactionaryBehaviors>(false);
    
    // Disable keep face alive animation
    robot.GetAnimationStreamer().SetParam(Anki::Cozmo::LiveIdleAnimationParameter::EnableKeepFaceAlive, 0);

    // Only enable vision modes we actually need
    // NOTE: we do not (yet) restore vision modes afterwards!
    robot.GetVisionComponent().EnableMode(VisionMode::Idle, true); // first, turn everything off
    robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);
    
    
    // NOTE: Playpen fixutre will do the memory wipe since WipeAll reboots the robot
    /*
    if (ENABLE_NVSTORAGE_WRITES) {
      // Erase all entries in flash
      robot.GetNVStorageComponent().WipeAll(true,
                                            [this,&robot](NVStorage::NVResult res){
                                              if (res != NVStorage::NVResult::NV_OKAY) {
                                                PRINT_NAMED_WARNING("BehaviorFactoryTest.WipeAll.Failed",
                                                                    "Result: %s", EnumToString(res));
                                              } else {
                                                PRINT_NAMED_INFO("BehaviorFactoryTest.WipeAll.Succeeded", "");
                                              }
                                            });
    }
     */

    _stateTransitionTimestamps.resize(16);
    SetCurrState(FactoryTestState::InitRobot);

    return lastResult;
  } // Init()

  

  // Print result and display lights on robot
  void BehaviorFactoryTest::PrintAndLightResult(Robot& robot, FactoryTestResultCode res)
  {
    // Backpack lights
    static const size_t NUM_LIGHTS = (size_t)LEDId::NUM_BACKPACK_LEDS;
    static const std::array<u32,NUM_LIGHTS> pass_onColor{{NamedColors::BLACK,NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> pass_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> pass_onPeriod_ms{{1000,1000,1000,1000,1000}};
    static const std::array<u32,NUM_LIGHTS> pass_offPeriod_ms{{100,100,100,100,100}};
    static const std::array<u32,NUM_LIGHTS> pass_transitionOnPeriod_ms{{450,450,450,450,450}};
    static const std::array<u32,NUM_LIGHTS> pass_transitionOffPeriod_ms{{450,450,450,450,450}};
    
    static const std::array<u32,NUM_LIGHTS> fail_onColor{{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> fail_offColor{{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}};
    static const std::array<u32,NUM_LIGHTS> fail_onPeriod_ms{{500,500,500,500,500}};
    static const std::array<u32,NUM_LIGHTS> fail_offPeriod_ms{{500,500,500,500,500}};
    static const std::array<u32,NUM_LIGHTS> fail_transitionOnPeriod_ms{};
    static const std::array<u32,NUM_LIGHTS> fail_transitionOffPeriod_ms{};
    
    if (res == FactoryTestResultCode::SUCCESS) {
      PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.TestPASSED", "");
      robot.SetBackpackLights(pass_onColor, pass_offColor,
                              pass_onPeriod_ms, pass_offPeriod_ms,
                              pass_transitionOnPeriod_ms, pass_transitionOffPeriod_ms);
    } else {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.TestFAILED",
                          "%s (code %d, state %s)",
                          EnumToString(res), static_cast<u8>(res), GetStateName().c_str());
      robot.SetBackpackLights(fail_onColor, fail_offColor,
                              fail_onPeriod_ms, fail_offPeriod_ms,
                              fail_transitionOnPeriod_ms, fail_transitionOffPeriod_ms);
    }
    
  };
  
  void BehaviorFactoryTest::EndTest(Robot& robot, FactoryTestResultCode resCode)
  {
    // Send test result out and make this behavior stop running
    if (_testResult == FactoryTestResultCode::UNKNOWN) {
      _testResult = resCode;
      
      // Generate result struct
      ExternalInterface::FactoryTestResult testResMsg;
      FactoryTestResultEntry &testRes = testResMsg.resultEntry;
      testRes.result = resCode;
      testRes.engineSHA1 = 0;   // TODO
      testRes.utcTime = time(0);
      
      // Mark end time
      _stateTransitionTimestamps[testRes.timestamps.size()-1] = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      std::copy(_stateTransitionTimestamps.begin(), _stateTransitionTimestamps.begin() + testRes.timestamps.size(), testRes.timestamps.begin());
      
      testRes.stationID = 0;   // TODO: How to get this?

      
      if (ENABLE_NVSTORAGE_WRITES) {
        u8 buf[testRes.Size()];
        size_t numBytes = testRes.Pack(buf, sizeof(buf));
        
        // Store test result to robot flash
        robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_PlaypenTestResults, buf, numBytes,
                                            [this,&robot,&testResMsg](NVStorage::NVResult res){
                                              if (res != NVStorage::NVResult::NV_OKAY) {
                                                PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.WriteFailed",
                                                                    "WriteResult: %s (Original test result: %s)",
                                                                    EnumToString(res), EnumToString(testResMsg.resultEntry.result));
                                                _testResult = FactoryTestResultCode::TEST_RESULT_WRITE_FAILED;
                                              }
                                              PrintAndLightResult(robot,_testResult);
                                              testResMsg.resultEntry.result = _testResult;
                                              robot.Broadcast( ExternalInterface::MessageEngineToGame( ExternalInterface::FactoryTestResult(std::move(testResMsg))));
                                            });
      } else {
        PrintAndLightResult(robot,_testResult);
        robot.Broadcast( ExternalInterface::MessageEngineToGame( ExternalInterface::FactoryTestResult(std::move(testResMsg))));
      }
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
    
    // Check for pickup
    if (robot.IsPickedUp()) {
      END_TEST(FactoryTestResultCode::ROBOT_PICKUP);
    }
    
    if (IsActing()) {
      return Status::Running;
    }

    
    switch(_currentState)
    {
      case FactoryTestState::InitRobot:
      {
        // Too much stuff is changing now.
        // Maybe put this back later.
        /*
        // Check for mismatched CLAD
        if (robot.HasMismatchedCLAD()) {
          END_TEST(FactoryTestResultCode::MISMATCHED_CLAD);
        }
         */
        
        if (TEST_CHARGER_CONNECT) {
          // Check if charger is discovered
          robot.BroadcastAvailableObjects(true);
        }
        
        // Set fake calibration if not already set so that we can actually run
        // calibration from images.
        if (!robot.GetVisionComponent().IsCameraCalibrationSet()) {
          PRINT_NAMED_INFO("BehaviorFactoryTest.Update.SettingFakeCalib", "");
          Vision::CameraCalibration fakeCalib(240, 320,
                                              290, 290,
                                              160, 120);
          robot.GetVisionComponent().SetCameraCalibration(fakeCalib);
        }
        
        robot.GetVisionComponent().ClearCalibrationImages();
        
        // Move lift to correct height and head to correct angle
        CompoundActionParallel* headAndLiftAction = new CompoundActionParallel(robot, {
          new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK),
          new MoveHeadToAngleAction(robot, 0),
        });
        StartActing(robot, headAndLiftAction,
            [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
              if (result != ActionResult::SUCCESS) {
                EndTest(robot, FactoryTestResultCode::INIT_LIFT_HEIGHT_FAILED);
                return false;
              }
              SetCurrState(FactoryTestState::ChargerAndIMUCheck);
              return true;
            });
        
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
          
          // Take photo for checking starting pose
          robot.GetVisionComponent().StoreNextImageForCameraCalibration(firstCalibImageROI);
        } else if (currentTime_sec > _holdUntilTime) {
          f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
          if(angleChange > _kIMUDriftAngleThreshDeg) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.IMUDrift",
                                "Angle change of %f deg detected in %f seconds",
                                angleChange, _kIMUDriftDetectPeriod_sec);
            END_TEST(FactoryTestResultCode::IMU_DRIFTING);
          }
          
          // Confirm that the first calib photo was taken
          if(robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() == 0) {
            END_TEST(FactoryTestResultCode::FIRST_CALIB_IMAGE_NOT_TAKEN);
          }
          
          if (TEST_CHARGER_CONNECT) {
            // Verify that charger was discovered
            if (!_chargerAvailable) {
              PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingChargerAvailable","");
              END_TEST(FactoryTestResultCode::CHARGER_UNAVAILABLE);
            }
            
            // Connect to charger
            const std::unordered_set<FactoryID> connectToIDs = {_kChargerFactoryID};
            robot.ConnectToBlocks(connectToIDs);
          }
          
          // Drive off charger
          StartActing(robot, new DriveStraightAction(robot, 250, 100),
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.3;
                        return true;
                      });
          SetCurrState(FactoryTestState::DriveToSlot);
        }
        break;
      }
      case FactoryTestState::DriveToSlot:
      {
        if (!robot.IsCliffSensorOn()) {
          if (currentTime_sec > _holdUntilTime) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingCliff", "");
            END_TEST(FactoryTestResultCode::CLIFF_UNDETECTED);
          }
          break;
        }
        
        // Verify robot is not still on charger
        if (robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOffCharger", "");
          END_TEST(FactoryTestResultCode::STILL_ON_CHARGER);
        }
        
        // Verify robot is connected to charger
        if (TEST_CHARGER_CONNECT && !_chargerConnected) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingChargerConnected","");
          END_TEST(FactoryTestResultCode::CHARGER_UNCONNECTED);
        }
        
        
        // Set pose to expected
        // TODO: Create a function that's shared by LocalizeToObject and LocalizeToMat that does this?
        robot.SetNewPose(_cliffDetectPose);
        
        f32 distToCamCalibPose = _camCalibPose.GetTranslation().x() - robot.GetPose().GetTranslation().x();
        DriveStraightAction* action = new DriveStraightAction(robot, distToCamCalibPose, 100);
        action->SetAccel(1000);
        action->SetDecel(1000);
        
        // Go to camera calibration pose
        StartActing(robot, action,
                    [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                      if (result != ActionResult::SUCCESS) {
                        EndTest(robot, FactoryTestResultCode::GOTO_CALIB_POSE_ACTION_FAILED);
                      } else {
                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.0f;
                      }
                      return true;
                    });
        SetCurrState(FactoryTestState::GotoCalibrationPose);
        break;
      }
        
      case FactoryTestState::GotoCalibrationPose:
      {
        // Check that robot is in correct pose
        if (!robot.GetPose().IsSameAs(_camCalibPose, _kRobotPoseSamenessDistThresh_mm, _kRobotPoseSamenessAngleThresh_rad)) {
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
        
        _camCalibPoseIndex = 0;
        SetCurrState(FactoryTestState::TakeCalibrationImages);
        break;
      }
        
      case FactoryTestState::TakeCalibrationImages:
      {
        // All calibration images acquired. (NOTE: the "+1" is for the initial image taken on the charger,
        // which has no stored pose in _camCalibPanAndTiltAngles)
        // Start computing calibration.
        if (robot.GetVisionComponent().GetNumStoredCameraCalibrationImages() >= _camCalibPanAndTiltAngles.size() + 1) {
          SetCurrState(FactoryTestState::ComputeCameraCalibration);
          break;
        }

        // Move to fixed calibration pose and take image
        if (!robot.GetMoveComponent().IsMoving()) {
          // NOTE: "-1" is due to initial image take on the charger, which has no stored pose in _camCalibPoseIndex
          if (robot.GetVisionComponent().GetNumStoredCameraCalibrationImages()-1 == _camCalibPoseIndex) {
            PanAndTiltAction *ptAction = new PanAndTiltAction(robot,
                                                              _camCalibPanAndTiltAngles[_camCalibPoseIndex].first,
                                                              _camCalibPanAndTiltAngles[_camCalibPoseIndex].second,
                                                              true,
                                                              true);
            ptAction->SetMaxPanSpeed(_motionProfile.pointTurnSpeed_rad_per_sec);
            ptAction->SetPanAccel(_motionProfile.pointTurnAccel_rad_per_sec2);
            ptAction->SetMoveEyes(false);
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
        // Move head down to line up for readToolCode.
        // Hopefully this reduces some readToolCode errors
        MoveHeadToAngleAction* headAction = new MoveHeadToAngleAction(robot, MIN_HEAD_ANGLE);
        DriveStraightAction* backupAction = new DriveStraightAction(robot, -20.0, -100.f);
        backupAction->SetAccel(1000);
        backupAction->SetDecel(1000);
        CompoundActionParallel* compoundAction = new CompoundActionParallel(robot, {headAction, backupAction});
        StartActing(robot, compoundAction);
        
        
        // Start calibration computation
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.StartingCalibration",
                         "%zu images", robot.GetVisionComponent().GetNumStoredCameraCalibrationImages());
        robot.GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, true);
        _calibrationReceived = false;
        _holdUntilTime = currentTime_sec + _kCalibrationTimeout_sec;
        SetCurrState(FactoryTestState::WaitForCameraCalibration);
        break;
      }
      case FactoryTestState::WaitForCameraCalibration:
      {
        if (_calibrationReceived) {
          
          ReadToolCodeAction* toolCodeAction = new ReadToolCodeAction(robot, false);
          
          // Read lift tool code
          StartActing(robot, toolCodeAction,
                      [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                        
                        // Save tool code images to robot (whether it succeeded to read code or not)
                        _toolCodeImagesStored = false;
                        if (robot.GetVisionComponent().WriteToolCodeImagesToRobot([this,&robot](std::vector<NVStorage::NVResult>& results){
                                                                                    // Clear tool code images from VisionSystem
                                                                                    robot.GetVisionComponent().ClearToolCodeImages();
                                                                                    
                                                                                    u32 numFailures = 0;
                                                                                    for (auto r : results) {
                                                                                      if (r != NVStorage::NVResult::NV_OKAY) {
                                                                                        ++numFailures;
                                                                                      }
                                                                                    }
                                                                                    
                                                                                    if (numFailures > 0) {
                                                                                      PRINT_NAMED_WARNING("BehaviorFactoryTest.WriteToolCodeImages.FAILED", "%d failures", numFailures);
                                                                                      EndTest(robot, FactoryTestResultCode::TOOL_CODE_IMAGES_WRITE_FAILED);
                                                                                    } else {
                                                                                      PRINT_NAMED_INFO("BehaviorFactoryTest.WriteToolCodeImages.SUCCESS", "");
                                                                                      _toolCodeImagesStored = true;
                                                                                    }
                                                                                  }) != RESULT_OK)
                        {
                          EndTest(robot, FactoryTestResultCode::TOOL_CODE_IMAGES_WRITE_FAILED);
                          return false;
                        }
                        
                        // Check result of tool code read
                        if (result != ActionResult::SUCCESS) {
                          EndTest(robot, FactoryTestResultCode::READ_TOOL_CODE_FAILED);
                          return false;
                        }
                        
                        const ToolCodeInfo &info = completionInfo.Get_readToolCodeCompleted().info;
                        PRINT_NAMED_INFO("BehaviorFactoryTest.RecvdToolCodeInfo.Info",
                                         "Code: %s, Expected L: (%f, %f), R: (%f, %f), Observed L: (%f, %f), R: (%f, %f)",
                                         EnumToString(info.code),
                                         info.expectedCalibDotLeft_x, info.expectedCalibDotLeft_y,
                                         info.expectedCalibDotRight_x, info.expectedCalibDotRight_y,
                                         info.observedCalibDotLeft_x, info.observedCalibDotLeft_y,
                                         info.observedCalibDotRight_x, info.observedCalibDotRight_y);
                        
                        // Store results to nvStorage
                        if (ENABLE_NVSTORAGE_WRITES) {
                          u8 buf[info.Size()];
                          size_t numBytes = info.Pack(buf, sizeof(buf));
                          if (!robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_ToolCodeInfo, buf, numBytes,
                                                              [this,&robot](NVStorage::NVResult res) {
                                                                if (res != NVStorage::NVResult::NV_OKAY) {
                                                                  EndTest(robot, FactoryTestResultCode::TOOL_CODE_WRITE_FAILED);
                                                                } else {
                                                                  PRINT_NAMED_INFO("BehaviorFactoryTest.WriteToolCode.Success", "");
                                                                }
                                                              }))
                          {
                            EndTest(robot, FactoryTestResultCode::TOOL_CODE_WRITE_FAILED);
                            return false;
                          }
                        }
                        
                        
                        // Verify tool code data is in range
                        static const f32 pixelDistThresh_x = 20.f;
                        static const f32 pixelDistThresh_y = 40.f;
                        f32 distL_x = std::fabsf(info.expectedCalibDotLeft_x - info.observedCalibDotLeft_x);
                        f32 distL_y = std::fabsf(info.expectedCalibDotLeft_y - info.observedCalibDotLeft_y);
                        f32 distR_x = std::fabsf(info.expectedCalibDotRight_x - info.observedCalibDotRight_x);
                        f32 distR_y = std::fabsf(info.expectedCalibDotRight_y - info.observedCalibDotRight_y);
                        
                        if (distL_x > pixelDistThresh_x || distL_y > pixelDistThresh_y || distR_x > pixelDistThresh_x || distR_y > pixelDistThresh_y) {
                          EndTest(robot, FactoryTestResultCode::TOOL_CODE_POSITIONS_OOR);
                          return false;
                        }
                        
                        return true;
                      });
      
          SetCurrState(FactoryTestState::ReadLiftToolCode);
        } else if (currentTime_sec > _holdUntilTime) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CalibrationTimedout", "");
          END_TEST(FactoryTestResultCode::CALIBRATION_TIMED_OUT);
        }
        break;
      }
      case FactoryTestState::ReadLiftToolCode:
      {
        // Wait for it to finish backing up
        if (robot.GetMoveComponent().IsMoving()) {
          break;
        }
        
        
        // Goto pose where block is visible
        DriveToPoseAction* action = new DriveToPoseAction(robot, _prePickupPose);
        //action->SetMotionProfile(_motionProfile);
        StartActing(robot, action,
                    [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                      // NOTE: This result check should be ok, but in sim the action often doesn't result in
                      // the robot being exactly where it's supposed to be so the action itself sometimes fails.
                      // When robot path following is improved (particularly in sim) this physical check can be removed.
                      if (result != ActionResult::SUCCESS && robot.IsPhysical()) {
                        PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.GotoPrePickupPoseFailed",
                                            "actual: (x,y,deg) = %f, %f, %f; expected: %f %f %f",
                                            robot.GetPose().GetTranslation().x(),
                                            robot.GetPose().GetTranslation().y(),
                                            robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                                            _prePickupPose.GetTranslation().x(),
                                            _prePickupPose.GetTranslation().y(),
                                            _prePickupPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
                        EndTest(robot, FactoryTestResultCode::GOTO_PRE_PICKUP_POSE_ACTION_FAILED);
                      } else {
                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.0f;
                      }
                      return true;
                    });
        
        SetCurrState(FactoryTestState::GotoPickupPose);
        break;
      }
      case FactoryTestState::GotoPickupPose:
      {
        // Verify that robot is where expected
        if (!robot.GetPose().IsSameAs(_prePickupPose, _kRobotPoseSamenessDistThresh_mm, _kRobotPoseSamenessAngleThresh_rad)) {
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
                                                       _kExpectedCubePoseDistThresh_mm,
                                                       _kExpectedCubePoseAngleThresh_rad,
                                                       true,
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
        if (SKIP_BLOCK_PICKUP) {
          if (ENABLE_NVSTORAGE_WRITES) {
            if (!_toolCodeImagesStored) {
              break;
            }
          }
          
          // %%%%%%%%%%%  END OF TEST %%%%%%%%%%%%%%%%%%
          EndTest(robot, FactoryTestResultCode::SUCCESS);
          return Status::Complete;
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        }
        
        auto pickupCallback = [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
          if (result == ActionResult::SUCCESS && robot.GetCarryingObject() == _blockObjectID) {
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
        //DriveToPickupObjectAction* action = new DriveToPickupObjectAction(robot, _blockObjectID);
        //action->SetMotionProfile(_motionProfile);
        PickupObjectAction* action = new PickupObjectAction(robot, _blockObjectID);
        StartActing(robot,
                    action,
                    pickupCallback);
        SetCurrState(FactoryTestState::PickingUpBlock);
        break;
      }
      case FactoryTestState::PickingUpBlock:
      {
        
        auto placementCallback = [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
          if (result == ActionResult::SUCCESS && !robot.IsCarryingObject()) {
            PRINT_NAMED_INFO("BehaviorFactoryTest.placementCallback.Success", "");
          } else {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.placementCallback.Failed", "");
            EndTest(robot, FactoryTestResultCode::PLACEMENT_FAILED);
          }
          return true;
        };
        
        // Put block down
        PlaceObjectOnGroundAction* action = new PlaceObjectOnGroundAction(robot);
        //action->SetMotionProfile(_motionProfile);
        StartActing(robot,
                    action,
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

        // %%%%%%%%%%%  END OF TEST %%%%%%%%%%%%%%%%%%
        EndTest(robot, FactoryTestResultCode::SUCCESS);
        return Status::Complete;
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        
        // Look at charger
        StartActing(robot, new TurnTowardsPoseAction(robot, _expectedChargerPose, DEG_TO_RAD(180)),
                    [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
                      if (result != ActionResult::SUCCESS) {
                        EndTest(robot, FactoryTestResultCode::GOTO_PRE_MOUNT_CHARGER_POSE_ACTION_FAILED);
                      } else {
                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.0f;
                      }
                      return true;
                    });
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
        
        auto chargerCallback = [this,&robot](const ActionResult& result, const ActionCompletedUnion& completionInfo){
          if (result == ActionResult::SUCCESS && robot.IsOnCharger()) {
            EndTest(robot, FactoryTestResultCode::SUCCESS);
          } else {
            EndTest(robot, FactoryTestResultCode::CHARGER_DOCK_FAILED);
          }
          return true;
        };
        
        DriveToAndMountChargerAction* action = new DriveToAndMountChargerAction(robot, _chargerObjectID);
        //action->SetMotionProfile(_motionProfile);
        StartActing(robot,
                    action,
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
  
  void BehaviorFactoryTest::StopInternal(Robot& robot)
  {
    // Cancel all actions
    for (const auto& tag : _actionCallbackMap) {
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
        
      case EngineToGameTag::RobotStopped:
        _lastHandlerResult = HandleRobotStopped(robot, event.GetData().Get_RobotStopped());
        break;

      case EngineToGameTag::RobotPickedUp:
        _lastHandlerResult = HandleRobotPickedUp(robot, event.GetData().Get_RobotPickedUp());
        break;
        
      case EngineToGameTag::MotorCalibration:
        _lastHandlerResult = HandleMotorCalibration(robot, event.GetData().Get_MotorCalibration());
        break;
        
      case EngineToGameTag::ObjectAvailable:
        _lastHandlerResult = HandleObjectAvailable(robot, event.GetData().Get_ObjectAvailable());
        break;
        
      case EngineToGameTag::ObjectConnectionState:
        _lastHandlerResult = HandleObjectConnectionState(robot, event.GetData().Get_ObjectConnectionState());
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

    _stateTransitionTimestamps[static_cast<u32>(s)] = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    
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
        _actionCallbackMap[msg.idTag](msg.result, msg.completionInfo);
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
      case ObjectType::Block_LIGHTCUBE2:
      case ObjectType::Block_LIGHTCUBE3:
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
    
    if (ENABLE_NVSTORAGE_WRITES) {
      
      // Save calibration to robot
      CameraCalibration calibMsg;
      calibMsg.focalLength_x = camCalib.GetFocalLength_x();
      calibMsg.focalLength_y = camCalib.GetFocalLength_y();
      calibMsg.center_x = camCalib.GetCenter_x();
      calibMsg.center_y = camCalib.GetCenter_y();
      calibMsg.skew = camCalib.GetSkew();
      calibMsg.nrows = camCalib.GetNrows();
      calibMsg.ncols = camCalib.GetNcols();
      
      ASSERT_NAMED_EVENT(camCalib.GetDisortionCoeffs().size() <= calibMsg.distCoeffs.size(),
                         "BehaviorFactoryTest.HandleCameraCalibration.TooManyDistCoeffs",
                         "%zu > %zu", camCalib.GetDisortionCoeffs().size(),
                         calibMsg.distCoeffs.size());
      std::copy(camCalib.GetDisortionCoeffs().begin(), camCalib.GetDisortionCoeffs().end(),
                calibMsg.distCoeffs.begin());

      u8 buf[calibMsg.Size()];
      size_t numBytes = calibMsg.Pack(buf, sizeof(buf));
      
      robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_CameraCalib, buf, numBytes,
                                          [this,&robot](NVStorage::NVResult res) {
                                            if (res == NVStorage::NVResult::NV_OKAY) {
                                              PRINT_NAMED_INFO("BehaviorFactoryTest.WriteCameraCalib.SUCCESS", "");
                                            } else {
                                              EndTest(robot, FactoryTestResultCode::CAMERA_CALIB_WRITE_FAILED);
                                            }
                                          });
      
      // Save calibration images to robot
      Result writeImagesResult = robot.GetVisionComponent().WriteCalibrationImagesToRobot(
                                                                                          
        [this,&robot](std::vector<NVStorage::NVResult>& results){
          
          // Clear calibration images from VisionSystem
          robot.GetVisionComponent().ClearCalibrationImages();
          
          u32 numFailures = 0;
          for (auto r : results) {
            if (r != NVStorage::NVResult::NV_OKAY) {
              ++numFailures;
            }
          }
          
          if (numFailures > 0) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.WriteCalibImages.FAILED", "%d failures", numFailures);
            EndTest(robot, FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED);
          } else {
            PRINT_NAMED_INFO("BehaviorFactoryTest.WriteCalibImages.SUCCESS", "");
          }
        }
      );
      
      if (writeImagesResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorFactoryTest.WriteCalibImages.SendFAILED", "");
        EndTest(robot, FactoryTestResultCode::CALIB_IMAGES_SEND_FAILED);
      }
      
      // Save computed camera pose when robot was on charger
      Result writePoseResult = robot.GetVisionComponent().WriteCalibrationPoseToRobot(0,
        [this,&robot](NVStorage::NVResult res)
        {
          if (res == NVStorage::NVResult::NV_OKAY) {
            PRINT_NAMED_INFO("BehaviorFactoryTest.WriteCalibPose.SUCCESS", "");
          } else {
            EndTest(robot, FactoryTestResultCode::CALIB_POSE_WRITE_FAILED);
          }
        }
      );
      if (writePoseResult != RESULT_OK) {
        PRINT_NAMED_WARNING("BehaviorFactoryTest.WriteCalibPose.SendFAILED", "");
        EndTest(robot, FactoryTestResultCode::CALIB_POSE_SEND_FAILED);
      }
      
    }

    _calibrationReceived = true;
    return RESULT_OK;
  }
  
  Result BehaviorFactoryTest::HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg)
  {
    // This is expected when driving to slot
    if (_currentState == FactoryTestState::DriveToSlot) {
      _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 0.06f;
    } else {
      EndTest(robot, FactoryTestResultCode::CLIFF_UNEXPECTED);
    }
    
    return RESULT_OK;
  }

  Result BehaviorFactoryTest::HandleRobotPickedUp(Robot& robot, const ExternalInterface::RobotPickedUp &msg)
  {
    EndTest(robot, FactoryTestResultCode::ROBOT_PICKUP);
    return RESULT_OK;
  }

  
  Result BehaviorFactoryTest::HandleMotorCalibration(Robot& robot, const MotorCalibration &msg)
  {
    // This should never happen during the test!
    EndTest(robot, FactoryTestResultCode::MOTOR_CALIB_UNEXPECTED);
    return RESULT_OK;
  }
  
  Result BehaviorFactoryTest::HandleObjectAvailable(Robot& robot, const ExternalInterface::ObjectAvailable &msg) {
    if (msg.factory_id == _kChargerFactoryID) {
      _chargerAvailable = true;
    }
    return RESULT_OK;
  }
  
  Result BehaviorFactoryTest::HandleObjectConnectionState(Robot& robot, const ObjectConnectionState &msg)
  {
    if (msg.factoryID == _kChargerFactoryID && msg.connected) {
      _chargerConnected = true;
    }
    return RESULT_OK;
  }
  
  
  void BehaviorFactoryTest::StartActing(Robot& robot, IActionRunner* action, ActionResultCallback callback, u32 actionCallbackTag)
  {
    if (actionCallbackTag == static_cast<u32>(ActionConstants::INVALID_TAG)) {
      actionCallbackTag = action->GetTag();
    }
    
    assert(_actionCallbackMap.count(actionCallbackTag) == 0);
    
    if (robot.GetActionList().QueueActionNow(action) == RESULT_OK) {
      _actionCallbackMap[actionCallbackTag] = callback;
    } else {
      PRINT_NAMED_WARNING("BehaviorFactory.StartActing.QueueActionFailed", "Action type %s", EnumToString(action->GetType()));
      EndTest(robot, FactoryTestResultCode::QUEUE_ACTION_FAILED);
    }
  }


  
} // namespace Cozmo
} // namespace Anki
