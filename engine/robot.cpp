
//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "engine/robot.h"
#include "camera/cameraService.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/poseOriginList.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationLoader.h"

#include "engine/actions/actionContainers.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/activeCube.h"
#include "engine/activeObjectHelpers.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponent.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/ankiEventUtil.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/block.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/charger.h"
#include "engine/components/animationComponent.h"
#include "engine/components/batteryComponent.h"
#include "engine/components/blockTapFilterComponent.h"
#include "engine/components/backpackLights/backpackLightComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeAccelComponent.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/habitatDetectorComponent.h"
#include "engine/components/inventoryComponent.h"
#include "engine/components/mics/beatDetectorComponent.h"
#include "engine/components/mics/micComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/nvStorageComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/components/photographyManager.h"
#include "engine/components/powerStateManager.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/components/sdkComponent.h"
#include "engine/components/settingsCommManager.h"
#include "engine/components/settingsManager.h"
#include "engine/components/sensors/cliffSensorComponent.h"
#include "engine/components/sensors/proxSensorComponent.h"
#include "engine/components/sensors/touchSensorComponent.h"
#include "engine/components/textToSpeech/textToSpeechCoordinator.h"
#include "engine/components/variableSnapshot/variableSnapshotComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/faceWorld.h"
#include "engine/fullRobotPose.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/moodSystem/stimulationFaceDisplay.h"
#include "engine/navMap/mapComponent.h"
#include "engine/objectPoseConfirmer.h"
#include "engine/petWorld.h"
#include "engine/robotDataLoader.h"
#include "engine/robotGyroDriftDetector.h"
#include "engine/robotIdleTimeoutComponent.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"
#include "engine/robotStateHistory.h"
#include "engine/robotToEngineImplMessaging.h"
#include "engine/viz/vizManager.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "coretech/vision/engine/visionMarker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/gameStatusFlag.h"
#include "clad/types/robotStatusAndActions.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/transport/reliableConnection.h"
#include "util/transport/connectionStats.h"

#include "proto/external_interface/shared.pb.h"
#include "osState/osState.h"

#include "anki/cozmo/shared/factory/emrHelper.h"
#include "anki/cozmo/shared/factory/faultCodes.h"

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp" // For imwrite() in ProcessImage

#include <algorithm>
#include <fstream>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>

#define LOG_CHANNEL "RobotState"

#define IS_STATUS_FLAG_SET(x) ((msg.status & (uint32_t)RobotStatusFlag::x) != 0)

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kFakeRobotBeingHeld, "Robot", false);

CONSOLE_VAR(bool, kDebugPossibleBlockInteraction, "Robot", false);

// if false, vision system keeps running while picked up, on side, etc.
CONSOLE_VAR(bool, kUseVisionOnlyWhileOnTreads,    "Robot", false);

// Enable to enable example code of face image drawing
CONSOLE_VAR(bool, kEnableTestFaceImageRGBDrawing,  "Robot", false);

// TEMP support for 'old' chargers with black stripe and white body (VIC-2755)
// Set to true to allow the robot to dock with older white chargers (that have a light-on-dark marker sticker)
CONSOLE_VAR(bool, kChargerStripeIsBlack, "Robot",
#ifdef SIMULATOR
            false  // Simulated chargers are gray with white stripe
#else
            true   // *Most* real chargers are white with a sticker, and thus have a black stripe
#endif
            );

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
    _thisRobot->GetActionList().QueueAction(QueueActionPosition::NOW,
                                            new PlayAnimationAction(animName));
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
          auto spritePaths = _thisRobot->GetComponent<DataAccessorComponent>().GetSpritePaths();
          auto spriteSequenceContainer = _thisRobot->GetComponent<DataAccessorComponent>().GetSpriteSequenceContainer();
          std::atomic<float> loadingCompleteRatio(0);
          std::atomic<bool> abortLoad(false);
          CannedAnimationLoader animLoader(platform, spritePaths, spriteSequenceContainer, loadingCompleteRatio, abortLoad);

          animLoader.LoadAnimationIntoContainer(animationPath.c_str(), animContainer);
          PRINT_NAMED_INFO("Robot.AddAnimation", "Loaded animation from %s", animationPath.c_str());
        }
      }
    }
  }
}

CONSOLE_FUNC(PlayAnimationByName, "Animation", const char* animName);
CONSOLE_FUNC(AddAnimation, "Animation", const char* animFile);

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


static void EnableCalmPowerMode(ConsoleFunctionContextRef context)
{
  if (_thisRobot != nullptr) {
    const bool enableCalm = ConsoleArg_Get_Bool(context, "enable");
    const bool calibOnDisable = ConsoleArg_GetOptional_Bool(context, "calibOnDisable", false);
    _thisRobot->SendMessage(RobotInterface::EngineToRobot(RobotInterface::CalmPowerMode(enableCalm, calibOnDisable)));
  }
}

CONSOLE_FUNC(EnableCalmPowerMode, "EnableCalmPowerMode", bool enable, optional bool calibOnDisable);

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


Robot::Robot(const RobotID_t robotID, CozmoContext* context)
: _context(context)
, _poseOrigins(new PoseOriginList())
, _ID(robotID)
, _syncRobotAcked(false)
, _lastMsgTimestamp(0)
, _offTreadsState(OffTreadsState::OnTreads)
, _awaitingConfirmationTreadState(OffTreadsState::OnTreads)
, _robotAccelFiltered(0.f, 0.f, 0.f)
{
  DEV_ASSERT(_context != nullptr, "Robot.Constructor.ContextIsNull");

  LOG_INFO("Robot.Robot", "Created");

  // create all components
  {
    _components = std::make_unique<EntityType>();
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
    _components->AddDependentComponent(RobotComponentID::NVStorage,                  new NVStorageComponent());
    _components->AddDependentComponent(RobotComponentID::AIComponent,                new AIComponent());
    _components->AddDependentComponent(RobotComponentID::ObjectPoseConfirmer,        new ObjectPoseConfirmer());
    _components->AddDependentComponent(RobotComponentID::CubeLights,                 new CubeLightComponent());
    _components->AddDependentComponent(RobotComponentID::BackpackLights,             new BackpackLightComponent());
    _components->AddDependentComponent(RobotComponentID::CubeAccel,                  new CubeAccelComponent());
    _components->AddDependentComponent(RobotComponentID::CubeComms,                  new CubeCommsComponent());
    _components->AddDependentComponent(RobotComponentID::CubeConnectionCoordinator,  new CubeConnectionCoordinator());
    _components->AddDependentComponent(RobotComponentID::GyroDriftDetector,          new RobotGyroDriftDetector());
    _components->AddDependentComponent(RobotComponentID::HabitatDetector,            new HabitatDetectorComponent());
    _components->AddDependentComponent(RobotComponentID::Docking,                    new DockingComponent());
    _components->AddDependentComponent(RobotComponentID::Carrying,                   new CarryingComponent());
    _components->AddDependentComponent(RobotComponentID::CliffSensor,                new CliffSensorComponent());
    _components->AddDependentComponent(RobotComponentID::ProxSensor,                 new ProxSensorComponent());
    _components->AddDependentComponent(RobotComponentID::TouchSensor,                new TouchSensorComponent());
    _components->AddDependentComponent(RobotComponentID::Animation,                  new AnimationComponent());
    _components->AddDependentComponent(RobotComponentID::StateHistory,               new RobotStateHistory());
    _components->AddDependentComponent(RobotComponentID::MoodManager,                new MoodManager());
    _components->AddDependentComponent(RobotComponentID::StimulationFaceDisplay,     new StimulationFaceDisplay());
    _components->AddDependentComponent(RobotComponentID::Inventory,                  new InventoryComponent());
    _components->AddDependentComponent(RobotComponentID::ProgressionUnlock,          new ProgressionUnlockComponent());
    _components->AddDependentComponent(RobotComponentID::BlockTapFilter,             new BlockTapFilterComponent());
    _components->AddDependentComponent(RobotComponentID::RobotToEngineImplMessaging, new RobotToEngineImplMessaging());
    _components->AddDependentComponent(RobotComponentID::RobotIdleTimeout,           new RobotIdleTimeoutComponent());
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
    _components->AddDependentComponent(RobotComponentID::SettingsManager,            new SettingsManager());
    _components->AddDependentComponent(RobotComponentID::RobotStatsTracker,          new RobotStatsTracker());
    _components->AddDependentComponent(RobotComponentID::VariableSnapshotComponent,  new VariableSnapshotComponent());
    _components->InitComponents(this);
  }

  GetComponent<FullRobotPose>().GetPose().SetName("Robot_" + std::to_string(_ID));
  _driveCenterPose.SetName("RobotDriveCenter_" + std::to_string(_ID));

  // Initializes FullRobotPose, _poseOrigins, and _worldOrigin:
  Delocalize(false);

  // The call to Delocalize() will increment frameID, but we want it to be
  // initialized to 0, to match the physical robot's initialization
  // It will also add to history so clear it
  // It will also flag that a localization update is needed when it increments the frameID so set the flag
  // to false
  _frameId = 0;
  GetStateHistory()->Clear();
  _needToSendLocalizationUpdate = false;

  GetRobotToEngineImplMessaging().InitRobotMessageComponent(GetContext()->GetRobotManager()->GetMsgHandler(), this);

  _lastDebugStringHash = 0;

  // Setting camera pose according to current head angle.
  // (Not using SetHeadAngle() because _isHeadCalibrated is initially false making the function do nothing.)
  GetVisionComponent().GetCamera().SetPose(GetCameraPose(GetComponent<FullRobotPose>().GetHeadAngle()));

  // Used for CONSOLE_FUNCTION "PlayAnimationByName" above
#if REMOTE_CONSOLE_ENABLED
  _thisRobot = this;
#endif

  // This will create the AndroidHAL instance if it doesn't yet exist
  CameraService::getInstance();


} // Constructor: Robot

Robot::~Robot()
{
  // save variable snapshots before rest of robot components start destructing
  _components->RemoveComponent(RobotComponentID::VariableSnapshotComponent);

  // VIC-1961: Remove touch sensor component before aborting all since there's a DEV_ASSERT crash
  // and we need to write data out from the touch sensor component out. This explicit destruction
  // can be removed once the DEV_ASSERT is fixed
  _components->RemoveComponent(RobotComponentID::TouchSensor);

  // force an update to the freeplay data manager, so we'll send a DAS event before the tracker is destroyed
  GetAIComponent().GetComponent<FreeplayDataTracker>().ForceUpdate();

  AbortAll();

  // Destroy our actionList before things like the path planner, since actions often rely on those.
  // ActionList must be cleared before it is destroyed because pending actions may attempt to make use of the pointer.
  GetActionList().Clear();

  // Remove (destroy) certain components explicitly since
  // they contains poses that use the contents of FullRobotPose as a parent
  // and there's no guarantee on entity/component destruction order
  _components->RemoveComponent(RobotComponentID::Vision);
  _components->RemoveComponent(RobotComponentID::Map);
  _components->RemoveComponent(RobotComponentID::ObjectPoseConfirmer);
  _components->RemoveComponent(RobotComponentID::PathPlanning);

  PRINT_NAMED_INFO("robot.destructor", "%d", GetID());
}

external_interface::RobotStatsResponse* Robot::GetRobotStats() {
  Robot* robot =  GetContext()->GetRobotManager()->GetRobot();
  const auto& batteryComponent = robot->GetBatteryComponent();
  external_interface::NetworkStats* networkStats = new external_interface::NetworkStats{Util::gNetStat1NumConnections, \
                                                                                        Util::gNetStat2LatencyAvg, \
                                                                                        Util::gNetStat3LatencySD, \
                                                                                        Util::gNetStat4LatencyMin, \
                                                                                        Util::gNetStat5LatencyMax, \
                                                                                        Util::gNetStat6PingArrivedPC, \
                                                                                        Util::gNetStat7ExtQueuedAvg_ms, \
                                                                                        Util::gNetStat8ExtQueuedMin_ms, \
                                                                                        Util::gNetStat9ExtQueuedMax_ms, \
                                                                                        Util::gNetStatAQueuedAvg_ms, \
                                                                                        Util::gNetStatBQueuedMin_ms, \
                                                                                        Util::gNetStatCQueuedMax_ms};
  external_interface::RobotStatsResponse* response = new external_interface::RobotStatsResponse{batteryComponent.GetBatteryVolts(), \
                                                                                                (external_interface::BatteryLevel)batteryComponent.GetBatteryLevel(), \
                                                                                                OSState::getInstance()->GetOSBuildVersion(), \
                                                                                                networkStats};
  return response;
}

bool Robot::CheckAndUpdateTreadsState(const RobotState& msg)
{
  if (!IsHeadCalibrated()) {
    return false;
  }

  const bool isPickedUp = IS_STATUS_FLAG_SET(IS_PICKED_UP);
  const bool isFalling = IS_STATUS_FLAG_SET(IS_FALLING);
  const TimeStamp_t currentTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();

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
      PRINT_NAMED_INFO("Robot.CheckAndUpdateTreadsState.FallingStarted",
                       "t=%dms",
                       _fallingStartedTime_ms);

      // Stop all actions
      GetActionList().Cancel();

    } else if (_offTreadsState == OffTreadsState::Falling) {
      // This is not an exact measurement of fall time since it includes some detection delays on the robot side
      // It may also include kRobotTimeToConsiderOfftreads_ms depending on how the robot lands
      PRINT_NAMED_INFO("Robot.CheckAndUpdateTreadsState.FallingStopped",
                       "t=%dms, duration=%dms",
                       GetLastMsgTimestamp(), GetLastMsgTimestamp() - _fallingStartedTime_ms);
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

      DEV_ASSERT(!IsLocalized(), "Robot should be delocalized when first put back down!");

      // If we are not localized and there is nothing else left in the world that
      // we could localize to, then go ahead and mark us as localized (via
      // odometry alone)
      if (false == GetBlockWorld().AnyRemainingLocalizableObjects()) {
        LOG_INFO("Robot.UpdateOfftreadsState.NoMoreRemainingLocalizableObjects",
                 "Marking previously-unlocalized robot %d as localized to odometry because "
                 "there are no more objects to localize to in the world.", GetID());
        SetLocalizedTo(nullptr); // marks us as localized to odometry only
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

  // Send viz message with current treads states
  const bool awaitingNewTreadsState = (_offTreadsState != _awaitingConfirmationTreadState);
  const auto vizManager = GetContext()->GetVizManager();
  vizManager->SetText(VizManager::OFF_TREADS_STATE,
                      NamedColors::GREEN,
                      "OffTreadsState: %s  %s",
                      EnumToString(_offTreadsState),
                      awaitingNewTreadsState ? EnumToString(_awaitingConfirmationTreadState) : "");

  if (offTreadsStateChanged) {
    // pause the freeplay tracking if we are not on the treads
    const bool isPaused = (_offTreadsState != OffTreadsState::OnTreads);
    GetAIComponent().GetComponent<FreeplayDataTracker>().SetFreeplayPauseFlag(isPaused, FreeplayPauseFlag::OffTreads);
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

void Robot::Delocalize(bool isCarryingObject)
{
  _isLocalized = false;
  _localizedToID.UnSet();
  _localizedToFixedObject = false;
  _localizedMarkerDistToCameraSq = -1.f;

  // NOTE: no longer doing this here because Delocalize() can be called by
  //  BlockWorld::ClearAllExistingObjects, resulting in a weird loop...
  //_blockWorld.ClearAllExistingObjects();

  // TODO rsam:
  // origins are no longer destroyed to prevent children from having to rejigger as cubes do. This however
  // has the problem of leaving zombie origins, and having systems never deleting dead poses that can never
  // be transformed withRespectTo a current origin. The origins growing themselves is not a big problem since
  // they are merely a Pose3d instance. However systems that keep Poses around because they have a valid
  // origin could potentially be a problem. This would have to be profiled to identify those systems, so not
  // really worth adding here a warning for "number of zombies is too big" without actually keeping track
  // of how many children they hold, or for how long. Eg: zombies with no children could auto-delete themselves,
  // but is the cost of bookkeeping bigger than what we are currently losing to zombies? That's the question
  // to profile

  // Store a copy of the old origin ID
  const PoseOriginID_t oldOriginID = GetPoseOriginList().GetCurrentOriginID();

  // Add a new origin
  const PoseOriginID_t worldOriginID = _poseOrigins->AddNewOrigin();
  const Pose3d& worldOrigin = GetPoseOriginList().GetCurrentOrigin();
  DEV_ASSERT_MSG(worldOriginID == GetPoseOriginList().GetCurrentOriginID(),
                 "Robot.Delocalize.UnexpectedNewWorldOriginID", "%d vs. %d",
                 worldOriginID, GetPoseOriginList().GetCurrentOriginID());
  DEV_ASSERT_MSG(worldOriginID == worldOrigin.GetID(),
                 "Robot.Delocalize.MismatchedWorldOriginID", "%d vs. %d",
                 worldOriginID, worldOrigin.GetID());

  // Log delocalization, new origin name, and num origins to DAS
  PRINT_NAMED_INFO("Robot.Delocalize", "Delocalizing robot %d. New origin: %s. NumOrigins=%zu",
                   GetID(), worldOrigin.GetName().c_str(), GetPoseOriginList().GetSize());

  GetComponent<FullRobotPose>().GetPose().SetRotation(0, Z_AXIS_3D());
  GetComponent<FullRobotPose>().GetPose().SetTranslation({0.f, 0.f, 0.f});
  GetComponent<FullRobotPose>().GetPose().SetParent(worldOrigin);

  _driveCenterPose.SetRotation(0, Z_AXIS_3D());
  _driveCenterPose.SetTranslation({0.f, 0.f, 0.f});
  _driveCenterPose.SetParent(worldOrigin);

  // Create a new pose frame so that we can't get pose history entries with the same pose
  // frame that have different origins (Not 100% sure this is totally necessary but seems
  // like the cleaner / safer thing to do.)
  Result res = SetNewPose(GetComponent<FullRobotPose>().GetPose());
  if (res != RESULT_OK)
  {
    LOG_WARNING("Robot.Delocalize.SetNewPose", "Failed to set new pose");
  }

  if (_syncRobotAcked)
  {
    // Need to update the robot's pose history with our new origin and pose frame IDs
    LOG_INFO("Robot.Delocalize.SendingNewOriginID",
             "Sending new localization update at t=%u, with pose frame %u and origin ID=%u",
             GetLastMsgTimestamp(),
             GetPoseFrameID(),
             worldOrigin.GetID());
    SendAbsLocalizationUpdate(GetComponent<FullRobotPose>().GetPose(), GetLastMsgTimestamp(), GetPoseFrameID());
  }

  // Update VizText
  GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: <nothing>");
  GetContext()->GetVizManager()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         GetPoseOriginList().GetSize(),
                                         worldOrigin.GetName().c_str());
  GetContext()->GetVizManager()->EraseAllVizObjects();


  // clear the pose confirmer now that we've changed pose origins
  GetObjectPoseConfirmer().Clear();

  // Sanity check carrying state
  if (isCarryingObject != GetCarryingComponent().IsCarryingObject())
  {
    LOG_WARNING("Robot.Delocalize.IsCarryingObjectMismatch",
                "Passed-in isCarryingObject=%c, IsCarryingObject()=%c",
                isCarryingObject   ? 'Y' : 'N',
                GetCarryingComponent().IsCarryingObject() ? 'Y' : 'N');
  }

  // Have to do this _after_ clearing the pose confirmer because UpdateObjectOrigin
  // adds the carried objects to the pose confirmer in their newly updated pose,
  // but _before_ deleting zombie objects (since dirty carried objects may get
  // deleted)
  if (GetCarryingComponent().IsCarryingObject())
  {
    // Carried objects are in the pose chain of the robot, whose origin has now changed.
    // Thus the carried objects' actual origin no longer matches the way they are stored
    // in BlockWorld.
    for(auto const& objectID : GetCarryingComponent().GetCarryingObjects())
    {
      const Result result = GetBlockWorld().UpdateObjectOrigin(objectID, oldOriginID);
      if(RESULT_OK != result)
      {
        LOG_WARNING("Robot.Delocalize.UpdateObjectOriginFailed", "Object %d", objectID.GetValue());
      }

    }
  }

  // notify blockworld
  GetBlockWorld().OnRobotDelocalized(worldOriginID);

  // notify faceworld
  GetFaceWorld().OnRobotDelocalized(worldOriginID);

  // notify behavior whiteboard
  GetAIComponent().OnRobotDelocalized();

  GetMoveComponent().OnRobotDelocalized();

  // send message to game. At the moment I implement this so that Webots can update the render, but potentially
  // any system can listen to this
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDelocalized()));

} // Delocalize()

Result Robot::SetLocalizedTo(const ObservableObject* object)
{
  if (object == nullptr) {
    GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                           "LocalizedTo: Odometry");
    _localizedToID.UnSet();
    _isLocalized = true;
    return RESULT_OK;
  }

  if (object->GetID().IsUnknown()) {
    LOG_ERROR("Robot.SetLocalizedTo.IdNotSet", "Cannot localize to an object with no ID set");
    return RESULT_FAIL;
  }

  // Find the closest, most recently observed marker on the object
  TimeStamp_t mostRecentObsTime = 0;
  for(const auto& marker : object->GetMarkers()) {
    if(marker.GetLastObservedTime() >= mostRecentObsTime) {
      Pose3d markerPoseWrtCamera;
      if (false == marker.GetPose().GetWithRespectTo(GetVisionComponent().GetCamera().GetPose(), markerPoseWrtCamera)) {
        LOG_ERROR("Robot.SetLocalizedTo.MarkerOriginProblem", "Could not get pose of marker w.r.t. robot camera");
        return RESULT_FAIL;
      }
      const f32 distToMarkerSq = markerPoseWrtCamera.GetTranslation().LengthSq();
      if(_localizedMarkerDistToCameraSq < 0.f || distToMarkerSq < _localizedMarkerDistToCameraSq) {
        _localizedMarkerDistToCameraSq = distToMarkerSq;
        mostRecentObsTime = marker.GetLastObservedTime();
      }
    }
  }
  assert(_localizedMarkerDistToCameraSq >= 0.f);

  _localizedToID = object->GetID();
  _hasMovedSinceLocalization = false;
  _isLocalized = true;

  // notify behavior whiteboard
  GetAIComponent().OnRobotRelocalized();

  // Update VizText
  GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: %s_%d",
                                         ObjectTypeToString(object->GetType()), _localizedToID.GetValue());
  GetContext()->GetVizManager()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         GetPoseOriginList().GetSize(),
                                         GetWorldOrigin().GetName().c_str());

  return RESULT_OK;

} // SetLocalizedTo()

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

  // Update sensor components:
  GetCliffSensorComponent().NotifyOfRobotState(msg);
  GetProxSensorComponent().NotifyOfRobotState(msg);
  GetTouchSensorComponent().NotifyOfRobotState(msg);

  // update current path segment in the path component
  GetPathComponent().UpdateCurrentPathSegment(msg.currPathSegment);

  // Update IMU data
  _robotAccel = msg.accel;
  _robotGyro = msg.gyro;

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

  // this flag can have a small delay with respect to when we actually picked up the block, since Engine notifies
  // the robot, and the robot updates on the next state update. But that delay guarantees that the robot knows what
  // we think it's true, rather than mixing timestamps of when it started carrying vs when the robot knows that it was
  // carrying
  const bool isCarryingObject = IS_STATUS_FLAG_SET(IS_CARRYING_BLOCK);
  //robot->SetCarryingBlock( isCarryingObject ); // Still needed?
  GetDockingComponent().SetPickingOrPlacing(IS_STATUS_FLAG_SET(IS_PICKING_OR_PLACING));
  _isPickedUp = IS_STATUS_FLAG_SET(IS_PICKED_UP);
  _powerButtonPressed = IS_STATUS_FLAG_SET(IS_BUTTON_PRESSED);

  // Save the entire flag for sending to game
  _lastStatusFlags = msg.status;

  GetBatteryComponent().NotifyOfRobotState(msg);

  GetMoveComponent().NotifyOfRobotState(msg);

  _leftWheelSpeed_mmps = msg.lwheel_speed_mmps;
  _rightWheelSpeed_mmps = msg.rwheel_speed_mmps;

  _hasMovedSinceLocalization |= (GetMoveComponent().IsCameraMoving() || _offTreadsState != OffTreadsState::OnTreads);

  if (isDelocalizing)
  {
    _numMismatchedFrameIDs = 0;

    Delocalize(isCarryingObject);
  }
  else
  {
    DEV_ASSERT(msg.pose_frame_id <= GetPoseFrameID(), "Robot.UpdateFullRobotState.FrameFromFuture");
    const bool frameIsCurrent = msg.pose_frame_id == GetPoseFrameID();

    // This is "normal" mode, where we update pose history based on the
    // reported odometry from the physical robot

    // Ignore physical robot's notion of z from the message? (msg.pose_z)
    f32 pose_z = 0.f;


    // Need to put the odometry update in terms of the current robot origin
    if (!GetPoseOriginList().ContainsOriginID(msg.pose_origin_id))
    {
      LOG_WARNING("Robot.UpdateFullRobotState.BadOriginID",
                  "Received RobotState with originID=%u, only %zu pose origins available",
                  msg.pose_origin_id, GetPoseOriginList().GetSize());
      return RESULT_FAIL;
    }

    const Pose3d& origin = GetPoseOriginList().GetOriginByID(msg.pose_origin_id);

    // Initialize new pose to be within the reported origin
    Pose3d newPose(msg.pose.angle, Z_AXIS_3D(), {msg.pose.x, msg.pose.y, msg.pose.z}, origin);

    // It's possible the pose origin to which this update refers has since been
    // rejiggered and is now the child of another origin. To add to history below,
    // we must first flatten it. We do all this before "fixing" pose_z because pose_z
    // will be w.r.t. robot origin so we want newPose to already be as well.
    newPose = newPose.GetWithRespectToRoot();

    if(msg.pose_frame_id == GetPoseFrameID()) {
      // Frame IDs match. Use the robot's current Z (but w.r.t. world origin)
      pose_z = GetPose().GetWithRespectToRoot().GetTranslation().z();
    } else {
      // This is an old odometry update from a previous pose frame ID. We
      // need to look up the correct Z value to use for putting this
      // message's (x,y) odometry info into history. Since it comes from
      // pose history, it will already be w.r.t. world origin, since that's
      // how we store everything in pose history.
      HistRobotState histState;
      lastResult = GetStateHistory()->GetLastStateWithFrameID(msg.pose_frame_id, histState);
      if (lastResult != RESULT_OK) {
        LOG_ERROR("Robot.UpdateFullRobotState.GetLastPoseWithFrameIdError",
                  "Failed to get last pose from history with frame ID=%d",
                  msg.pose_frame_id);
        return lastResult;
      }
      pose_z = histState.GetPose().GetWithRespectToRoot().GetTranslation().z();
    }

    newPose.SetTranslation({newPose.GetTranslation().x(), newPose.GetTranslation().y(), pose_z});


    // Add to history
    const HistRobotState histState(newPose,
                                   msg,
                                   GetProxSensorComponent().GetLatestProxData(),
                                   GetCliffSensorComponent().GetCliffDetectedFlags() );
    lastResult = GetStateHistory()->AddRawOdomState(msg.timestamp, histState);

    if (lastResult != RESULT_OK) {
      LOG_WARNING("Robot.UpdateFullRobotState.AddPoseError",
                  "AddRawOdomStateToHistory failed for timestamp=%d", msg.timestamp);
      return lastResult;
    }

    Pose3d prevDriveCenterPose ;
    ComputeDriveCenterPose(GetPose(), prevDriveCenterPose);

    if (UpdateCurrPoseFromHistory() == false) {
      lastResult = RESULT_FAIL;
    }

    if (frameIsCurrent)
    {
      _numMismatchedFrameIDs = 0;
    } else {
      // COZMO-5850 (Al) This is to catch the issue where our frameID is incremented but fails to send
      // to the robot due to some origin issue. Somehow the robot's pose becomes an origin and doesn't exist
      // in the PoseOriginList. The frameID mismatch then causes all sorts of issues in things (ie VisionSystem
      // won't process the next image). Delocalizing will fix the mismatch by creating a new origin and sending
      // a localization update
      static const u32 kNumTicksWithMismatchedFrameIDs = 100; // 3 seconds (called each RobotState msg)

      ++_numMismatchedFrameIDs;

      if (_numMismatchedFrameIDs > kNumTicksWithMismatchedFrameIDs)
      {
        LOG_ERROR("Robot.UpdateFullRobotState.MismatchedFrameIDs",
                  "Robot[%u] and engine[%u] frameIDs are mismatched, delocalizing",
                  msg.pose_frame_id,
                  GetPoseFrameID());

        _numMismatchedFrameIDs = 0;

        Delocalize(GetCarryingComponent().IsCarryingObject());

        return RESULT_FAIL;
      }

    }

  }

# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
  GetComponent<RobotGyroDriftDetector>().DetectGyroDrift(msg);
# pragma clang diagnostic pop
  GetComponent<RobotGyroDriftDetector>().DetectBias(msg);

  /*
    PRINT_NAMED_INFO("Robot.UpdateFullRobotState.OdometryUpdate",
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

  const float imageFrameRate = 1000.0f / GetVisionComponent().GetFramePeriod_ms();
  const float imageProcRate = 1000.0f / GetVisionComponent().GetProcessingPeriod_ms();

  // Send state to visualizer for displaying
  GetContext()->GetVizManager()->SendRobotState(
    stateMsg,
    (u8)MIN(((u8)imageFrameRate), std::numeric_limits<u8>::max()),
    (u8)MIN(((u8)imageProcRate), std::numeric_limits<u8>::max()),
    GetAnimationComponent().GetAnimState_NumProcAnimFaceKeyframes(),
    GetAnimationComponent().GetAnimState_LockedTracks(),
    GetAnimationComponent().GetAnimState_TracksInUse(),
    _robotImuTemperature_degC,
    GetCliffSensorComponent().GetCliffDetectThresholds(),
    GetBatteryComponent().GetBatteryVolts()
    );

  return lastResult;

} // UpdateFullRobotState()

bool Robot::HasReceivedRobotState() const
{
  return _newStateMsgAvailable;
}


void Robot::SetPhysicalRobot(bool isPhysical)
{
  // TODO: Move somewhere else? This might not the best place for this, but it's where we
  // know whether or not we're talking to a physical robot or not so do things that depend on that here.
  // Assumes this function is only called once following connection.

  _isPhysical = isPhysical;

  // Modify net timeout depending on robot type - simulated robots shouldn't timeout so we can pause and debug
  // them We do this regardless of previous state to ensure it works when adding 1st simulated robot (as
  // _isPhysical already == false in that case) Note: We don't do this on phone by default, they also have a
  // remote connection to the simulator so removing timeout would force user to restart both sides each time.
  #if !(defined(ANKI_PLATFORM_IOS) || defined(ANKI_PLATFORM_ANDROID))
  {
    static const double kPhysicalRobotNetConnectionTimeoutInMS =
      Anki::Util::ReliableConnection::GetConnectionTimeoutInMS(); // grab default on 1st call
    const double kSimulatedRobotNetConnectionTimeoutInMS = FLT_MAX;
    const double netConnectionTimeoutInMS =
      isPhysical ? kPhysicalRobotNetConnectionTimeoutInMS : kSimulatedRobotNetConnectionTimeoutInMS;
    LOG_INFO("Robot.SetPhysicalRobot", "ReliableConnection::SetConnectionTimeoutInMS(%f) for %s Robot",
             netConnectionTimeoutInMS, isPhysical ? "Physical" : "Simulated");
    Anki::Util::ReliableConnection::SetConnectionTimeoutInMS(netConnectionTimeoutInMS);
  }
  #endif // !(ANKI_PLATFORM_IOS || ANKI_PLATFORM_ANDROID)

  GetVisionComponent().SetPhysicalRobot(_isPhysical);
}

Result Robot::GetHistoricalCamera(TimeStamp_t t_request, Vision::Camera& camera) const
{
  HistRobotState histState;
  TimeStamp_t t;
  Result result = GetStateHistory()->GetRawStateAt(t_request, t, histState);
  if (RESULT_OK != result)
  {
    return result;
  }

  camera = GetHistoricalCamera(histState, t);
  return RESULT_OK;
}

Pose3d Robot::GetHistoricalCameraPose(const HistRobotState& histState, TimeStamp_t t) const
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

  camPose.SetName("PoseHistoryCamera_" + std::to_string(t));

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

Vision::Camera Robot::GetHistoricalCamera(const HistRobotState& histState, TimeStamp_t t) const
{
  Vision::Camera camera(GetVisionComponent().GetCamera());

  // Update the head camera's pose
  camera.SetPose(GetHistoricalCameraPose(histState, t));

  return camera;
}


Result Robot::Update()
{
  ANKI_CPU_PROFILE("Robot::Update");

  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // Check for syncRobotAck taking too long to arrive
  if (_syncRobotSentTime_sec > 0.0f && currentTime > _syncRobotSentTime_sec + kMaxSyncRobotAckDelay_sec) {
    LOG_WARNING("Robot.Update.SyncRobotAckNotReceived", "");
    _syncRobotSentTime_sec = 0.0f;
  }

  //////////// CameraService Update ////////////
  CameraService::getInstance()->Update();

  const Result factoryRes = UpdateStartupChecks();
  if(factoryRes != RESULT_OK)
  {
    return factoryRes;
  }

  if (!_gotStateMsgAfterRobotSync)
  {
    LOG_DEBUG("Robot.Update", "Waiting for first full robot state to be handled");
    return RESULT_OK;
  }

  // Always send the "charger stripe color" the first time,
  // then only if the console var changes thereafter.
  static bool wasChargerStripeIsBlack = !kChargerStripeIsBlack;
  if (kChargerStripeIsBlack != wasChargerStripeIsBlack) {
    SendMessage(RobotInterface::EngineToRobot(RobotInterface::ChargerDockingStripeColor(kChargerStripeIsBlack)));
    wasChargerStripeIsBlack = kChargerStripeIsBlack;
  }

#if(0)
  ActiveBlockLightTest(1);
  return RESULT_OK;
#endif

  GetContext()->GetVizManager()->SendStartRobotUpdate();

  _components->UpdateComponents();

  // If anything in updating block world caused a localization update, notify
  // the physical robot now:
  if (_needToSendLocalizationUpdate) {
    SendAbsLocalizationUpdate();
    _needToSendLocalizationUpdate = false;
  }

  /////////// Update visualization ////////////
  // Draw All Objects by calling their Visualize() methods.
  GetBlockWorld().DrawAllObjects();

  // Always draw robot w.r.t. the origin, not in its current frame
  Pose3d robotPoseWrtOrigin = GetPose().GetWithRespectToRoot();

  // Triangle pose marker
  GetContext()->GetVizManager()->DrawRobot(GetID(), robotPoseWrtOrigin);

  // Full Webots CozmoBot model
  if (IsPhysical()) {
    GetContext()->GetVizManager()->DrawRobot(GetID(), robotPoseWrtOrigin, GetComponent<FullRobotPose>().GetHeadAngle(), GetComponent<FullRobotPose>().GetLiftAngle());
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

  GetContext()->GetVizManager()->SendEndRobotUpdate();

  // update time since last image received
  _timeSinceLastImage_s = std::max(0.0, currentTime - GetRobotToEngineImplMessaging().GetLastImageReceivedTime());

  // Sending debug string to game and viz
  char buffer [128];

  const float updateRatePerSec = 1.0f / (currentTime - _prevCurrentTime_sec);
  _prevCurrentTime_sec = currentTime;

  // So we can have an arbitrary number of data here that is likely to change want just hash it all
  // together if anything changes without spamming
  snprintf(buffer, sizeof(buffer),
           "%c%c%c%c%c%c %2dHz",
           GetMoveComponent().IsLiftMoving() ? 'L' : ' ',
           GetMoveComponent().IsHeadMoving() ? 'H' : ' ',
           GetMoveComponent().IsMoving() ? 'B' : ' ',
           GetCarryingComponent().IsCarryingObject() ? 'C' : ' ',
           GetBatteryComponent().IsOnChargerPlatform() ? 'P' : ' ',
           GetNVStorageComponent().HasPendingRequests() ? 'R' : ' ',
           // SimpleMoodTypeToString(GetMoodManager().GetSimpleMood()),
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK) ? 'L' : ' ',
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::HEAD_TRACK) ? 'H' : ' ',
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK) ? 'B' : ' ',
           (u8)MIN(((u8)updateRatePerSec), std::numeric_limits<u8>::max()));

  std::hash<std::string> hasher;
  size_t curr_hash = hasher(std::string(buffer));
  if (_lastDebugStringHash != curr_hash)
  {
    SendDebugString(buffer);
    _lastDebugStringHash = curr_hash;
  }

  if (kDebugPossibleBlockInteraction) {
    // print a bunch of info helpful for debugging block states
    BlockWorldFilter filter;
    filter.SetAllowedFamilies({ObjectFamily::LightCube});
    std::vector<ObservableObject*> matchingObjects;
    GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects); // note this doesn't retrieve unknowns anymore
    for( const auto obj : matchingObjects ) {
        const ObservableObject* topObj __attribute__((unused)) =
            GetBlockWorld().FindLocatedObjectOnTopOf(*obj, STACKED_HEIGHT_TOL_MM);
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
                  "block:%d poseState:%8s moving?%d RestingFlat?%d carried?%d poseWRT?%d objOnTop:%d"
                  " z=%6.2f UpAxis:%s CanStack?%d CanPickUp?%d FromGround?%d",
                  obj->GetID().GetValue(),
                  PoseStateToString( obj->GetPoseState() ),
                  obj->IsMoving(),
                  obj->IsRestingFlat(),
                  (GetCarryingComponent().IsCarryingObject() && GetCarryingComponent().
                   GetCarryingObject() == obj->GetID()),
                  gotRelPose,
                  topObj ? topObj->GetID().GetValue() : -1,
                  relPose.GetTranslation().z(),
                  axisStr,
                  GetDockingComponent().CanStackOnTopOfObject(*obj),
                  GetDockingComponent().CanPickUpObject(*obj),
                  GetDockingComponent().CanPickUpObjectFromGround(*obj));
    }
  }

  return RESULT_OK;

} // Update()

static f32 ClipHeadAngle(f32 head_angle)
{
  if (head_angle < MIN_HEAD_ANGLE - HEAD_ANGLE_LIMIT_MARGIN) {
    //PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too small.\n", head_angle);
    return MIN_HEAD_ANGLE;
  }
  else if (head_angle > MAX_HEAD_ANGLE + HEAD_ANGLE_LIMIT_MARGIN) {
    //PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too large.\n", head_angle);
    return MAX_HEAD_ANGLE;
  }

  return head_angle;

} // ClipHeadAngle()


Result Robot::SetNewPose(const Pose3d& newPose)
{
  SetPose(newPose.GetWithRespectToRoot());

  // Note: using last message timestamp instead of newest timestamp in history
  //  because it's possible we did not put the last-received state message into
  //  history (if it had old frame ID), but we still want the latest time we
  //  can get.
  const TimeStamp_t timeStamp = GetLastMsgTimestamp();

  return AddVisionOnlyStateToHistory(timeStamp, GetComponent<FullRobotPose>().GetPose(), GetComponent<FullRobotPose>().GetHeadAngle(), GetComponent<FullRobotPose>().GetLiftAngle());
}

void Robot::SetPose(const Pose3d &newPose)
{
  // The new pose should have our current world origin as its origin
  if (!ANKI_VERIFY(newPose.HasSameRootAs(GetWorldOrigin()),
                  "Robot.SetPose.NewPoseOriginAndWorldOriginMismatch",
                  ""))
  {
    return;
  }

  // Update our current pose and keep the name consistent
  const std::string name = GetComponent<FullRobotPose>().GetPose().GetName();
  GetComponent<FullRobotPose>().SetPose(newPose);
  GetComponent<FullRobotPose>().GetPose().SetName(name);

  ComputeDriveCenterPose(GetComponent<FullRobotPose>().GetPose(), _driveCenterPose);

} // SetPose()

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
                  "Angle %.3frad / %.1f (TODO: Send correction or just recalibrate?)",
                  angle, RAD_TO_DEG(angle));
    }
  }

} // SetHeadAngle()


void Robot::SetHeadCalibrated(bool isCalibrated)
{
  _isHeadCalibrated = isCalibrated;
}

void Robot::SetLiftCalibrated(bool isCalibrated)
{
  _isLiftCalibrated = isCalibrated;
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
  // TODO: Add lift angle limits?
  GetComponent<FullRobotPose>().SetLiftAngle(angle);

  Robot::ComputeLiftPose(GetComponent<FullRobotPose>().GetLiftAngle(), GetComponent<FullRobotPose>().GetLiftPose());

  DEV_ASSERT(GetComponent<FullRobotPose>().GetLiftPose().IsChildOf(GetComponent<FullRobotPose>().GetLiftBasePose()), "Robot.SetLiftAngle.InvalidPose");
}

Radians Robot::GetPitchAngle() const
{
  return GetComponent<FullRobotPose>().GetPitchAngle();
}

bool Robot::WasObjectTappedRecently(const ObjectID& objectID) const
{
  return GetComponent<BlockTapFilterComponent>().ShouldIgnoreMovementDueToDoubleTap(objectID);
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

Result Robot::LocalizeToObject(const ObservableObject* seenObject,
                               ObservableObject* existingObject)
{
  Result lastResult = RESULT_OK;

  if (existingObject == nullptr) {
    LOG_ERROR("Robot.LocalizeToObject.ExistingObjectPieceNullPointer", "");
    return RESULT_FAIL;
  }

  if (existingObject->GetID() != GetLocalizedTo())
  {
    LOG_DEBUG("Robot.LocalizeToObject",
              "Robot attempting to localize to %s object %d",
              EnumToString(existingObject->GetType()),
              existingObject->GetID().GetValue());
  }

  if (!existingObject->CanBeUsedForLocalization() || WasObjectTappedRecently(existingObject->GetID())) {
    LOG_ERROR("Robot.LocalizeToObject.UnlocalizedObject",
              "Refusing to localize to object %d, which claims not to be localizable.",
              existingObject->GetID().GetValue());
    return RESULT_FAIL;
  }

  /* Useful for Debug:
     PRINT_NAMED_INFO("Robot.LocalizeToMat.MatSeenChain",
     "%s\n", matSeen->GetPose().GetNamedPathToOrigin(true).c_str());

     PRINT_NAMED_INFO("Robot.LocalizeToMat.ExistingMatChain",
     "%s\n", existingMatPiece->GetPose().GetNamedPathToOrigin(true).c_str());
  */

  HistStateKey histStateKey;
  HistRobotState* histStatePtr = nullptr;
  Pose3d robotPoseWrtObject;
  float  headAngle;
  float  liftAngle;
  if (nullptr == seenObject)
  {
    if (false == GetPose().GetWithRespectTo(existingObject->GetPose(), robotPoseWrtObject)) {
      LOG_ERROR("Robot.LocalizeToObject.ExistingObjectOriginMismatch",
                "Could not get robot pose w.r.t. to existing object %d.",
                existingObject->GetID().GetValue());
      return RESULT_FAIL;
    }
    liftAngle = GetComponent<FullRobotPose>().GetLiftAngle();
    headAngle = GetComponent<FullRobotPose>().GetHeadAngle();
  } else {
    // Get computed HistRobotState at the time the object was observed.
    if ((lastResult = GetStateHistory()->GetComputedStateAt(seenObject->GetLastObservedTime(), &histStatePtr, &histStateKey)) != RESULT_OK) {
      LOG_ERROR("Robot.LocalizeToObject.CouldNotFindHistoricalPose", "Time %d", seenObject->GetLastObservedTime());
      return lastResult;
    }

    // The computed historical pose is always stored w.r.t. the robot's world
    // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
    // will work correctly
    Pose3d robotPoseAtObsTime = histStatePtr->GetPose();
    robotPoseAtObsTime.SetParent(GetWorldOrigin());

    // Get the pose of the robot with respect to the observed object
    if (robotPoseAtObsTime.GetWithRespectTo(seenObject->GetPose(), robotPoseWrtObject) == false) {
      LOG_ERROR("Robot.LocalizeToObject.ObjectPoseOriginMisMatch",
                "Could not get HistRobotState w.r.t. seen object pose.");
      return RESULT_FAIL;
    }

    liftAngle = histStatePtr->GetLiftAngle_rad();
    headAngle = histStatePtr->GetHeadAngle_rad();
  }

  // Make the computed robot pose use the existing object as its parent
  robotPoseWrtObject.SetParent(existingObject->GetPose());
  //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));

  // Add the new vision-based pose to the robot's history. Note that we use
  // the pose w.r.t. the origin for storing poses in history.
  Pose3d robotPoseWrtOrigin = robotPoseWrtObject.GetWithRespectToRoot();

  if (IsLocalized()) {
    // Filter Z so it doesn't change too fast (unless we are switching from
    // delocalized to localized)

    // Make z a convex combination of new and previous value
    static const f32 zUpdateWeight = 0.1f; // weight of new value (previous gets weight of 1 - this)
    Vec3f T = robotPoseWrtOrigin.GetTranslation();
    T.z() = (zUpdateWeight*robotPoseWrtOrigin.GetTranslation().z() +
             (1.f - zUpdateWeight) * GetPose().GetTranslation().z());
    robotPoseWrtOrigin.SetTranslation(T);
  }

  if (nullptr != seenObject)
  {
    //
    if ((lastResult = AddVisionOnlyStateToHistory(seenObject->GetLastObservedTime(),
                                                robotPoseWrtOrigin,
                                                headAngle, liftAngle)) != RESULT_OK)
    {
      LOG_ERROR("Robot.LocalizeToObject.FailedAddingVisionOnlyPoseToHistory", "");
      return lastResult;
    }
  }

  // If the robot's world origin is about to change by virtue of being localized
  // to existingObject, rejigger things so anything seen while the robot was
  // rooted to this world origin will get updated to be w.r.t. the new origin.
  const Pose3d& origOrigin = GetPoseOriginList().GetCurrentOrigin();
  if (!existingObject->GetPose().HasSameRootAs(origOrigin))
  {
    PRINT_NAMED_INFO("Robot.LocalizeToObject.RejiggeringOrigins",
                     "Robot %d's current origin is %s, about to localize to origin %s.",
                     GetID(), origOrigin.GetName().c_str(),
                     existingObject->GetPose().FindRoot().GetName().c_str());

    const PoseOriginID_t origOriginID = GetPoseOriginList().GetCurrentOriginID();

    // Update the origin to which _worldOrigin currently points to contain
    // the transformation from its current pose to what is about to be the
    // robot's new origin.
    Transform3d transform(GetPose().GetTransform().GetInverse());
    transform.PreComposeWith(robotPoseWrtOrigin.GetTransform());

    Result result = _poseOrigins->Rejigger(robotPoseWrtObject.FindRoot(), transform);
    if (ANKI_VERIFY(RESULT_OK == result, "Robot.LocalizeToObject.RejiggerFailed", ""))
    {
      const PoseOriginID_t newOriginID = GetPoseOriginList().GetCurrentOriginID();

      // Now we need to go through all objects whose poses have been adjusted
      // by this origin switch and notify the outside world of the change.
      // Note that map component must be updated before blockworld in case blockworld
      // tries to insert a new object into the map.
      GetMapComponent().UpdateMapOrigins(origOriginID, newOriginID);
      GetBlockWorld().UpdateObjectOrigins(origOriginID, newOriginID);
      GetFaceWorld().UpdateFaceOrigins(origOriginID, newOriginID);

      // after updating all block world objects, flatten out origins to remove grandparents
      _poseOrigins->Flatten(newOriginID);
    }

  } // if(_worldOrigin != &existingObject->GetPose().FindRoot())


  if (nullptr != histStatePtr)
  {
    // Update the computed historical pose as well so that subsequent block
    // pose updates use obsMarkers whose camera's parent pose is correct.
    histStatePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, headAngle, liftAngle);
  }


  // Compute the new "current" pose from history which uses the
  // past vision-based "ground truth" pose we just computed.
  DEV_ASSERT_MSG(existingObject->GetPose().HasSameRootAs(GetWorldOrigin()),
                 "Robot.LocalizeToObject.ExistingObjectHasWrongOrigin",
                 "ObjectOrigin:%s WorldOrigin:%s",
                 existingObject->GetPose().FindRoot().GetName().c_str(),
                 GetWorldOrigin().GetName().c_str());

  if (UpdateCurrPoseFromHistory() == false) {
    LOG_ERROR("Robot.LocalizeToObject.FailedUpdateCurrPoseFromHistory", "");
    return RESULT_FAIL;
  }

  // Mark the robot as now being localized to this object
  // NOTE: this should be _after_ calling AddVisionOnlyStateToHistory, since
  //    that function checks whether the robot is already localized
  lastResult = SetLocalizedTo(existingObject);
  if (RESULT_OK != lastResult) {
    LOG_ERROR("Robot.LocalizeToObject.SetLocalizedToFail", "");
    return lastResult;
  }

  // Overly-verbose. Use for debugging localization issues
  /*
    PRINT_NAMED_INFO("Robot.LocalizeToObject",
    "Using %s object %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f), frameID=%d\n",
    ObjectTypeToString(existingObject->GetType()),
    existingObject->GetID().GetValue(), GetID(),
    GetPose().GetTranslation().x(),
    GetPose().GetTranslation().y(),
    GetPose().GetTranslation().z(),
    GetPose().GetRotationAngle<'Z'>().getDegrees(),
    GetPose().GetRotationAxis().x(),
    GetPose().GetRotationAxis().y(),
    GetPose().GetRotationAxis().z(),
    GetPoseFrameID());
  */

  // Don't actually send the update here, because it's possible that we are going to
  // call LocalizeToObject more than once in this tick, which could cause the pose
  // frame ID to update multiple times, replacing what's stored for this timestamp
  // in pose history. We don't want to send a pose frame to the robot that's about
  // to be replaced in history and never used again (causing errors in looking for it
  // when we later receive a RobotState message with that invalidated frameID), so just
  // set this flag to do the localization update once out in Update().
  _needToSendLocalizationUpdate = true;

  return RESULT_OK;
} // LocalizeToObject()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result Robot::LocalizeToMat(const MatPiece* matSeen, MatPiece* existingMatPiece)
{
  Result lastResult;

  if (matSeen == nullptr) {
    LOG_ERROR("Robot.LocalizeToMat.MatSeenNullPointer", "");
    return RESULT_FAIL;
  } else if (existingMatPiece == nullptr) {
    LOG_ERROR("Robot.LocalizeToMat.ExistingMatPieceNullPointer", "");
    return RESULT_FAIL;
  }

  /* Useful for Debug:
     PRINT_NAMED_INFO("Robot.LocalizeToMat.MatSeenChain",
     "%s\n", matSeen->GetPose().GetNamedPathToOrigin(true).c_str());

     PRINT_NAMED_INFO("Robot.LocalizeToMat.ExistingMatChain",
     "%s\n", existingMatPiece->GetPose().GetNamedPathToOrigin(true).c_str());
  */

  // Get computed HistRobotState at the time the mat was observed.
  HistStateKey histStateKey;
  HistRobotState* histStatePtr = nullptr;
  if ((lastResult = GetStateHistory()->GetComputedStateAt(matSeen->GetLastObservedTime(), &histStatePtr, &histStateKey)) != RESULT_OK) {
    LOG_ERROR("Robot.LocalizeToMat.CouldNotFindHistoricalPose", "Time %d", matSeen->GetLastObservedTime());
    return lastResult;
  }

  // The computed historical pose is always stored w.r.t. the robot's world
  // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
  // will work correctly
  Pose3d robotPoseAtObsTime = histStatePtr->GetPose();
  robotPoseAtObsTime.SetParent(GetWorldOrigin());

  /*
  // Get computed Robot pose at the time the mat was observed (note that this
  // also makes the pose have the robot's current world origin as its parent
  Pose3d robotPoseAtObsTime;
  if(robot->GetComputedStateAt(matSeen->GetLastObservedTime(), robotPoseAtObsTime) != RESULT_OK) {
  PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.CouldNotComputeHistoricalPose",
                    "Time %d\n",
                    matSeen->GetLastObservedTime());
  return false;
  }
  */

  // Get the pose of the robot with respect to the observed mat piece
  Pose3d robotPoseWrtMat;
  if (robotPoseAtObsTime.GetWithRespectTo(matSeen->GetPose(), robotPoseWrtMat) == false) {
    LOG_ERROR("Robot.LocalizeToMat.MatPoseOriginMisMatch", "Could not get HistRobotState w.r.t. matPose.");
    return RESULT_FAIL;
  }

  // Make the computed robot pose use the existing mat piece as its parent
  robotPoseWrtMat.SetParent(existingMatPiece->GetPose());
  //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));


  // If there is any significant rotation, make sure that it is roughly
  // around the Z axis
  Radians rotAngle;
  Vec3f rotAxis;
  robotPoseWrtMat.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);

  if (std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) && !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(15))) {
    LOG_WARNING("Robot.LocalizeToMat.OutOfPlaneRotation",
                "Refusing to localize to %s because "
                "Robot %d's Z axis would not be well aligned with the world Z axis. "
                "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)",
                ObjectTypeToString(existingMatPiece->GetType()), GetID(),
                rotAngle.getDegrees(), rotAxis.x(), rotAxis.y(), rotAxis.z());
    return RESULT_FAIL;
  }

  // Snap to purely horizontal rotation and surface of the mat
  if (existingMatPiece->IsPoseOn(robotPoseWrtMat, 0, 10.f)) {
    Vec3f robotPoseWrtMat_trans = robotPoseWrtMat.GetTranslation();
    robotPoseWrtMat_trans.z() = existingMatPiece->GetDrivingSurfaceHeight();
    robotPoseWrtMat.SetTranslation(robotPoseWrtMat_trans);
  }
  robotPoseWrtMat.SetRotation( robotPoseWrtMat.GetRotationAngle<'Z'>(), Z_AXIS_3D() );


  if (!_localizedToFixedObject && !existingMatPiece->IsMoveable()) {
    // If we have not yet seen a fixed mat, and this is a fixed mat, rejigger
    // the origins so that we use it as the world origin
    LOG_INFO("Robot.LocalizeToMat.LocalizingToFirstFixedMat",
             "Localizing robot %d to fixed %s mat for the first time.",
             GetID(), ObjectTypeToString(existingMatPiece->GetType()));

    if ((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
      LOG_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                "Failed to update robot %d's pose origin when (re-)localizing it.", GetID());
      return lastResult;
    }

    _localizedToFixedObject = true;
  }
  else if (IsLocalized() == false) {
    // If the robot is not yet localized, it is about to be, so we need to
    // update pose origins so that anything it has seen so far becomes rooted
    // to this mat's origin (whether mat is fixed or not)
    LOG_INFO("Robot.LocalizeToMat.LocalizingRobotFirstTime",
             "Localizing robot %d for the first time (to %s mat).",
             GetID(), ObjectTypeToString(existingMatPiece->GetType()));

    if ((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
      LOG_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                "Failed to update robot %d's pose origin when (re-)localizing it.", GetID());
      return lastResult;
    }

    if (!existingMatPiece->IsMoveable()) {
      // If this also happens to be a fixed mat, then we have now localized
      // to a fixed mat
      _localizedToFixedObject = true;
    }
  }

  // Add the new vision-based pose to the robot's history. Note that we use
  // the pose w.r.t. the origin for storing poses in history.
  // HistRobotState p(robot->GetPoseFrameID(),
  //                  robotPoseWrtMat.GetWithRespectToRoot(),
  //                  posePtr->GetComponent<FullRobotPose>().GetHeadAngle(),
  //                  posePtr->GetComponent<FullRobotPose>().GetLiftAngle());
  Pose3d robotPoseWrtOrigin = robotPoseWrtMat.GetWithRespectToRoot();

  if ((lastResult = AddVisionOnlyStateToHistory(existingMatPiece->GetLastObservedTime(),
                                                robotPoseWrtOrigin,
                                                histStatePtr->GetHeadAngle_rad(),
                                                histStatePtr->GetLiftAngle_rad())) != RESULT_OK)
  {
    LOG_ERROR("Robot.LocalizeToMat.FailedAddingVisionOnlyPoseToHistory", "");
    return lastResult;
  }


  // Update the computed historical pose as well so that subsequent block
  // pose updates use obsMarkers whose camera's parent pose is correct.
  // Note again that we store the pose w.r.t. the origin in history.
  // TODO: Should SetPose() do the flattening w.r.t. origin?
  histStatePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, histStatePtr->GetHeadAngle_rad(), histStatePtr->GetLiftAngle_rad());

  // Compute the new "current" pose from history which uses the
  // past vision-based "ground truth" pose we just computed.
  if (UpdateCurrPoseFromHistory() == false) {
    LOG_ERROR("Robot.LocalizeToMat.FailedUpdateCurrPoseFromHistory", "");
    return RESULT_FAIL;
  }

  // Mark the robot as now being localized to this mat
  // NOTE: this should be _after_ calling AddVisionOnlyStateToHistory, since
  //    that function checks whether the robot is already localized
  lastResult = SetLocalizedTo(existingMatPiece);
  if (RESULT_OK != lastResult) {
    LOG_ERROR("Robot.LocalizeToMat.SetLocalizedToFail", "");
    return lastResult;
  }

  // Overly-verbose. Use for debugging localization issues
  /*
    PRINT_INFO("Using %s mat %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f)\n",
    existingMatPiece->GetType().GetName().c_str(),
    existingMatPiece->GetID().GetValue(), GetID(),
    GetPose().GetTranslation().x(),
    GetPose().GetTranslation().y(),
    GetPose().GetTranslation().z(),
    GetPose().GetRotationAngle<'Z'>().getDegrees(),
    GetPose().GetRotationAxis().x(),
    GetPose().GetRotationAxis().y(),
    GetPose().GetRotationAxis().z());
  */

  // Send the ground truth pose that was computed instead of the new current
  // pose and let the robot deal with updating its current pose based on the
  // history that it keeps.
  SendAbsLocalizationUpdate();

  return RESULT_OK;

} // LocalizeToMat()


Result Robot::SetPoseOnCharger()
{
  ANKI_CPU_PROFILE("Robot::SetPoseOnCharger");

  Charger* charger = dynamic_cast<Charger*>(GetBlockWorld().GetLocatedObjectByID(_chargerID));
  if (charger == nullptr) {
    LOG_WARNING("Robot.SetPoseOnCharger.NoChargerWithID",
                "Robot %d has docked to charger, but Charger object with ID=%d not found in the world.",
                _ID, _chargerID.GetValue());
    return RESULT_FAIL;
  }

  // Just do an absolute pose update, setting the robot's position to
  // where we "know" he should be when he finishes ascending the charger.
  Result lastResult = SetNewPose(charger->GetRobotDockedPose().GetWithRespectToRoot());
  if (lastResult != RESULT_OK) {
    LOG_WARNING("Robot.SetPoseOnCharger.SetNewPose", "Robot %d failed to set new pose", _ID);
    return lastResult;
  }

  const TimeStamp_t timeStamp = GetStateHistory()->GetNewestTimeStamp();

  LOG_INFO("Robot.SetPoseOnCharger.SetPose",
           "Robot %d now on charger %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d",
           _ID, charger->GetID().GetValue(),
           GetComponent<FullRobotPose>().GetPose().GetTranslation().x(), GetComponent<FullRobotPose>().GetPose().GetTranslation().y(), GetComponent<FullRobotPose>().GetPose().GetTranslation().z(),
           GetComponent<FullRobotPose>().GetPose().GetRotationAngle<'Z'>().getDegrees(),
            timeStamp);

  return RESULT_OK;

} // SetPoseOnCharger()


Result Robot::SetPosePostRollOffCharger()
{
  auto* charger = dynamic_cast<Charger*>(GetBlockWorld().GetLocatedObjectByID(_chargerID));
  if (charger == nullptr) {
    PRINT_NAMED_WARNING("Robot.SetPosePostRollOffCharger.NoChargerWithID",
                        "Charger object with ID %d not found in the world.",
                        _chargerID.GetValue());
    return RESULT_FAIL;
  }

  // Just do an absolute pose update, setting the robot's position to
  // where we "know" he should be when he finishes rolling off the charger.
  Result lastResult = SetNewPose(charger->GetRobotPostRollOffPose().GetWithRespectToRoot());
  if (lastResult != RESULT_OK) {
    PRINT_NAMED_WARNING("Robot.SetPosePostRollOffCharger.SetNewPose", "Failed to set new pose");
    return lastResult;
  }

  PRINT_NAMED_INFO("Robot.SetPosePostRollOffCharger.NewRobotPose",
                   "Updated robot pose to be in front of the charger, as if it had just rolled off.");
  return RESULT_OK;
}


// ============ Messaging ================

Result Robot::SendMessage(const RobotInterface::EngineToRobot& msg, bool reliable, bool hot) const
{
  Result sendResult = GetContext()->GetRobotManager()->GetMsgHandler()->SendMessage(msg, reliable, hot);
  if (sendResult != RESULT_OK) {
    const char* msgTypeName = EngineToRobotTagToString(msg.GetTag());
    Util::sWarningF("Robot.SendMessage", { {DDATA, msgTypeName} }, "Robot %d failed to send a message type %s", _ID, msgTypeName);
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
    return SendAbsLocalizationUpdate(zeroPose, 0, GetPoseFrameID());
  }

  if (result != RESULT_OK) {
    LOG_WARNING("Robot.SendSyncRobot.FailedToSend","");
  }

  return result;
}

Result Robot::SendAbsLocalizationUpdate(const Pose3d&        pose,
                                        const TimeStamp_t&   t,
                                        const PoseFrameID_t& frameId) const
{
  // Send flattened poses to the robot, because when we get them back in odometry
  // updates with origin IDs, we can only hook them back up directly to the
  // corresponding pose origin (we can't know the chain that led there anymore)

  const Pose3d& poseWrtOrigin = pose.GetWithRespectToRoot();
  const Pose3d& origin = poseWrtOrigin.GetParent(); // poseWrtOrigin's parent is, by definition, the root / an origin
  DEV_ASSERT(origin.IsRoot(), "Robot.SendAbsLocalizationUpdate.OriginNotRoot");
  DEV_ASSERT(pose.HasSameRootAs(origin), "Robot.SendAbsLocalizationUpdate.ParentOriginMismatch");

  const PoseOriginID_t originID = origin.GetID();
  if (!GetPoseOriginList().ContainsOriginID(originID))
  {
    LOG_ERROR("Robot.SendAbsLocalizationUpdate.InvalidPoseOriginID",
              "Origin %d(%s)", originID, origin.GetName().c_str());
    return RESULT_FAIL;
  }

  return SendMessage(RobotInterface::EngineToRobot(
                       RobotInterface::AbsoluteLocalizationUpdate(
                         t,
                         frameId,
                         originID,
                         poseWrtOrigin.GetTranslation().x(),
                         poseWrtOrigin.GetTranslation().y(),
                         poseWrtOrigin.GetRotation().GetAngleAroundZaxis().ToFloat()
                         )));
}

Result Robot::SendAbsLocalizationUpdate() const
{
  // Look in history for the last vis pose and send it.
  TimeStamp_t t;
  HistRobotState histState;
  if (GetStateHistory()->GetLatestVisionOnlyState(t, histState) == RESULT_FAIL) {
    LOG_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
    return RESULT_FAIL;
  }

  return SendAbsLocalizationUpdate(histState.GetPose().GetWithRespectToRoot(), t, histState.GetFrameId());
}

Result Robot::SendHeadAngleUpdate() const
{
  return SendMessage(RobotInterface::EngineToRobot(
                       RobotInterface::HeadAngleUpdate(GetComponent<FullRobotPose>().GetHeadAngle())));
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
void Robot::HandleMessage(const ExternalInterface::EnableDroneMode& msg)
{
  _isCliffReactionDisabled = msg.isStarted;
  SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(!msg.isStarted)));
}

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
                                                    GetBodySerialNumber(),
                                                    _modelNumber,
                                                    GetBodyHWVersion(),
                                                    std::move(cameraConfig),
                                                    GetBodyColor());

  Broadcast( ExternalInterface::MessageEngineToGame(std::move(robotSettings)) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t Robot::GetLastImageTimeStamp() const {
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
  if(kFakeRobotBeingHeld){
    return OffTreadsState::Held;
  }
  return _offTreadsState;
}


Result Robot::RequestIMU(const u32 length_ms) const
{
  return SendIMURequest(length_ms);
}

// ============ Pose history ===============

Result Robot::UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin)
{
  // Reverse the connection between origin and robot, and connect the new
  // reversed connection
  //ASSERT_NAMED(p.GetPose().GetParent() == _poseOrigin, "Robot.UpdateWorldOrigin.InvalidPose");
  //Pose3d originWrtRobot = GetComponent<FullRobotPose>().GetPose().GetInverse();
  //originWrtRobot.SetParent(&newPoseOrigin);

  // TODO: Update to use PoseOriginList::Rejigger
  // This is only called by LocalizeToMat, which is not currently used.
  DEV_ASSERT(false, "Robot.UpdateWorldOrigin.NeedsUpdateToUseRejigger");

# if 0
  // TODO: get rid of nasty const_cast somehow
  Pose3d* newOrigin = const_cast<Pose3d*>(newPoseWrtNewOrigin.GetParent());
  newOrigin->SetParent(nullptr);

  // TODO: We should only be doing this (modifying what _worldOrigin points to) when it is one of the
  // placeHolder poseOrigins, not if it is a mat!
  std::string origName(_worldOrigin->GetName());
  *_worldOrigin = GetComponent<FullRobotPose>().GetPose().GetInverse();
  _worldOrigin->SetParent(&newPoseWrtNewOrigin);


  // Connect the old origin's pose to the same root the robot now has.
  // It is no longer the robot's origin, but for any of its children,
  // it is now in the right coordinates.
  if (_worldOrigin->GetWithRespectTo(*newOrigin, *_worldOrigin) == false) {
    LOG_ERROR("Robot.UpdateWorldOrigin.NewLocalizationOriginProblem",
              "Could not get pose origin w.r.t. new origin pose.");
    return RESULT_FAIL;
  }

  //_worldOrigin->PreComposeWith(*newOrigin);

  // Preserve the old world origin's name, despite updates above
  _worldOrigin->SetName(origName);

  // Now make the robot's world origin point to the new origin
  _worldOrigin = newOrigin;

  newOrigin->SetRotation(0, Z_AXIS_3D());
  newOrigin->SetTranslation({0,0,0});

  // Now make the robot's origin point to the new origin
  // TODO: avoid the icky const_cast here...
  _worldOrigin = const_cast<Pose3d*>(newPoseWrtNewOrigin.GetParent());

  _robotWorldOriginChangedSignal.emit(GetID());
# endif

  return RESULT_OK;

} // UpdateWorldOrigin()


Result Robot::AddVisionOnlyStateToHistory(const TimeStamp_t t,
                                         const Pose3d& pose,
                                         const f32 head_angle,
                                         const f32 lift_angle)
{
  // We have a new ("ground truth") key frame. Increment the pose frame!
  ++_frameId;

  // Set needToSendLocalizationUpdate to true so we send an update on the next tick
  _needToSendLocalizationUpdate = true;

  HistRobotState histState;
  histState.SetPose(_frameId, pose, head_angle, lift_angle);
  return GetStateHistory()->AddVisionOnlyState(t, histState);
}

Result Robot::GetComputedStateAt(const TimeStamp_t t_request, Pose3d& pose) const
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
bool Robot::UpdateCurrPoseFromHistory()
{
  bool poseUpdated = false;

  TimeStamp_t t;
  HistRobotState histState;
  if (GetStateHistory()->ComputeStateAt(GetStateHistory()->GetNewestTimeStamp(), t, histState) == RESULT_OK)
  {
    const Pose3d& worldOrigin = GetWorldOrigin();
    Pose3d newPose;
    if ((histState.GetPose().GetWithRespectTo(worldOrigin, newPose))==false)
    {
      // This is not necessarily an error anymore: it's possible we've received an
      // odometry update from the robot w.r.t. an old origin (before being delocalized),
      // in which case we can't use it to update the current pose of the robot
      // in its new frame.
      LOG_INFO("Robot.UpdateCurrPoseFromHistory.GetWrtParentFailed",
               "Could not update robot %d's current pose using historical pose w.r.t. %s because we are now in frame %s.",
               _ID, histState.GetPose().FindRoot().GetName().c_str(),
               worldOrigin.GetName().c_str());
    }
    else
    {
      SetPose(newPose);
      poseUpdated = true;
    }

  }

  return poseUpdated;
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

Result Robot::SendDebugString(const char* format, ...)
{
  int len = 0;
  const int kMaxDebugStringLen = std::numeric_limits<u8>::max();
  char text[kMaxDebugStringLen];
  strcpy(text, format);

  // Create formatted text
  va_list argptr;
  va_start(argptr, format);
  len = vsnprintf(text, kMaxDebugStringLen, format, argptr);
  va_end(argptr);

  std::string str(text);

  // Send message to game
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::DebugString(str)));

  return RESULT_OK;
}


void Robot::ComputeDriveCenterPose(const Pose3d &robotPose, Pose3d &driveCenterPose) const
{
  MoveRobotPoseForward(robotPose, GetDriveCenterOffset(), driveCenterPose);
}

void Robot::ComputeOriginPose(const Pose3d &driveCenterPose, Pose3d &robotPose) const
{
  MoveRobotPoseForward(driveCenterPose, -GetDriveCenterOffset(), robotPose);
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

void Robot::BroadcastEngineErrorCode(EngineErrorCode error)
{
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::EngineErrorCodeMessage(error)));
  LOG_ERROR("Robot.BroadcastEngineErrorCode",
            "Engine failing with error code %s[%hhu]",
            EnumToString(error),
            error);
}

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
    msg.carryingObjectID = GetCarryingComponent().GetCarryingObject();
    msg.carryingObjectOnTopID = GetCarryingComponent().GetCarryingObjectOnTop();
  } else {
    msg.carryingObjectID = -1;
    msg.carryingObjectOnTopID = -1;
  }

  msg.gameStatus = 0;
  if (IsLocalized() && _offTreadsState == OffTreadsState::OnTreads) { msg.gameStatus |= (uint8_t)GameStatusFlag::IsLocalized; }

  msg.headTrackingObjectID = GetMoveComponent().GetTrackToObject();

  msg.localizedToObjectID = GetLocalizedTo();

  msg.batteryVoltage = GetBatteryComponent().GetBatteryVolts();

  msg.lastImageTimeStamp = GetVisionComponent().GetLastProcessedImageTimeStamp();

  return msg;
}

RobotState Robot::GetDefaultRobotState()
{
  const auto kDefaultStatus = (Util::EnumToUnderlying(RobotStatusFlag::HEAD_IN_POS) |
                               Util::EnumToUnderlying(RobotStatusFlag::LIFT_IN_POS));

  const RobotPose kDefaultPose(0.f, 0.f, 0.f, 0.f, 0.f);

  std::array<uint16_t, Util::EnumToUnderlying(CliffSensor::CLIFF_COUNT)> defaultCliffRawVals;
  defaultCliffRawVals.fill(std::numeric_limits<uint16_t>::max());

  const RobotState state(1, //uint32_t timestamp, (Robot does not report at t=0
                         0, //uint32_t pose_frame_id,
                         1, //uint32_t pose_origin_id,
                         kDefaultPose, //const Anki::Cozmo::RobotPose &pose,
                         0.f, //float lwheel_speed_mmps,
                         0.f, //float rwheel_speed_mmps,
                         0.f, //float headAngle
                         0.f, //float liftAngle,
                         AccelData(), //const Anki::Cozmo::AccelData &accel,
                         GyroData(), //const Anki::Cozmo::GyroData &gyro,
                         0.f, // float batteryVoltage
                         0.f, // float chargerVoltage
                         kDefaultStatus, //uint32_t status,
                         std::move(defaultCliffRawVals), //std::array<uint16_t, 4> cliffDataRaw,
                         ProxSensorDataRaw(), //const Anki::Cozmo::ProxSensorDataRaw &proxData,
                         0, // touch intensity value when not touched (from capacitive touch sensor)
                         0, // uint8_t cliffDetectedFlags,
                         0, // uint8_t whiteDetectedFlags
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

Result Robot::ComputeTurnTowardsImagePointAngles(const Point2f& imgPoint, const TimeStamp_t timestamp,
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
  TimeStamp_t t;
  Result result = GetStateHistory()->ComputeStateAt(timestamp, t, histState);
  if (RESULT_OK != result)
  {
    LOG_WARNING("Robot.ComputeTurnTowardsImagePointAngles.ComputeHistPoseFailed", "t=%u", timestamp);
    absPanAngle = GetPose().GetRotation().GetAngleAroundZaxis();
    absTiltAngle = GetComponent<FullRobotPose>().GetHeadAngle();
    return result;
  }

  absTiltAngle = std::atan2f(-pt.y(), calib->GetFocalLength_y()) + histState.GetHeadAngle_rad();
  absPanAngle  = std::atan2f(-pt.x(), calib->GetFocalLength_x()) + histState.GetPose().GetRotation().GetAngleAroundZaxis();

  return RESULT_OK;
}

void Robot::SetBodyColor(const s32 color)
{
  const BodyColor bodyColor = static_cast<BodyColor>(color);
  if (bodyColor <= BodyColor::UNKNOWN ||
      bodyColor >= BodyColor::COUNT ||
      bodyColor == BodyColor::RESERVED)
  {
    LOG_ERROR("Robot.SetBodyColor.InvalidColor", "Robot has invalid body color %d", color);
    return;
  }

  _bodyColor = bodyColor;
}

void Robot::DevReplaceAIComponent(AIComponent* aiComponent, bool shouldManage)
{
  IDependencyManagedComponent<Anki::Cozmo::RobotComponentID>* explicitUpcast = aiComponent;
  _components->DevReplaceDependentComponent(RobotComponentID::AIComponent,
                                            explicitUpcast,
                                            shouldManage);
}

Result Robot::UpdateStartupChecks()
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
      u8* buf = nullptr;
      u32 id = 0;
      TimeStamp_t t = 0;
      ImageEncoding format;
      if(CameraService::getInstance()->CameraGetFrame(buf, id, t, format))
      {
        CameraService::getInstance()->CameraReleaseFrame(id);
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

  return (state == State::FAILED ? RESULT_FAIL : RESULT_OK);
}

bool Robot::SetLocale(const std::string & locale)
{
  // Note: Since Locale utility has no error returns, we can't detect error;
  // an invalid locale results in setting to the default en-US
  DEV_ASSERT(_context != nullptr, "Robot.SetLocale.InvalidContext");
  _context->SetLocale(locale);

  // Notify animation process
  SendRobotMessage<RobotInterface::SetLocale>(locale);

  return true;
}

} // namespace Cozmo
} // namespace Anki
