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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorFactoryTest.h"

#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/activeObjectHelpers.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/components/carryingComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/types/fwTestMessages.h"

#include "util/console/consoleInterface.h"

#include <iomanip>

// If you are running from webots only set this to one
#define RUNNING_FROM_WEBOTS 0

#define DEBUG_FACTORY_TEST_BEHAVIOR 1

#define ENABLE_FACTORY_TEST 0

#define END_TEST_IN_HANDLER(ERRCODE) EndTest(robot, ERRCODE); return RESULT_OK;

namespace Anki {
namespace Cozmo {
namespace{
static const char* kBehaviorTestName = "Behavior factory test";
}

  ////////////////////////////
  // Console vars
  ////////////////////////////
  
  // Does the test without doing any of the writing to NVStorage
  CONSOLE_VAR(bool, kBFT_EnableNVStorageWrites,   "BehaviorFactoryTest",  true);
  
  // Should be set to true once pilot build begins.
  // EP3 robots in the office however will not have this data so skip this check.
  CONSOLE_VAR(bool, kBFT_CheckPrevFixtureResults, "BehaviorFactoryTest",  false);
  
  // Save logs on device
  CONSOLE_VAR(bool,  kBFT_SaveLogsOnDevice,       "BehaviorFactoryTest",  true);

  // Write to NVStorage when test complete (versus throughout the test)
  CONSOLE_VAR(bool,  kBFT_WriteToNVStorageAtEnd, "BehaviorFactoryTest",  true);

  // Read the centroid locations stored on the robot from the prePlaypen test and calculate camera pose
  CONSOLE_VAR(bool,  kBFT_ReadCentroidsFromRobot, "BehaviorFactoryTest",  false);
  
  // Play sound
  // (Sound resource not available in webots tests because they run before game configure)
  CONSOLE_VAR(bool,  kBFT_PlaySound,              "BehaviorFactoryTest",  true);
  
  // Turns off robot wifi at end of test
  CONSOLE_VAR(bool,  kBFT_DisconnectAtEnd,        "BehaviorFactoryTest",  false);
  
  // Just connect normally and don't actually run test to allow things like viewing
  // camera POV via Debug screen
  CONSOLE_VAR(bool,  kBFT_ConnectToRobotOnly,     "BehaviorFactoryTest",  false);
  
  // Whether or not to check for expected robot FW version
  CONSOLE_VAR(bool,  kBFT_CheckFWVersion,         "BehaviorFactoryTest",  true);
  
  // Whether or not to check that battery voltage stays above minimum throughtout test
  CONSOLE_VAR(bool,  kBFT_CheckBattVoltage,       "BehaviorFactoryTest",  true);
  
  // Whether or not to pack the queued nvStorage writes
  CONSOLE_VAR(bool,  kBFT_PackWrites,             "BehaviorFactoryTest",  true);
  
  ////////////////////////////
  // Static consts
  ////////////////////////////
  
  static Pose3d _camCalibPose;
  static Pose3d _prePickupPose;
  static Pose3d _expectedLightCubePose;
  static Pose3d _expectedChargerPose;
  
  // Pan and tilt angles to command robot to look at calibration targets
  static const std::vector<std::pair<f32,f32> > _camCalibPanAndTiltAngles =
   {{0,               DEG_TO_RAD(25)},
    {0,               0},
    {DEG_TO_RAD(-90), DEG_TO_RAD(-8)},
    {DEG_TO_RAD(-40), DEG_TO_RAD(-8)},
    {DEG_TO_RAD( 45), DEG_TO_RAD(-8)},
  };

  static constexpr f32 _kWaitForPrevTestResultsTimeout_sec = 2;
  static constexpr f32 _kMotorCalibrationTimeout_sec = 4.f;
  static constexpr f32 _kCalibrationTimeout_sec = 4.f;
  static constexpr f32 _kRobotPoseSamenessDistThresh_mm = 15;
  static constexpr f32 _kRobotPoseSamenessAngleThresh_rad = DEG_TO_RAD(10);
  static constexpr f32 _kExpectedCubePoseDistThresh_mm = 30;
  static constexpr f32 _kExpectedCubePoseHeightThresh_mm = 10;
  static constexpr f32 _kExpectedCubePoseAngleThresh_rad = DEG_TO_RAD(10);
  static constexpr u32 _kNumPickupRetries = 1;
  static constexpr f32 _kIMUDriftDetectPeriod_sec = 2.f;
  static constexpr f32 _kIMUDriftAngleThreshDeg = 0.2f;
  static constexpr f32 _kMaxRobotAngleChangeDuringBackup_deg = 10.f;
  static constexpr f32 _kMinBattVoltage = 3.6f;
  
  static constexpr u16 _kMaxCliffValueOverDrop = 300;
  static constexpr u16 _kMinCliffValueOnGround = 800;
  
  // Check for these firmware versions if kBFT_CheckFWVersion is true
  // TODO: Update these once we have a good build
  static constexpr u32 _kMinBodyHWVersion = 3;
  static const std::string _kFWVersion = "F1.5.1";
  
  // If no change in behavior state for this long then trigger failure
  static constexpr f32 _kWatchdogTimeout = 20.0f;

  
  // Rotation ambiguities for observed blocks.
  // We only care that the block is upright.
  const RotationAmbiguities _kBlockRotationAmbiguities(true, {
    RotationMatrix3d({1,0,0,  0,1,0,  0,0,1}),
    RotationMatrix3d({0,1,0,  1,0,0,  0,0,1})
  });

  static const Rectangle<s32> firstCalibImageROI(55, 0, 210, 90);
  
  static constexpr u16 _kExposure_ms = 3;
  static constexpr f32 _kGain = 2.f;
  
  static constexpr int _kBackupFromCliffDist_mm = 70;
  
  BehaviorFactoryTest::BehaviorFactoryTest(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _currentState(FactoryTestState::GetPrevTestResults)
  , _lastHandlerResult(RESULT_OK)
  , _stationID(-1)
  , _prevResult(FactoryTestResultCode::UNKNOWN)
  , _prevResultReceived(false)
  , _hasBirthCertificate(false)
  , _writeTestResult(true)
  , _eraseBirthCertificate(false)
  , _testResult(FactoryTestResultCode::UNKNOWN)
  , _robot(robot)
  {

    // start with defaults
    _motionProfile = DEFAULT_PATH_MOTION_PROFILE;

    _motionProfile.speed_mmps = 100.0f;
    _motionProfile.accel_mmps2 = 200.0f;
    _motionProfile.decel_mmps2 = 200.0f;
    _motionProfile.pointTurnSpeed_rad_per_sec = 0.5f * MAX_BODY_ROTATION_SPEED_RAD_PER_SEC;
    _motionProfile.pointTurnAccel_rad_per_sec2 = MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2;
    _motionProfile.pointTurnDecel_rad_per_sec2 = 0.5f * MAX_BODY_ROTATION_ACCEL_RAD_PER_SEC2;
    _motionProfile.dockSpeed_mmps = 80.0f; // slow it down a bit for reliability
    _motionProfile.reverseSpeed_mmps = 80.0f;
    _motionProfile.isCustom = true;
    
    // Subscribe to EngineToGame messages
    SubscribeToTags({{
      EngineToGameTag::RobotObservedObject,
      EngineToGameTag::RobotDeletedLocatedObject,
      EngineToGameTag::ObjectMoved,
      EngineToGameTag::CameraCalibration,
      EngineToGameTag::RobotStopped,
      EngineToGameTag::RobotOffTreadsStateChanged,
      EngineToGameTag::MotorCalibration,
      EngineToGameTag::RobotConnectionResponse
    }});

    
    // Setup robot message handlers
    RobotInterface::MessageHandler *messageHandler = robot.GetContext()->GetRobotManager()->GetMsgHandler();
    RobotID_t robotId = robot.GetID();
    
    
    // Subscribe to RobotToEngine messages
    using localHandlerType = void(BehaviorFactoryTest::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
    // Create a helper lambda for subscribing to a tag with a local handler
    auto doRobotSubscribe = [this, robotId, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
    {
      _signalHandles.push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1)));
    };
    
    
    // bind to specific handlers in the robot class
    doRobotSubscribe(RobotInterface::RobotToEngineTag::factoryTestParam, &BehaviorFactoryTest::HandleFactoryTestParameter);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::activeObjectAvailable, &BehaviorFactoryTest::HandleActiveObjectAvailable);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::pickAndPlaceResult, &BehaviorFactoryTest::HandlePickAndPlaceResult);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::firmwareVersion, &BehaviorFactoryTest::HandleFirmwareVersion);
    doRobotSubscribe(RobotInterface::RobotToEngineTag::factoryFirmwareVersion, &BehaviorFactoryTest::HandleFactoryFirmwareVersion);
    
    _stateTransitionTimestamps.resize(_testResultEntry.timestamps.size());
    
    // If we are running from webots we need to disable block pool immediately since we will have
    // already connected to a robot. The factory app is able to disable block pool before connecting
    if(ENABLE_FACTORY_TEST && RUNNING_FROM_WEBOTS)
    {
      // Disable block pool from auto connecting
      robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::BlockPoolEnabledMessage>(0, false);
    }
  }
  
#pragma mark -
#pragma mark Inherited Virtual Implementations
  
  bool BehaviorFactoryTest::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
  {
    return _testResult == FactoryTestResultCode::UNKNOWN;
  }
  
  Result BehaviorFactoryTest::InitInternal(Robot& robot)
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    Result lastResult = RESULT_OK;
    
    _testResult = FactoryTestResultCode::UNKNOWN;
    _holdUntilTime = -1;
    _watchdogTriggerTime = currentTime_sec + _kWatchdogTimeout;
    _queuedWrites.clear();
    
    robot.GetActionList().Cancel();
 
    // Clear motor calibration flags
    _headCalibrated = false;
    _liftCalibrated = false;
    
    _blockPickedUpReceived = false;
    _robotAngleAtPickup = 0;
    _robotAngleAfterBackup = 0;
    
    // Set known poses
    _camCalibPose = Pose3d(0, Z_AXIS_3D(), {-50, 0, 0}, &robot.GetPose().FindOrigin()); // Relative to cliff
    _prePickupPose = Pose3d( DEG_TO_RAD(90), Z_AXIS_3D(), {-50, 100, 0}, &robot.GetPose().FindOrigin());
    _expectedLightCubePose = Pose3d(0, Z_AXIS_3D(), {-90, 250, 22}, &robot.GetPose().FindOrigin());
    _expectedChargerPose = Pose3d(0, Z_AXIS_3D(), {-300, 200, 0}, &robot.GetPose().FindOrigin());

    _numPlacementAttempts = 0;
    
    _activeObjectAvailable = false;
    
    // Sim robot won't hear from any blocks and also won't send back body firmware version
    if(!robot.IsPhysical())
    {
      _activeObjectAvailable = true;
      _gotHWVersion = true;
    }
    
    // Setup logging to device
    if (kBFT_SaveLogsOnDevice) {
      std::stringstream serialNumString;
      serialNumString << std::hex << robot.GetHeadSerialNumber();
      _factoryTestLogger.StartLog( serialNumString.str(), true, robot.GetContextDataPlatform());
      PRINT_NAMED_INFO("BehaviorFactoryTest.WillLogToDevice",
                       "Log name: %s",
                       _factoryTestLogger.GetLogName().c_str());
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
    robot.GetBehaviorManager().DisableReactionsWithLock(
                                 kBehaviorTestName,
                                 ReactionTriggerHelpers::kAffectAllArray);
    
    // Only enable vision modes we actually need
    // NOTE: we do not (yet) restore vision modes afterwards!
    robot.GetVisionComponent().EnableMode(VisionMode::Idle, true); // first, turn everything off
    robot.GetVisionComponent().EnableMode(VisionMode::DetectingMarkers, true);
    
    // Set and disable auto exposure
    robot.GetVisionComponent().SetAndDisableAutoExposure(_kExposure_ms, _kGain);
    
    // Disable block pool from auto connecting
    robot.GetExternalInterface()->BroadcastToEngine<ExternalInterface::BlockPoolEnabledMessage>(0, false);
    
    // Enable writing factory data
    robot.GetNVStorageComponent().EnableWritingFactory(true);
    
    _stateTransitionTimestamps.resize(_testResultEntry.timestamps.size());
    SetCurrState(FactoryTestState::GetPrevTestResults);

    return lastResult;
  } // Init()

  

  // Print result and display lights on robot
  void BehaviorFactoryTest::PrintAndLightResult(Robot& robot, FactoryTestResultCode res)
  {
    // Backpack lights
    static const BackpackLights passLights = {
      .onColors               = {{NamedColors::BLACK,NamedColors::GREEN,NamedColors::GREEN,NamedColors::GREEN,NamedColors::BLACK}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{1000,1000,1000,1000,1000}},
      .offPeriod_ms           = {{100,100,100,100,100}},
      .transitionOnPeriod_ms  = {{450,450,450,450,450}},
      .transitionOffPeriod_ms = {{450,450,450,450,450}},
      .offset                 = {{0,0,0,0,0}}
    };
    
    static const BackpackLights failLights = {
      .onColors               = {{NamedColors::BLACK,NamedColors::RED,NamedColors::RED,NamedColors::RED,NamedColors::BLACK}},
      .offColors              = {{NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK,NamedColors::BLACK}},
      .onPeriod_ms            = {{500,500,500,500,500}},
      .offPeriod_ms           = {{500,500,500,500,500}},
      .transitionOnPeriod_ms  = {{0,0,0,0,0}},
      .transitionOffPeriod_ms = {{0,0,0,0,0}},
      .offset                 = {{0,0,0,0,0}}
    };
    
    if (res == FactoryTestResultCode::SUCCESS) {
      PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.TestPASSED", "");
      robot.GetBodyLightComponent().SetBackpackLights(passLights);
    } else {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.TestFAILED",
                          "%s (code %d, state %s)",
                          EnumToString(res), static_cast<u8>(res), GetDebugStateName().c_str());
      robot.GetBodyLightComponent().SetBackpackLights(failLights);
    }
    
  };
  
  
  void BehaviorFactoryTest::SendTestResultToGame(Robot& robot, FactoryTestResultCode resCode)
  {
    _testResultEntry.result = resCode;
    
    PrintAndLightResult(robot,resCode);
    robot.Broadcast( ExternalInterface::MessageEngineToGame( FactoryTestResultEntry(_testResultEntry)));

    // Write engine log to log folder if test failed
    if (resCode != FactoryTestResultCode::SUCCESS && !_factoryTestLogger.CopyEngineLog(robot.GetContextDataPlatform())) {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.CopyEngineLogFailed", "");
    }
    _factoryTestLogger.CloseLog();
    
    // Immediately disconnect wifi
    if (kBFT_DisconnectAtEnd) {
      RobotInterface::EnterFactoryTestMode m;
      m.mode = RobotInterface::FactoryTestMode::FTM_Off;
      robot.SendMessage(RobotInterface::EngineToRobot(std::move(m)));
    }
  }
  
  
  
  void BehaviorFactoryTest::QueueWriteToRobot(Robot& robot, NVStorage::NVEntryTag tag, const u8* data, size_t size)
  {
    if (kBFT_EnableNVStorageWrites) {
      
      static const std::map<NVStorage::NVEntryTag, FactoryTestResultCode> _tagToCode = {
        {NVStorage::NVEntryTag::NVEntry_ToolCodeImageLeft,  FactoryTestResultCode::TOOL_CODE_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_ToolCodeImageRight, FactoryTestResultCode::TOOL_CODE_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_ToolCodeInfo,       FactoryTestResultCode::TOOL_CODE_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_IMUInfo,            FactoryTestResultCode::IMU_INFO_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibImage1,        FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibImage2,        FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibImage3,        FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibImage4,        FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibImage5,        FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibImage6,        FactoryTestResultCode::CALIB_IMAGES_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibMetaInfo,      FactoryTestResultCode::CALIB_META_INFO_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CameraCalib,        FactoryTestResultCode::CAMERA_CALIB_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_CalibPose,          FactoryTestResultCode::CALIB_POSE_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_ObservedCubePose,   FactoryTestResultCode::CUBE_POSE_WRITE_FAILED},
        {NVStorage::NVEntryTag::NVEntry_PlaypenTestResults, FactoryTestResultCode::TEST_RESULT_WRITE_FAILED},
      };
      
      FactoryTestResultCode resCode = FactoryTestResultCode::NVSTORAGE_WRITE_FAILED;
      if (_tagToCode.count(tag) > 0) {
        resCode = _tagToCode.at(tag);
      }
      
      // Create callback for the nvStorage write
      auto callback = [this,&robot,tag,resCode](NVStorage::NVResult res) {
        if (res != NVStorage::NVResult::NV_OKAY) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.WriteToRobot.Failed", "Tag: %s", EnumToString(tag));
          
          if (kBFT_WriteToNVStorageAtEnd) {
            _writeFailureCode = resCode;
          } else {
            EndTest(robot, resCode);
          }
          
        } else {
          PRINT_NAMED_INFO("BehaviorFactoryTest.WriteToRobot.Success", "Tag: %s", EnumToString(tag));
        }
      };

      if (kBFT_WriteToNVStorageAtEnd) {
        _queuedWrites.emplace_back(tag, data, size, callback);
      } else {
        robot.GetNVStorageComponent().Write(tag, data, size, callback);
      }

    }
  }
  
  bool BehaviorFactoryTest::SendQueuedWrites(Robot& robot)
  {
    if(kBFT_PackWrites)
    {
      std::vector<u8> packedData;
      PackWrites(_queuedWrites, packedData);
      _queuedWrites.clear();

      if(!robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_FactoryBaseTagWithBCOffset,
                                              packedData.data(), packedData.size(),
                                              [this, &robot](NVStorage::NVResult res) {
                                                const NVStorage::NVEntryTag tag = NVStorage::NVEntryTag::NVEntry_FactoryBaseTagWithBCOffset;
                                                if (res != NVStorage::NVResult::NV_OKAY) {
                                                  PRINT_NAMED_WARNING("BehaviorFactoryTest.WriteToRobot.Failed", "Tag: %s[0x%x]", EnumToString(tag), static_cast<u32>(tag));
                                                  
                                                  if (kBFT_WriteToNVStorageAtEnd) {
                                                    _writeFailureCode = FactoryTestResultCode::NVSTORAGE_WRITE_FAILED;
                                                  } else {
                                                    EndTest(robot, FactoryTestResultCode::NVSTORAGE_WRITE_FAILED);
                                                  }
                                                  
                                                } else {
                                                  PRINT_NAMED_INFO("BehaviorFactoryTest.WriteToRobot.Success", "Tag: %s[0x%x]", EnumToString(tag), static_cast<u32>(tag));
                                                }
                                              }))
      {
        return false;
      }
    }
    else
    {
      // Send queued writes to robot
      for (auto const& entry : _queuedWrites) {
        if (!robot.GetNVStorageComponent().Write(entry._tag, &(entry._data), entry._callback)) {
          _queuedWrites.clear();
          return false;
        }
      }
      _queuedWrites.clear();
    }
    return true;
  }

  void BehaviorFactoryTest::PackWrites(const std::list<WriteEntry>& writes,
                                       std::vector<u8>& packedData)
  {
    // Helper to print the packed data
    auto print = [this](const WriteEntry& e){
      if(DEBUG_FACTORY_TEST_BEHAVIOR)
      {
        std::vector<u8> packedData;
        // Size of the entire entry, size + tag + successor + data + footer
        const u32 size = 4 + 4 + 4 + static_cast<u32>(e._data.size()) + 4;
        const u32 alignedSize = static_cast<u32>(_robot.GetNVStorageComponent().MakeWordAligned(size));
        for(int i = 0; i < sizeof(u32); ++i)
        {
          packedData.push_back(static_cast<u32>(alignedSize) >> i*8 & 0xFF);
        }
        
        // Tag
        for(int i = 0; i < sizeof(e._tag); ++i)
        {
          packedData.push_back(static_cast<u32>(e._tag) >> i*8 & 0xFF);
        }
        
        // Successor (should be 0xFFFFffff)
        for(int i = 0; i < sizeof(u32); ++i)
        {
          packedData.push_back(0xFF);
        }
        
        // Data
        packedData.insert(packedData.end(), e._data.begin(), e._data.end());
        
        // Footer (should be 0)
        for(int i = 0; i < sizeof(u32); ++i)
        {
          packedData.push_back(0);
        }
        
        // Padding
        for(int i = 0; i < sizeof(u8)*(alignedSize-size); ++i)
        {
          packedData.push_back(0);
        }
      
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for(auto i : packedData)
        {
          ss << std::setw(2) << static_cast<u32>(i);
        }
        PRINT_NAMED_DEBUG("BehaviorFactoryTest.PrintPackedData","%s", ss.str().c_str());
      }
    };
  
  
    packedData.clear();
    
    // First we need to break entries with data size > 1024 into multiple entries with sequential tags
    std::list<WriteEntry> fixedWrites;
    for(const auto& entry : writes)
    {
      u32 size = static_cast<u32>(entry._data.size());
      u32 dataCount = 0;
      u32 tagCount = 0;
      if(size > 1024)
      {
        while(size > 1024)
        {
          NVStorage::NVEntryTag tag = static_cast<NVStorage::NVEntryTag>(static_cast<u32>(entry._tag) + tagCount);
          WriteEntry e(tag,
                       entry._data.data() + dataCount*1024,
                       1024, nullptr);
          fixedWrites.push_back(e);
          print(e);
          size -= 1024;
          dataCount++;
          tagCount++;
        }
      }
      
      NVStorage::NVEntryTag tag = static_cast<NVStorage::NVEntryTag>(static_cast<u32>(entry._tag) + tagCount);
      WriteEntry e(tag,
                   entry._data.data() + dataCount*1024,
                   size, entry._callback);
      fixedWrites.push_back(e);
      print(e);
    }
    
    // Once all the entries are <= 1024 bytes in size pack them into the old nvStorage factory format
    // [size, tag, successor, data(+ padding), footer]
    for(const auto& entry : fixedWrites)
    {
      // Size of the entire entry, size + tag + successor + data + footer
      const u32 size = 4 + 4 + 4 + static_cast<u32>(entry._data.size()) + 4;
      const u32 alignedSize = static_cast<u32>(_robot.GetNVStorageComponent().MakeWordAligned(size));
      for(int i = 0; i < sizeof(u32); ++i)
      {
        packedData.push_back(static_cast<u32>(alignedSize) >> i*8 & 0xFF);
      }
      
      // Tag
      for(int i = 0; i < sizeof(entry._tag); ++i)
      {
        packedData.push_back(static_cast<u32>(entry._tag) >> i*8 & 0xFF);
      }
      
      // Successor (should be 0xFFFFffff)
      for(int i = 0; i < sizeof(u32); ++i)
      {
        packedData.push_back(0xFF);
      }
      
      // Data
      packedData.insert(packedData.end(), entry._data.begin(), entry._data.end());
      
      // Padding
      for(int i = 0; i < sizeof(u8)*(alignedSize-size); ++i)
      {
        packedData.push_back(0);
      }
      
      // Footer (should be 0)
      for(int i = 0; i < sizeof(u32); ++i)
      {
        packedData.push_back(0);
      }
    }
  }
  
  void BehaviorFactoryTest::EndTest(Robot& robot, FactoryTestResultCode resCode)
  {
    // Send test result out and make this behavior stop running
    if (_testResult == FactoryTestResultCode::UNKNOWN) {
    
      _testResult = resCode;
      PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.PreWriteResult", "%s", EnumToString(_testResult));
      
      // Mark end time
      _stateTransitionTimestamps[_testResultEntry.timestamps.size()-1] = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      
      // Stop robot actions
      robot.GetActionList().Cancel();
      
      
      // If robot had previously passed, and it passed this time, we don't write anything to the robot
      //
      // Previously passed  | Currently passed | What to do
      // ==========================================================
      //         0          |         0        | Do nothing (No writes on failure)
      //         0          |         1        | Write everything
      //         1          |         0        | Erase everything
      //         1          |         1        | Do nothing
      bool previouslyPassed = _hasBirthCertificate;
      bool currentlyPassed = _testResult == FactoryTestResultCode::SUCCESS;
      bool writeTestData = !previouslyPassed && currentlyPassed;
      _writeTestResult = previouslyPassed != currentlyPassed;
      _eraseBirthCertificate = previouslyPassed && !currentlyPassed;
      PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.PassFailStatus",
                       "prevPassed: %d, currPassed: %d", previouslyPassed, currentlyPassed);
      
      // Fill out result
      _testResultEntry.result = _testResult;
      _testResultEntry.engineSHA1 = 0;   // TODO
      _testResultEntry.utcTime = time(0);
      _testResultEntry.stationID = _stationID;
      std::copy(_stateTransitionTimestamps.begin(), _stateTransitionTimestamps.begin() + _testResultEntry.timestamps.size(), _testResultEntry.timestamps.begin());
      
      u8 buf[_testResultEntry.Size()];
      const size_t numBytes = _testResultEntry.Pack(buf, sizeof(buf));
      QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_PlaypenTestResults, buf, numBytes);
      
      // Sending all queued writes to robot
      _writeFailureCode = FactoryTestResultCode::UNKNOWN;
      if (writeTestData && !SendQueuedWrites(robot))
      {
        _testResult = FactoryTestResultCode::NVSTORAGE_SEND_FAILED;
      }
      else if(_eraseBirthCertificate)
      {
        auto callback = [this](NVStorage::NVResult res){
          if(res != NVStorage::NVResult::NV_OKAY)
          {
            _testResult = FactoryTestResultCode::NVSTORAGE_ERASE_FAILED;
          }
        };
      
        if(!robot.GetNVStorageComponent().WipeFactory(callback))
        {
          _testResult = FactoryTestResultCode::NVSTORAGE_SEND_FAILED;
        }
      }
      
      SetCurrState(FactoryTestState::WaitingForWritesToRobot);
      
    } else {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.EndTest.TestAlreadyComplete",
                          "Existing result %s (new result %s)",
                          EnumToString(_testResult), EnumToString(resCode) );
    }
  }
  
  
  IBehavior::Status BehaviorFactoryTest::UpdateInternal(Robot& robot)
  {
    #define END_TEST(ERRCODE) EndTest(robot, ERRCODE); return Status::Running;
    
    if(!ENABLE_FACTORY_TEST)
    {
      PRINT_NAMED_ERROR("BehaviorFactoryTest.UpdateInternal.NotEnabled",
                        "Factory test is not enabled");
      return Status::Complete;
    }

    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    
    // Check to see if we had any problems with any handlers
    if(_lastHandlerResult != RESULT_OK) {
      PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.HandlerFailure",
                        "Event handler failed, returning Status::FAILURE.");
      return Status::Failure;
    }
    
    UpdateStateName();

    // Possible failure conditions at any point during the test
    if (_testResult == FactoryTestResultCode::UNKNOWN) {
      
      // Check watchdog timer
      if (currentTime_sec > _watchdogTriggerTime) {
        END_TEST(FactoryTestResultCode::TEST_TIMED_OUT);
      }
      
      // Check for pickup
      if (robot.GetOffTreadsState() != OffTreadsState::OnTreads && _currentState > FactoryTestState::ChargerAndIMUCheck) {
        END_TEST(FactoryTestResultCode::ROBOT_PICKUP);
      }
      
    }
    
    // If robot just picked up block then record robot angle
    if (_blockPickedUpReceived) {
      _robotAngleAtPickup = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
      PRINT_NAMED_INFO("BehaviorFactoryTest.Update.BlockPickedUpReceived",
                       "Robot angle: %f deg", _robotAngleAtPickup.getDegrees());
      _blockPickedUpReceived = false;
    }
    
    if (IsActing()) {
      return Status::Running;
    }

    
    switch(_currentState)
    {
      // - - - - - - - - - - - - - - GET PREVIOUS TEST RESULS - - - - - - - - - - - - - - -
      case FactoryTestState::GetPrevTestResults:
      {
        robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_BirthCertificate,
                                           [this,&robot](u8* data, size_t size, NVStorage::NVResult res){
                                             _hasBirthCertificate = (res == NVStorage::NVResult::NV_OKAY);
                                             PRINT_NAMED_INFO("BehaviorFactoryTest.Update.BirthCertificateReceived",
                                                              "%d", _hasBirthCertificate);
                                           });
        
        robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_PlaypenTestResults,
                                           [this,&robot](u8* data, size_t size, NVStorage::NVResult res){
                                             FactoryTestResultEntry entry;
                                             if (res == NVStorage::NVResult::NV_OKAY &&
                                                 NVStorageComponent::MakeWordAligned(entry.Size()) == size) {
                                               entry.Unpack(data, size);
                                               _prevResult = entry.result;
                                             }
                                             PRINT_NAMED_INFO("BehaviorFactoryTest.Update.PrevTestResultReceived",
                                                              "%s (%s)", EnumToString(_prevResult), EnumToString(res));
                                             _prevResultReceived = true;
                                           });
        
        _holdUntilTime = currentTime_sec + _kWaitForPrevTestResultsTimeout_sec;
        SetCurrState(FactoryTestState::InitRobot);
        break;
      }
        
      // - - - - - - - - - - - - - - INIT ROBOT - - - - - - - - - - - - - - -
      case FactoryTestState::InitRobot:
      {
        if (!_prevResultReceived) {
          if (currentTime_sec > _holdUntilTime) {
            END_TEST(FactoryTestResultCode::PREV_TEST_RESULTS_READ_FAILED);
          }
          break;
        }
        
        if(_hasWrongFirmware)
        {
          PRINT_NAMED_ERROR("BehaviorFactoryTest.HandleFirmwareVersion.WrongVersion",
                            "Expected %s FACTORY got %s %s",
                            _kFWVersion.c_str(), _fwVersion.c_str(), _fwBuildType.c_str());
          END_TEST(FactoryTestResultCode::WRONG_FIRMWARE_VERSION);
        }
        
        // Subscribe and request manufacturing information from the robot
        RobotInterface::MessageHandler *messageHandler = robot.GetContext()->GetRobotManager()->GetMsgHandler();
        _signalHandles.push_back(messageHandler->Subscribe(robot.GetID(),
                                                           RobotInterface::RobotToEngineTag::mfgId,
                                                           std::bind(&BehaviorFactoryTest::HandleMfgID, this, std::placeholders::_1)));
        robot.SendMessage(RobotInterface::EngineToRobot{RobotInterface::GetManufacturingInfo{}});
        
        // Too much stuff is changing now.
        // Maybe put this back later.
        /*
        // Check for mismatched CLAD
        if (robot.HasMismatchedCLAD()) {
          END_TEST(FactoryTestResultCode::MISMATCHED_CLAD);
        }
        */
        
        if (robot.IsCliffSensorOn()) {
          END_TEST(FactoryTestResultCode::CLIFF_UNEXPECTED);
        }
        
        // Check pre-playpen fixture test result
        if (kBFT_CheckPrevFixtureResults) {
          robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_PrePlaypenResults,
                                             [this,&robot](u8* data, size_t size, NVStorage::NVResult res){
                                               if (res == NVStorage::NVResult::NV_OKAY) {
                                                 if (data[0] != 0) {
                                                   EndTest(robot, FactoryTestResultCode::ROBOT_FAILED_PREPLAYPEN_TESTS);
                                                 }
                                                 
                                                 // Write data to log on device
                                                 std::vector<u8> dataVec(data, data+size);
                                                 _factoryTestLogger.AddFile("PrePlaypenResult.bin", dataVec);
                                                 
                                               } else {
                                                 EndTest(robot, FactoryTestResultCode::ROBOT_NOT_TESTED);
                                               }
                                             });
          
        }
        
        // Start motor calibration
        robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::StartMotorCalibration(true, true)));
        _holdUntilTime = currentTime_sec + _kMotorCalibrationTimeout_sec;
        
        SetCurrState(FactoryTestState::WaitingForMotorCalibration);
      }
        
      // - - - - - - - - - - - - - - WAITING FOR MOTOR CALIBRATION - - - - - - - - - - - - - - -
      case FactoryTestState::WaitingForMotorCalibration:
      {
        // Check that head and lift are calibrated
        if (!_headCalibrated || !_liftCalibrated) {
          if (_holdUntilTime < currentTime_sec) {
            END_TEST(FactoryTestResultCode::MOTORS_UNCALIBRATED);
          }
          break;
        }
        
        // Check that we received body version
        if(!_gotHWVersion)
        {
          END_TEST(FactoryTestResultCode::NO_BODY_VERSION_MESSAGE);
          break;
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
        
        
        // Is this a debug mode where we're only connecting to the robot?
        if (kBFT_ConnectToRobotOnly) {
          PRINT_NAMED_INFO("BehaviorFactorytest.Update.ConnectToRobotOnly", "");
          return Status::Complete;
        }
        
        
        // Move lift to correct height and head to correct angle
        CompoundActionParallel* headAndLiftAction = new CompoundActionParallel(robot, {
          new MoveLiftToHeightAction(robot, LIFT_HEIGHT_CARRY),
          new MoveHeadToAngleAction(robot, DEG_TO_RAD(2.f)),
        });
        StartActing(headAndLiftAction,
                    [this,&robot](ActionResult result){
                      if (result != ActionResult::SUCCESS) {
                        EndTest(robot, FactoryTestResultCode::INIT_LIFT_OR_HEAD_FAILED);
                        return;
                      }
                      
                      // Capture initial robot orientation and check if it changes over some period of time
                      _startingRobotOrientation = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
                      _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _kIMUDriftDetectPeriod_sec;
                      
                      // Take photo for checking starting pose
                      robot.GetVisionComponent().StoreNextImageForCameraCalibration(firstCalibImageROI);
                      
                      // Play sound
                      if (kBFT_PlaySound) {
                        PlayAnimationAction* soundAction = new PlayAnimationAction(robot, "soundTestAnim");
                        StartActing(soundAction,
                                    [this,&robot](ActionResult result){
                                      if (result != ActionResult::SUCCESS) {
                                        EndTest(robot, FactoryTestResultCode::PLAY_SOUND_FAILED);
                                      }
                                    });
                      }
                      
                      
                      SetCurrState(FactoryTestState::ChargerAndIMUCheck);
                    });
        
        break;
      }

      // - - - - - - - - - - - - - - CHARGER AND IMU CHECK - - - - - - - - - - - - - - -
      case FactoryTestState::ChargerAndIMUCheck:
      {
        // Check that IMU data history has been received
        if (robot.GetVisionComponent().GetImuDataHistory().empty()) {
          END_TEST(FactoryTestResultCode::NO_IMU_DATA);
        }
        
        // Check that robot is on charger
        if (!robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOnCharger", "");
          END_TEST(FactoryTestResultCode::CHARGER_UNDETECTED);
        }
        
        // When drift detection period has expired, and NVStorage has no pending requests in case we're
        // in the middle of pulling down lots of data (like face data) that could slow down other unreliable image data
        // from coming through, proceed to checking the drift amount and whether or not an image was stored
        // for camera calibration. For factory robots, the NVStorage pulldown shouldn't affect this since
        // there's no face data to pull.
        if (currentTime_sec > _holdUntilTime && !robot.GetNVStorageComponent().HasPendingRequests()) {
          f32 angleChange = std::fabsf((robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>() - _startingRobotOrientation).getDegrees());
          
          // Write drift rate to robot
          IMUInfo imuInfo;
          imuInfo.driftRate_degPerSec = angleChange / _kIMUDriftDetectPeriod_sec;
          QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_IMUInfo, (u8*)&imuInfo, sizeof(imuInfo));
          
          // Write drift rate to log
          _factoryTestLogger.Append(imuInfo);
          
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
          
          // Make sure cliff (and pickup) detection is enabled
          robot.SetEnableCliffSensor(true);
        
          // 1) Drive off charger towards slot.
          // 2) Move head down slowly. If head is stiff, hopefully this will catch it
          //    by triggering a motor calibration.
          auto driveAction = new DriveStraightAction(robot, 250, 100);
          auto headAction = new MoveHeadToAngleAction(robot, MAX_HEAD_ANGLE);
          auto liftAction = new MoveLiftToHeightAction(robot, LIFT_HEIGHT_LOWDOCK);
          headAction->SetMaxSpeed(DEG_TO_RAD(20.f));
          CompoundActionParallel* compoundAction = new CompoundActionParallel(robot, {driveAction, headAction, liftAction});
          compoundAction->ShouldEmitCompletionSignal(true);
          
          StartActing(compoundAction,
                      [this,&robot](ActionResult result){
                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 2.0f;
                      });
          SetCurrState(FactoryTestState::DriveToSlot);
        }
        break;
      }
        
      // - - - - - - - - - - - - - - DRIVE TO SLOT - - - - - - - - - - - - - - -
      case FactoryTestState::DriveToSlot:
      {
        if (!robot.IsCliffSensorOn() || robot.GetMoveComponent().IsMoving() ) {
          if (currentTime_sec > _holdUntilTime) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingCliff", "");
            END_TEST(FactoryTestResultCode::CLIFF_UNDETECTED);
          }
          break;
        }
        
        // Record cliff sensor value over drop
        const CliffSensorValue cliffVal(robot.GetCliffDataRaw());
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.CliffOnDrop", "%u", cliffVal.val);
        
        // Write cliff val to log on device
        _factoryTestLogger.AppendCliffValueOnDrop(cliffVal);
        
        // Store results to nvStorage
        u8 buf[cliffVal.Size()];
        size_t numBytes = cliffVal.Pack(buf, sizeof(buf));
        QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_CliffValOnDrop, buf, numBytes);
        
        
        // Check cliff sensor value
        if (robot.GetCliffDataRaw() > _kMaxCliffValueOverDrop) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CliffValueOverDropTooHigh", "Val: %d", robot.GetCliffDataRaw());
          END_TEST(FactoryTestResultCode::CLIFF_VALUE_TOO_HIGH);
        }
        
        // Verify robot is not still on charger
        if (robot.IsOnCharger()) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingOffCharger", "");
          END_TEST(FactoryTestResultCode::STILL_ON_CHARGER);
        }
        
        // Verify that battery voltage exceeds minimum threshold
        if (kBFT_CheckBattVoltage && (robot.GetBatteryVoltage() < _kMinBattVoltage)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.BattTooLow", "%fV", robot.GetBatteryVoltage());
          END_TEST(FactoryTestResultCode::BATTERY_TOO_LOW);
        }
        
        // Force the robot to delocalize to reset pose
        robot.Delocalize(false);
        
        DriveStraightAction* action = new DriveStraightAction(robot, -_kBackupFromCliffDist_mm, 100);
        action->SetAccel(1000);
        action->SetDecel(1000);
        
        // Go to camera calibration pose
        StartActing(action,
                    [this,&robot](ActionResult result){
                      if (result != ActionResult::SUCCESS) {
                        EndTest(robot, FactoryTestResultCode::GOTO_CALIB_POSE_ACTION_FAILED);
                      } else {
                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.0f;
                      }
                    });
        SetCurrState(FactoryTestState::GotoCalibrationPose);
        break;
      }
        
      // - - - - - - - - - - - - - - GOTO CALIBRATION POSE - - - - - - - - - - - - - - -
      case FactoryTestState::GotoCalibrationPose:
      {
        // Record cliff sensor value over ground
        const CliffSensorValue cliffVal(robot.GetCliffDataRaw());
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.CliffOnGround", "%u", cliffVal.val);
        
        // Write cliff val to log on device
        _factoryTestLogger.AppendCliffValueOnGround(cliffVal);
        
        // Store results to nvStorage
        u8 buf[cliffVal.Size()];
        size_t numBytes = cliffVal.Pack(buf, sizeof(buf));
        QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_CliffValOnGround, buf, numBytes);
        
        // Check cliff sensor value
        if (robot.GetCliffDataRaw() < _kMinCliffValueOnGround) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CliffValueOnGroundTooLow", "Val: %d", robot.GetCliffDataRaw());
          END_TEST(FactoryTestResultCode::CLIFF_VALUE_TOO_LOW);
        }
        
        // Update _camCalibPose parent since we delocalized since it was set
        _camCalibPose.SetParent(&robot.GetPose().FindOrigin());
        
        // Check that robot is in correct pose
        if (!robot.GetPose().IsSameAs(_camCalibPose, _kRobotPoseSamenessDistThresh_mm, _kRobotPoseSamenessAngleThresh_rad)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingInCalibPose",
                              "actual: (x,y,z,deg) = %f, %f, %f, %fdeg; expected: %f, %f, %f, %fdeg",
                              robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y(),
                              robot.GetPose().GetTranslation().z(),
                              robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _camCalibPose.GetTranslation().x(),
                              _camCalibPose.GetTranslation().y(),
                              _camCalibPose.GetTranslation().z(),
                              _camCalibPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          END_TEST(FactoryTestResultCode::NOT_IN_CALIBRATION_POSE);
        }
        
        _camCalibPoseIndex = 0;
        SetCurrState(FactoryTestState::TakeCalibrationImages);
        break;
      }
        
      // - - - - - - - - - - - - - - TAKE CALIBRATION IMAGES - - - - - - - - - - - - - - -
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
            StartActing(ptAction);
            ++_camCalibPoseIndex;
          } else {
            robot.GetVisionComponent().StoreNextImageForCameraCalibration();
          }
        }
        break;
      }
        
      // - - - - - - - - - - - - - - COMPUTE CAMERA CALIBRATION - - - - - - - - - - - - - - -
      case FactoryTestState::ComputeCameraCalibration:
      {
        // Update _expectedLightCubePose parent since we delocalized since it was set
        _expectedLightCubePose.SetParent(&robot.GetPose().FindOrigin());
        // Turn towards block
        TurnTowardsPoseAction* turnAction = new TurnTowardsPoseAction(robot, _expectedLightCubePose, M_PI_2_F);
        StartActing(turnAction);

        
        // Start calibration computation
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.StartingCalibration",
                         "%zu images", robot.GetVisionComponent().GetNumStoredCameraCalibrationImages());
        robot.GetVisionComponent().EnableMode(VisionMode::ComputingCalibration, true);
        _calibrationReceived = false;
        _holdUntilTime = currentTime_sec + _kCalibrationTimeout_sec;
        SetCurrState(FactoryTestState::WaitForCameraCalibration);
        break;
      }
        
      // - - - - - - - - - - - - - - WAIT FOR CAMERA CALIBRATION - - - - - - - - - - - - - - -
      case FactoryTestState::WaitForCameraCalibration:
      {
        if (_calibrationReceived) {
        
          // Verify that block exists
          if (!_blockObjectID.IsSet()) {
            if (currentTime_sec > _holdUntilTime) {  // Note: This is using the same holdUntilTime specified in FactoryTestState::ComputeCameraCalibration
              PRINT_NAMED_ERROR("BehaviorFactoryTest.Update.ExpectingCubeToExist", "currTime %f", currentTime_sec);
              END_TEST(FactoryTestResultCode::CUBE_NOT_FOUND);
            }
            
            // Waiting for block to exist. Should be seeing it very soon!
            break;
          }
          
        
          if(kBFT_ReadCentroidsFromRobot)
          {
            robot.GetNVStorageComponent().Read(NVStorage::NVEntryTag::NVEntry_PrePlaypenCentroids,
                                               [this,&robot](u8* data, size_t size, NVStorage::NVResult res){
                                                 if (res == NVStorage::NVResult::NV_OKAY) {
                                                   
                                                   ExternalInterface::RobotCompletedFactoryDotTest msg;
                                                   msg.success = true;
                                                   
                                                   // TODO:
                                                   // Populate msg.dotCenX/Y_pix and headAngle from data (need to get format of data from nathan)
                                                   
                                                   const Quad2f obsQuad(Point2f(msg.dotCenX_pix[0], msg.dotCenY_pix[0]),
                                                                        Point2f(msg.dotCenX_pix[1], msg.dotCenY_pix[1]),
                                                                        Point2f(msg.dotCenX_pix[2], msg.dotCenY_pix[2]),
                                                                        Point2f(msg.dotCenX_pix[3], msg.dotCenY_pix[3]));
                                                   
                                                   Pose3d pose;
                                                   if(robot.GetVisionComponent().ComputeCameraPoseVsIdeal(obsQuad, pose) != RESULT_OK)
                                                   {
                                                     msg.didComputePose = false;
                                                     _factoryTestLogger.Append(msg);
                                                     EndTest(robot, FactoryTestResultCode::COMPUTE_CAM_POSE_FAILED);
                                                   }
                                                   else
                                                   {
                                                     msg.didComputePose = true;
                                                     msg.camPoseX_mm = pose.GetTranslation().x();
                                                     msg.camPoseY_mm = pose.GetTranslation().y();
                                                     msg.camPoseZ_mm = pose.GetTranslation().z();
                                                     
                                                     msg.camPoseRoll_rad  = pose.GetRotation().GetAngleAroundZaxis().ToFloat();
                                                     msg.camPosePitch_rad = pose.GetRotation().GetAngleAroundXaxis().ToFloat();
                                                     msg.camPoseYaw_rad   = pose.GetRotation().GetAngleAroundYaxis().ToFloat();
                                                     
                                                     _factoryTestLogger.Append(msg);
                                                     
                                                     static const f32 rollThresh_rad = DEG_TO_RAD(5.f);
                                                     static const f32 pitchThresh_rad = DEG_TO_RAD(5.f);
                                                     static const f32 yawThresh_rad = DEG_TO_RAD(5.f);
                                                     static const f32 xThresh_mm = 5;
                                                     static const f32 yThresh_mm = 5;
                                                     static const f32 zThresh_mm = 5;
                                                     
                                                     bool exceedsThresh = false;
                                                     if(!NEAR(msg.camPoseRoll_rad, 0, rollThresh_rad))
                                                     {
                                                       PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Roll exceeds threshold");
                                                       exceedsThresh = true;
                                                     }
                                                     if(!NEAR(msg.camPosePitch_rad - msg.headAngle, DEG_TO_RAD(-4.f), pitchThresh_rad))
                                                     {
                                                       PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Pitch exceeds threshold");
                                                       exceedsThresh = true;
                                                     }
                                                     if(!NEAR(msg.camPoseYaw_rad, 0, yawThresh_rad))
                                                     {
                                                       PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "Yaw exceeds threshold");
                                                       exceedsThresh = true;
                                                     }
                                                     if(!NEAR(msg.camPoseX_mm, 0, xThresh_mm))
                                                     {
                                                       PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "xTrans exceeds threshold");
                                                       exceedsThresh = true;
                                                     }
                                                     if(!NEAR(msg.camPoseY_mm, 0, yThresh_mm))
                                                     {
                                                       PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "yTrans exceeds threshold");
                                                       exceedsThresh = true;
                                                     }
                                                     if(!NEAR(msg.camPoseZ_mm, 0, zThresh_mm))
                                                     {
                                                       PRINT_NAMED_WARNING("BehaviorFactoryCentroidExtractor.CamPose", "zTrans exceeds threshold");
                                                       exceedsThresh = true;
                                                     }
                                                     
                                                     if(exceedsThresh)
                                                     {
                                                       EndTest(robot, FactoryTestResultCode::CAM_POSE_OOR);
                                                     }
                                                     
                                                   }
                                                   
                                                 } else {
                                                   EndTest(robot, FactoryTestResultCode::NO_PREPLAYPEN_CENTROIDS);
                                                 }
                                               });
          }
        
          
          ReadToolCodeAction* toolCodeAction = new ReadToolCodeAction(robot, false);
          
          // Read lift tool code
          StartActing(toolCodeAction,
                      [this,&robot](const ExternalInterface::RobotCompletedAction& rca){
                        const ActionResult& result = rca.result;
                        
                        // Save tool code images to robot (whether it succeeded to read code or not)
                        std::list<std::vector<u8> > rawJpegData = robot.GetVisionComponent().GetToolCodeImageJpegData();

                        const u32 NUM_TOOL_CODE_IMAGES = 2;
                        static const NVStorage::NVEntryTag toolCodeImageTags[NUM_TOOL_CODE_IMAGES] = {NVStorage::NVEntryTag::NVEntry_ToolCodeImageLeft, NVStorage::NVEntryTag::NVEntry_ToolCodeImageRight};
                        

                        // Verify number of images
                        bool tooManyToolCodeImages = rawJpegData.size() > NUM_TOOL_CODE_IMAGES;
                        if (tooManyToolCodeImages) {
                          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.TooManyToolCodeImagesFound",
                                              "%zu images found. Why?", rawJpegData.size());
                          rawJpegData.resize(NUM_TOOL_CODE_IMAGES);
                        }
                        
                        // Write images to robot
                        u32 imgIdx = 0;
                        for (auto const& img : rawJpegData) {
                          QueueWriteToRobot(robot, toolCodeImageTags[imgIdx], img.data(), img.size());
                          ++imgIdx;
                          
                          // Save calibration images to log on device
                          _factoryTestLogger.AddFile("toolCodeImage_" + std::to_string(imgIdx) + ".jpg", img);
                        }
                        
                        if (tooManyToolCodeImages) {
                          EndTest(robot, FactoryTestResultCode::TOO_MANY_TOOL_CODE_IMAGES);
                          return;
                        }
                        
                        
                        // Check result of tool code read
                        if (result != ActionResult::SUCCESS) {
                          EndTest(robot, FactoryTestResultCode::READ_TOOL_CODE_FAILED);
                          return;
                        }
                        
                        const ToolCodeInfo &info = rca.completionInfo.Get_readToolCodeCompleted().info;
                        PRINT_NAMED_INFO("BehaviorFactoryTest.RecvdToolCodeInfo.Info",
                                         "Code: %s, Expected L: (%f, %f), R: (%f, %f), Observed L: (%f, %f), R: (%f, %f)",
                                         EnumToString(info.code),
                                         info.expectedCalibDotLeft_x, info.expectedCalibDotLeft_y,
                                         info.expectedCalibDotRight_x, info.expectedCalibDotRight_y,
                                         info.observedCalibDotLeft_x, info.observedCalibDotLeft_y,
                                         info.observedCalibDotRight_x, info.observedCalibDotRight_y);
                        
                        // Write tool code to log on device
                        _factoryTestLogger.Append(info);
                        
                        // Store results to nvStorage
                        u8 buf[info.Size()];
                        size_t numBytes = info.Pack(buf, sizeof(buf));
                        QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_ToolCodeInfo, buf, numBytes);
                        
                        
                        // Verify tool code data is in range
                        static const f32 pixelDistThresh_x = 20.f;
                        static const f32 pixelDistThresh_y = 40.f;
                        f32 distL_x = std::fabsf(info.expectedCalibDotLeft_x - info.observedCalibDotLeft_x);
                        f32 distL_y = std::fabsf(info.expectedCalibDotLeft_y - info.observedCalibDotLeft_y);
                        f32 distR_x = std::fabsf(info.expectedCalibDotRight_x - info.observedCalibDotRight_x);
                        f32 distR_y = std::fabsf(info.expectedCalibDotRight_y - info.observedCalibDotRight_y);
                        
                        if (distL_x > pixelDistThresh_x || distL_y > pixelDistThresh_y || distR_x > pixelDistThresh_x || distR_y > pixelDistThresh_y) {
                          EndTest(robot, FactoryTestResultCode::TOOL_CODE_POSITIONS_OOR);
                        }
                      });
      
          SetCurrState(FactoryTestState::ReadLiftToolCode);
        } else if (currentTime_sec > _holdUntilTime) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CalibrationTimedout", "");
          END_TEST(FactoryTestResultCode::CALIBRATION_TIMED_OUT);
        }
        break;
      }
        
      // - - - - - - - - - - - - - - READ LIFT TOOL CODE - - - - - - - - - - - - - - -
      case FactoryTestState::ReadLiftToolCode:
      {
        
        // Get closest predock pose. Default to _prePickupPose.
        _closestPredockPose = _prePickupPose;
        Pose3d blockPose = _expectedLightCubePose;
        ObservableObject* obsObj = robot.GetBlockWorld().GetLocatedObjectByID(_blockObjectID);
        if (nullptr != obsObj) {
          blockPose = obsObj->GetPose();
          ActionableObject* actObj = dynamic_cast<ActionableObject*>(obsObj);
          if (nullptr != actObj) {
            std::vector<PreActionPose> preActionPoses;
            actObj->GetCurrentPreActionPoses(preActionPoses,
                                             robot.GetPose(),
                                             {PreActionPose::DOCKING},
                                             std::set<Vision::Marker::Code>());
            
            if (!preActionPoses.empty()) {
              f32 distToClosestPredockPose = 1000;
              for(auto & p : preActionPoses) {
                f32 dist = ComputeDistanceBetween(p.GetPose(), robot.GetPose());
                if (dist < distToClosestPredockPose) {
                  distToClosestPredockPose = dist;
                  _closestPredockPose = p.GetPose();
                }
              }
            }

            
          } else {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.FailedToCastObservableObjectToActionableObject", "");
          }
        } else {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.FailedToFindCubeObject", "BlockObjectID=%d",
                              _blockObjectID.GetValue());
        }
        
        // Zero predock pose height
        _closestPredockPose.SetTranslation(Vec3f(_closestPredockPose.GetTranslation().x(),
                                                 _closestPredockPose.GetTranslation().y(),
                                                 0));
        
        // Zero predock pose xy-rotation
        _closestPredockPose.SetRotation(_closestPredockPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>(), Z_AXIS_3D());
      
        
        
        // Goto predock pose and then turn towards the block again for good alignment
        DriveToPoseAction* driveAction = new DriveToPoseAction(robot, _closestPredockPose, false);
        TurnTowardsPoseAction* turnAction = new TurnTowardsPoseAction(robot, blockPose, M_PI_2_F);
        CompoundActionSequential* compoundAction = new CompoundActionSequential(robot, {driveAction, turnAction});
        
        StartActing(compoundAction,
                    [this,&robot](ActionResult result){
//                      // NOTE: This result check should be ok, but in sim the action often doesn't result in
//                      // the robot being exactly where it's supposed to be so the action itself sometimes fails.
//                      // When robot path following is improved (particularly in sim) this physical check can be removed.
//                      if (result != ActionResult::SUCCESS && robot.IsPhysical()) {
//                        PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.GotoPrePickupPoseFailed",
//                                            "actual: (x,y,deg) = %f, %f, %f, %fdeg; expected: %f, %f, %f, %fdeg",
//                                            robot.GetPose().GetTranslation().x(),
//                                            robot.GetPose().GetTranslation().y(),
//                                            robot.GetPose().GetTranslation().z(),
//                                            robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
//                                            _closestPredockPose.GetTranslation().x(),
//                                            _closestPredockPose.GetTranslation().y(),
//                                            _closestPredockPose.GetTranslation().z(),
//                                            _closestPredockPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
//                        EndTest(robot, FactoryTestResultCode::GOTO_PRE_PICKUP_POSE_ACTION_FAILED);
//                      } else {
//                        _holdUntilTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + 1.0f;
//                      }
                      return;
                    });
        
        SetCurrState(FactoryTestState::GotoPickupPose);
        break;
      }
        
      // - - - - - - - - - - - - - - GOTO PICKUP POSE - - - - - - - - - - - - - - -
      case FactoryTestState::GotoPickupPose:
      {

        if (!robot.GetPose().IsSameAs(_closestPredockPose, _kRobotPoseSamenessDistThresh_mm, _kRobotPoseSamenessAngleThresh_rad)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.ExpectingInPrePickupPose",
                              "actual: (x,y,deg) = %f, %f, %f, %fdeg; expected: %f, %f, %f, %fdeg",
                              robot.GetPose().GetTranslation().x(),
                              robot.GetPose().GetTranslation().y(),
                              robot.GetPose().GetTranslation().z(),
                              robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees(),
                              _closestPredockPose.GetTranslation().x(),
                              _closestPredockPose.GetTranslation().y(),
                              _closestPredockPose.GetTranslation().z(),
                              _closestPredockPose.GetRotationMatrix().GetAngleAroundAxis<'Z'>().getDegrees());
          _closestPredockPose.Print();
          robot.GetPose().Print();
          END_TEST(FactoryTestResultCode::NOT_IN_PRE_PICKUP_POSE);
        }

        // Write cube's pose to nv storage
        ObservableObject* oObject = robot.GetBlockWorld().GetLocatedObjectByID(_blockObjectID);
        if(nullptr == oObject)
        {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.FailedToFindObject",
                              "BlockObjectID=%d", _blockObjectID.GetValue());
          END_TEST(FactoryTestResultCode::CUBE_NOT_FOUND);
        }
        
        const Pose3d& cubePose = oObject->GetPose();
        PoseData poseData = ConvertToPoseData(cubePose);
        
        // Store to robot
        QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_ObservedCubePose, (u8*)&poseData, sizeof(poseData));
        
        // Write pose data to log on device
        _factoryTestLogger.AppendObservedCubePose(poseData);
        
        
        
        // Verify that block is approximately where expected
        Vec3f Tdiff;
        Radians angleDiff;
        if (!oObject->GetPose().IsSameAs_WithAmbiguity(_expectedLightCubePose,
                                                       _kBlockRotationAmbiguities,
                                                       _kExpectedCubePoseDistThresh_mm,
                                                       _kExpectedCubePoseAngleThresh_rad,
                                                       Tdiff, angleDiff) ||
            (std::fabsf(oObject->GetPose().GetTranslation().z() - (0.5f*oObject->GetSize().z())) > _kExpectedCubePoseHeightThresh_mm) ) {
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
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.CubeNotWhereExpectedZ", "%f %f",
                              oObject->GetPose().GetTranslation().z(),
                              (0.5f*oObject->GetSize().z()));
          END_TEST(FactoryTestResultCode::CUBE_NOT_WHERE_EXPECTED);
        }
        
        _attemptCounter = 0;
        SetCurrState(FactoryTestState::StartPickup);
        break;
      }
        
      // - - - - - - - - - - - - - - START PICKUP - - - - - - - - - - - - - - -
      case FactoryTestState::StartPickup:
      {
        // If robot hasn't discovered any active objects by now it probably won't so fail
        if(!_activeObjectAvailable)
        {
          PRINT_NAMED_INFO("BehaviorFactoryTest.EndTest.NoActiveObjectsDiscovered",
                           "Test ending no active objects discovered");
          END_TEST(FactoryTestResultCode::NO_ACTIVE_OBJECTS_DISCOVERED);
        }
        
        auto pickupCallback = [this,&robot](ActionResult result){
          if (result == ActionResult::SUCCESS &&
              robot.GetCarryingComponent().GetCarryingObject() == _blockObjectID) {
            PRINT_NAMED_INFO("BehaviorFactoryTest.pickupCallback.Success", "");
          } else if (_attemptCounter <= _kNumPickupRetries) {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.pickupCallback.FailedRetrying", "");
            SetCurrState(FactoryTestState::StartPickup);
          } else {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.pickupCallback.Failed", "");
            EndTest(robot, FactoryTestResultCode::PICKUP_FAILED);
          }
        };
       
        // Pickup block
        PRINT_NAMED_INFO("BehaviorFactory.Update.PickingUp", "Attempt %d", _attemptCounter);
        ++_attemptCounter;
        PickupObjectAction* action = new PickupObjectAction(robot, _blockObjectID);
        action->SetShouldCheckForObjectOnTopOf(false);
        StartActing(action,
                    pickupCallback);
        SetCurrState(FactoryTestState::PickingUpBlock);
        break;
      }
        
      // - - - - - - - - - - - - - - PICKING UP BLOCK - - - - - - - - - - - - - - -
      case FactoryTestState::PickingUpBlock:
      {
        // Check that robot orientation didn't change much during backup
        _robotAngleAfterBackup = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
        f32 angleChangeDeg = std::fabsf((_robotAngleAfterBackup - _robotAngleAtPickup).getDegrees());
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.AngleChangeDuringBackup",
                         "%f deg", angleChangeDeg );
        if (angleChangeDeg > _kMaxRobotAngleChangeDuringBackup_deg) {
          END_TEST(FactoryTestResultCode::BACKUP_NOT_STRAIGHT);
        }
        
        
        auto placementCallback = [this,&robot](ActionResult result){
          if (result == ActionResult::SUCCESS && !robot.GetCarryingComponent().IsCarryingObject()) {
            PRINT_NAMED_INFO("BehaviorFactoryTest.placementCallback.Success", "");
          } else {
            PRINT_NAMED_WARNING("BehaviorFactoryTest.placementCallback.Failed", "");
            
            // HACK: Sometimes the robot doesn't even attempt to lower the lift during placement
            //       but still thinks that it succeeded block placement. Just try it again.
            if (_numPlacementAttempts <= 1 && robot.GetLiftHeight() > 70) {
              PRINT_NAMED_INFO("BehaviorFactoryTest.placementCallback.Retrying", "");
              SetCurrState(FactoryTestState::PickingUpBlock);
            } else {
              EndTest(robot, FactoryTestResultCode::PLACEMENT_FAILED);
            }
          }
        };
        
        // Put block down
        PlaceObjectOnGroundAction* action = new PlaceObjectOnGroundAction(robot);
        StartActing(action,
                    placementCallback);
        ++_numPlacementAttempts;
        SetCurrState(FactoryTestState::PlacingBlock);
        break;
      }
        
      // - - - - - - - - - - - - - - PLACING BLOCK - - - - - - - - - - - - - - -
      case FactoryTestState::PlacingBlock:
      {
        // Play animation that backs up 3 times. If there's a sticky wheel hopefully this cause some turns.
        auto action = new PlayAnimationAction(robot, "anim_triple_backup");
        StartActing(action);
        
        SetCurrState(FactoryTestState::BackAndForth);
        break;
      }
      case FactoryTestState::BackAndForth:
      {
        // Check that robot orientation didn't change much since it lifted up the block
        Radians robotAngleAfterBackAndForth = robot.GetPose().GetRotationMatrix().GetAngleAroundAxis<'Z'>();
        f32 angleChangeDeg = std::fabsf((robotAngleAfterBackAndForth - _robotAngleAtPickup).getDegrees());
        PRINT_NAMED_INFO("BehaviorFactoryTest.Update.AngleChangeDuringBackAndForth",
                         "%f deg", angleChangeDeg );
        if (angleChangeDeg > _kMaxRobotAngleChangeDuringBackup_deg) {
          END_TEST(FactoryTestResultCode::BACK_AND_FORTH_NOT_STRAIGHT);
        }
        
        // Verify that battery voltage exceeds minimum threshold
        if (kBFT_CheckBattVoltage && (robot.GetBatteryVoltage() < _kMinBattVoltage)) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.Update.BattTooLow", "%fV", robot.GetBatteryVoltage());
          END_TEST(FactoryTestResultCode::BATTERY_TOO_LOW);
        }
        
        // %%%%%%%%%%%  END OF TEST %%%%%%%%%%%%%%%%%%
        EndTest(robot, FactoryTestResultCode::SUCCESS);
        break;
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
      }
        
      // - - - - - - - - - - - - - - WAITING FOR WRITES TO ROBOT - - - - - - - - - - - - - - -
      case FactoryTestState::WaitingForWritesToRobot:
      {
        if (robot.GetNVStorageComponent().HasPendingRequests() || robot.GetMoveComponent().IsMoving()) {
          break;
        }
       
        // Check if there were write failures
        if (_writeFailureCode != FactoryTestResultCode::UNKNOWN) {
          PRINT_NAMED_WARNING("BehaviorFactoryTest.WaitingForWritesToRobot.WritesFailed", "");
          _testResult = _writeFailureCode;
          _testResultEntry.result = _testResult;
          
          // If there was a write failure, don't bother attempting to write anything more to robot.
          _writeTestResult = false;
        }
        
        
        // Write result to device
        _factoryTestLogger.Append(_testResultEntry);
        
        
        if (kBFT_EnableNVStorageWrites &&
            _writeTestResult &&
            _testResult == FactoryTestResultCode::SUCCESS)
        {
          // If test passed, write birth certificate
          time_t nowTime = time(0);
          struct tm* tmStruct = gmtime(&nowTime);
          
          BirthCertificate bc;
          bc.year   = static_cast<u8>(tmStruct->tm_year % 100);
          bc.month  = static_cast<u8>(tmStruct->tm_mon + 1); // Months start at zero
          bc.day    = static_cast<u8>(tmStruct->tm_mday);
          bc.hour   = static_cast<u8>(tmStruct->tm_hour);
          bc.minute = static_cast<u8>(tmStruct->tm_min);
          bc.second = static_cast<u8>(tmStruct->tm_sec);
          
          u8 bcBuf[bc.Size()];
          size_t bcNumBytes = bc.Pack(bcBuf, sizeof(bcBuf));
          WriteEntry writeEntryBC(NVStorage::NVEntryTag::NVEntry_BirthCertificate, bcBuf, bcNumBytes, nullptr);
          
          // Write birth certificate to log on device
          _factoryTestLogger.Append(bc);
          
          u32 vmNumBytes = sizeof(NVStorage::NVConst::NVConst_VERSION_MAGIC);
          u8 vmBuf[vmNumBytes];
          for(int i = vmNumBytes-1; i >= 0; --i)
          {
            vmBuf[i] = (static_cast<u32>(NVStorage::NVConst::NVConst_VERSION_MAGIC) >> i*8 & 0xFF);
          }
          WriteEntry writeEntryVM(NVStorage::NVEntryTag::NVEntry_VersionMagic, vmBuf, vmNumBytes, nullptr);
          
          // Pack version magic and birth certificate into one blob of data and write it to FactoryBaseTag
          std::vector<u8> packedBC;
          PackWrites({writeEntryVM, writeEntryBC}, packedBC);
          
          DEV_ASSERT(packedBC.size() == static_cast<u32>(NVStorage::NVConst::NVConst_SIZE_OF_VERSION_AND_BC),
                     "BehaviorFactoryTest.PackedVMAndBCUnexpectedSize");
          
          robot.GetNVStorageComponent().Write(NVStorage::NVEntryTag::NVEntry_FactoryBaseTag,
                                              packedBC.data(), packedBC.size(),
                                              [this,&robot](NVStorage::NVResult res) {
                                                if (res != NVStorage::NVResult::NV_OKAY) {
                                                  PRINT_NAMED_WARNING("BehaviorFactoryTest.BCWriteFail","");
                                                  _testResult = FactoryTestResultCode::BIRTH_CERTIFICATE_WRITE_FAILED;
                                                } else {
                                                  PRINT_NAMED_INFO("BehaviorFactoryTest.BCWriteSuccess","");
                                                }
                                                SendTestResultToGame(robot, _testResult);
                                              });
        }
        else
        {
            SendTestResultToGame(robot, _testResult);
        }

        
        return Status::Complete;
        
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
    // If EnableNVStorageWrites is true then the log will be closed in the nvStorage write in EndTest
    // Otherwise we need to close it here
    if(!kBFT_EnableNVStorageWrites)
    {
      _factoryTestLogger.CloseLog();
    }
    
    if (_testResult == FactoryTestResultCode::UNKNOWN) {
      EndTest(robot, FactoryTestResultCode::TEST_CANCELLED);
    }
  }

  
  void BehaviorFactoryTest::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
  {
    switch(event.GetData().GetTag())
    {
      case EngineToGameTag::RobotObservedObject:
        _lastHandlerResult = HandleObservedObject(robot,
                                                  event.GetData().Get_RobotObservedObject());
        break;
        
      case EngineToGameTag::RobotDeletedLocatedObject:
        _lastHandlerResult = HandleDeletedLocatedObject(event.GetData().Get_RobotDeletedLocatedObject());
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

      case EngineToGameTag::RobotOffTreadsStateChanged:
        _lastHandlerResult = HandleRobotOfftreadsStateChanged(robot, event.GetData().Get_RobotOffTreadsStateChanged());
        break;
        
      case EngineToGameTag::MotorCalibration:
        _lastHandlerResult = HandleMotorCalibration(robot, event.GetData().Get_MotorCalibration());
        break;
        
      default:
        PRINT_NAMED_ERROR("BehaviorFactoryTest.HandleWhileRunning.InvalidTag",
                          "Received unexpected event with tag %hhu.", event.GetData().GetTag());
        _lastHandlerResult = RESULT_FAIL;
        break;
    }
  }
  
  void BehaviorFactoryTest::HandleMfgID(const AnkiEvent<RobotInterface::RobotToEngine>& message)
  {
    const auto& payload = message.GetData().Get_mfgId();
    _gotHWVersion = true;
    if(kBFT_CheckFWVersion && payload.hw_version < _kMinBodyHWVersion)
    {
      PRINT_NAMED_ERROR("BehaviorFactoryTest.HandleMfgID.WrongVersion",
                        "Expected %u got %u", _kMinBodyHWVersion, payload.hw_version);
      EndTest(_robot, FactoryTestResultCode::WRONG_BODY_HW_VERSION);
    }
  }
  
  void BehaviorFactoryTest::HandleFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>& message)
  {
    if(kBFT_CheckFWVersion)
    {
      const auto& fwData = message.GetData().Get_firmwareVersion().json;
      std::string jsonString{fwData.begin(), fwData.end()};
      Json::Reader reader;
      Json::Value headerData;
      if(!reader.parse(jsonString, headerData))
      {
        EndTest(_robot, FactoryTestResultCode::PARSE_HEADER_FAILED);
        return;
      }
      
      _fwVersion = headerData["version"].asString();
      _fwBuildType = headerData["build"].asString();
      
      if(_fwVersion != _kFWVersion ||
         _fwBuildType != "FACTORY")
      {
        // This message is handled before the behavior starts so we need to set a flag
        _hasWrongFirmware = true;
      }
    }
  }
  
  void BehaviorFactoryTest::HandleFactoryFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>& message)
  {
    if(ENABLE_FACTORY_TEST)
    {
      // Only 1.0 factory firmware has this message
      SendTestResultToGame(_robot, FactoryTestResultCode::ONE_POINT_ZERO_FIRMWARE);
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

    //SetStateName(name);
  }


  Result BehaviorFactoryTest::HandleObservedObject(Robot& robot,
                                                   const ExternalInterface::RobotObservedObject &msg)
  {

    ObjectID objectID = msg.objectID;
    const ObservableObject* oObject = robot.GetBlockWorld().GetLocatedObjectByID(objectID);
    
    if(nullptr == oObject)
    {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.NullObject",
                          "Object %d is NULL", objectID.GetValue());
      return RESULT_FAIL;
    }
    
    // Check if this is the markerless object for the cliff
    if (oObject->GetFamily() == ObjectFamily::MarkerlessObject) {
    }
    
    const auto& objType = oObject->GetType();
    
    if (objType == ObjectType::ProxObstacle) {
      if (!_cliffObjectID.IsSet() || _cliffObjectID == objectID) {
        _cliffObjectID = objectID;
        return RESULT_OK;
      } else {
        PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedProxObstacle", "ID: %d, Type: %d", objectID.GetValue(), oObject->GetType());
        END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
      }
    } else if (IsValidLightCube(objType, false)) {
      if (!_blockObjectID.IsSet() || _blockObjectID == objectID) {
        _blockObjectID = objectID;
        return RESULT_OK;
      } else {
        PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedBlock", "ID: %d, Type: %d", objectID.GetValue(), objType);
        END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
        return RESULT_OK;
      }
    } else {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleObservedObject.UnexpectedObjectType", "ID: %d, Type: %d", objectID.GetValue(), objType);
      END_TEST_IN_HANDLER(FactoryTestResultCode::UNEXPECTED_OBSERVED_OBJECT);
    }
    
    return RESULT_OK;
  }

  Result BehaviorFactoryTest::HandleDeletedLocatedObject(const ExternalInterface::RobotDeletedLocatedObject &msg)
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
  
  
  void BehaviorFactoryTest::HandleFactoryTestParameter(const AnkiEvent<RobotInterface::RobotToEngine>& message)
  {
    const RobotInterface::FactoryTestParameter& payload = message.GetData().Get_factoryTestParam();
    
    PRINT_NAMED_INFO("BehaviorFactoryTest.HandleFactoryTestParameter.Recvd", "StationID: %d", payload.param);
    
    _stationID = payload.param;
  }
  
  Result BehaviorFactoryTest::HandleCameraCalibration(Robot &robot, const CameraCalibration &calibMsg)
  {
 
    Vision::CameraCalibration camCalib(calibMsg.nrows, calibMsg.ncols,
                                       calibMsg.focalLength_x, calibMsg.focalLength_y,
                                       calibMsg.center_x, calibMsg.center_y,
                                       calibMsg.skew,
                                       calibMsg.distCoeffs);
    
    // Set camera calibration
    PRINT_NAMED_INFO("BehaviorFactoryTest.HandleCameraCalibration.SettingNewCalibration", "");
    robot.GetVisionComponent().SetCameraCalibration(camCalib);
    
    // Write camera calibration to log on device
    _factoryTestLogger.Append(calibMsg);
    
    
    // Write calibration to robot
    u8 buf[calibMsg.Size()];
    size_t numBytes = calibMsg.Pack(buf, sizeof(buf));
    QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_CameraCalib, buf, numBytes);

    
    // Get calibration image data
    CalibMetaInfo calibMetaInfo;
    std::list<std::vector<u8> > rawJpegData = robot.GetVisionComponent().GetCalibrationImageJpegData(&calibMetaInfo.dotsFoundMask);
    
    const u32 NUM_CAMERA_CALIB_IMAGES = 6;
    static const NVStorage::NVEntryTag calibImageTags[NUM_CAMERA_CALIB_IMAGES] = {NVStorage::NVEntryTag::NVEntry_CalibImage1,
      NVStorage::NVEntryTag::NVEntry_CalibImage2,
      NVStorage::NVEntryTag::NVEntry_CalibImage3,
      NVStorage::NVEntryTag::NVEntry_CalibImage4,
      NVStorage::NVEntryTag::NVEntry_CalibImage5,
      NVStorage::NVEntryTag::NVEntry_CalibImage6};

    // Verify number of images
    bool tooManyCalibImages = rawJpegData.size() > NUM_CAMERA_CALIB_IMAGES;
    if (tooManyCalibImages) {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleCameraCalibration.TooManyCalibImagesFound",
                          "%zu images found. Why?", rawJpegData.size());
      rawJpegData.resize(NUM_CAMERA_CALIB_IMAGES);
    }

    // Write calibration images to robot
    u32 imgIdx = 0;
    for (auto const& img : rawJpegData) {
      QueueWriteToRobot(robot, calibImageTags[imgIdx], img.data(), img.size());
      ++imgIdx;
      
      // Save calibration images to log on device
      _factoryTestLogger.AddFile("calibImage_" + std::to_string(imgIdx) + ".jpg", img);
    }
    
    // Write bit flag indicating in which images dots were found
    QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_CalibMetaInfo, (u8*)&calibMetaInfo, sizeof(calibMetaInfo));
    _factoryTestLogger.Append(calibMetaInfo);
    
    // Error if too many images found for some reason
    if (tooManyCalibImages) {
      END_TEST_IN_HANDLER(FactoryTestResultCode::TOO_MANY_CALIB_IMAGES);
    }
    
    // Save computed camera pose when robot was on charger
    // NOTE: If this fails a lot on the line, demote this to a non-error.
    Pose3d calibPose;
    Result writePoseResult = robot.GetVisionComponent().GetCalibrationPoseToRobot(0, calibPose);
    if (writePoseResult != RESULT_OK) {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.GetCalibPose.Failed", "");
      //END_TEST_IN_HANDLER(FactoryTestResultCode::CALIB_POSE_GET_FAILED);
    } else {
      // Write calib pose to log on device
      PoseData poseData = ConvertToPoseData(calibPose);
      _factoryTestLogger.AppendCalibPose(poseData);

      // Write calib pose to robot
      QueueWriteToRobot(robot, NVStorage::NVEntryTag::NVEntry_CalibPose, (u8*)&poseData, sizeof(poseData));
    }
    
    
    // Check if calibration values are sane
    #define CHECK_OOR(value, min, max) (value < min || value > max)
    if (CHECK_OOR(calibMsg.focalLength_x, 250, 310) ||
        CHECK_OOR(calibMsg.focalLength_y, 250, 310) ||
        CHECK_OOR(calibMsg.center_x, 130, 190) ||
        CHECK_OOR(calibMsg.center_y, 90, 150) ||
        calibMsg.nrows != 240 ||
        calibMsg.ncols != 320)
    {
      PRINT_NAMED_WARNING("BehaviorFactoryTest.HandleCameraCalibration.OOR",
                          "focalLength (%f, %f), center (%f, %f)",
                          calibMsg.focalLength_x, calibMsg.focalLength_y, calibMsg.center_x, calibMsg.center_y);
      END_TEST_IN_HANDLER(FactoryTestResultCode::CALIBRATION_VALUES_OOR);
    }
    
    _calibrationReceived = true;
    return RESULT_OK;
  }
  
  Result BehaviorFactoryTest::HandleRobotStopped(Robot& robot, const ExternalInterface::RobotStopped &msg)
  {
    // This is expected when driving to slot but not anytime else
    if (_currentState != FactoryTestState::DriveToSlot) {
      EndTest(robot, FactoryTestResultCode::CLIFF_UNEXPECTED);
    }
    
    return RESULT_OK;
  }

  Result BehaviorFactoryTest::HandleRobotOfftreadsStateChanged(Robot& robot, const ExternalInterface::RobotOffTreadsStateChanged &msg)
  {
    if(msg.treadsState != OffTreadsState::OnTreads){
      EndTest(robot, FactoryTestResultCode::ROBOT_PICKUP);
    }
    return RESULT_OK;
  }

  
  Result BehaviorFactoryTest::HandleMotorCalibration(Robot& robot, const MotorCalibration &msg)
  {
    // This should never happen during the test, except at the beginning
    if (_currentState == FactoryTestState::WaitingForMotorCalibration) {
      
      // If a motor has finished calibrating, mark it as calibrated
      if (!msg.calibStarted){
        if (msg.motorID == MotorID::MOTOR_HEAD) {
          PRINT_NAMED_INFO("BehaviorFactoryTest.HandleMotorCalibration.HeadCalibrated", "");
          _headCalibrated = true;
        } else if (msg.motorID == MotorID::MOTOR_LIFT) {
          PRINT_NAMED_INFO("BehaviorFactoryTest.HandleMotorCalibration.LiftCalibrated", "");
          _liftCalibrated = true;
        }
      }
      
    } else if (_currentState > FactoryTestState::WaitingForMotorCalibration) {
      switch(msg.motorID) {
        case MotorID::MOTOR_HEAD:
          EndTest(robot, FactoryTestResultCode::HEAD_MOTOR_CALIB_UNEXPECTED);
          break;
        case MotorID::MOTOR_LIFT:
          EndTest(robot, FactoryTestResultCode::LIFT_MOTOR_CALIB_UNEXPECTED);
          break;
        default:
          EndTest(robot, FactoryTestResultCode::MOTOR_CALIB_UNEXPECTED);
          break;
      }
    }

    return RESULT_OK;
  }
  
  void BehaviorFactoryTest::HandleActiveObjectAvailable(const AnkiEvent<RobotInterface::RobotToEngine>& msg)
  {
    const ObjectAvailable payload = msg.GetData().Get_activeObjectAvailable();
    if (IsValidLightCube(payload.objectType, false) || IsCharger(payload.objectType, false)) {
      _activeObjectAvailable = true;
    }
  }
 
  void BehaviorFactoryTest::HandlePickAndPlaceResult(const AnkiEvent<RobotInterface::RobotToEngine>& msg)
  {
    const PickAndPlaceResult payload = msg.GetData().Get_pickAndPlaceResult();
    _blockPickedUpReceived = (payload.didSucceed && payload.blockStatus == BlockStatus::BLOCK_PICKED_UP);
  }
  
  PoseData BehaviorFactoryTest::ConvertToPoseData(const Pose3d& p)
  {
    PoseData poseData;
    Radians angleX, angleY, angleZ;
    p.GetRotationMatrix().GetEulerAngles(angleX, angleY, angleZ);
    poseData.angleX_rad = angleX.ToFloat();
    poseData.angleY_rad = angleY.ToFloat();
    poseData.angleZ_rad = angleZ.ToFloat();
    poseData.transX_mm = p.GetTranslation().x();
    poseData.transY_mm = p.GetTranslation().y();
    poseData.transZ_mm = p.GetTranslation().z();
    return poseData;
  }
  
} // namespace Cozmo
} // namespace Anki
