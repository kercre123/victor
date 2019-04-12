//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "engine/robot.h"
#include "camera/cameraService.h"
#include "whiskeyToF/tof.h"

#include "coretech/common/engine/math/poseOriginList.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/charger.h"
#include "engine/components/accountSettingsManager.h"
#include "engine/components/animationComponent.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/components/blockTapFilterComponent.h"
#include "engine/components/backpackLights/engineBackpackLightComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/appCubeConnectionSubscriber.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeBatteryComponent.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/habitatDetectorComponent.h"
#include "engine/components/jdocsManager.h"
#include "engine/components/localeComponent.h"
#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/photographyManager.h"
#include "engine/components/powerStateManager.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/components/robotHealthReporter.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sdkComponent.h"
#include "engine/components/settingsCommManager.h"
#include "engine/components/settingsManager.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/imuComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/sensors/rangeSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/textToSpeech/textToSpeechCoordinator.h"
#include "engine/components/userEntitlementsManager.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/components/localizationComponent.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/faceWorld.h"
#include "engine/fullRobotPose.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/moodSystem/stimulationFaceDisplay.h"
#include "engine/navMap/mapComponent.h"
#include "engine/petWorld.h"
#include "engine/robotDataLoader.h"
#include "engine/robotGyroDriftDetector.h"
#include "engine/robotManager.h"
#include "engine/robotStateHistory.h"
#include "engine/robotToEngineImplMessaging.h"
#include "engine/viz/vizManager.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/environment/locale.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "util/messageProfiler/messageProfiler.h"

#include "osState/osState.h"

#include "anki/cozmo/shared/factory/faultCodes.h"

// Giving this its own local define, in case we want to control it independently of DEV_CHEATS / SHIPPING, etc.
#define ENABLE_DRAWING ANKI_DEV_CHEATS

#define LOG_CHANNEL "Robot"

#define IS_STATUS_FLAG_SET(x) ((msg.status & (uint32_t)RobotStatusFlag::x) != 0)

namespace Anki {
namespace Vector {

CONSOLE_VAR(bool, kDebugPossibleBlockInteraction, "Robot", false);

// if false, vision system keeps running while picked up, on side, etc.
CONSOLE_VAR(bool, kUseVisionOnlyWhileOnTreads,    "Robot", false);

// Enable to enable example code of face image drawing
CONSOLE_VAR(bool, kEnableTestFaceImageRGBDrawing,  "Robot", false);

#if REMOTE_CONSOLE_ENABLED

// Robot singleton
static Robot* _thisRobot = nullptr;

// Play an animation by name from the debug console.
// Note: If COZMO-11199 is implemented (more user-friendly playing animations by name
//   on the Unity side), then this console func can be removed.
static void PlayAnimationByName(ConsoleFunctionContextRef context)
{
  if (_thisRobot != nullptr) {
    const char* animName = ConsoleArg_Get_String(context, "animName");
    if (animName) {
      int numLoops = ConsoleArg_GetOptional_Int(context, "numLoops", 1);
      bool renderInEyeHue = ConsoleArg_GetOptional_Bool(context, "renderInEyeHue", true);
      auto* action = new PlayAnimationAction(animName, numLoops);
      action->SetRenderInEyeHue(renderInEyeHue);
      _thisRobot->GetActionList().QueueAction(QueueActionPosition::NOW, action);
    }
  }
}

static void AddAnimation(ConsoleFunctionContextRef context)
{
  if (_thisRobot != nullptr) {
    const char* animFile = ConsoleArg_Get_String(context, "animFile");
    if (animFile) {
      const std::string animationFolder = _thisRobot->GetContextDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources, "/assets/animations/");
      std::string animationPath = animationFolder + animFile;

      auto* robotDataLoader = _thisRobot->GetContext()->GetDataLoader();
      if (robotDataLoader != nullptr) {
        auto* animContainer = robotDataLoader->GetCannedAnimationContainer();
        if (animContainer != nullptr) {

          auto platform = _thisRobot->GetContextDataPlatform();
          auto spriteSequenceContainer = _thisRobot->GetComponent<DataAccessorComponent>().GetSpriteSequenceContainer();
          std::atomic<float> loadingCompleteRatio(0);
          std::atomic<bool> abortLoad(false);
          CannedAnimationLoader animLoader(platform, spriteSequenceContainer, loadingCompleteRatio, abortLoad);

          animLoader.LoadAnimationIntoContainer(animationPath.c_str(), animContainer);
          LOG_INFO("Robot.AddAnimation", "Loaded animation from %s", animationPath.c_str());
        }
      }
    }
  }
}

CONSOLE_FUNC(PlayAnimationByName, "Animation", const char* animName, optional int numLoops, optional bool renderInEyeHue);
CONSOLE_FUNC(AddAnimation, "Animation", const char* animFile);

static void PrintBodyData(ConsoleFunctionContextRef context)
{
  if (_thisRobot != nullptr) {
    // 0 means disable printing
    const uint32_t period_tics = ConsoleArg_Get_UInt(context, "printPeriod_tics");
    const bool motors = ConsoleArg_GetOptional_Bool(context, "motors", true);
    const bool prox = ConsoleArg_GetOptional_Bool(context, "prox", false);
    const bool battery = ConsoleArg_GetOptional_Bool(context, "battery", false);
    _thisRobot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::PrintBodyData(period_tics, motors, prox, battery)));
    LOG_INFO("Robot.PrintBodyData", "Period: %d tic, (m: %d, p: %d, b: %d)", period_tics, motors, prox, battery);
  }
}

CONSOLE_FUNC(PrintBodyData, "Syscon", uint32_t printPeriod_tics, optional bool motors, optional bool prox, optional bool battery);

// Perform Text to Speech Coordinator from debug console
namespace {

constexpr const char * kTtsCoordinatorPath = "TtSCoordinator";
// NOTE: Need to keep kVoiceStyles in sync with AudioMetaData::SwitchState::Robot_Vic_External_Processing in
//       clad/audio/audioSwitchTypes.clad
constexpr const char * kVoiceStyles = "Default_Processed,Unprocessed";

CONSOLE_VAR_ENUM(u8, kVoiceStyle, kTtsCoordinatorPath, 0, kVoiceStyles);
CONSOLE_VAR_RANGED(f32, kDurationScalar, kTtsCoordinatorPath, 1.f, 0.25f, 4.f);

void SayText(ConsoleFunctionContextRef context)
{
  auto * robot = _thisRobot;
  if (robot == nullptr) {
    LOG_ERROR("Robot.TtSCoordinator.NoRobot", "No robot connected");
    return;
  }

  const char * text = ConsoleArg_Get_String(context, "text");
  if (text == nullptr) {
    LOG_ERROR("Robot.TtSCoordinator.NoText", "No text string");
    return;
  }

  // VIC-4364 Replace `_` with spaces. Hack to allows to use spaces
  std::string textStr = text;
  std::replace(textStr.begin(), textStr.end(), '_', ' ');

  // Handle processing state
  using TtsProcessingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;
  auto style = TtsProcessingStyle::Invalid;
  switch (kVoiceStyle) {
    case 0:
      style = TtsProcessingStyle::Default_Processed;
      break;

    case 1:
      style = TtsProcessingStyle::Unprocessed;
      break;

    default:
      LOG_ERROR("Robot.SayText.InvalidVoiceStyleEnum", "Unknown value");
      break;
  }

 LOG_INFO("Robot.TtSCoordinator", "text(%s) style(%s) duration(%f)",
          Util::HidePersonallyIdentifiableInfo(textStr.c_str()), EnumToString(style), kDurationScalar);

  robot->GetTextToSpeechCoordinator().CreateUtterance(textStr, UtteranceTriggerType::Immediate, style);
}

CONSOLE_FUNC(SayText, kTtsCoordinatorPath, const char* text);

} // end namespace

#endif

////////
// Consts for robot offtreadsState
///////

// timeToConsiderOfftreads is tuned based on the fact that we have to wait half a second from the time the cliff sensor detects
// ground to when the robot state message updates to the fact that it is no longer picked up
static const TimeStamp_t kRobotTimeToConsiderOfftreads_ms = 250;
static const TimeStamp_t kRobotTimeToConsiderOfftreadsOnBack_ms = kRobotTimeToConsiderOfftreads_ms * 3;

// Laying flat angles
static const float kPitchAngleOntreads_rads = DEG_TO_RAD(0);
static const float kPitchAngleOntreadsTolerance_rads = DEG_TO_RAD(45);

//Constants for on back
static const float kPitchAngleOnBack_rads = DEG_TO_RAD(74.5f);
static const float kPitchAngleOnBack_sim_rads = DEG_TO_RAD(96.4f);
static const float kPitchAngleOnBackTolerance_deg = 15.0f;

//Constants for on side
static const float kOnSideAccel_mmps2 = 9800.0f;
static const float kOnSideToleranceAccel_mmps2 = 3000.0f;

// On face angles
static const float kPitchAngleOnFacePlantMin_rads = DEG_TO_RAD(110.f);
static const float kPitchAngleOnFacePlantMax_rads = DEG_TO_RAD(-80.f);
static const float kPitchAngleOnFacePlantMin_sim_rads = DEG_TO_RAD(110.f); //This has not been tested
static const float kPitchAngleOnFacePlantMax_sim_rads = DEG_TO_RAD(-80.f); //This has not been tested

// Too long in-air condition
static const TimeStamp_t kInAirTooLongTimeReportTime_ms = 60000;

// As long as robot's orientation doesn't change by more than
// this amount, we assume it's not being held by a person.
// Note that if he's placed on a platform that's vibrating so
// much that his orientation changes by more than this amount,
// we would not be able to differentiate this from being held.
static const float kRobotAngleChangedThresh_rad = DEG_TO_RAD(1);

Robot::Robot(const RobotID_t robotID, CozmoContext* context)
: _context(context)
, _ID(robotID)
, _serialNumberHead(OSState::getInstance()->GetSerialNumber())
, _syncRobotAcked(false)
, _lastMsgTimestamp(0)
, _offTreadsState(OffTreadsState::OnTreads)
, _awaitingConfirmationTreadState(OffTreadsState::OnTreads)
, _robotAccelFiltered(0.f, 0.f, 0.f)
{
  DEV_ASSERT(_context != nullptr, "Robot.Constructor.ContextIsNull");

  LOG_INFO("Robot.Robot", "Created");

  // Check for /tmp/data_cleared file
  // VIC-6069: OS needs to write this file following Clear User Data reboot
  static const std::string dataClearedFile = "/tmp/data_cleared";
  if (Util::FileUtils::FileExists(dataClearedFile)) {
    DASMSG(robot_cleared_user_data, "robot.cleared_user_data", "User data was cleared");
    DASMSG_SEND();
    Util::FileUtils::DeleteFile(dataClearedFile);
  }


  // DAS message "power on"
  float idleTime_sec;
  auto upTime_sec = static_cast<uint32_t>(OSState::getInstance()->GetUptimeAndIdleTime(idleTime_sec));
  DASMSG(robot_power_on, "robot.power_on", "Robot (engine) object created");
  DASMSG_SET(i1, upTime_sec, "Uptime (seconds)");
  DASMSG_SEND();

  // create all components
  {
    _components = std::make_unique<EntityType>();
    _components->AddDependentComponent(RobotComponentID::AppCubeConnectionSubscriber,new AppCubeConnectionSubscriber());
    _components->AddDependentComponent(RobotComponentID::CozmoContextWrapper,        new ContextWrapper(context));
    _components->AddDependentComponent(RobotComponentID::BlockWorld,                 new BlockWorld());
    _components->AddDependentComponent(RobotComponentID::FaceWorld,                  new FaceWorld());
    _components->AddDependentComponent(RobotComponentID::PetWorld,                   new PetWorld());
    _components->AddDependentComponent(RobotComponentID::PublicStateBroadcaster,     new PublicStateBroadcaster());
    _components->AddDependentComponent(RobotComponentID::EngineAudioClient,          new Audio::EngineRobotAudioClient());
    _components->AddDependentComponent(RobotComponentID::PathPlanning,               new PathComponent());
    _components->AddDependentComponent(RobotComponentID::DrivingAnimationHandler,    new DrivingAnimationHandler());
    _components->AddDependentComponent(RobotComponentID::ActionList,                 new ActionList());
    _components->AddDependentComponent(RobotComponentID::Movement,                   new MovementComponent());
    _components->AddDependentComponent(RobotComponentID::Vision,                     new VisionComponent());
    _components->AddDependentComponent(RobotComponentID::VisionScheduleMediator,     new VisionScheduleMediator());
    _components->AddDependentComponent(RobotComponentID::Map,                        new MapComponent());
    _components->AddDependentComponent(RobotComponentID::Localization,               new LocalizationComponent());
    _components->AddDependentComponent(RobotComponentID::NVStorage,                  new NVStorageComponent());
    _components->AddDependentComponent(RobotComponentID::AIComponent,                new AIComponent());
    _components->AddDependentComponent(RobotComponentID::CubeLights,                 new CubeLightComponent());
    _components->AddDependentComponent(RobotComponentID::BackpackLights,             new BackpackLightComponent());
    _components->AddDependentComponent(RobotComponentID::CubeAccel,                  new CubeAccelComponent());
    _components->AddDependentComponent(RobotComponentID::CubeBattery,                new CubeBatteryComponent());
    _components->AddDependentComponent(RobotComponentID::CubeComms,                  new CubeCommsComponent());
    _components->AddDependentComponent(RobotComponentID::CubeConnectionCoordinator,  new CubeConnectionCoordinator());
    _components->AddDependentComponent(RobotComponentID::CubeInteractionTracker,     new CubeInteractionTracker());
    _components->AddDependentComponent(RobotComponentID::GyroDriftDetector,          new RobotGyroDriftDetector());
    _components->AddDependentComponent(RobotComponentID::HabitatDetector,            new HabitatDetectorComponent());
    _components->AddDependentComponent(RobotComponentID::Docking,                    new DockingComponent());
    _components->AddDependentComponent(RobotComponentID::Carrying,                   new CarryingComponent());
    _components->AddDependentComponent(RobotComponentID::CliffSensor,                new CliffSensorComponent());
    _components->AddDependentComponent(RobotComponentID::ProxSensor,                 new ProxSensorComponent());
    _components->AddDependentComponent(RobotComponentID::ImuSensor,                  new ImuComponent());
    _components->AddDependentComponent(RobotComponentID::RangeSensor,                new RangeSensorComponent());
    _components->AddDependentComponent(RobotComponentID::TouchSensor,                new TouchSensorComponent());
    _components->AddDependentComponent(RobotComponentID::Animation,                  new AnimationComponent());
    _components->AddDependentComponent(RobotComponentID::StateHistory,               new RobotStateHistory());
    _components->AddDependentComponent(RobotComponentID::MoodManager,                new MoodManager());
    _components->AddDependentComponent(RobotComponentID::StimulationFaceDisplay,     new StimulationFaceDisplay());
    _components->AddDependentComponent(RobotComponentID::BlockTapFilter,             new BlockTapFilterComponent());
    _components->AddDependentComponent(RobotComponentID::RobotToEngineImplMessaging, new RobotToEngineImplMessaging());
    _components->AddDependentComponent(RobotComponentID::MicComponent,               new MicComponent());
    _components->AddDependentComponent(RobotComponentID::Battery,                    new BatteryComponent());
    _components->AddDependentComponent(RobotComponentID::FullRobotPose,              new FullRobotPose());
    _components->AddDependentComponent(RobotComponentID::DataAccessor,               new DataAccessorComponent());
    _components->AddDependentComponent(RobotComponentID::BeatDetector,               new BeatDetectorComponent());
    _components->AddDependentComponent(RobotComponentID::TextToSpeechCoordinator,    new TextToSpeechCoordinator());
    _components->AddDependentComponent(RobotComponentID::SDK,                        new SDKComponent());
    _components->AddDependentComponent(RobotComponentID::PhotographyManager,         new PhotographyManager());
    _components->AddDependentComponent(RobotComponentID::PowerStateManager,          new PowerStateManager());
    _components->AddDependentComponent(RobotComponentID::SettingsCommManager,        new SettingsCommManager());
    _components->AddDependentComponent(RobotComponentID::RobotHealthReporter,        new RobotHealthReporter());
    _components->AddDependentComponent(RobotComponentID::SettingsManager,            new SettingsManager());
    _components->AddDependentComponent(RobotComponentID::RobotStatsTracker,          new RobotStatsTracker());
    _components->AddDependentComponent(RobotComponentID::VariableSnapshotComponent,  new VariableSnapshotComponent());
    _components->AddDependentComponent(RobotComponentID::JdocsManager,               new JdocsManager());
    _components->AddDependentComponent(RobotComponentID::AccountSettingsManager,     new AccountSettingsManager());
    _components->AddDependentComponent(RobotComponentID::UserEntitlementsManager,    new UserEntitlementsManager());
    _components->AddDependentComponent(RobotComponentID::LocaleComponent,            new LocaleComponent());
    _components->InitComponents(this);
  }

  GetComponent<FullRobotPose>().GetPose().SetName("Robot_" + std::to_string(_ID));

  // Initializes FullRobotPose, _poseOrigins, and _worldOrigin:
  GetLocalizationComponent().Delocalize();

  // The call to Delocalize() will increment frameID, but we want it to be
  // initialized to 0, to match the physical robot's initialization
  // It will also add to history so clear it
  // It will also flag that a localization update is needed when it increments the frameID so set the flag
  // to false
  GetStateHistory()->Clear();

  GetRobotToEngineImplMessaging().InitRobotMessageComponent(GetContext()->GetRobotManager()->GetMsgHandler(), this);

  // Setting camera pose according to current head angle.
  // (Not using SetHeadAngle() because _isHeadCalibrated is initially false making the function do nothing.)
  GetVisionComponent().GetCamera().SetPose(GetCameraPose(GetComponent<FullRobotPose>().GetHeadAngle()));

  // Used for CONSOLE_FUNCTION "PlayAnimationByName" above
#if REMOTE_CONSOLE_ENABLED
  _thisRobot = this;
#endif

  // These will create the instances if they don't yet exist
  CameraService::getInstance();
  ToFSensor::getInstance();

} // Constructor: Robot

Robot::~Robot()
{
  // save variable snapshots before rest of robot components start destructing
  _components->RemoveComponent(RobotComponentID::VariableSnapshotComponent);

  // VIC-1961: Remove touch sensor component before aborting all since there's a DEV_ASSERT crash
  // and we need to write data out from the touch sensor component out. This explicit destruction
  // can be removed once the DEV_ASSERT is fixed
  _components->RemoveComponent(RobotComponentID::TouchSensor);

  AbortAll();

  // Destroy our actionList before things like the path planner, since actions often rely on those.
  // ActionList must be cleared before it is destroyed because pending actions may attempt to make use of the pointer.
  GetActionList().Clear();

  // Remove (destroy) certain components explicitly since
  // they contains poses that use the contents of FullRobotPose as a parent
  // and there's no guarantee on entity/component destruction order
  _components->RemoveComponent(RobotComponentID::Vision);
  _components->RemoveComponent(RobotComponentID::Map);
  _components->RemoveComponent(RobotComponentID::PathPlanning);

  // Ensure JdocsManager destructor gets called before the destructors of the
  // four components that it needs to talk to
  _components->RemoveComponent(RobotComponentID::JdocsManager);

  LOG_INFO("Robot.Destructor", "");
}


bool Robot::CheckAndUpdateTreadsState(const RobotState& msg)
{
  if (!IsHeadCalibrated()) {
    return false;
  }

  const bool isPickedUp = IS_STATUS_FLAG_SET(IS_PICKED_UP);
  const bool isFalling = IS_STATUS_FLAG_SET(IS_FALLING);
  const EngineTimeStamp_t currentTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

  //////////
  // Check the robot's orientation
  //////////

  //// COZMO_UP_RIGHT
  const bool currOnTreads = std::abs(GetPitchAngle().ToDouble() - kPitchAngleOntreads_rads) <= kPitchAngleOntreadsTolerance_rads;

  //// COZMO_ON_BACK
  const float backAngle = IsPhysical() ? kPitchAngleOnBack_rads : kPitchAngleOnBack_sim_rads;
  const bool currOnBack = std::abs( GetPitchAngle().ToDouble() - backAngle ) <= DEG_TO_RAD( kPitchAngleOnBackTolerance_deg );
  //// COZMO_ON_SIDE
  const bool currOnSide = Util::IsNear(std::abs(_robotAccelFiltered.y), kOnSideAccel_mmps2, kOnSideToleranceAccel_mmps2);
  const bool onRightSide = currOnSide && (_robotAccelFiltered.y > 0.f);
  //// COZMO_ON_FACE
  const float facePlantMinAngle = IsPhysical() ? kPitchAngleOnFacePlantMin_rads : kPitchAngleOnFacePlantMin_sim_rads;
  const float facePlantMaxAngle = IsPhysical() ? kPitchAngleOnFacePlantMax_rads : kPitchAngleOnFacePlantMax_sim_rads;

  const bool currFacePlant = GetPitchAngle() > facePlantMinAngle || GetPitchAngle() < facePlantMaxAngle;

  /////
  // Orientation based state transitions
  ////

  if (isFalling) {
    if (_awaitingConfirmationTreadState != OffTreadsState::Falling) {
      _awaitingConfirmationTreadState = OffTreadsState::Falling;
      _timeOffTreadStateChanged_ms = currentTimestamp - kRobotTimeToConsiderOfftreads_ms;
    }
  }
  else if (currOnSide) {
    if (_awaitingConfirmationTreadState != OffTreadsState::OnRightSide
       && _awaitingConfirmationTreadState != OffTreadsState::OnLeftSide)
    {
      // Transition to Robot on Side
      if(onRightSide) {
        _awaitingConfirmationTreadState = OffTreadsState::OnRightSide;
      } else {
        _awaitingConfirmationTreadState = OffTreadsState::OnLeftSide;
      }
      _timeOffTreadStateChanged_ms = currentTimestamp;
    }
  }
  else if (currFacePlant) {
    if (_awaitingConfirmationTreadState != OffTreadsState::OnFace) {
      // Transition to Robot on Face
      _awaitingConfirmationTreadState = OffTreadsState::OnFace;
      _timeOffTreadStateChanged_ms = currentTimestamp;
    }
  }
  else if (currOnBack) {
    if (_awaitingConfirmationTreadState != OffTreadsState::OnBack) {
      // Transition to Robot on Back
      _awaitingConfirmationTreadState = OffTreadsState::OnBack;
      // On Back is a special case as it is also an intermediate state for coming from onFace -> onTreads. hence we wait a little longer than usual(kRobotTimeToConsiderOfftreads_ms) to check if it's on back.
      _timeOffTreadStateChanged_ms = currentTimestamp + kRobotTimeToConsiderOfftreadsOnBack_ms;
    }
  }
  else if (currOnTreads) {
    if (_awaitingConfirmationTreadState != OffTreadsState::InAir
        && _awaitingConfirmationTreadState != OffTreadsState::OnTreads)
    {
      _awaitingConfirmationTreadState = OffTreadsState::InAir;
      _timeOffTreadStateChanged_ms = currentTimestamp;
    }
  }// end if(isFalling)

  /////
  // Message based tread state transitions
  ////

  // Transition from ontreads to InAir - happens instantly
  if (_awaitingConfirmationTreadState == OffTreadsState::OnTreads && isPickedUp == true) {
    // Robot is being picked up from not being picked up, notify systems
    _awaitingConfirmationTreadState = OffTreadsState::InAir;
    // Allows this to be called instantly
    _timeOffTreadStateChanged_ms = currentTimestamp - kRobotTimeToConsiderOfftreads_ms;
  }

  // Transition from inAir to Ontreads
  // there is a delay for the cliff sensor to confirm the robot is no longer picked up
  if (_awaitingConfirmationTreadState != OffTreadsState::OnTreads && isPickedUp != true
      && !currOnBack && !currOnSide && !currFacePlant) {
    _awaitingConfirmationTreadState = OffTreadsState::OnTreads;
    // Allows this to be called instantly
    _timeOffTreadStateChanged_ms = currentTimestamp - kRobotTimeToConsiderOfftreads_ms;
  }

  //////////
  // A new tread state has been confirmed
  //////////
  bool offTreadsStateChanged = false;
  if (_timeOffTreadStateChanged_ms + kRobotTimeToConsiderOfftreads_ms <= currentTimestamp
     && _offTreadsState != _awaitingConfirmationTreadState)
  {
    if (kUseVisionOnlyWhileOnTreads && _offTreadsState == OffTreadsState::OnTreads)
    {
      // Pause vision if we just left treads
      GetVisionComponent().Pause(true);
    }

    // Falling seems worthy of a DAS event
    if (_awaitingConfirmationTreadState == OffTreadsState::Falling) {
      _fallingStartedTime_ms = GetLastMsgTimestamp();
      LOG_INFO("Robot.CheckAndUpdateTreadsState.FallingStarted",
                       "t=%dms",
                       (TimeStamp_t)_fallingStartedTime_ms);

      // Stop all actions
      GetActionList().Cancel();

    } else if (_offTreadsState == OffTreadsState::Falling) {
      // This is not an exact measurement of fall time since it includes some detection delays on the robot side
      // It may also include kRobotTimeToConsiderOfftreads_ms depending on how the robot lands
      LOG_INFO("Robot.CheckAndUpdateTreadsState.FallingStopped",
                       "t=%dms, duration=%dms",
                       (TimeStamp_t)GetLastMsgTimestamp(), (TimeStamp_t)(GetLastMsgTimestamp() - _fallingStartedTime_ms));
      _fallingStartedTime_ms = 0;
    }

    _offTreadsState = _awaitingConfirmationTreadState;
    Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotOffTreadsStateChanged(_offTreadsState)));

    LOG_INFO("Robot.OfftreadsState.TreadStateChanged", "TreadState changed to:%s", EnumToString(_offTreadsState));

    // Special case logic for returning to treads
    if (_offTreadsState == OffTreadsState::OnTreads){

      if(kUseVisionOnlyWhileOnTreads)
      {
        // Re-enable vision if we've returned to treads
        GetVisionComponent().Pause(false);
      }

      // If we are not localized and there is nothing else left in the world (in any origin) that we could localize to,
      // then go ahead and mark us as localized (via odometry alone)
      if (!GetLocalizationComponent().IsLocalized() &&
          !GetBlockWorld().AnyRemainingLocalizableObjects(PoseOriginList::UnknownOriginID)) {
        LOG_INFO("Robot.UpdateOfftreadsState.NoMoreRemainingLocalizableObjects",
                 "Marking previously-unlocalized robot as localized to odometry because "
                 "there are no more objects to localize to in the world.");
        GetLocalizationComponent().SetLocalizedTo(nullptr); // marks us as localized to odometry only
      }
    }
    else if (GetCarryingComponent().IsCarryingObject() &&
            _offTreadsState != OffTreadsState::InAir)
    {
      // If we're falling or not upright and were carrying something, assume we
      // are no longer carrying that something and don't know where it is anymore
      const bool clearObjects = true; // To mark as Unknown, not just Dirty
      GetCarryingComponent().SetCarriedObjectAsUnattached(clearObjects);
    }

    offTreadsStateChanged = true;
  }

  //////////////////////////////////
  // Too long InAir DAS message
  //////////////////////////////////
  // Check if robot is stuck in-air for a long time, but is likely not being held.
  // Might be indicative of a vibrating surface or overly sensitive conditions
  // for staying in the picked up state.
  static bool        reportedInAirTooLong      = false;
  static EngineTimeStamp_t inAirTooLongReportTime_ms = 0;
  static Radians     lastStableRobotAngle_rad  = msg.pose.angle;

  if (!reportedInAirTooLong) {

    // Schedule reporting of DAS message when InAir, but reset
    // timer if robot orientation changes indicating that it's probably
    // still being held.
    const bool robotAngleChanged = (lastStableRobotAngle_rad - msg.pose.angle).getAbsoluteVal().ToFloat() > kRobotAngleChangedThresh_rad;
    if ((_offTreadsState == OffTreadsState::InAir) && robotAngleChanged) {
      inAirTooLongReportTime_ms = currentTimestamp + kInAirTooLongTimeReportTime_ms;
      lastStableRobotAngle_rad = msg.pose.angle;
    }

    if ((inAirTooLongReportTime_ms > 0) &&
        (inAirTooLongReportTime_ms < currentTimestamp)) {
      DASMSG(robot_too_long_in_air, "robot.too_long_in_air",
            "Robot has been in InAir picked up state for too long. Vibrating surface?");
      DASMSG_SEND();
      reportedInAirTooLong = true;
    }
  }

  // Reset reporting flag when no longer picked up
  const bool cliffDetected = GetCliffSensorComponent().IsCliffDetected();
  if (_offTreadsState == OffTreadsState::OnTreads || cliffDetected) {
    reportedInAirTooLong = false;
    inAirTooLongReportTime_ms = 0;
  }

  return offTreadsStateChanged;
}

const Util::RandomGenerator& Robot::GetRNG() const
{
  return *GetContext()->GetRandom();
}

Util::RandomGenerator& Robot::GetRNG()
{
  return *GetContext()->GetRandom();
}

const Pose3d& Robot::GetWorldOrigin() const
{
  return GetPoseOriginList().GetCurrentOrigin();
}

PoseOriginID_t Robot::GetWorldOriginID() const
{
  return GetPoseOriginList().GetCurrentOriginID();
}

bool Robot::IsPoseInWorldOrigin(const Pose3d& pose) const
{
  return GetPoseOriginList().IsPoseInCurrentOrigin(pose);
}


// Example update call for animating color image to face
void UpdateFaceImageRGBExample(Robot& robot)
{
  static Point2f pos(5,5);
  static Vision::PixelRGB backgroundPixel(10,10,10);
  static int framesToSend = 0;  // 0 == send frames forever

  // Frame send counter
  if (framesToSend > 0) {
    if (--framesToSend < 0) {
      return;
    }
  }

  // Throttle frames
  // Don't send if the number of procAnim keyframes gets large enough
  // (One keyframe == 33ms)
  if (robot.GetAnimationComponent().GetAnimState_NumProcAnimFaceKeyframes() > 30) {
    return;
  }

  // Move 'X' through the image
  const f32 xStep = 5.f;
  pos.x() += xStep;
  if (pos.x() >= FACE_DISPLAY_WIDTH - 1) {
    pos.x() = 0;
    pos.y() += 1.f;
    if (pos.y() >= FACE_DISPLAY_HEIGHT - 1) {
      pos.x() = 0;
      pos.y() = 0;
    }
  }

  // Update background color
  // Increase R, increase G, increase B, decrease R, decrease G, decrease B
  const u8 highVal = 230;
  const u8 lowVal  = 30;
  const u8 step    = 10;
  static bool goingUp = true;
  if (goingUp) {
    if (backgroundPixel.r() < highVal) {
      backgroundPixel.r() += step;
    } else if (backgroundPixel.g() < highVal) {
      backgroundPixel.g() += step;
    } else if (backgroundPixel.b() < highVal) {
      backgroundPixel.b() += step;
    } else {
      goingUp = false;
    }
  } else {
    if (backgroundPixel.r() > lowVal) {
      backgroundPixel.r() -= step;
    } else if (backgroundPixel.g() > lowVal) {
      backgroundPixel.g() -= step;
    } else if (backgroundPixel.b() > lowVal) {
      backgroundPixel.b() -= step;
    } else {
      goingUp = true;
    }
  }

  static Vision::ImageRGB img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img.FillWith(backgroundPixel);
  img.DrawText(pos, "x", ColorRGBA(0xff), 0.5f);

  // std::ostringstream ss;
  // ss << "files/images/img_" << GetLastMsgTimestamp() << ".jpg";
  // img.Save(ss.str().c_str());

  // The duration of the image should ideally be some multiple of ANIM_TIME_STEP_MS,
  // especially if you're playing a bunch of images in sequence, otherwise the
  // speed of the animation may not be as expected.
  u32 duration_ms = 2 * ANIM_TIME_STEP_MS;
  robot.GetAnimationComponent().DisplayFaceImage(img, duration_ms);
}

Result Robot::UpdateFullRobotState(const RobotState& msg)
{
  ANKI_CPU_PROFILE("Robot::UpdateFullRobotState");

  Result lastResult = RESULT_OK;

  // Ignore state messages received before sync
  if (!_syncRobotAcked) {
    return lastResult;
  }

  if (kEnableTestFaceImageRGBDrawing) {
    // Example update function for animating to face
    UpdateFaceImageRGBExample(*this);
  }

  _gotStateMsgAfterRobotSync = true;

  // Set flag indicating that robot state messages have been received
  _lastMsgTimestamp = msg.timestamp;
  _newStateMsgAvailable = true;

  // Update head angle
  SetHeadAngle(msg.headAngle);

  // Update lift angle
  SetLiftAngle(msg.liftAngle);

  // Update robot pitch angle
  GetComponent<FullRobotPose>().SetPitchAngle(Radians(msg.pose.pitch_angle));

  // Update robot roll angle
  GetComponent<FullRobotPose>().SetRollAngle(Radians(msg.pose.roll_angle));

  // Update IMU data
  _robotAccel = msg.accel;
  _robotGyro = msg.gyro;

  for (auto imuDataFrame : msg.imuData) {
    if (imuDataFrame.timestamp > 0) {
      GetImuComponent().AddData(std::move(imuDataFrame));
    }
  }

  _robotAccelMagnitude = sqrtf(_robotAccel.x * _robotAccel.x
                             + _robotAccel.y * _robotAccel.y
                             + _robotAccel.z * _robotAccel.z);

  const float kAccelMagFilterConstant = 0.95f; // between 0 and 1
  _robotAccelMagnitudeFiltered = (kAccelMagFilterConstant * _robotAccelMagnitudeFiltered)
                              + ((1.0f - kAccelMagFilterConstant) * _robotAccelMagnitude);

  const float kAccelFilterConstant = 0.90f; // between 0 and 1
  _robotAccelFiltered.x = (kAccelFilterConstant * _robotAccelFiltered.x)
                        + ((1.0f - kAccelFilterConstant) * msg.accel.x);
  _robotAccelFiltered.y = (kAccelFilterConstant * _robotAccelFiltered.y)
                        + ((1.0f - kAccelFilterConstant) * msg.accel.y);
  _robotAccelFiltered.z = (kAccelFilterConstant * _robotAccelFiltered.z)
                        + ((1.0f - kAccelFilterConstant) * msg.accel.z);

  // Update cozmo's internal offTreadsState knowledge
  const OffTreadsState prevOffTreadsState = _offTreadsState;
  const bool wasTreadsStateUpdated = CheckAndUpdateTreadsState(msg);
  const bool isDelocalizing = wasTreadsStateUpdated && (prevOffTreadsState == OffTreadsState::OnTreads || _offTreadsState == OffTreadsState::OnTreads);

  if( isDelocalizing && (prevOffTreadsState == OffTreadsState::OnTreads) )
  {
    // Robot is delocalized, and not because it was put back down. Tell the map component to send relevant info about the
    // previous map. This is commanded here instead of map component's CreateLocalizedMemoryMap so that the map info from when
    // the robot is in the air is not sent when the robot is put back down, since MapComponent doesn't (need to) track OffTreadsState
    GetMapComponent().SendDASInfoAboutCurrentMap();
  }

  if (wasTreadsStateUpdated)
  {
    DASMSG(robot_offtreadsstatechanged, "robot.offtreadsstatechanged", "The robot off treads state changed");
    DASMSG_SET(s1, OffTreadsStateToString(_offTreadsState), "New state");
    DASMSG_SET(s2, OffTreadsStateToString(prevOffTreadsState), "Previous state");
    DASMSG_SEND();
  }

  // this flag can have a small delay with respect to when we actually picked up the block, since Engine notifies
  // the robot, and the robot updates on the next state update. But that delay guarantees that the robot knows what
  // we think it's true, rather than mixing timestamps of when it started carrying vs when the robot knows that it was
  // carrying
  //robot->SetCarryingBlock( isCarryingObject ); // Still needed?
  GetDockingComponent().SetPickingOrPlacing(IS_STATUS_FLAG_SET(IS_PICKING_OR_PLACING));
  _isPickedUp = IS_STATUS_FLAG_SET(IS_PICKED_UP);
  const bool wasBeingHeld = _isBeingHeld;
  _isBeingHeld = IS_STATUS_FLAG_SET(IS_BEING_HELD);
  if ( wasBeingHeld != _isBeingHeld ) {
    _timeHeldStateChanged_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }
  _powerButtonPressed = IS_STATUS_FLAG_SET(IS_BUTTON_PRESSED);

  const bool isHeadMoving = !IS_STATUS_FLAG_SET(HEAD_IN_POS);
  const bool areWheelsMoving = IS_STATUS_FLAG_SET(ARE_WHEELS_MOVING);
  GetLocalizationComponent().SetRobotHasMoved(isHeadMoving || areWheelsMoving || _offTreadsState != OffTreadsState::OnTreads);
  
  // Save the entire flag for sending to game
  _lastStatusFlags = msg.status;

  _leftWheelSpeed_mmps = msg.lwheel_speed_mmps;
  _rightWheelSpeed_mmps = msg.rwheel_speed_mmps;

  if (isDelocalizing) {
    GetLocalizationComponent().Delocalize();
  } else {
    auto localizationResult = GetLocalizationComponent().NotifyOfRobotState(msg);
    if (localizationResult != RESULT_OK) {
      return localizationResult;
    }
  }

  // Update sensor components:
  GetBatteryComponent().NotifyOfRobotState(msg);
  GetMoveComponent().NotifyOfRobotState(msg);
  GetCliffSensorComponent().NotifyOfRobotState(msg);
  GetProxSensorComponent().NotifyOfRobotState(msg);
  GetTouchSensorComponent().NotifyOfRobotState(msg);

  // Update processed proxSensorData in history after ProxSensorComponent was updated
  GetStateHistory()->UpdateProxSensorData(msg.timestamp, GetProxSensorComponent().GetLatestProxData());

  // update current path segment in the path component
  GetPathComponent().UpdateCurrentPathSegment(msg.currPathSegment);


# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
  GetComponent<RobotGyroDriftDetector>().DetectGyroDrift(msg);
# pragma clang diagnostic pop
  GetComponent<RobotGyroDriftDetector>().DetectBias(msg);

  /*
    LOG_INFO("Robot.UpdateFullRobotState.OdometryUpdate",
    "Robot %d's pose updated to (%.3f, %.3f, %.3f) @ %.1fdeg based on "
    "msg at time=%d, frame=%d saying (%.3f, %.3f) @ %.1fdeg\n",
    _ID, GetComponent<FullRobotPose>().GetPose().GetTranslation().x(), GetComponent<FullRobotPose>().GetPose().GetTranslation().y(), GetComponent<FullRobotPose>().GetPose().GetTranslation().z(),
    GetComponent<FullRobotPose>().GetPose().GetRotationAngle<'Z'>().getDegrees(),
    msg.timestamp, msg.pose_frame_id,
    msg.pose_x, msg.pose_y, msg.pose_angle*180.f/M_PI);
  */

  // Engine modifications to state message.
  // TODO: Should this just be a different message? Or one that includes the state message from the robot?
  RobotState stateMsg(msg);

  const u16 imageFramePeriod_ms = Util::numeric_cast<u16>( GetVisionComponent().GetFramePeriod_ms() );
  const u16 imageProcPeriod_ms  = Util::numeric_cast<u16>( GetVisionComponent().GetProcessingPeriod_ms() );

  // Send state to visualizer for displaying
  VizInterface::RobotStateMessage vizState(stateMsg,
                                           _robotImuTemperature_degC,
                                           GetAnimationComponent().GetAnimState_NumProcAnimFaceKeyframes(),
                                           GetCliffSensorComponent().GetCliffDetectThresholds(),
                                           imageFramePeriod_ms,
                                           imageProcPeriod_ms,
                                           GetMoveComponent().GetLockedTracks(),
                                           GetAnimationComponent().GetAnimState_TracksInUse(),
                                           GetBatteryComponent().GetBatteryVolts(),
                                           _offTreadsState,
                                           _awaitingConfirmationTreadState);
  GetContext()->GetVizManager()->SendRobotState(std::move(vizState));

  return lastResult;

} // UpdateFullRobotState()

bool Robot::HasReceivedRobotState() const
{
  return _newStateMsgAvailable;
}

Result Robot::GetHistoricalCamera(RobotTimeStamp_t t_request, Vision::Camera& camera) const
{
  HistRobotState histState;
  RobotTimeStamp_t t;
  Result result = GetStateHistory()->GetRawStateAt(t_request, t, histState);
  if (RESULT_OK != result)
  {
    return result;
  }

  camera = GetHistoricalCamera(histState, t);
  return RESULT_OK;
}

Pose3d Robot::GetHistoricalCameraPose(const HistRobotState& histState, RobotTimeStamp_t t) const
{
  // Compute pose from robot body to camera
  // Start with canonical (untilted) headPose
  Pose3d camPose(GetComponent<FullRobotPose>().GetHeadCamPose());

  // Rotate that by the given angle
  RotationVector3d Rvec(-histState.GetHeadAngle_rad(), Y_AXIS_3D());
  camPose.RotateBy(Rvec);

  // Precompose with robot body to neck pose
  camPose.PreComposeWith(GetComponent<FullRobotPose>().GetNeckPose());

  // Set parent pose to be the historical robot pose
  camPose.SetParent(histState.GetPose());

  camPose.SetName("PoseHistoryCamera_" + std::to_string((TimeStamp_t)t));

  return camPose;
}

//
// Return constant display parameters for Cozmo v1.0.
// Future hardware may support different values.
u32 Robot::GetDisplayWidthInPixels() const
{
  return FACE_DISPLAY_WIDTH;
}

u32 Robot::GetDisplayHeightInPixels() const
{
  return FACE_DISPLAY_HEIGHT;
}

Vision::Camera Robot::GetHistoricalCamera(const HistRobotState& histState, RobotTimeStamp_t t) const
{
  Vision::Camera camera(GetVisionComponent().GetCamera());

  // Update the head camera's pose
  camera.SetPose(GetHistoricalCameraPose(histState, t));

  return camera;
}


Result Robot::Update()
{
  ANKI_CPU_PROFILE("Robot::Update");

  _cpuStats.Update();

  //////////// CameraService Update ////////////
  CameraService::getInstance()->Update();

  auto* tof = ToFSensor::getInstance();
  if(tof != nullptr)
  {
    tof->Update();
  }

  Result factoryRes;
  const bool checkDone = UpdateStartupChecks(factoryRes);
  if(!checkDone)
  {
    return RESULT_OK;
  }
  else if(factoryRes != RESULT_OK)
  {
    return factoryRes;
  }

  if (!_gotStateMsgAfterRobotSync)
  {
    LOG_DEBUG("Robot.Update", "Waiting for first full robot state to be handled");
    return RESULT_OK;
  }

  const bool trackingPowerButtonPress = (_timePowerButtonPressed_ms != 0);
  // Keep track of how long the power button has been pressed
  if(!trackingPowerButtonPress && _powerButtonPressed){
    _timePowerButtonPressed_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  }else if(trackingPowerButtonPress && !_powerButtonPressed){
    _timePowerButtonPressed_ms = 0;
  }

#if(0)
  ActiveBlockLightTest(1);
  return RESULT_OK;
#endif

  _components->UpdateComponents();

#if ENABLE_DRAWING
  /////////// Update visualization ////////////
  ANKI_CPU_PROFILE_START(prof_UpdateVis, "UpdateVisualization");

  // Draw All Objects by calling their Visualize() methods.
  GetBlockWorld().DrawAllObjects();

  // Always draw robot w.r.t. the origin, not in its current frame
  Pose3d robotPoseWrtOrigin = GetPose().GetWithRespectToRoot();

  // Triangle pose marker
  GetContext()->GetVizManager()->DrawRobot(GetID(), robotPoseWrtOrigin);

  // Full Webots CozmoBot model
  if (IsPhysical()) {
    GetContext()->GetVizManager()->DrawRobot(robotPoseWrtOrigin, GetComponent<FullRobotPose>().GetHeadAngle(), GetComponent<FullRobotPose>().GetLiftAngle());
  }

  // Robot bounding box
  static const ColorRGBA ROBOT_BOUNDING_QUAD_COLOR(0.0f, 0.8f, 0.0f, 0.75f);

  using namespace Quad;
  Quad2f quadOnGround2d = GetBoundingQuadXY(robotPoseWrtOrigin);
  const f32 zHeight = robotPoseWrtOrigin.GetTranslation().z() + WHEEL_RAD_TO_MM;
  Quad3f quadOnGround3d(Point3f(quadOnGround2d[TopLeft].x(),     quadOnGround2d[TopLeft].y(),     zHeight),
                        Point3f(quadOnGround2d[BottomLeft].x(),  quadOnGround2d[BottomLeft].y(),  zHeight),
                        Point3f(quadOnGround2d[TopRight].x(),    quadOnGround2d[TopRight].y(),    zHeight),
                        Point3f(quadOnGround2d[BottomRight].x(), quadOnGround2d[BottomRight].y(), zHeight));

  GetContext()->GetVizManager()->DrawRobotBoundingBox(GetID(), quadOnGround3d, ROBOT_BOUNDING_QUAD_COLOR);

  /*
  // Draw 3d bounding box
  Vec3f vizTranslation = GetPose().GetTranslation();
  vizTranslation.z() += 0.5f*ROBOT_BOUNDING_Z;
  Pose3d vizPose(GetPose().GetRotation(), vizTranslation);

  GetContext()->GetVizManager()->DrawCuboid(999, {ROBOT_BOUNDING_X, ROBOT_BOUNDING_Y, ROBOT_BOUNDING_Z},
  vizPose, ROBOT_BOUNDING_QUAD_COLOR);
  */

  if (kDebugPossibleBlockInteraction) {
    // print a bunch of info helpful for debugging block states
    BlockWorldFilter filter;
    filter.SetFilterFcn(&BlockWorldFilter::IsLightCubeFilter);
    std::vector<ObservableObject*> matchingObjects;
    GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects); // note this doesn't retrieve unknowns anymore
    for( const auto obj : matchingObjects ) {
        Pose3d relPose;
        bool gotRelPose __attribute__((unused)) =
            obj->GetPose().GetWithRespectTo(GetPose(), relPose);

        const char* axisStr = "";
        switch( obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() ) {
          case AxisName::X_POS: axisStr="+X"; break;
          case AxisName::X_NEG: axisStr="-X"; break;
          case AxisName::Y_POS: axisStr="+Y"; break;
          case AxisName::Y_NEG: axisStr="-Y"; break;
          case AxisName::Z_POS: axisStr="+Z"; break;
          case AxisName::Z_NEG: axisStr="-Z"; break;
        }

        LOG_DEBUG("Robot.ObjectInteractionState",
                  "block:%d poseState:%8s moving?%d RestingFlat?%d carried?%d poseWRT?%d"
                  " z=%6.2f UpAxis:%s CanStack?%d CanPickUp?%d FromGround?%d",
                  obj->GetID().GetValue(),
                  PoseStateToString( obj->GetPoseState() ),
                  obj->IsMoving(),
                  obj->IsRestingFlat(),
                  (GetCarryingComponent().IsCarryingObject() && GetCarryingComponent().
                   GetCarryingObjectID() == obj->GetID()),
                  gotRelPose,
                  relPose.GetTranslation().z(),
                  axisStr,
                  GetDockingComponent().CanStackOnTopOfObject(*obj),
                  GetDockingComponent().CanPickUpObject(*obj),
                  GetDockingComponent().CanPickUpObjectFromGround(*obj));
    }
  }
  ANKI_CPU_PROFILE_STOP(prof_UpdateVis);
#endif  // ENABLE_DRAWING

  // Send a message indicating we are fully loaded and capable of running
  // after the first tick
  if(!_sentEngineLoadedMsg)
  {
    _sentEngineLoadedMsg = true;
    SendRobotMessage<RobotInterface::EngineFullyLoaded>();

    auto onCharger  = GetBatteryComponent().IsOnChargerContacts() ? 1 : 0;
    auto battery_mV = static_cast<uint32_t>(GetBatteryComponent().GetBatteryVolts() * 1000);

    LOG_INFO("Robot.Update.EngineFullyLoaded",
             "OnCharger: %d, Battery_mV: %d",
             onCharger, battery_mV);

    DASMSG(robot_engine_ready, "robot.engine_ready", "All robot processes are ready");
    DASMSG_SET(i1, onCharger,  "On charger status");
    DASMSG_SET(i2, battery_mV, "Battery voltage (mV)");
    DASMSG_SEND();
  }

  return RESULT_OK;

} // Update()

static f32 ClipHeadAngle(f32 head_angle)
{
  if (head_angle < MIN_HEAD_ANGLE - HEAD_ANGLE_LIMIT_MARGIN) {
    //LOG_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too small.\n", head_angle);
    return MIN_HEAD_ANGLE;
  }
  else if (head_angle > MAX_HEAD_ANGLE + HEAD_ANGLE_LIMIT_MARGIN) {
    //LOG_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too large.\n", head_angle);
    return MAX_HEAD_ANGLE;
  }

  return head_angle;

} // ClipHeadAngle()

Pose3d Robot::GetCameraPose(const f32 atAngle) const
{
  // Start with canonical (untilted) headPose
  Pose3d newHeadPose(GetComponent<FullRobotPose>().GetHeadCamPose());

  // Rotate that by the given angle
  RotationVector3d Rvec(-atAngle, Y_AXIS_3D());
  newHeadPose.RotateBy(Rvec);
  newHeadPose.SetName("Camera");

  return newHeadPose;
} // GetCameraPose()

void Robot::SetHeadAngle(const f32& angle)
{
  if (_isHeadCalibrated) {
    f32 clippedHeadAngle = ClipHeadAngle(angle);
    GetComponent<FullRobotPose>().SetHeadAngle(clippedHeadAngle);
    GetVisionComponent().GetCamera().SetPose(GetCameraPose(GetComponent<FullRobotPose>().GetHeadAngle()));
    if(clippedHeadAngle != angle){
      LOG_WARNING("Robot.GetCameraHeadPose.HeadAngleOOB",
                  "Angle %.3frad / %.1f",
                  angle, RAD_TO_DEG(angle));
    }
  }

  // note: moving the motor inside bounds shouldn't erase previous state
  _isHeadMotorOutOfBounds |= angle < (MIN_HEAD_ANGLE-HEAD_ANGLE_LIMIT_MARGIN) ||
                             angle > (MAX_HEAD_ANGLE+HEAD_ANGLE_LIMIT_MARGIN);

} // SetHeadAngle()


void Robot::SetHeadCalibrated(bool isCalibrated)
{
  _isHeadCalibrated = isCalibrated;
  if(_isHeadCalibrated) {
    // clears the out of bounds flag when set to calibrated
    _isHeadMotorOutOfBounds = false;
  }
}

void Robot::SetLiftCalibrated(bool isCalibrated)
{
  _isLiftCalibrated = isCalibrated;
  if(_isLiftCalibrated) {
    // clears the out of bounds flag when set to calibrated
    _isLiftMotorOutOfBounds = false;
  }
}

bool Robot::IsHeadCalibrated() const
{
  return _isHeadCalibrated;
}

bool Robot::IsLiftCalibrated() const
{
  return _isLiftCalibrated;
}

void Robot::ComputeLiftPose(const f32 atAngle, Pose3d& liftPose)
{
  // Reset to canonical position
  liftPose.SetRotation(atAngle, Y_AXIS_3D());
  liftPose.SetTranslation({LIFT_ARM_LENGTH, 0.f, 0.f});

  // Rotate to the given angle
  RotationVector3d Rvec(-atAngle, Y_AXIS_3D());
  liftPose.RotateBy(Rvec);
}

void Robot::SetLiftAngle(const f32& angle)
{
  // note: moving the motor inside bounds shouldn't erase previous state
  _isLiftMotorOutOfBounds |=  angle < (MIN_LIFT_ANGLE-LIFT_ANGLE_LIMIT_MARGIN) ||
                              angle > (MAX_LIFT_ANGLE+LIFT_ANGLE_LIMIT_MARGIN);

  // TODO: Add lift angle limits?
  GetComponent<FullRobotPose>().SetLiftAngle(angle);

  Robot::ComputeLiftPose(GetComponent<FullRobotPose>().GetLiftAngle(), GetComponent<FullRobotPose>().GetLiftPose());

  DEV_ASSERT(GetComponent<FullRobotPose>().GetLiftPose().IsChildOf(GetComponent<FullRobotPose>().GetLiftBasePose()), "Robot.SetLiftAngle.InvalidPose");
}

Radians Robot::GetPitchAngle() const
{
  return GetComponent<FullRobotPose>().GetPitchAngle();
}

Radians Robot::GetRollAngle() const
{
  return GetComponent<FullRobotPose>().GetRollAngle();
}

bool Robot::WasObjectTappedRecently(const ObjectID& objectID) const
{
  return GetComponent<BlockTapFilterComponent>().ShouldIgnoreMovementDueToDoubleTap(objectID);
}

TimeStamp_t Robot::GetTimeSincePowerButtonPressed_ms() const {
  // this is a time difference, so could be any type, but to avoid someone thinking that all
  // power button times are robot times, this returns an engine timestmap to match _timePowerButtonPressed_ms
  return _timePowerButtonPressed_ms == 0 ? 0 :
          TimeStamp_t(BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _timePowerButtonPressed_ms);
}

void Robot::HandlePokeEvent() {
  _timeLastPoked = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  LOG_INFO("Robot.HandlePokeEvent", "Last poke event timestamp recorded as %u", static_cast<u32>(_timeLastPoked));
}

EngineTimeStamp_t Robot::GetTimeSinceLastPoke_ms() const {
  // If the robot has never reported being poked, then set the time difference to the maximum allowable value
  return _timeLastPoked == 0 ? std::numeric_limits<EngineTimeStamp_t>::max() :
          TimeStamp_t(BaseStationTimer::getInstance()->GetCurrentTimeStamp() - _timeLastPoked);
}

Result Robot::SyncRobot()
{
  _syncRobotAcked = false;
  GetStateHistory()->Clear();

  Result res = SendSyncRobot();
  if (res == RESULT_OK) {
    _syncRobotSentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  return res;
}

// ============ Messaging ================

Result Robot::SendMessage(const RobotInterface::EngineToRobot& msg, bool reliable, bool hot) const
{
  static Util::MessageProfiler msgProfiler("Robot::SendMessage");

  Result sendResult = GetContext()->GetRobotManager()->GetMsgHandler()->SendMessage(msg, reliable, hot);
  if (sendResult == RESULT_OK) {
    msgProfiler.Update((int)msg.GetTag(), msg.Size());
  } else {
    const char* msgTypeName = EngineToRobotTagToString(msg.GetTag());
    LOG_WARNING("Robot.SendMessage", "Robot %d failed to send a message type %s", _ID, msgTypeName);
    msgProfiler.ReportOnFailure();
  }
  return sendResult;
}

// Sync with physical robot
Result Robot::SendSyncRobot() const
{
  Result result = SendMessage(RobotInterface::EngineToRobot(RobotInterface::SyncRobot()));

  if (result == RESULT_OK) {
    // Reset pose on connect
    LOG_INFO("Robot.SendSyncRobot", "Setting pose to (0,0,0)");
    Pose3d zeroPose(0, Z_AXIS_3D(), {0,0,0}, GetWorldOrigin());
    return GetLocalizationComponent().SendAbsLocalizationUpdate(zeroPose, 0, GetPoseFrameID());
  }

  if (result != RESULT_OK) {
    LOG_WARNING("Robot.SendSyncRobot.FailedToSend","");
  }

  return result;
}

Result Robot::SendIMURequest(const u32 length_ms) const
{
  return SendRobotMessage<IMURequest>(length_ms);
}


bool Robot::HasExternalInterface() const
{
  if (HasComponent<ContextWrapper>()){
    return GetContext()->GetExternalInterface() != nullptr;
  }
  return false;
}

IExternalInterface* Robot::GetExternalInterface() const
{
  DEV_ASSERT(GetContext()->GetExternalInterface() != nullptr, "Robot.ExternalInterface.nullptr");
  return GetContext()->GetExternalInterface();
}

bool Robot::HasGatewayInterface() const
{
  if (HasComponent<ContextWrapper>()){
    return GetContext()->GetGatewayInterface() != nullptr;
  }
  return false;
}

IGatewayInterface* Robot::GetGatewayInterface() const
{
  DEV_ASSERT(GetContext()->GetGatewayInterface() != nullptr, "Robot.GatewayInterface.nullptr");
  return GetContext()->GetGatewayInterface();
}

Util::Data::DataPlatform* Robot::GetContextDataPlatform()
{
  return GetContext()->GetDataPlatform();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Message handlers subscribed to in RobotToEngineImplMessaging::InitRobotMessageComponent

template<>
void Robot::HandleMessage(const ExternalInterface::RequestRobotSettings& msg)
{
  const VisionComponent& visionComponent = GetVisionComponent();
  std::shared_ptr<Vision::CameraCalibration> cameraCalibration;

  cameraCalibration = visionComponent.GetCameraCalibration();

  if (cameraCalibration == nullptr)
  {
    LOG_WARNING("Robot.HandleRequestRobotSettings.CameraNotCalibrated", "");
    cameraCalibration = std::make_shared<Vision::CameraCalibration>(0,0,1.f,1.f,0.f,0.f);
  }

  ExternalInterface::CameraConfig cameraConfig(cameraCalibration->GetFocalLength_x(),
                                               cameraCalibration->GetFocalLength_y(),
                                               cameraCalibration->GetCenter_x(),
                                               cameraCalibration->GetCenter_y(),
                                               cameraCalibration->ComputeHorizontalFOV().getDegrees(),
                                               cameraCalibration->ComputeVerticalFOV().getDegrees(),
                                               visionComponent.GetMinCameraExposureTime_ms(),
                                               visionComponent.GetMaxCameraExposureTime_ms(),
                                               visionComponent.GetMinCameraGain(),
                                               visionComponent.GetMaxCameraGain());

  ExternalInterface::PerRobotSettings robotSettings(GetHeadSerialNumber(),
                                                    std::move(cameraConfig));

  Broadcast( ExternalInterface::MessageEngineToGame(std::move(robotSettings)) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotTimeStamp_t Robot::GetLastImageTimeStamp() const {
  return GetVisionComponent().GetLastProcessedImageTimeStamp();
}

/*
  const Pose3d Robot::ProxDetectTransform[] = { Pose3d(0, Z_AXIS_3D(), Vec3f(50, 25, 0)),
  Pose3d(0, Z_AXIS_3D(), Vec3f(50, 0, 0)),
  Pose3d(0, Z_AXIS_3D(), Vec3f(50, -25, 0)) };
*/


Quad2f Robot::GetBoundingQuadXY(const f32 padding_mm) const
{
  return GetBoundingQuadXY(GetComponent<FullRobotPose>().GetPose(), padding_mm);
}

Quad2f Robot::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm)
{
  const RotationMatrix2d R(atPose.GetRotation().GetAngleAroundZaxis());

  static const Quad2f CanonicalBoundingBoxXY(
    {ROBOT_BOUNDING_X_FRONT, -0.5f*ROBOT_BOUNDING_Y},
    {ROBOT_BOUNDING_X_FRONT,  0.5f*ROBOT_BOUNDING_Y},
    {ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X, -0.5f*ROBOT_BOUNDING_Y},
    {ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X,  0.5f*ROBOT_BOUNDING_Y});

  Quad2f boundingQuad(CanonicalBoundingBoxXY);
  if(padding_mm != 0.f) {
    Quad2f paddingQuad({ padding_mm, -padding_mm},
                       { padding_mm,  padding_mm},
                       {-padding_mm, -padding_mm},
                       {-padding_mm,  padding_mm});
    boundingQuad += paddingQuad;
  }

  using namespace Quad;
  for (CornerName iCorner = FirstCorner; iCorner < NumCorners; ++iCorner) {
    // Rotate to given pose
    boundingQuad[iCorner] = R * boundingQuad[iCorner];
  }

  // Re-center
  Point2f center(atPose.GetTranslation().x(), atPose.GetTranslation().y());
  boundingQuad += center;

  return boundingQuad;

} // GetBoundingQuadXY()

f32 Robot::GetHeight() const
{
  return std::max(ROBOT_BOUNDING_Z, GetLiftHeight() + LIFT_HEIGHT_ABOVE_GRIPPER);
}

f32 Robot::GetLiftHeight() const
{
  return ConvertLiftAngleToLiftHeightMM(GetComponent<FullRobotPose>().GetLiftAngle());
}

Transform3d Robot::GetLiftTransformWrtCamera(const f32 atLiftAngle, const f32 atHeadAngle) const
{
  Pose3d liftPose(GetComponent<FullRobotPose>().GetLiftPose());
  ComputeLiftPose(atLiftAngle, liftPose);

  Pose3d camPose = GetCameraPose(atHeadAngle);

  Pose3d liftPoseWrtCam;
  const bool result = liftPose.GetWithRespectTo(camPose, liftPoseWrtCam);

  DEV_ASSERT(result, "Robot.GetLiftTransformWrtCamera.LiftWrtCamPoseFailed");
# pragma unused(result)

  return liftPoseWrtCam.GetTransform();
}

OffTreadsState Robot::GetOffTreadsState() const
{
  return _offTreadsState;
}


Result Robot::RequestIMU(const u32 length_ms) const
{
  return SendIMURequest(length_ms);
}

// ============ Pose history ===============

Result Robot::GetComputedStateAt(const RobotTimeStamp_t t_request, Pose3d& pose) const
{
  HistStateKey histStateKey;
  const HistRobotState* histStatePtr = nullptr;
  Result lastResult = GetStateHistory()->GetComputedStateAt(t_request, &histStatePtr, &histStateKey);
  if (lastResult == RESULT_OK) {
    // Grab the pose stored in the pose stamp we just found, and hook up
    // its parent to the robot's current world origin (since pose history
    // doesn't keep track of pose parent chains)
    pose = histStatePtr->GetPose();
    pose.SetParent(GetWorldOrigin());
  }
  return lastResult;
}

Result Robot::AbortAll()
{
  bool anyFailures = false;

  GetActionList().Cancel();

  if (GetPathComponent().Abort() != RESULT_OK) {
    anyFailures = true;
  }

  if (GetDockingComponent().AbortDocking() != RESULT_OK) {
    anyFailures = true;
  }

  if (AbortAnimation() != RESULT_OK) {
    anyFailures = true;
  }

  GetMoveComponent().StopAllMotors();

  if (anyFailures) {
    return RESULT_FAIL;
  }
  return RESULT_OK;
}

Result Robot::AbortAnimation()
{
  return SendAbortAnimation();
}

Result Robot::SendAbortAnimation()
{
  return SendMessage(RobotInterface::EngineToRobot(RobotInterface::AbortAnimation()));
}

void Robot::ComputeDriveCenterPose(const Pose3d &robotPose, Pose3d &driveCenterPose) const
{
  MoveRobotPoseForward(robotPose, GetDriveCenterOffset(), driveCenterPose);
}

void Robot::ComputeOriginPose(const Pose3d &driveCenterPose, Pose3d &robotPose) const
{
  MoveRobotPoseForward(driveCenterPose, -GetDriveCenterOffset(), robotPose);
}

const PoseOriginList& Robot::GetPoseOriginList() const
{
  return GetLocalizationComponent().GetPoseOriginList();
}

const Pose3d& Robot::GetDriveCenterPose(void) const
{
  return GetLocalizationComponent().GetDriveCenterPose();
}

const PoseFrameID_t Robot::GetPoseFrameID()  const 
{ 
  return GetLocalizationComponent().GetPoseFrameID(); 
}

void Robot::MoveRobotPoseForward(const Pose3d &startPose, f32 distance, Pose3d &movedPose)
{
  movedPose = startPose;
  f32 angle = startPose.GetRotationAngle<'Z'>().ToFloat();
  Vec3f trans;
  trans.x() = startPose.GetTranslation().x() + distance * cosf(angle);
  trans.y() = startPose.GetTranslation().y() + distance * sinf(angle);
  movedPose.SetTranslation(trans);
}

f32 Robot::GetDriveCenterOffset() const
{
  f32 driveCenterOffset = DRIVE_CENTER_OFFSET;
  if (GetCarryingComponent().IsCarryingObject()) {
    driveCenterOffset = 0;
  }
  return driveCenterOffset;
}

bool Robot::Broadcast(ExternalInterface::MessageEngineToGame&& event)
{
  if (HasExternalInterface()) {
    GetExternalInterface()->Broadcast(event);
    return true;
  }
  return false;
}

bool Robot::Broadcast(VizInterface::MessageViz&& event)
{
  auto* vizMgr = GetContext()->GetVizManager();
  if (vizMgr != nullptr) {
    vizMgr->SendVizMessage(std::move(event));
    return true;
  }
  return false;
}

// @TODO: Refactor internal components to use proto state, and remove clad state utilities/messages VIC-4998
ExternalInterface::RobotState Robot::GetRobotState() const
{
  ExternalInterface::RobotState msg;

  msg.pose = GetPose().ToPoseStruct3d(GetPoseOriginList());
  if (msg.pose.originID == PoseOriginList::UnknownOriginID)
  {
    LOG_WARNING("Robot.GetRobotState.BadOriginID", "");
  }

  msg.poseAngle_rad = GetPose().GetRotationAngle<'Z'>().ToFloat();
  msg.posePitch_rad = GetPitchAngle().ToFloat();

  msg.leftWheelSpeed_mmps  = GetLeftWheelSpeed();
  msg.rightWheelSpeed_mmps = GetRightWheelSpeed();

  msg.headAngle_rad = GetComponent<FullRobotPose>().GetHeadAngle();
  msg.liftHeight_mm = GetLiftHeight();

  msg.accel = GetHeadAccelData();
  msg.gyro = GetHeadGyroData();

  msg.status = _lastStatusFlags;
  if (GetAnimationComponent().IsAnimating()) {
    msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING;
  }

  if (GetCarryingComponent().IsCarryingObject()) {
    msg.status |= (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK;
    msg.carryingObjectID = GetCarryingComponent().GetCarryingObjectID();
  } else {
    msg.carryingObjectID = -1;
  }
  msg.carryingObjectOnTopID = -1;

  msg.headTrackingObjectID = GetMoveComponent().GetTrackToObject();

  msg.localizedToObjectID = GetLocalizationComponent().GetLocalizedTo();

  msg.batteryVoltage = GetBatteryComponent().GetBatteryVolts();

  msg.lastImageTimeStamp = (TimeStamp_t)GetVisionComponent().GetLastProcessedImageTimeStamp();

  return msg;
}

external_interface::RobotState* Robot::GenerateRobotStateProto() const
{
  auto* msg = new external_interface::RobotState;

  auto srcPoseStruct = GetPose().ToPoseStruct3d(GetPoseOriginList());
  auto* dstPoseStruct = new external_interface::PoseStruct(
    srcPoseStruct.x, srcPoseStruct.y, srcPoseStruct.z,
    srcPoseStruct.q0, srcPoseStruct.q1, srcPoseStruct.q2, srcPoseStruct.q3,
    srcPoseStruct.originID);

  msg->set_allocated_pose(dstPoseStruct);

  if (srcPoseStruct.originID == PoseOriginList::UnknownOriginID)
  {
    LOG_WARNING("Robot.GetRobotStateProto.BadOriginID", "");
  }

  msg->set_pose_angle_rad(GetPose().GetRotationAngle<'Z'>().ToFloat());
  msg->set_pose_pitch_rad(GetPitchAngle().ToFloat());

  msg->set_left_wheel_speed_mmps(GetLeftWheelSpeed());
  msg->set_right_wheel_speed_mmps(GetRightWheelSpeed());

  msg->set_head_angle_rad(GetComponent<FullRobotPose>().GetHeadAngle());
  msg->set_lift_height_mm(GetLiftHeight());

  const auto& srcAccelStruct = GetHeadAccelData();
  auto* dstAccelStruct = new external_interface::AccelData(srcAccelStruct.x, srcAccelStruct.y, srcAccelStruct.z);
  msg->set_allocated_accel(dstAccelStruct);

  const auto& srcGyroStruct = GetHeadGyroData();
  auto* dstGyroStruct = new external_interface::GyroData(srcGyroStruct.x, srcGyroStruct.y, srcGyroStruct.z);
  dstGyroStruct->set_x(srcGyroStruct.x);
  dstGyroStruct->set_y(srcGyroStruct.y);
  dstGyroStruct->set_z(srcGyroStruct.z);
  msg->set_allocated_gyro(dstGyroStruct);

  auto status = _lastStatusFlags;
  if (GetAnimationComponent().IsAnimating()) {
    status |= (uint32_t)RobotStatusFlag::IS_ANIMATING;
  }

  if (GetCarryingComponent().IsCarryingObject()) {
    status |= (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK;
    msg->set_carrying_object_id(GetCarryingComponent().GetCarryingObjectID());
  } else {
    msg->set_carrying_object_id(-1);
  }
  msg->set_carrying_object_on_top_id(-1);

  msg->set_status(status);

  msg->set_head_tracking_object_id(GetMoveComponent().GetTrackToObject());

  msg->set_localized_to_object_id(GetLocalizationComponent().GetLocalizedTo());

  msg->set_last_image_time_stamp((TimeStamp_t)GetVisionComponent().GetLastProcessedImageTimeStamp());

  const auto& srcProxData = GetProxSensorComponent().GetLatestProxData();
  auto* dstProxData = new external_interface::ProxData(
    srcProxData.distance_mm,
    srcProxData.signalQuality,
    srcProxData.unobstructed,
    srcProxData.foundObject,
    srcProxData.isLiftInFOV);
  msg->set_allocated_prox_data(dstProxData);

  auto* dstTouchData = new external_interface::TouchData(
    GetTouchSensorComponent().GetLatestRawTouchValue(),
    GetTouchSensorComponent().GetIsPressed());
  msg->set_allocated_touch_data(dstTouchData);

  return msg;
}

RobotState Robot::GetDefaultRobotState()
{
  const auto kDefaultStatus = (Util::EnumToUnderlying(RobotStatusFlag::HEAD_IN_POS) |
                               Util::EnumToUnderlying(RobotStatusFlag::LIFT_IN_POS));

  const RobotPose kDefaultPose(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);

  std::array<uint16_t, Util::EnumToUnderlying(CliffSensor::CLIFF_COUNT)> defaultCliffRawVals;
  defaultCliffRawVals.fill(std::numeric_limits<uint16_t>::max());

  std::array<IMUDataFrame, IMUConstants::IMU_FRAMES_PER_ROBOT_STATE> defaultImuDataFrames;
  defaultImuDataFrames.fill(IMUDataFrame{0, GyroData{0, 0, 0}});

  const RobotState state(1, //uint32_t timestamp, (Robot does not report at t=0
                         0, //uint32_t pose_frame_id,
                         1, //uint32_t pose_origin_id,
                         kDefaultPose, //const Anki::Vector::RobotPose &pose,
                         0.f, //float lwheel_speed_mmps,
                         0.f, //float rwheel_speed_mmps,
                         0.f, //float headAngle
                         0.f, //float liftAngle,
                         AccelData(), //const Anki::Vector::AccelData &accel,
                         GyroData(), //const Anki::Vector::GyroData &gyro,
                         std::move(defaultImuDataFrames), // std::array<Anki::Vector::IMUDataFrame, 6> imuData,
                         0.f, // float batteryVoltage
                         0.f, // float chargerVoltage
                         kDefaultStatus, //uint32_t status,
                         std::move(defaultCliffRawVals), //std::array<uint16_t, 4> cliffDataRaw,
                         ProxSensorDataRaw(), //const Anki::Cozmo::ProxSensorDataRaw &proxData,
                         0, // touch intensity value
                         0, // uint8_t cliffDetectedFlags,
                         0, // uint8_t whiteDetectedFlags
                         40, // battery temp C
                         -1); //int8_t currPathSegment

  return state;
}

RobotInterface::MessageHandler* Robot::GetRobotMessageHandler() const
{
  if ((!_components->GetComponent<ContextWrapper>().IsComponentValid()) ||
      (GetContext()->GetRobotManager() == nullptr))
  {
    DEV_ASSERT(false, "Robot.GetRobotMessageHandler.nullptr");
    return nullptr;
  }

  return GetContext()->GetRobotManager()->GetMsgHandler();
}

RobotEventHandler& Robot::GetRobotEventHandler()
{
  return GetContext()->GetRobotManager()->GetRobotEventHandler();
}

Result Robot::ComputeHeadAngleToSeePose(const Pose3d& pose, Radians& headAngle, f32 yTolFrac) const
{
  Pose3d poseWrtNeck;
  const bool success = pose.GetWithRespectTo(GetComponent<FullRobotPose>().GetNeckPose(), poseWrtNeck);
  if (!success)
  {
    LOG_WARNING("Robot.ComputeHeadAngleToSeePose.OriginMismatch", "");
    return RESULT_FAIL_ORIGIN_MISMATCH;
  }

  // Assume the given point is in the XZ plane in front of the camera (i.e. so
  // if we were to turn and face it with the robot's body, we then just need to
  // find the right head angle)
  const Point3f pointWrtNeck(Point2f(poseWrtNeck.GetTranslation()).Length(), // Drop z and get length in XY plane
                             0.f,
                             poseWrtNeck.GetTranslation().z());

  Vision::Camera camera(GetVisionComponent().GetCamera());

  auto calib = camera.GetCalibration();
  if (nullptr == calib)
  {
    LOG_ERROR("Robot.ComputeHeadAngleToSeePose.NullCamera", "");
    return RESULT_FAIL;
  }

  const f32 dampening = 0.8f;
  const f32 kYTol = yTolFrac * calib->GetNrows();

  f32 searchAngle_rad = 0.f;
  s32 iteration = 0;
  const s32 kMaxIterations = 25;

# define DEBUG_HEAD_ANGLE_ITERATIONS 0
  while (iteration++ < kMaxIterations)
  {
    if (DEBUG_HEAD_ANGLE_ITERATIONS) {
      LOG_DEBUG("ComputeHeadAngle", "%d: %.1fdeg", iteration, RAD_TO_DEG(searchAngle_rad));
    }

    // Get point w.r.t. camera at current search angle
    const Pose3d& cameraPoseWrtNeck = GetCameraPose(searchAngle_rad);
    const Point3f& pointWrtCam = cameraPoseWrtNeck.GetInverse() * pointWrtNeck;

    // Project point into the camera
    // Note: not using camera's Project3dPoint() method because it does special handling
    //  for points not in the image limits, which we don't want here. We also don't need
    //  to add ycen, because we'll just subtract it right back off to see how far from
    //  centered we are. And we're only using y, so we'll just special-case this here.
    if (Util::IsFltLE(pointWrtCam.z(), 0.f))
    {
      LOG_WARNING("Robot.ComputeHeadAngleToSeePose.BadProjectedZ", "");
      return RESULT_FAIL;
    }
    const f32 y = calib->GetFocalLength_y() * (pointWrtCam.y() / pointWrtCam.z());

    // See if the projection is close enough to center
    if (Util::IsFltLE(std::abs(y), kYTol))
    {
      if (DEBUG_HEAD_ANGLE_ITERATIONS) {
        LOG_DEBUG("ComputeHeadAngle", "CONVERGED: %.1fdeg", RAD_TO_DEG(searchAngle_rad));
      }

      headAngle = searchAngle_rad;
      break;
    }

    // Nope: keep searching. Adjust angle proportionally to how far off we are.
    const f32 angleInc = std::atan2f(y, calib->GetFocalLength_y());
    searchAngle_rad -= dampening*angleInc;
  }

  if (iteration == kMaxIterations)
  {
    LOG_WARNING("Robot.ComputeHeadAngleToSeePose.MaxIterations", "");
    return RESULT_FAIL;
  }

  return RESULT_OK;
}

Result Robot::ComputeTurnTowardsImagePointAngles(const Point2f& imgPoint, const RobotTimeStamp_t timestamp,
                                                 Radians& absPanAngle, Radians& absTiltAngle,
                                                 const bool isPointNormalized) const
{
  if(!GetVisionComponent().GetCamera().IsCalibrated())
  {
    LOG_ERROR("Robot.ComputeTurnTowardsImagePointAngles.MissingCalibration", "");
    return RESULT_FAIL;
  }

  auto calib = GetVisionComponent().GetCamera().GetCalibration();

  Point2f pt( imgPoint );
  if(isPointNormalized)
  {
    if(!Util::InRange(pt.x(), 0.f, 1.f) || !Util::InRange(pt.y(), 0.f, 1.f))
    {
      LOG_ERROR("Robot.ComputeTurnTowardsImagePointAngles.PointNotNormalized",
                "%s not on interval [0,1]", pt.ToString().c_str());
      return RESULT_FAIL;
    }

    // Scale normalized point to "calibration" resolution
    pt.x() *= (f32)calib->GetNcols();
    pt.y() *= (f32)calib->GetNrows();
  }
  pt -= calib->GetCenter();

  HistRobotState histState;
  RobotTimeStamp_t t;
  Result result = GetStateHistory()->ComputeStateAt(timestamp, t, histState);
  if (RESULT_OK != result)
  {
    LOG_WARNING("Robot.ComputeTurnTowardsImagePointAngles.ComputeHistPoseFailed", "t=%u", (TimeStamp_t)timestamp);
    absPanAngle = GetPose().GetRotation().GetAngleAroundZaxis();
    absTiltAngle = GetComponent<FullRobotPose>().GetHeadAngle();
    return result;
  }

  absTiltAngle = std::atan2f(-pt.y(), calib->GetFocalLength_y()) + histState.GetHeadAngle_rad();
  absPanAngle  = std::atan2f(-pt.x(), calib->GetFocalLength_x()) + histState.GetPose().GetRotation().GetAngleAroundZaxis();

  return RESULT_OK;
}

void Robot::DevReplaceAIComponent(AIComponent* aiComponent, bool shouldManage)
{
  IDependencyManagedComponent<Anki::Vector::RobotComponentID>* explicitUpcast = aiComponent;
  _components->DevReplaceDependentComponent(RobotComponentID::AIComponent,
                                            explicitUpcast,
                                            shouldManage);
}

bool Robot::UpdateCameraStartupChecks(Result& res)
{
  enum State {
    FAILED = -1,
    WAITING,
    PASSED,
  };

  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  static float firstUpdateTime_sec = currentTime_sec;
  static State state = State::WAITING;

  if(state == State::WAITING)
  {
    // Manually capture images here until VisionComponent is up and running
    if(!GetVisionComponent().HasStartedCapturingImages())
    {
      // Try to get a frame
      Vision::ImageBuffer buffer;
      if(CameraService::getInstance()->CameraGetFrame(0, buffer))
      {
        CameraService::getInstance()->CameraReleaseFrame(buffer.GetImageId());
      }
    }

    // After 4 seconds, check if we have gotten a frame
    if(currentTime_sec - firstUpdateTime_sec > 4.f)
    {
      using namespace RobotInterface;

      // If we haven't gotten a frame then display an error code
      if(!CameraService::getInstance()->HaveGottenFrame())
      {
        state = State::FAILED;
        FaultCode::DisplayFaultCode(FaultCode::CAMERA_FAILURE);
      }
      // Otherwise the camera works
      else
      {
        state = State::PASSED;
      }
    }
  }

  res = (state == State::FAILED ? RESULT_FAIL : RESULT_OK);
  return (state != State::WAITING);
}

bool Robot::UpdateToFStartupChecks(Result& res)
{
  static bool isDone = false;

  enum class State
  {
   WaitingForCallback,
   Setup,
   StartRanging,
   EndRanging,
   Success,
   Failure,
  };
  static State state = State::Setup;

  auto* tof = ToFSensor::getInstance();
  if(tof == nullptr)
  {
    res = RESULT_OK;
    return true;
  }

#define HANDLE_RESULT(res, nextState) {                                 \
    if(res != ToFSensor::CommandResult::Success) {                      \
      PRINT_NAMED_ERROR("Robot.UpdateToFStartupChecks.Fail", "State: %u", state); \
      FaultCode::DisplayFaultCode(FaultCode::TOF_FAILURE);              \
      state = State::Failure;                                           \
    } else {                                                            \
      state = nextState;                                                \
    }                                                                   \
  }

  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  static float startTime_sec = currentTime_sec;

  // If the ToF check has been running for more than 10 seconds assume failure
  // This will handle failing should we not get any valid ROIs or for some reason
  // one of the command callbacks is not called (never seen it happen but who knows...)
  if((state != State::Failure && state != State::Success) &&
     currentTime_sec - startTime_sec > 10.f)
  {
    HANDLE_RESULT(ToFSensor::CommandResult::Failure, State::Failure);
  }

  switch(state)
  {
    case State::Setup:
      {
        state = State::WaitingForCallback;
        tof->SetupSensors([](ToFSensor::CommandResult res)
                          {
                            HANDLE_RESULT(res, State::StartRanging);
                          });
      }
      break;

    case State::StartRanging:
      {
        state = State::WaitingForCallback;
        tof->StartRanging([](ToFSensor::CommandResult res)
                          {
                            HANDLE_RESULT(res, State::EndRanging);
                          });
      }
      break;

    case State::EndRanging:
      {
        bool isDataNew = false;
        RangeDataRaw data = tof->GetData(isDataNew);
        if(isDataNew)
        {
          bool atLeastOneValidRoi = false;
          for(const auto& roiReading : data.data)
          {
            if(tof->IsValidRoiStatus(roiReading.roiStatus))
            {
              atLeastOneValidRoi = true;
            }
          }

          if(atLeastOneValidRoi)
          {
            state = State::WaitingForCallback;
            tof->StopRanging([](ToFSensor::CommandResult res)
                             {
                               PRINT_NAMED_INFO("Robot.UpdateToFStartupChecks.Success","");
                               HANDLE_RESULT(res, State::Success);
                             });
          }
        }
      }
      break;

    case State::Success:
      {
        isDone = true;
      }
      // Intentional fallthrough
    case State::WaitingForCallback:
      {
        res =  RESULT_OK;
      }
      break;

    case State::Failure:
      {
        isDone = true;
        res =  RESULT_FAIL;
      }
      break;
  }

  return isDone;

  #undef HANDLE_RESULT
}

bool Robot::UpdateGyroCalibChecks(Result& res)
{
  // Wait this much time after sending sync to robot before checking if we
  // should be displaying the low battery image to encourage user to put the robot down.
  // Note that by the time that the sync has been sent, the face has already
  // been blank for around 7 seconds.
  const float kTimeAfterSyncSent_sec = 2.f;

  const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  static bool displayedImage = false;

  if(!displayedImage &&
     _syncRobotSentTime_sec > 0 &&
     currentTime_sec - _syncRobotSentTime_sec > kTimeAfterSyncSent_sec &&
     !_syncRobotAcked)
  {
    // Manually init AnimationComponent
    // Normally it would init when we receive syncTime from robot process
    // but we haven't received syncTime yet likely because the gyro hasn't calibrated
    GetAnimationComponent().Init();

    static const std::string kGyroNotCalibratedImg = "config/devOnlySprites/independentSprites/battery_low.png";
    const std::string imgPath = GetContextDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                         kGyroNotCalibratedImg);
    Vision::ImageRGB img;
    img.Load(imgPath);
    // Display the image indefinitely or atleast until something else is displayed
    GetAnimationComponent().DisplayFaceImage(img, 0, true);
    // Move the head to look up to show the image clearly
    GetMoveComponent().MoveHeadToAngle(MAX_HEAD_ANGLE,
                                       MAX_HEAD_SPEED_RAD_PER_S,
                                       MAX_HEAD_ACCEL_RAD_PER_S2,
                                       1.f);
    displayedImage = true;

  }

  res = RESULT_OK;
  return true;
}

bool Robot::UpdateStartupChecks(Result& res)
{
#define RUN_CHECK(func)        \
  {                            \
    Result result = RESULT_OK; \
    checkDone &= func(result); \
    if(checkDone) {            \
      res = result;            \
      if(res != RESULT_OK) {   \
        return res;            \
      }                        \
    }                          \
  }

  bool checkDone = true;
  res = RESULT_OK;
  RUN_CHECK(UpdateGyroCalibChecks);
  RUN_CHECK(UpdateCameraStartupChecks);
  RUN_CHECK(UpdateToFStartupChecks);
  return checkDone;

#undef RUN_CHECK
}

bool Robot::SetLocale(const std::string & locale)
{
  if (!Anki::Util::Locale::IsValidLocaleString(locale))
  {
    LOG_ERROR("Robot.SetLocale", "Invalid locale: %s", locale.c_str());
    return false;
  }

  DEV_ASSERT(_context != nullptr, "Robot.SetLocale.InvalidContext");
  _context->SetLocale(locale);

  //
  // Attempt to load localized strings for given locale.
  // If that fails, fall back to default locale.
  //
  auto & localeComponent = GetLocaleComponent();
  if (!localeComponent.SetLocale(locale)) {
    LOG_WARNING("Robot.SetLocale", "Unable to set locale %s", locale.c_str());
    localeComponent.SetLocale(Anki::Util::Locale::kDefaultLocale.ToString());
  }

  // Notify animation process
  SendRobotMessage<RobotInterface::SetLocale>(locale);

  return true;
}

void Robot::Shutdown(ShutdownReason reason)
{
  if (_toldToShutdown) {
    LOG_WARNING("Robot.Shutdown.AlreadyShuttingDown", "Ignoring new reason %s", EnumToString(reason));
    return;
  }
  _toldToShutdown = true;
  _shutdownReason = reason;
}

} // namespace Vector
} // namespace Anki
