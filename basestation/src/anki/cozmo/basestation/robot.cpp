
//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/robot.h"

#ifdef COZMO_V2
#include "anki/cozmo/basestation/androidHAL/androidHAL.h"
#endif

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/poseOriginList.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/activeObjectHelpers.h"
#include "anki/cozmo/basestation/animations/engineAnimationController.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blocks/blockFilter.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/components/blockTapFilterComponent.h"
#include "anki/cozmo/basestation/components/bodyLightComponent.h"
#include "anki/cozmo/basestation/components/cubeAccelComponent.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/components/pathComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/components/publicStateBroadcaster.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/objectPoseConfirmer.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/cozmo/basestation/proceduralFace.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotGyroDriftDetector.h"
#include "anki/cozmo/basestation/robotIdleTimeoutComponent.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotStateHistory.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"
#include "anki/cozmo/basestation/textToSpeech/textToSpeechComponent.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/vision/basestation/visionMarker.h"
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

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp" // For imwrite() in ProcessImage

#include <fstream>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>

#define IS_STATUS_FLAG_SET(x) ((msg.status & (uint32_t)RobotStatusFlag::x) != 0)

namespace Anki {
namespace Cozmo {
  
/*
// static initializers
const RotationMatrix3d Robot::_kDefaultHeadCamRotation = RotationMatrix3d({
0, 0, 1,
-1, 0, 0,
0,-1, 0
});
*/

CONSOLE_VAR(bool, kDebugPossibleBlockInteraction, "Robot", false);
  
// if false, vision system keeps running while picked up, on side, etc.
CONSOLE_VAR(bool, kUseVisionOnlyWhileOnTreads,    "Robot", false);

// Whether or not to lower the cliff detection threshold on the robot
// whenever a suspicious cliff is encountered.
CONSOLE_VAR(bool, kDoProgressiveThresholdAdjustOnSuspiciousCliff, "Robot", true);

// Play an animation by name from the debug console.
// Note: If COZMO-11199 is implemented (more user-friendly playing animations by name
//   on the Unity side), then this console func can be removed.
#if REMOTE_CONSOLE_ENABLED
static Robot* _thisRobot = nullptr;
static void PlayAnimationByName(ConsoleFunctionContextRef context)
{
  if (_thisRobot != nullptr) {
    const char* animName = ConsoleArg_Get_String(context, "animName");
    _thisRobot->GetActionList().QueueAction(QueueActionPosition::NOW,
                                            new PlayAnimationAction(*_thisRobot, animName));
  }
}
CONSOLE_FUNC(PlayAnimationByName, "PlayAnimationByName", const char* animName);
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
static const float kOnLeftSide_rawYAccel = -9000.0f;
static const float kOnRightSide_rawYAccel = 10500.0f;
static const float kOnSideTolerance_rawYAccel =3000.0f;

// On face angles
static const float kPitchAngleOnFacePlantMin_rads = DEG_TO_RAD(110.f);
static const float kPitchAngleOnFacePlantMax_rads = DEG_TO_RAD(-80.f);
static const float kPitchAngleOnFacePlantMin_sim_rads = DEG_TO_RAD(110.f); //This has not been tested
static const float kPitchAngleOnFacePlantMax_sim_rads = DEG_TO_RAD(-80.f); //This has not been tested

// For tool code reading
// 4-degree look down: (Make sure to update cozmoBot.proto to match!)
const RotationMatrix3d Robot::_kDefaultHeadCamRotation = RotationMatrix3d({
  0,             -0.0698f,   0.9976f,
 -1.0000f,        0,         0,
  0,             -0.9976f,  -0.0698f,
});

  
  
// Minimum value that cliff detection threshold can be dynamically lowered to
static const u32 kCliffSensorMinDetectionThresh = 150;

// Amount by which cliff detection level can be lowered when suspiciously cliff-y floor detected
static const u32 kCliffSensorDetectionThreshStep = 250;

// Number of suspicious cliffs that must be encountered before detection threshold is lowered
static const u32 kCliffSensorSuspiciousCliffCount = 1;

// Size of running window of cliff data for computing running mean/variance in number of RobotState messages (which arrive every 30ms)
static const u32 kCliffSensorRunningStatsWindowSize = 100;
  
// Cliff variance threshold for detecting suspiciously cliffy floor/carpet
static const f32 kCliffSensorSuspiciouslyCliffyFloorThresh = 10000;
  
// The minimum amount by which the cliff data must be above the minimum value observed since stopping
// began in order to be considered a suspicious cliff. (i.e. We might be on crazy carpet)
static const u16 kMinRiseDuringStopForSuspiciousCliff = 15;

Robot::Robot(const RobotID_t robotID, const CozmoContext* context)
  : _context(context)
  , _ID(robotID)
  , _timeSynced(false)
  , _lastMsgTimestamp(0)
  , _blockWorld(new BlockWorld(this))
  , _faceWorld(new FaceWorld(*this))
  , _petWorld(new PetWorld(*this))
  , _publicStateBroadcaster(new PublicStateBroadcaster())
  , _behaviorMgr(new BehaviorManager(*this))
  , _audioClient(new Audio::RobotAudioClient(this))
  , _pathComponent(new PathComponent(*this, robotID, context))
  , _animationStreamer(_context, *_audioClient)
  , _drivingAnimationHandler(new DrivingAnimationHandler(*this))
#if BUILD_NEW_ANIMATION_CODE
  , _animationController(new RobotAnimation::EngineAnimationController(_context, _audioClient.get()))
#endif
  , _actionList(new ActionList())
  , _movementComponent(new MovementComponent(*this))
  , _visionComponent( new VisionComponent(*this, _context))
  , _nvStorageComponent(new NVStorageComponent(*this, _context))
  , _aiComponent(new AIComponent(*this))
  , _textToSpeechComponent(new TextToSpeechComponent(_context))
  , _objectPoseConfirmerPtr(new ObjectPoseConfirmer(*this))
  , _cubeLightComponent(new CubeLightComponent(*this, _context))
  , _bodyLightComponent(new BodyLightComponent(*this, _context))
  , _cubeAccelComponent(new CubeAccelComponent(*this))
  , _gyroDriftDetector(std::make_unique<RobotGyroDriftDetector>(*this))
  , _poseOriginList(new PoseOriginList())
  , _neckPose(0.f,Y_AXIS_3D(),
              {NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}, &_pose, "RobotNeck")
  , _headCamPose(_kDefaultHeadCamRotation,
                 {HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}, &_neckPose, "RobotHeadCam")
  , _liftBasePose(0.f, Y_AXIS_3D(),
                  {LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}, &_pose, "RobotLiftBase")
  , _liftPose(0.f, Y_AXIS_3D(), {LIFT_ARM_LENGTH, 0.f, 0.f}, &_liftBasePose, "RobotLift")
  , _currentHeadAngle(MIN_HEAD_ANGLE)
  , _cliffDetectThreshold(CLIFF_SENSOR_DROP_LEVEL)
  , _stateHistory(new RobotStateHistory())
  , _moodManager(new MoodManager(this))
  , _needsManager(new NeedsManager(*this))
  , _progressionUnlockComponent(new ProgressionUnlockComponent(*this))
  , _blockFilter(new BlockFilter(this, context->GetExternalInterface()))
  , _tapFilterComponent(new BlockTapFilterComponent(*this))
  , _lastDisconnectedCheckTime(0)
  , _robotToEngineImplMessaging(new RobotToEngineImplMessaging(this))
  , _robotIdleTimeoutComponent(new RobotIdleTimeoutComponent(*this))
{
  PRINT_NAMED_INFO("Robot.Robot", "Created");
      
  _pose.SetName("Robot_" + std::to_string(_ID));
  _driveCenterPose.SetName("RobotDriveCenter_" + std::to_string(_ID));
      
  // Initializes _pose, _poseOrigins, and _worldOrigin:
  Delocalize(false);
  
  // The call to Delocalize() will increment frameID, but we want it to be
  // initialzied to 0, to match the physical robot's initialization
  // It will also add to history so clear it
  // It will also flag that a localization update is needed when it increments the frameID so set the flag
  // to false
  _frameId = 0;
  _stateHistory->Clear();
  _needToSendLocalizationUpdate = false;
  
  // Delocalize will mark isLocalized as false, but we are going to consider
  // the robot localized (by odometry alone) to start, until he gets picked up.
  _isLocalized = true;
  SetLocalizedTo(nullptr);

  _robotToEngineImplMessaging->InitRobotMessageComponent(_context->GetRobotManager()->GetMsgHandler(),robotID, this);
      
  _lastDebugStringHash = 0;
      
  // Read in Mood Manager Json
  if (nullptr != _context->GetDataPlatform())
  {
    _moodManager->Init(_context->GetDataLoader()->GetRobotMoodConfig());
    LoadEmotionEvents();
  }
  
  if (_context->GetDataPlatform() != nullptr)
  {
    _needsManager->Init(_context->GetDataLoader()->GetRobotNeedsConfig(),
                        _context->GetDataLoader()->GetStarRewardsConfig(),
                        _context->GetDataLoader()->GetRobotNeedsActionsConfig());
  }

  // Initialize progression
  _progressionUnlockComponent->Init();

  _behaviorMgr->InitConfiguration(_context->GetDataLoader()->GetRobotActivitiesConfig());
  _behaviorMgr->InitReactionTriggerMap(_context->GetDataLoader()->GetReactionTriggerMap());
  
  // Setting camera pose according to current head angle.
  // (Not using SetHeadAngle() because _isHeadCalibrated is initially false making the function do nothing.)
  _visionComponent->GetCamera().SetPose(GetCameraPose(_currentHeadAngle));
        
  if (nullptr != _context->GetDataPlatform())
  {
    _visionComponent->Init(_context->GetDataLoader()->GetRobotVisionConfig());
  }
  
# ifndef COZMO_V2   // TODO: RobotDataBackupManager needs to reside on the Unity-side for Cozmo 2.0
  // Read all necessary data off the robot and back it up
  // Potentially duplicates some reads like FaceAlbumData
  _nvStorageComponent->GetRobotDataBackupManager().ReadAllBackupDataFromRobot();
# endif

  // initialize AI
  _aiComponent->Init();
  
  // Used for CONSOLE_FUNCTION "PlayAnimationByName" above
#if REMOTE_CONSOLE_ENABLED
  _thisRobot = this;
#endif
  
} // Constructor: Robot
    
Robot::~Robot()
{
  AbortAll();
  
  // This needs to happen before ActionList is destroyed, because otherwise behaviors will try to respond
  // to actions shutting down
  _behaviorMgr.reset();
  
  // Destroy our actionList before things like the path planner, since actions often rely on those.
  // ActionList must be cleared before it is destroyed because pending actions may attempt to make use of the pointer.
  _actionList->Clear();
  _actionList.reset();
      
  // destroy vision component first because its thread might be using things from Robot. This fixes a crash
  // caused by the vision thread using _stateHistory when it was destroyed here
  _visionComponent.reset();
  
  // Destroy the state history
  _stateHistory.reset();
  
  Util::SafeDelete(_moodManager);
  Util::SafeDelete(_needsManager);
  Util::SafeDelete(_progressionUnlockComponent);
  Util::SafeDelete(_tapFilterComponent);
  Util::SafeDelete(_blockFilter);
}
    
void Robot::SetOnCharger(bool onCharger)
{
  // If we are being set on a charger, we can update the instance of the charger in the current world to
  // match the robot. If we don't have an instance, we can add an instance now
  if ( onCharger )
  {
    const Pose3d& poseWrtRobot = Charger::GetDockPoseRelativeToRobot(*this);
    
    // find instance in current origin
    BlockWorldFilter filter;
    filter.AddAllowedFamily(ObjectFamily::Charger);
    filter.AddAllowedType(ObjectType::Charger_Basic);
    ObservableObject* chargerInstance = GetBlockWorld().FindLocatedMatchingObject(filter);
    if ( nullptr == chargerInstance )
    {
      // there's currently no located instance, we need to create one.
      ActiveID unconnectedActiveID = ObservableObject::InvalidActiveID;
      FactoryID unconnectedFactoryID = ObservableObject::InvalidFactoryID;
      chargerInstance = CreateActiveObjectByType(ObjectType::Charger_Basic, unconnectedActiveID, unconnectedFactoryID);

      // check if there is a connected instance, because we can inherit its ID (objectID)
      // note that setting ActiveID and FactoryID is responsibility of whenever we Add the object to the BlockWorld
      ActiveObject* connectedInstance = GetBlockWorld().FindConnectedActiveMatchingObject(filter);
      if ( nullptr != connectedInstance ) {
        chargerInstance->CopyID(connectedInstance);
      } else {
        chargerInstance->SetID();
      }
    }

    // pretend the instance we created was an observation. Note that lastObservedTime will be 0 in this case, since
    // that timestamp refers to visual observations only (TODO: maybe that should be more explicit or any
    // observation should set that timestamp)
    GetObjectPoseConfirmer().AddRobotRelativeObservation(chargerInstance, poseWrtRobot, PoseState::Known);
  }
  
  // log events when onCharger status changes
  if (onCharger && !_isOnCharger)
  {
    // if we are on the charger, we must also be on the charger platform.
    SetOnChargerPlatform(true);
	  
    // offCharger -> onCharger
    LOG_EVENT("robot.on_charger", "");
    Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ChargerEvent(true)));
  }
  else if (!onCharger && _isOnCharger)
  {
    // onCharger -> offCharger
    LOG_EVENT("robot.off_charger", "");
    Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ChargerEvent(false)));
  }

  // update flag now (note this gets updated after notifying; this might be an issue for listeners)
  _isOnCharger = onCharger;
}

void Robot::SetOnChargerPlatform(bool onPlatform)
{
  // Can only not be on platform if not on charge contacts
  onPlatform = onPlatform || IsOnCharger();
  
  const bool shouldBroadcast = _isOnChargerPlatform != onPlatform;
  _isOnChargerPlatform = onPlatform;
  
  if( shouldBroadcast ) {
    Broadcast(
      ExternalInterface::MessageEngineToGame(
        ExternalInterface::RobotOnChargerPlatformEvent(_isOnChargerPlatform))
      );
  }
}
    
void Robot::IncrementSuspiciousCliffCount()
{
  if (_cliffDetectThreshold > kCliffSensorMinDetectionThresh) {
    ++_suspiciousCliffCnt;
    LOG_EVENT("IncrementSuspiciousCliffCount.Count", "%d", _suspiciousCliffCnt);
    if (_suspiciousCliffCnt >= kCliffSensorSuspiciousCliffCount) {
      _cliffDetectThreshold = MAX(_cliffDetectThreshold - kCliffSensorDetectionThreshStep, kCliffSensorMinDetectionThresh);
      LOG_EVENT("IncrementSuspiciousCliffCount.NewThreshold", "%d", _cliffDetectThreshold);
      SendRobotMessage<RobotInterface::SetCliffDetectThreshold>(_cliffDetectThreshold);
      _suspiciousCliffCnt = 0;
    }
  }
}

void Robot::EvaluateCliffSuspiciousnessWhenStopped() {
  if (kDoProgressiveThresholdAdjustOnSuspiciousCliff) {
    _cliffStartTimestamp = GetLastMsgTimestamp();
  }
}
  
// Updates mean and variance of cliff readings observed in last kCliffSensorRunningStatsWindowSize RobotState msgs.
// Based on Welford Algorithm. See http://stackoverflow.com/questions/5147378/rolling-variance-algorithm
void Robot::UpdateCliffRunningStats(const RobotState& msg) {
  
  u16 obs = msg.cliffDataRaw;
  if (GetMoveComponent().AreWheelsMoving() && (GetOffTreadsState() == OffTreadsState::OnTreads) && obs > (_cliffDetectThreshold)) {

    _cliffDataQueue.push_back(obs);
    if (_cliffDataQueue.size() > kCliffSensorRunningStatsWindowSize) {
      // Popping oldest value. Update stats.
      f32 oldestObs = _cliffDataQueue.front();
      _cliffDataQueue.pop_front();
      f32 prevMean = _cliffRunningMean;
      _cliffRunningMean += (obs - oldestObs) / kCliffSensorRunningStatsWindowSize;
      _cliffRunningVar_acc += (obs - prevMean) * (obs - _cliffRunningMean) - (oldestObs - prevMean) * (oldestObs - _cliffRunningMean);
    } else {
      // Queue not full yet. Just update stats
      f32 delta = obs - _cliffRunningMean;
      _cliffRunningMean += delta / _cliffDataQueue.size();
      _cliffRunningVar_acc += delta * (obs - _cliffRunningMean);
    }
    
    // Compute running variance based on number of samples in queue/window
    if (_cliffDataQueue.size() > 1) {
      _cliffRunningVar = _cliffRunningVar_acc / (_cliffDataQueue.size() - 1);
    }
  }
}
  

void Robot::ClearCliffRunningStats() {
  _cliffDataQueue.clear();
  _cliffRunningMean = 0;
  _cliffRunningVar = 0;
  _cliffRunningVar_acc = 0;
}

void Robot::UpdateCliffDetectThreshold() {
  // Check pose history for cliff readings to see if cliff readings went up
  // after stopping indicating a suspicious cliff.
  if (_cliffStartTimestamp > 0) {
    if( !GetMoveComponent().AreWheelsMoving() ) {
      
      auto poseMap = GetStateHistory()->GetRawPoses();
      u16 minVal = std::numeric_limits<u16>::max();
      for (auto startIt = poseMap.lower_bound(_cliffStartTimestamp); startIt != poseMap.end(); ++startIt) {
        u16 currVal = startIt->second.GetCliffData();
        PRINT_NAMED_DEBUG("Robot.UpdateCliffRunningStats.CliffValueWhileStopping", "%d - %d", startIt->first, currVal);
        
        if (minVal > currVal) {
          minVal = currVal;
        }
        
        // If a cliff reading is ever more than a certain amount above the minimum observed value since
        // it began stopping, the cliff it is reacting to is suspect (probably carpet) so don't bother doing the reaction.
        // Note: This is mostly from testing on the office carpet.
        if (IsFloorSuspiciouslyCliffy() && (currVal > minVal + kMinRiseDuringStopForSuspiciousCliff)) {
          IncrementSuspiciousCliffCount();
        }
      }
      
      _cliffStartTimestamp = 0;
    }
  }
}

  
// The variance of the recent cliff readings is what we use to determine the cliffyness of the floor
bool Robot::IsFloorSuspiciouslyCliffy() const {
  return (_cliffRunningVar > kCliffSensorSuspiciouslyCliffyFloorThresh);
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
  const bool currOntreads = std::abs(GetPitchAngle().ToDouble() - kPitchAngleOntreads_rads) <= kPitchAngleOntreadsTolerance_rads;
  
  //// COZMO_ON_BACK
  const float backAngle = IsPhysical() ? kPitchAngleOnBack_rads : kPitchAngleOnBack_sim_rads;
  const bool currOnBack = std::abs( GetPitchAngle().ToDouble() - backAngle ) <= DEG_TO_RAD( kPitchAngleOnBackTolerance_deg );
  //// COZMO_ON_SIDE
  const float rawY = msg.accel.y;
  const bool onRightSide =  (std::abs(rawY - kOnRightSide_rawYAccel) <= kOnSideTolerance_rawYAccel);
  const bool currOnSide = onRightSide || (std::abs(rawY - kOnLeftSide_rawYAccel) <= kOnSideTolerance_rawYAccel);
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
  else if(currOnSide){
    if(_awaitingConfirmationTreadState != OffTreadsState::OnRightSide
       && _awaitingConfirmationTreadState != OffTreadsState::OnLeftSide)
    {
      // Transition to Robot on Side
      if(onRightSide){
        _awaitingConfirmationTreadState = OffTreadsState::OnRightSide;
      }else{
        _awaitingConfirmationTreadState = OffTreadsState::OnLeftSide;
      }
      _timeOffTreadStateChanged_ms = currentTimestamp;
    }
  }
  else if(currFacePlant
          && _awaitingConfirmationTreadState != OffTreadsState::OnFace)
  {
    // Transition to Robot on Face
    _awaitingConfirmationTreadState = OffTreadsState::OnFace;
    _timeOffTreadStateChanged_ms = currentTimestamp;
  }
  else if(currOnBack
          && _awaitingConfirmationTreadState != OffTreadsState::OnBack)
  {
    // Transition to Robot on Back
    _awaitingConfirmationTreadState = OffTreadsState::OnBack;
    // On Back is a special case as it is also an intermediate state for coming from onface -> ontreads. hence we wait a little longer than usual(kRobotTimeToConsiderOfftreads_ms) to check if it's on back.
    _timeOffTreadStateChanged_ms = currentTimestamp + kRobotTimeToConsiderOfftreadsOnBack_ms;
  }
  else if(currOntreads
          && _awaitingConfirmationTreadState != OffTreadsState::InAir
          && _awaitingConfirmationTreadState != OffTreadsState::OnTreads)
  {
    _awaitingConfirmationTreadState = OffTreadsState::InAir;
    _timeOffTreadStateChanged_ms = currentTimestamp;
  }// end if(isFalling)
    
  /////
  // Message based tred state transitions
  ////
  
  // Transition from ontreads to InAir - happens instantly
  if(_awaitingConfirmationTreadState == OffTreadsState::OnTreads && isPickedUp == true) {
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
    
    // Check the lift to see if tool changed while we were picked up
    //_actionList->QueueActionNext(new ReadToolCodeAction(*this));
  }
  
  //////////
  // A new tred state has been confirmed
  //////////
  if(_timeOffTreadStateChanged_ms + kRobotTimeToConsiderOfftreads_ms <= currentTimestamp
     && _offTreadsState != _awaitingConfirmationTreadState)
  {
    if(kUseVisionOnlyWhileOnTreads && _offTreadsState == OffTreadsState::OnTreads)
    {
      // Pause vision if we just left treads
      _visionComponent->Pause(true);
    }
    
    // Falling seems worthy of a DAS event
    if (_awaitingConfirmationTreadState == OffTreadsState::Falling) {
      _fallingStartedTime_ms = GetLastMsgTimestamp();
      LOG_EVENT("Robot.CheckAndUpdateTreadsState.FallingStarted",
                        "t=%dms",
                        _fallingStartedTime_ms);
      
      // Stop all actions
      GetActionList().Cancel();
      
    } else if (_offTreadsState == OffTreadsState::Falling) {
      // This is not an exact measurement of fall time since it includes some detection delays on the robot side
      // It may also include kRobotTimeToConsiderOfftreads_ms depending on how the robot lands
      LOG_EVENT("Robot.CheckAndUpdateTreadsState.FallingStopped",
                        "t=%dms, duration=%dms",
                        GetLastMsgTimestamp(), GetLastMsgTimestamp() - _fallingStartedTime_ms);
      _fallingStartedTime_ms = 0;
    }
    
    _offTreadsState = _awaitingConfirmationTreadState;
    Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotOffTreadsStateChanged(GetID(), _offTreadsState)));
    PRINT_CH_INFO("RobotState", "Robot.OfftreadsState.TreadStateChanged", "TreadState changed to:%s", EnumToString(_offTreadsState));
    
    // Special case logic for returning to treads
    if(_offTreadsState == OffTreadsState::OnTreads){
      
      if(kUseVisionOnlyWhileOnTreads)
      {
        // Re-enable vision if we've returned to treads
        _visionComponent->Pause(false);
      }
      
      DEV_ASSERT(!IsLocalized(), "Robot should be delocalized when first put back down!");
      
      // If we are not localized and there is nothing else left in the world that
      // we could localize to, then go ahead and mark us as localized (via
      // odometry alone)
      if(false == _blockWorld->AnyRemainingLocalizableObjects()) {
        PRINT_NAMED_INFO("Robot.UpdateOfftreadsState.NoMoreRemainingLocalizableObjects",
                         "Marking previously-unlocalized robot %d as localized to odometry because "
                         "there are no more objects to localize to in the world.", GetID());
        SetLocalizedTo(nullptr); // marks us as localized to odometry only
      }
    }
    else if(IsCarryingObject() &&
            _offTreadsState != OffTreadsState::InAir &&
            _offTreadsState != OffTreadsState::Count)
    {
      // If we're falling or not upright and were carrying something, assume we
      // are no longer carrying that something and don't know where it is anymore
      const bool clearObjects = true; // To mark as Unknown, not just Dirty
      SetCarriedObjectAsUnattached(clearObjects);
    }

    // if the robot was on the charging platform and its state changes it's not on the platform anymore
    if(_offTreadsState != OffTreadsState::OnTreads)
    {
      SetOnChargerPlatform(false);
    }
    
    return true;
  }
  
  return false;
}
    
const Util::RandomGenerator& Robot::GetRNG() const
{
  return *_context->GetRandom();
}

Util::RandomGenerator& Robot::GetRNG()
{
  return *_context->GetRandom();
}

void Robot::Delocalize(bool isCarryingObject)
{
  _isLocalized = false;
  _localizedToID.UnSet();
  _localizedToFixedObject = false;
  _localizedMarkerDistToCameraSq = -1.f;
  
  // Reset cliff threshold
  _suspiciousCliffCnt = 0;
  if (_cliffDetectThreshold != CLIFF_SENSOR_DROP_LEVEL) {
    _cliffDetectThreshold = CLIFF_SENSOR_DROP_LEVEL;
    LOG_EVENT("Robot.Delocalize.RestoringCliffDetectThreshold", "%d", _cliffDetectThreshold);
    SendRobotMessage<RobotInterface::SetCliffDetectThreshold>(_cliffDetectThreshold);
  }
  
  ClearCliffRunningStats();

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
      
  // Add a new pose origin to use until the robot gets localized again
  const Pose3d* oldOrigin = _worldOrigin;
  _worldOrigin = new Pose3d();
  PoseOriginID_t originID = _poseOriginList->AddOrigin(_worldOrigin);
  _worldOrigin->SetName("Robot" + std::to_string(_ID) + "_PoseOrigin" + std::to_string(originID));
  
  // Log delocalization, new origin name, and num origins to DAS
  LOG_EVENT("Robot.Delocalize", "Delocalizing robot %d. New origin: %s. NumOrigins=%zu",
            GetID(), _worldOrigin->GetName().c_str(), _poseOriginList->GetSize());
  
  _pose.SetRotation(0, Z_AXIS_3D());
  _pose.SetTranslation({0.f, 0.f, 0.f});
  _pose.SetParent(_worldOrigin);
      
  _driveCenterPose.SetRotation(0, Z_AXIS_3D());
  _driveCenterPose.SetTranslation({0.f, 0.f, 0.f});
  _driveCenterPose.SetParent(_worldOrigin);
  
  // Create a new pose frame so that we can't get pose history entries with the same pose
  // frame that have different origins (Not 100% sure this is totally necessary but seems
  // like the cleaner / safer thing to do.)
  Result res = SetNewPose(_pose);
  if(res != RESULT_OK)
  {
    PRINT_NAMED_WARNING("Robot.Delocalize.SetNewPose", "Failed to set new pose");
  }
  
  if(_timeSynced)
  {
    // Need to update the robot's pose history with our new origin and pose frame IDs
    PRINT_NAMED_INFO("Robot.Delocalize.SendingNewOriginID",
                     "Sending new localization update at t=%u, with pose frame %u and origin ID=%u",
                     GetLastMsgTimestamp(),
                     GetPoseFrameID(),
                     _poseOriginList->GetOriginID(_worldOrigin));
    SendAbsLocalizationUpdate(_pose, GetLastMsgTimestamp(), GetPoseFrameID());
  }
  
  // Update VizText
  GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: <nothing>");
  GetContext()->GetVizManager()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOriginList->GetSize(),
                                         _worldOrigin->GetName().c_str());
  GetContext()->GetVizManager()->EraseAllVizObjects();
  
  
  // clear the pose confirmer now that we've changed pose origins
  GetObjectPoseConfirmer().Clear();
  
  // Sanity check carrying state
  if(isCarryingObject != IsCarryingObject())
  {
    PRINT_NAMED_WARNING("Robot.Delocalize.IsCarryingObjectMismatch",
                        "Passed-in isCarryingObject=%c, IsCarryingObject()=%c",
                        isCarryingObject   ? 'Y' : 'N',
                        IsCarryingObject() ? 'Y' : 'N');
  }
  
  // Have to do this _after_ clearing the pose confirmer because UpdateObjectOrigin
  // adds the carried objects to the pose confirmer in their newly updated pose,
  // but _before_ deleting zombie objects (since dirty carried objects may get
  // deleted)
  if(IsCarryingObject())
  {
    // Carried objects are in the pose chain of the robot, whose origin has now changed.
    // Thus the carried objects' actual origin no longer matches the way they are stored
    // in BlockWorld.
    for(auto const& objectID : GetCarryingObjects())
    {
      const Result result = _blockWorld->UpdateObjectOrigin(objectID, oldOrigin);
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("Robot.Delocalize.UpdateObjectOriginFailed",
                            "Object %d", objectID.GetValue());
      }
    }
  }

  // notify blockworld
  _blockWorld->OnRobotDelocalized(_worldOrigin);
  
  // notify faceworld
  _faceWorld->OnRobotDelocalized(_worldOrigin);
  
  // notify behavior whiteboard
  _aiComponent->OnRobotDelocalized();
  
  _behaviorMgr->OnRobotDelocalized();
  
  // send message to game. At the moment I implement this so that Webots can update the render, but potentially
  // any system can listen to this
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotDelocalized(GetID())));
      
} // Delocalize()
    
Result Robot::SetLocalizedTo(const ObservableObject* object)
{
  if(object == nullptr) {
    GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                           "LocalizedTo: Odometry");
    _localizedToID.UnSet();
    _isLocalized = true;
    return RESULT_OK;
  }
      
  if(object->GetID().IsUnknown()) {
    PRINT_NAMED_ERROR("Robot.SetLocalizedTo.IdNotSet",
                      "Cannot localize to an object with no ID set");
    return RESULT_FAIL;
  }
      
  // Find the closest, most recently observed marker on the object
  TimeStamp_t mostRecentObsTime = 0;
  for(const auto& marker : object->GetMarkers()) {
    if(marker.GetLastObservedTime() >= mostRecentObsTime) {
      Pose3d markerPoseWrtCamera;
      if(false == marker.GetPose().GetWithRespectTo(_visionComponent->GetCamera().GetPose(), markerPoseWrtCamera)) {
        PRINT_NAMED_ERROR("Robot.SetLocalizedTo.MarkerOriginProblem",
                          "Could not get pose of marker w.r.t. robot camera");
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
  _aiComponent->OnRobotRelocalized();
  
  // Update VizText
  GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: %s_%d",
                                         ObjectTypeToString(object->GetType()), _localizedToID.GetValue());
  GetContext()->GetVizManager()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOriginList->GetSize(),
                                         _worldOrigin->GetName().c_str());
      
  return RESULT_OK;
      
} // SetLocalizedTo()
  
  
Result Robot::UpdateFullRobotState(const RobotState& msg)
{
  
  ANKI_CPU_PROFILE("Robot::UpdateFullRobotState");
  
  Result lastResult = RESULT_OK;

  // Ignore state messages received before time sync
  if (!_timeSynced) {
    return lastResult;
  }
  
  _gotStateMsgAfterTimeSync = true;
    
  // Set flag indicating that robot state messages have been received
  _lastMsgTimestamp = msg.timestamp;
  _newStateMsgAvailable = true;
      
  // Update head angle
  SetHeadAngle(msg.headAngle);
      
  // Update lift angle
  SetLiftAngle(msg.liftAngle);
      
  // Update robot pitch angle
  _pitchAngle = Radians(msg.pose.pitch_angle);
            
  // Raw cliff data
  _cliffDataRaw = msg.cliffDataRaw;

  // update path following variables
  _pathComponent->UpdateRobotData(msg.currPathSegment, msg.lastPathID);
    
  // Update IMU data
  _robotAccel = msg.accel;
  _robotGyro = msg.gyro;
  
  _robotAccelMagnitude = sqrtf(_robotAccel.x * _robotAccel.x
                             + _robotAccel.y * _robotAccel.y
                             + _robotAccel.z * _robotAccel.z);
  
  const float kAccelFilterConstant = 0.95f; // between 0 and 1
  _robotAccelMagnitudeFiltered = (kAccelFilterConstant * _robotAccelMagnitudeFiltered)
                              + ((1.0f - kAccelFilterConstant) * _robotAccelMagnitude);
  
  
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
  SetPickingOrPlacing(IS_STATUS_FLAG_SET(IS_PICKING_OR_PLACING));
  _isPickedUp = IS_STATUS_FLAG_SET(IS_PICKED_UP);
  SetOnCharger(IS_STATUS_FLAG_SET(IS_ON_CHARGER));
  SetIsCharging(IS_STATUS_FLAG_SET(IS_CHARGING));
  _isCliffSensorOn = IS_STATUS_FLAG_SET(CLIFF_DETECTED);
  _chargerOOS = IS_STATUS_FLAG_SET(IS_CHARGER_OOS);
  _isBodyInAccessoryMode = IS_STATUS_FLAG_SET(IS_BODY_ACC_MODE);

  // Save the entire flag for sending to game
  _lastStatusFlags = msg.status;
  
  // If robot is not in accessary mode for some reason, send message to force it.
  // This shouldn't ever happen!
  if (!_isBodyInAccessoryMode && ++_setBodyModeTicDelay >= 16) {  // Triggers ~960ms (16 x 60ms engine tic) after syncTimeAck.
    PRINT_NAMED_WARNING("Robot.UpdateFullRobotState.BodyNotInAccessoryMode", "");
    SendMessage(RobotInterface::EngineToRobot(SetBodyRadioMode(BodyRadioMode::BODY_ACCESSORY_OPERATING_MODE, 0)));
    _setBodyModeTicDelay = 0;
  }



  GetMoveComponent().Update(msg);
      
  _battVoltage = msg.batteryVoltage;
      
  _leftWheelSpeed_mmps = msg.lwheel_speed_mmps;
  _rightWheelSpeed_mmps = msg.rwheel_speed_mmps;
      
  _hasMovedSinceLocalization |= GetMoveComponent().IsMoving() || _offTreadsState != OffTreadsState::OnTreads;
  
  if ( isDelocalizing )
  {
    _numMismatchedFrameIDs = 0;
    
    Delocalize(isCarryingObject);
  }
  else
  {
    DEV_ASSERT(msg.pose_frame_id <= GetPoseFrameID(), "Robot.UpdateFullRobotState.FrameFromFuture");
    const bool frameIsCurrent = msg.pose_frame_id == GetPoseFrameID();
    
    Pose3d newPose;
        
    if(IsOnRamp()) {
// Unsupported, remove in new PR
//
//      // Sanity check:
//      DEV_ASSERT(_rampID.IsSet(), "Robot.UpdateFullRobotState.InvalidRampID");
//          
//      // Don't update pose history while on a ramp.
//      // Instead, just compute how far the robot thinks it has gone (in the plane)
//      // and compare that to where it was when it started traversing the ramp.
//      // Adjust according to the angle of the ramp we know it's on.
//          
//      const f32 distanceTraveled = (Point2f(msg.pose.x, msg.pose.y) - _rampStartPosition).Length();
//          
//      Ramp* ramp = dynamic_cast<Ramp*>(_blockWorld->GetLocatedObjectByID(_rampID, ObjectFamily::Ramp));
//      if(ramp == nullptr) {
//        PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.NoRampWithID",
//                          "Updating robot %d's state while on a ramp, but Ramp object with ID=%d not found in the world.",
//                          _ID, _rampID.GetValue());
//        return RESULT_FAIL;
//      }
//          
//      // Progress must be along ramp's direction (init assuming ascent)
//      Radians headingAngle = ramp->GetPose().GetRotationAngle<'Z'>();
//          
//      // Initialize tilt angle assuming we are ascending
//      Radians tiltAngle = ramp->GetAngle();
//          
//      switch(_rampDirection)
//      {
//        case Ramp::DESCENDING:
//          tiltAngle    *= -1.f;
//          headingAngle += M_PI_F;
//          break;
//        case Ramp::ASCENDING:
//          break;
//              
//        default:
//          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.UnexpectedRampDirection",
//                            "Robot is on a ramp, expecting the ramp direction to be either "
//                            "ASCEND or DESCENDING, not %d", _rampDirection);
//          return RESULT_FAIL;
//      }
//
//      const f32 heightAdjust = distanceTraveled*sin(tiltAngle.ToFloat());
//      const Point3f newTranslation(_rampStartPosition.x() + distanceTraveled*cos(headingAngle.ToFloat()),
//                                   _rampStartPosition.y() + distanceTraveled*sin(headingAngle.ToFloat()),
//                                   _rampStartHeight + heightAdjust);
//          
//      const RotationMatrix3d R_heading(headingAngle, Z_AXIS_3D());
//      const RotationMatrix3d R_tilt(tiltAngle, Y_AXIS_3D());
//          
//      newPose = Pose3d(R_tilt*R_heading, newTranslation, _pose.GetParent());
//      //SetPose(newPose); // Done by UpdateCurrPoseFromHistory() below
      
    } else {
      // This is "normal" mode, where we udpate pose history based on the
      // reported odometry from the physical robot
          
      // Ignore physical robot's notion of z from the message? (msg.pose_z)
      f32 pose_z = 0.f;

      
      // Need to put the odometry update in terms of the current robot origin
      const Pose3d* origin = GetPoseOriginList().GetOriginByID(msg.pose_origin_id);
      if(nullptr == origin) {
        PRINT_NAMED_WARNING("Robot.UpdateFullRobotState.BadOriginID",
                            "Received RobotState with originID=%u, only %zu pose origins available",
                            msg.pose_origin_id, GetPoseOriginList().GetSize());
        return RESULT_FAIL;
      }

      // Initialize new pose to be within the reported origin
      newPose = Pose3d(msg.pose.angle, Z_AXIS_3D(), {msg.pose.x, msg.pose.y, msg.pose.z}, origin);
      
      // It's possible the pose origin to which this update refers has since been
      // rejiggered and is now the child of another origin. To add to history below,
      // we must first flatten it. We do all this before "fixing" pose_z because pose_z
      // will be w.r.t. robot origin so we want newPose to already be as well.
      newPose = newPose.GetWithRespectToOrigin();
      
      if(msg.pose_frame_id == GetPoseFrameID()) {
        // Frame IDs match. Use the robot's current Z (but w.r.t. world origin)
        pose_z = GetPose().GetWithRespectToOrigin().GetTranslation().z();
      } else {
        // This is an old odometry update from a previous pose frame ID. We
        // need to look up the correct Z value to use for putting this
        // message's (x,y) odometry info into history. Since it comes from
        // pose history, it will already be w.r.t. world origin, since that's
        // how we store everything in pose history.
        HistRobotState histState;
        lastResult = _stateHistory->GetLastStateWithFrameID(msg.pose_frame_id, histState);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.GetLastPoseWithFrameIdError",
                            "Failed to get last pose from history with frame ID=%d",
                            msg.pose_frame_id);
          return lastResult;
        }
        pose_z = histState.GetPose().GetWithRespectToOrigin().GetTranslation().z();
      }
      
      newPose.SetTranslation({newPose.GetTranslation().x(), newPose.GetTranslation().y(), pose_z});
      
    } // if/else on ramp
    
    // Add to history
    lastResult = AddRobotStateToHistory(newPose, msg);
    
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_WARNING("Robot.UpdateFullRobotState.AddPoseError",
                          "AddRawOdomStateToHistory failed for timestamp=%d", msg.timestamp);
      return lastResult;
    }

    if(UpdateCurrPoseFromHistory() == false) {
      lastResult = RESULT_FAIL;
    }
    
    if (frameIsCurrent) {
      _numMismatchedFrameIDs = 0;

    } else {
      // COZMO-5850 (Al) This is to catch the issue where our frameID is incremented but fails to send
      // to the robot due to some origin issue. Somehow the robot's pose becomes an origin and doesn't exist
      // in the PoseOriginList. The frameID mismatch then causes all sorts of issues in things (ie VisionSystem
      // won't process the next image). Delocalizing will fix the mismatch by creating a new origin and sending
      // a localization update
      static const u32 kNumTicksWithMismatchedFrameIDs = 100; // 3 seconds (called each RobotState msg)
      
      ++_numMismatchedFrameIDs;
      
      if(_numMismatchedFrameIDs > kNumTicksWithMismatchedFrameIDs)
      {
        PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.MismatchedFrameIDs",
                          "Robot[%u] and engine[%u] frameIDs are mismatched, delocalizing",
                          msg.pose_frame_id,
                          GetPoseFrameID());
        
        _numMismatchedFrameIDs = 0;
        
        Delocalize(IsCarryingObject());
        
        return RESULT_FAIL;
      }

    }
    
  }
  
  _gyroDriftDetector->DetectGyroDrift(msg);
  _gyroDriftDetector->DetectBias(msg);
  
  UpdateCliffRunningStats(msg);
  UpdateCliffDetectThreshold();
  
  /*
    PRINT_NAMED_INFO("Robot.UpdateFullRobotState.OdometryUpdate",
    "Robot %d's pose updated to (%.3f, %.3f, %.3f) @ %.1fdeg based on "
    "msg at time=%d, frame=%d saying (%.3f, %.3f) @ %.1fdeg\n",
    _ID, _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
    _pose.GetRotationAngle<'Z'>().getDegrees(),
    msg.timestamp, msg.pose_frame_id,
    msg.pose_x, msg.pose_y, msg.pose_angle*180.f/M_PI);
  */
  
  // Engine modifications to state message.
  // TODO: Should this just be a different message? Or one that includes the state message from the robot?
  RobotState stateMsg(msg);

  const float imageFrameRate = 1000.0f / _visionComponent->GetFramePeriod_ms();
  const float imageProcRate = 1000.0f / _visionComponent->GetProcessingPeriod_ms();
            
  // Send state to visualizer for displaying
  GetContext()->GetVizManager()->SendRobotState(
    stateMsg,
    static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (_numAnimationBytesStreamed - _numAnimationBytesPlayed),
    AnimationStreamer::NUM_AUDIO_FRAMES_LEAD-(_numAnimationAudioFramesStreamed - _numAnimationAudioFramesPlayed),
    (u8)MIN(((u8)imageFrameRate), std::numeric_limits<u8>::max()),
    (u8)MIN(((u8)imageProcRate), std::numeric_limits<u8>::max()),
    _enabledAnimTracks,
    _animationTag);
      
  return lastResult;
      
} // UpdateFullRobotState()
    
bool Robot::HasReceivedRobotState() const
{
  return _newStateMsgAvailable;
}
    
void Robot::SetCameraRotation(f32 roll, f32 pitch, f32 yaw)
{
  RotationMatrix3d rot(roll, -pitch, yaw);
  _headCamPose.SetRotation(rot * _kDefaultHeadCamRotation);
  PRINT_NAMED_INFO("Robot.SetCameraRotation", "yaw_corr=%f, pitch_corr=%f, roll_corr=%f", yaw, pitch, roll);
}
    
void Robot::SetPhysicalRobot(bool isPhysical)
{
  // TODO: Move somewhere else? This might not the best place for this, but it's where we
  // know whether or not we're talking to a physical robot or not so do things that depend on that here.
  // Assumes this function is only called once following connection.
      
  // Connect to active objects in saved blockpool, but only for physical robots.
  // For sim robots we make blocks connect automatically by sending BlockPoolEnabledMessage from UiGameController.
  // (Note that when using Unity+Webots, that message is not sent.)
  if (isPhysical) {
    if (_context->GetDataPlatform() != nullptr) {
      _blockFilter->Init(_context->GetDataPlatform()->pathToResource(Util::Data::Scope::External, "blockPool.txt"));
    }
  }
      
      
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
    PRINT_NAMED_INFO("Robot.SetPhysicalRobot", "ReliableConnection::SetConnectionTimeoutInMS(%f) for %s Robot",
                     netConnectionTimeoutInMS, isPhysical ? "Physical" : "Simulated");
    Anki::Util::ReliableConnection::SetConnectionTimeoutInMS(netConnectionTimeoutInMS);
  }
  #endif // !(ANKI_PLATFORM_IOS || ANKI_PLATFORM_ANDROID)
}

Result Robot::GetHistoricalCamera(TimeStamp_t t_request, Vision::Camera& camera) const
{
  HistRobotState histState;
  TimeStamp_t t;
  Result result = _stateHistory->GetRawStateAt(t_request, t, histState);
  if(RESULT_OK != result)
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
  Pose3d camPose(_headCamPose);
      
  // Rotate that by the given angle
  RotationVector3d Rvec(-histState.GetHeadAngle_rad(), Y_AXIS_3D());
  camPose.RotateBy(Rvec);
      
  // Precompose with robot body to neck pose
  camPose.PreComposeWith(_neckPose);
      
  // Set parent pose to be the historical robot pose
  camPose.SetParent(&(histState.GetPose()));
      
  camPose.SetName("PoseHistoryCamera_" + std::to_string(t));
      
  return camPose;
}

//
// Return constant display parameters for Cozmo v1.0.
// Future hardware may support different values.
u32 Robot::GetDisplayWidthInPixels() const
{
    return FaceAnimationManager::IMAGE_WIDTH;
}
  
u32 Robot::GetDisplayHeightInPixels() const
{
  return FaceAnimationManager::IMAGE_HEIGHT;
}
  
Vision::Camera Robot::GetHistoricalCamera(const HistRobotState& histState, TimeStamp_t t) const
{
  Vision::Camera camera(_visionComponent->GetCamera());
      
  // Update the head camera's pose
  camera.SetPose(GetHistoricalCameraPose(histState, t));
      
  return camera;
}
   
    
// Flashes a pattern on an active block
void Robot::ActiveObjectLightTest(const ObjectID& objectID) {
  /*
    static int p=0;
    static int currFrame = 0;
    const u32 onColor = 0x00ff00;
    const u32 offColor = 0x0;
    const u8 NUM_FRAMES = 4;
    const u32 LIGHT_PATTERN[NUM_FRAMES][NUM_BLOCK_LEDS] =
    {
    {onColor, offColor, offColor, offColor, onColor, offColor, offColor, offColor}
    ,{offColor, onColor, offColor, offColor, offColor, onColor, offColor, offColor}
    ,{offColor, offColor, offColor, onColor, offColor, offColor, offColor, onColor}
    ,{offColor, offColor, onColor, offColor, offColor, offColor, onColor, offColor}
    };
      
    if (p++ == 10) {
        
    SendSetBlockLights(blockID, LIGHT_PATTERN[currFrame]);
    //SendFlashBlockIDs();
        
    if (++currFrame == NUM_FRAMES) {
    currFrame = 0;
    }
        
    p = 0;
    }
  */
}
    

Result Robot::Update()
{
  ANKI_CPU_PROFILE("Robot::Update");
  
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  _robotIdleTimeoutComponent->Update(currentTime);
  
  // Check for syncTimeAck taking too long to arrive
  if (_syncTimeSentTime_sec > 0.0f && currentTime > _syncTimeSentTime_sec + kMaxSyncTimeAckDelay_sec) {
    PRINT_NAMED_WARNING("Robot.Update.SyncTimeAckNotReceived", "");
    _syncTimeSentTime_sec = 0.0f;
  }
  
  if (!_gotStateMsgAfterTimeSync)
  {
    PRINT_NAMED_DEBUG("Robot.Update", "Waiting for first full robot state to be handled");
    return RESULT_OK;
  }
  
#if(0)
  ActiveBlockLightTest(1);
  return RESULT_OK;
#endif
  
  GetContext()->GetVizManager()->SendStartRobotUpdate();
      
  /* DEBUG
     const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
     static float lastUpdateTime = currentTime_sec;
       
     const float updateTimeDiff = currentTime_sec - lastUpdateTime;
     if(updateTimeDiff > 1.0f) {
       PRINT_NAMED_WARNING("Robot.Update", "Gap between robot update calls = %f\n", updateTimeDiff);
     }
     lastUpdateTime = currentTime_sec;
  */
      
      
  if(_visionComponent->GetCamera().IsCalibrated())
  {
#   ifdef COZMO_V2
    _visionComponent->CaptureAndSendImage();
#   endif
    
    // NOTE: Also updates BlockWorld and FaceWorld using markers/faces that were detected
    Result visionResult = _visionComponent->UpdateAllResults();
    if(RESULT_OK != visionResult) {
      PRINT_NAMED_WARNING("Robot.Update.VisionComponentUpdateFail", "");
      return visionResult;
    }
  } // if (GetCamera().IsCalibrated())
  
  // If anything in updating block world caused a localization update, notify
  // the physical robot now:
  if(_needToSendLocalizationUpdate) {
    SendAbsLocalizationUpdate();
    _needToSendLocalizationUpdate = false;
  }
  
  ///////// MemoryMap ///////////
      
  // update the memory map based on the current's robot pose
  _blockWorld->UpdateRobotPoseInMemoryMap();
  
  // Check if we have driven off the charger platform - this has to happen before the behaviors which might
  // need this information. This state is useful for knowing not to play a cliff react when just driving off
  // the charger.

  if( _isOnChargerPlatform && _offTreadsState == OffTreadsState::OnTreads ) {  
    ObservableObject* charger = GetBlockWorld().GetLocatedObjectByID(_chargerID, ObjectFamily::Charger);
    if( nullptr != charger )
    {
      const bool isOnChargerPlatform = charger->GetBoundingQuadXY().Intersects(GetBoundingQuadXY());
      if( !isOnChargerPlatform )
      {
        SetOnChargerPlatform(false);
      }
    }
    else {
      // if we can't connect / talk to the charger, consider the robot to be off the platform
      SetOnChargerPlatform(false);
    }
  }
      
  ///////// Update the behavior manager ///////////
      
  // TODO: This object encompasses, for the time-being, what some higher level
  // module(s) would do.  e.g. Some combination of game state, build planner,
  // personality planner, etc.
      
  _moodManager->Update(currentTime);
  
  _needsManager->Update(currentTime);
      
  _progressionUnlockComponent->Update();
  
  _tapFilterComponent->Update();

  // Update AI component before behaviors so that behaviors can use the latest information
  _aiComponent->Update();
      
  const char* currentActivityName = "";
  std::string behaviorDebugStr("<disabled>");

  // https://ankiinc.atlassian.net/browse/COZMO-1242 : moving too early causes pose offset
  static int ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 = 60;
  if(ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242 <=0)
  {
    _behaviorMgr->Update(*this);
        
    const IBehavior* behavior = _behaviorMgr->GetCurrentBehavior();
    if(behavior != nullptr) {
      if( behavior->IsActing() ) {
        behaviorDebugStr = "A ";
      }
      else {
        behaviorDebugStr = "  ";
      }
      behaviorDebugStr += BehaviorIDToString(behavior->GetID());
      const std::string& stateName = behavior->GetDebugStateName();
      if (!stateName.empty())
      {
        behaviorDebugStr += "-" + stateName;
      }
    }

    currentActivityName = ActivityIDToString(_behaviorMgr->GetCurrentActivity()->GetID());
  } else {
    --ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242;
  }
      
  GetContext()->GetVizManager()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::MAGENTA,
                                         "%s", behaviorDebugStr.c_str());
  
  GetContext()->SetSdkStatus(SdkStatusType::Behavior,
                                 std::string(currentActivityName) + std::string(":") + behaviorDebugStr);
  
  //////// Update Robot's State Machine /////////////
  const RobotID_t robotID = GetID();
  
  Result result = _actionList->Update();
  if (result != RESULT_OK) {
    PRINT_NAMED_INFO("Robot.Update.ActionList", "Robot %d had an action list failure (%d)", robotID, result);
  }
  
  //////// Stream Animations /////////
  if (_timeSynced) { // Don't stream anything before we've connected
    // NEW Animations!
    if (BUILD_NEW_ANIMATION_CODE) {
      result = _animationController->Update(*this);
      if (result != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.Update.AnimationController",
                            "Robot %d had an animation controller failure (%d)", robotID, result);
      }
    } else {
      result = _animationStreamer.Update(*this);
      if (result != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.Update.AnimationStreamer",
                            "Robot %d had an animation streamer failure (%d)", robotID, result);
      }
    }
  }

  /////////// Update NVStorage //////////
  _nvStorageComponent->Update();

  /////////// Update path planning / following ////////////
  _pathComponent->Update();
      
  /////////// Update discovered active objects //////
  for (auto iter = _discoveredObjects.begin(); iter != _discoveredObjects.end();) {
    // Note not incrementing the iterator here
    const auto& obj = *iter;
    const int32_t maxTimestamp =
      10 * Util::numeric_cast<int32_t>(ActiveObjectConstants::ACTIVE_OBJECT_DISCOVERY_PERIOD_MS);
    const int32_t timeStampDiff =
      Util::numeric_cast<int32_t>(GetLastMsgTimestamp()) -
      Util::numeric_cast<int32_t>(obj.second.lastDiscoveredTimeStamp);
    if (timeStampDiff > maxTimestamp) {
      if (_enableDiscoveredObjectsBroadcasting) {
        PRINT_NAMED_INFO("Robot.Update.ObjectUndiscovered",
                         "FactoryID 0x%x (type: %s, lastObservedTime %d, currTime %d)",
                         obj.first, EnumToString(obj.second.objectType),
                         obj.second.lastDiscoveredTimeStamp, GetLastMsgTimestamp());

        // Send unavailable message for this object
        ExternalInterface::ObjectUnavailable m(obj.first);
        Broadcast(ExternalInterface::MessageEngineToGame(std::move(m)));
      }
      iter = _discoveredObjects.erase(iter);
    }
    else {
      ++iter;
    }
  }

  // Update the block filter before trying to connect to objects
  _blockFilter->Update();

  // Update object connectivity
  CheckDisconnectedObjects();
  
  // Connect to objects requested via ConnectToObjects
  ConnectToRequestedObjects();
  
  // Send nav memory map data
  _blockWorld->BroadcastNavMemoryMap();
      
  /////////// Update visualization ////////////
      
  // Draw All Objects by calling their Visualize() methods.
  _blockWorld->DrawAllObjects();
      
  // Nav memory map
  _blockWorld->DrawNavMemoryMap();
      
  // Always draw robot w.r.t. the origin, not in its current frame
  Pose3d robotPoseWrtOrigin = GetPose().GetWithRespectToOrigin();
      
  // Triangle pose marker
  GetContext()->GetVizManager()->DrawRobot(GetID(), robotPoseWrtOrigin);
      
  // Full Webots CozmoBot model
  GetContext()->GetVizManager()->DrawRobot(GetID(), robotPoseWrtOrigin, GetHeadAngle(), GetLiftAngle());
      
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
  _timeSinceLastImage_s = std::max(0.0, currentTime - _robotToEngineImplMessaging->GetLastImageReceivedTime());
      
  // Sending debug string to game and viz
  char buffer [128];

  const float imageProcRate = 1000.0f / _visionComponent->GetProcessingPeriod_ms();
      
  // So we can have an arbitrary number of data here that is likely to change want just hash it all
  // together if anything changes without spamming
  snprintf(buffer, sizeof(buffer),
           "%c%c%c%c%c %2dHz %s%s ",
           GetMoveComponent().IsLiftMoving() ? 'L' : ' ',
           GetMoveComponent().IsHeadMoving() ? 'H' : ' ',
           GetMoveComponent().IsMoving() ? 'B' : ' ',
           IsCarryingObject() ? 'C' : ' ',
           IsOnChargerPlatform() ? 'P' : ' ',
           // SimpleMoodTypeToString(GetMoodManager().GetSimpleMood()),
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK) ? 'L' : ' ',
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::HEAD_TRACK) ? 'H' : ' ',
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK) ? 'B' : ' ',
           (u8)MIN(((u8)imageProcRate), std::numeric_limits<u8>::max()),
           currentActivityName,
           behaviorDebugStr.c_str());
      
  std::hash<std::string> hasher;
  size_t curr_hash = hasher(std::string(buffer));
  if( _lastDebugStringHash != curr_hash )
  {
    SendDebugString(buffer);
    _lastDebugStringHash = curr_hash;
  }

  _cubeLightComponent->Update();
  _bodyLightComponent->Update();
  
  // Update user facing state information after everything else has been updated
  // so that relevant information is forwarded along to whoever's listening for
  // state changes
  _publicStateBroadcaster->Update(*this);


  if( kDebugPossibleBlockInteraction ) {
    // print a bunch of info helpful for debugging block states
    BlockWorldFilter filter;
    filter.SetAllowedFamilies({ObjectFamily::LightCube});
    std::vector<ObservableObject*> matchingObjects;
    GetBlockWorld().FindLocatedMatchingObjects(filter, matchingObjects); // note this doesn't retrieve unknowns anymore
    for( const auto obj : matchingObjects ) {
        const ObservableObject* topObj = GetBlockWorld().FindLocatedObjectOnTopOf(*obj, STACKED_HEIGHT_TOL_MM);
        Pose3d relPose;
        bool gotRelPose = obj->GetPose().GetWithRespectTo(GetPose(), relPose);

        const char* axisStr = "";
        switch( obj->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() ) {
          case AxisName::X_POS: axisStr="+X"; break;
          case AxisName::X_NEG: axisStr="-X"; break;
          case AxisName::Y_POS: axisStr="+Y"; break;
          case AxisName::Y_NEG: axisStr="-Y"; break;
          case AxisName::Z_POS: axisStr="+Z"; break;
          case AxisName::Z_NEG: axisStr="-Z"; break;
        }
              
        PRINT_NAMED_DEBUG("Robot.ObjectInteractionState",
                          "block:%d poseState:%8s moving?%d RestingFlat?%d carried?%d poseWRT?%d objOnTop:%d"
                          " z=%6.2f UpAxis:%s CanStack?%d CanPickUp?%d FromGround?%d",
                          obj->GetID().GetValue(),
                          PoseStateToString( obj->GetPoseState() ),
                          obj->IsMoving(),
                          obj->IsRestingFlat(),
                          (IsCarryingObject() && GetCarryingObject() == obj->GetID()),
                          gotRelPose,
                          topObj ? topObj->GetID().GetValue() : -1,
                          relPose.GetTranslation().z(),
                          axisStr,
                          CanStackOnTopOfObject(*obj),
                          CanPickUpObject(*obj),
                          CanPickUpObjectFromGround(*obj));
    }
  }
      
  return RESULT_OK;
      
} // Update()
      
static bool IsValidHeadAngle(f32 head_angle, f32* clipped_valid_head_angle)
{
  if(head_angle < MIN_HEAD_ANGLE - HEAD_ANGLE_LIMIT_MARGIN) {
    //PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too small.\n", head_angle);
    if (clipped_valid_head_angle) {
      *clipped_valid_head_angle = MIN_HEAD_ANGLE;
    }
    return false;
  }
  else if(head_angle > MAX_HEAD_ANGLE + HEAD_ANGLE_LIMIT_MARGIN) {
    //PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Head angle (%f rad) too large.\n", head_angle);
    if (clipped_valid_head_angle) {
      *clipped_valid_head_angle = MAX_HEAD_ANGLE;
    }
    return false;
  }
      
  if (clipped_valid_head_angle) {
    *clipped_valid_head_angle = head_angle;
  }
  return true;
      
} // IsValidHeadAngle()

    
Result Robot::SetNewPose(const Pose3d& newPose)
{
  SetPose(newPose.GetWithRespectToOrigin());
  
  // Note: using last message timestamp instead of newest timestamp in history
  //  because it's possible we did not put the last-received state message into
  //  history (if it had old frame ID), but we still want the latest time we
  //  can get.
  const TimeStamp_t timeStamp = GetLastMsgTimestamp();
  
  return AddVisionOnlyStateToHistory(timeStamp, _pose, GetHeadAngle(), GetLiftAngle());
}
    
void Robot::SetPose(const Pose3d &newPose)
{
  // Update our current pose and keep the name consistent
  const std::string name = _pose.GetName();
  _pose = newPose;
  _pose.SetName(name);
      
  ComputeDriveCenterPose(_pose, _driveCenterPose);
      
} // SetPose()
    
Pose3d Robot::GetCameraPose(f32 atAngle) const
{
  // Start with canonical (untilted) headPose
  Pose3d newHeadPose(_headCamPose);
      
  // Rotate that by the given angle
  RotationVector3d Rvec(-atAngle, Y_AXIS_3D());
  newHeadPose.RotateBy(Rvec);
  newHeadPose.SetName("Camera");

  return newHeadPose;
} // GetCameraHeadPose()
    
void Robot::SetHeadAngle(const f32& angle)
{
  if (_isHeadCalibrated) {
    if (!IsValidHeadAngle(angle, &_currentHeadAngle)) {
      PRINT_NAMED_WARNING("Robot.GetCameraHeadPose.HeadAngleOOB",
                          "Angle %.3frad / %.1f (TODO: Send correction or just recalibrate?)",
                          angle, RAD_TO_DEG(angle));
    }
        
    _visionComponent->GetCamera().SetPose(GetCameraPose(_currentHeadAngle));
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
  _currentLiftAngle = angle;
      
  Robot::ComputeLiftPose(_currentLiftAngle, _liftPose);

  DEV_ASSERT(_liftPose.GetParent() == &_liftBasePose, "Robot.SetLiftAngle.InvalidPose");
}
    
Radians Robot::GetPitchAngle() const
{
  return _pitchAngle;
}
  
Result Robot::PlaceObjectOnGround(const bool useManualSpeed)
{
  if(!IsCarryingObject()) {
    PRINT_NAMED_ERROR("Robot.PlaceObjectOnGround.NotCarryingObject",
                      "Robot told to place object on ground, but is not carrying an object.");
    return RESULT_FAIL;
  }

  _lastPickOrPlaceSucceeded = false;
      
  return SendRobotMessage<Anki::Cozmo::PlaceObjectOnGround>(0, 0, 0,
                                                            DEFAULT_PATH_MOTION_PROFILE.speed_mmps,
                                                            DEFAULT_PATH_MOTION_PROFILE.accel_mmps2,
                                                            DEFAULT_PATH_MOTION_PROFILE.decel_mmps2,
                                                            useManualSpeed);
}

bool Robot::WasObjectTappedRecently(const ObjectID& objectID) const
{
  return _tapFilterComponent->ShouldIgnoreMovementDueToDoubleTap(objectID);
}
    
void Robot::ShiftEyes(AnimationStreamer::Tag& tag, f32 xPix, f32 yPix,
                      TimeStamp_t duration_ms, const std::string& name)
{
  ProceduralFace procFace;
  ProceduralFace::Value xMin=0, xMax=0, yMin=0, yMax=0;
  procFace.GetEyeBoundingBox(xMin, xMax, yMin, yMax);
  procFace.LookAt(xPix, yPix,
                  std::max(xMin, ProceduralFace::WIDTH-xMax),
                  std::max(yMin, ProceduralFace::HEIGHT-yMax),
                  1.1f, 0.85f, 0.1f);
      
  ProceduralFaceKeyFrame keyframe(procFace, duration_ms);
      
  if(AnimationStreamer::NotAnimatingTag == tag) {
    AnimationStreamer::FaceTrack faceTrack;
    if(duration_ms > 0) {
      // Add an initial no-adjustment frame so we have something to interpolate
      // from on our way to the specified shift
      faceTrack.AddKeyFrameToBack(ProceduralFaceKeyFrame());
    }
    faceTrack.AddKeyFrameToBack(std::move(keyframe));
    tag = GetAnimationStreamer().AddPersistentFaceLayer(name, std::move(faceTrack));
  } else {
    GetAnimationStreamer().AddToPersistentFaceLayer(tag, std::move(keyframe));
  }
}

void Robot::LoadEmotionEvents()
{
  const auto& emotionEventData = _context->GetDataLoader()->GetEmotionEventJsons();
  for (const auto& fileJsonPair : emotionEventData)
  {
    const auto& filename = fileJsonPair.first;
    const auto& eventJson = fileJsonPair.second;
    if (!eventJson.empty() && _moodManager->LoadEmotionEvents(eventJson))
    {
      PRINT_NAMED_DEBUG("Robot.LoadEmotionEvents", "Loaded '%s'", filename.c_str());
    }
    else
    {
      PRINT_NAMED_WARNING("Robot.LoadEmotionEvents", "Failed to read '%s'", filename.c_str());
    }
  }
}



Result Robot::SyncTime()
{
  _timeSynced = false;
  _stateHistory->Clear();
      
  Result res = SendSyncTime();
  if (res == RESULT_OK) {
    _syncTimeSentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  return res;
}
    
Result Robot::LocalizeToObject(const ObservableObject* seenObject,
                               ObservableObject* existingObject)
{
  Result lastResult = RESULT_OK;
  
  if(existingObject == nullptr) {
    PRINT_NAMED_ERROR("Robot.LocalizeToObject.ExistingObjectPieceNullPointer", "");
    return RESULT_FAIL;
  }
  
  if(existingObject->GetID() != GetLocalizedTo())
  {
    PRINT_NAMED_DEBUG("Robot.LocalizeToObject",
                      "Robot attempting to localize to %s object %d",
                      EnumToString(existingObject->GetType()),
                      existingObject->GetID().GetValue());
  }
      
  if(!existingObject->CanBeUsedForLocalization() || WasObjectTappedRecently(existingObject->GetID())) {
    PRINT_NAMED_ERROR("Robot.LocalizeToObject.UnlocalizedObject",
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
  if(nullptr == seenObject)
  {
    if(false == GetPose().GetWithRespectTo(existingObject->GetPose(), robotPoseWrtObject)) {
      PRINT_NAMED_ERROR("Robot.LocalizeToObject.ExistingObjectOriginMismatch",
                        "Could not get robot pose w.r.t. to existing object %d.",
                        existingObject->GetID().GetValue());
      return RESULT_FAIL;
    }
    liftAngle = GetLiftAngle();
    headAngle = GetHeadAngle();
  } else {
    // Get computed HistRobotState at the time the object was observed.
    if ((lastResult = _stateHistory->GetComputedStateAt(seenObject->GetLastObservedTime(), &histStatePtr, &histStateKey)) != RESULT_OK) {
      PRINT_NAMED_ERROR("Robot.LocalizeToObject.CouldNotFindHistoricalPose",
                        "Time %d", seenObject->GetLastObservedTime());
      return lastResult;
    }
        
    // The computed historical pose is always stored w.r.t. the robot's world
    // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
    // will work correctly
    Pose3d robotPoseAtObsTime = histStatePtr->GetPose();
    robotPoseAtObsTime.SetParent(_worldOrigin);
        
    // Get the pose of the robot with respect to the observed object
    if(robotPoseAtObsTime.GetWithRespectTo(seenObject->GetPose(), robotPoseWrtObject) == false) {
      PRINT_NAMED_ERROR("Robot.LocalizeToObject.ObjectPoseOriginMisMatch",
                        "Could not get HistRobotState w.r.t. seen object pose.");
      return RESULT_FAIL;
    }
        
    liftAngle = histStatePtr->GetLiftAngle_rad();
    headAngle = histStatePtr->GetHeadAngle_rad();
  }
      
  // Make the computed robot pose use the existing object as its parent
  robotPoseWrtObject.SetParent(&existingObject->GetPose());
  //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));
      
# if 0
  // Don't snap to horizontal or discrete Z levels when we see a mat marker
  // while on a ramp
  if(IsOnRamp() == false)
  {
    // If there is any significant rotation, make sure that it is roughly
    // around the Z axis
    Radians rotAngle;
    Vec3f rotAxis;
    robotPoseWrtObject.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
        
    if(std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) && !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(15))) {
      PRINT_NAMED_WARNING("Robot.LocalizeToObject.OutOfPlaneRotation",
                          "Refusing to localize to %s because "
                          "Robot %d's Z axis would not be well aligned with the world Z axis. "
                          "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)",
                          existingObject->GetType().GetName().c_str(), GetID(),
                          rotAngle.getDegrees(), rotAxis.x(), rotAxis.y(), rotAxis.z());
      return RESULT_FAIL;
    }
        
    // Snap to purely horizontal rotation
    // TODO: Snap to surface of mat?
    /*
      if(existingMatPiece->IsPoseOn(robotPoseWrtObject, 0, 10.f)) {
      Vec3f robotPoseWrtObject_trans = robotPoseWrtObject.GetTranslation();
      robotPoseWrtObject_trans.z() = existingObject->GetDrivingSurfaceHeight();
      robotPoseWrtObject.SetTranslation(robotPoseWrtObject_trans);
      }
    */
    robotPoseWrtObject.SetRotation( robotPoseWrtObject.GetRotationAngle<'Z'>(), Z_AXIS_3D() );
        
  } // if robot is on ramp
# endif
      
  // Add the new vision-based pose to the robot's history. Note that we use
  // the pose w.r.t. the origin for storing poses in history.
  Pose3d robotPoseWrtOrigin = robotPoseWrtObject.GetWithRespectToOrigin();
      
  if(IsLocalized()) {
    // Filter Z so it doesn't change too fast (unless we are switching from
    // delocalized to localized)
        
    // Make z a convex combination of new and previous value
    static const f32 zUpdateWeight = 0.1f; // weight of new value (previous gets weight of 1 - this)
    Vec3f T = robotPoseWrtOrigin.GetTranslation();
    T.z() = (zUpdateWeight*robotPoseWrtOrigin.GetTranslation().z() +
             (1.f - zUpdateWeight) * GetPose().GetTranslation().z());
    robotPoseWrtOrigin.SetTranslation(T);
  }
      
  if(nullptr != seenObject)
  {
    //
    if((lastResult = AddVisionOnlyStateToHistory(seenObject->GetLastObservedTime(),
                                                robotPoseWrtOrigin,
                                                headAngle, liftAngle)) != RESULT_OK)
    {
      PRINT_NAMED_ERROR("Robot.LocalizeToObject.FailedAddingVisionOnlyPoseToHistory", "");
      return lastResult;
    }
  }
      
  // If the robot's world origin is about to change by virtue of being localized
  // to existingObject, rejigger things so anything seen while the robot was
  // rooted to this world origin will get updated to be w.r.t. the new origin.
  if(_worldOrigin != &existingObject->GetPose().FindOrigin())
  {
    LOG_EVENT("Robot.LocalizeToObject.RejiggeringOrigins",
              "Robot %d's current origin is %s, about to localize to origin %s.",
              GetID(), _worldOrigin->GetName().c_str(),
              existingObject->GetPose().FindOrigin().GetName().c_str());
    
    // Store the current origin we are about to change so that we can
    // find objects that are using it below
    const Pose3d* oldOrigin = _worldOrigin;
        
    // Update the origin to which _worldOrigin currently points to contain
    // the transformation from its current pose to what is about to be the
    // robot's new origin.
    _worldOrigin->SetRotation(GetPose().GetRotation());
    _worldOrigin->SetTranslation(GetPose().GetTranslation());
    _worldOrigin->Invert();
    _worldOrigin->PreComposeWith(robotPoseWrtOrigin);
    _worldOrigin->SetParent(&robotPoseWrtObject.FindOrigin());
    _worldOrigin->SetName( _worldOrigin->GetName() + "_REJ");
        
    assert(_worldOrigin->IsOrigin() == false);
        
    // Now that the previous origin is hooked up to the new one (which is
    // now the old one's parent), point the worldOrigin at the new one.
    _worldOrigin = const_cast<Pose3d*>(_worldOrigin->GetParent()); // TODO: Avoid const cast?
        
    // Now we need to go through all objects and faces whose poses have been adjusted
    // by this origin switch and notify the outside world of the change.
    _blockWorld->UpdateObjectOrigins(oldOrigin, _worldOrigin);
    _faceWorld->UpdateFaceOrigins(oldOrigin, _worldOrigin);

    // after updating all block world objects, flatten out origins to remove grandparents
    _poseOriginList->Flatten(_worldOrigin);
        
  } // if(_worldOrigin != &existingObject->GetPose().FindOrigin())
      
      
  if(nullptr != histStatePtr)
  {
    // Update the computed historical pose as well so that subsequent block
    // pose updates use obsMarkers whose camera's parent pose is correct.
    histStatePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, headAngle, liftAngle);
  }

      
  // Compute the new "current" pose from history which uses the
  // past vision-based "ground truth" pose we just computed.
  assert(&existingObject->GetPose().FindOrigin() == _worldOrigin);
  assert(_worldOrigin != nullptr);
  if(UpdateCurrPoseFromHistory() == false) {
    PRINT_NAMED_ERROR("Robot.LocalizeToObject.FailedUpdateCurrPoseFromHistory", "");
    return RESULT_FAIL;
  }
      
  // Mark the robot as now being localized to this object
  // NOTE: this should be _after_ calling AddVisionOnlyStateToHistory, since
  //    that function checks whether the robot is already localized
  lastResult = SetLocalizedTo(existingObject);
  if(RESULT_OK != lastResult) {
    PRINT_NAMED_ERROR("Robot.LocalizeToObject.SetLocalizedToFail", "");
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
      
  if(matSeen == nullptr) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.MatSeenNullPointer", "");
    return RESULT_FAIL;
  } else if(existingMatPiece == nullptr) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.ExistingMatPieceNullPointer", "");
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
  if ((lastResult = _stateHistory->GetComputedStateAt(matSeen->GetLastObservedTime(), &histStatePtr, &histStateKey)) != RESULT_OK) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.CouldNotFindHistoricalPose", "Time %d", matSeen->GetLastObservedTime());
    return lastResult;
  }
      
  // The computed historical pose is always stored w.r.t. the robot's world
  // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
  // will work correctly
  Pose3d robotPoseAtObsTime = histStatePtr->GetPose();
  robotPoseAtObsTime.SetParent(_worldOrigin);
      
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
  if(robotPoseAtObsTime.GetWithRespectTo(matSeen->GetPose(), robotPoseWrtMat) == false) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.MatPoseOriginMisMatch",
                      "Could not get HistRobotState w.r.t. matPose.");
    return RESULT_FAIL;
  }
      
  // Make the computed robot pose use the existing mat piece as its parent
  robotPoseWrtMat.SetParent(&existingMatPiece->GetPose());
  //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));
      
  // Don't snap to horizontal or discrete Z levels when we see a mat marker
  // while on a ramp
  if(IsOnRamp() == false)
  {
    // If there is any significant rotation, make sure that it is roughly
    // around the Z axis
    Radians rotAngle;
    Vec3f rotAxis;
    robotPoseWrtMat.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);

    if(std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) && !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(15))) {
      PRINT_NAMED_WARNING("Robot.LocalizeToMat.OutOfPlaneRotation",
                          "Refusing to localize to %s because "
                          "Robot %d's Z axis would not be well aligned with the world Z axis. "
                          "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)",
                          ObjectTypeToString(existingMatPiece->GetType()), GetID(),
                          rotAngle.getDegrees(), rotAxis.x(), rotAxis.y(), rotAxis.z());
      return RESULT_FAIL;
    }
        
    // Snap to purely horizontal rotation and surface of the mat
    if(existingMatPiece->IsPoseOn(robotPoseWrtMat, 0, 10.f)) {
      Vec3f robotPoseWrtMat_trans = robotPoseWrtMat.GetTranslation();
      robotPoseWrtMat_trans.z() = existingMatPiece->GetDrivingSurfaceHeight();
      robotPoseWrtMat.SetTranslation(robotPoseWrtMat_trans);
    }
    robotPoseWrtMat.SetRotation( robotPoseWrtMat.GetRotationAngle<'Z'>(), Z_AXIS_3D() );
        
  } // if robot is on ramp
      
  if(!_localizedToFixedObject && !existingMatPiece->IsMoveable()) {
    // If we have not yet seen a fixed mat, and this is a fixed mat, rejigger
    // the origins so that we use it as the world origin
    PRINT_NAMED_INFO("Robot.LocalizeToMat.LocalizingToFirstFixedMat",
                     "Localizing robot %d to fixed %s mat for the first time.",
                     GetID(), ObjectTypeToString(existingMatPiece->GetType()));
        
    if((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
      PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                        "Failed to update robot %d's pose origin when (re-)localizing it.", GetID());
      return lastResult;
    }
        
    _localizedToFixedObject = true;
  }
  else if(IsLocalized() == false) {
    // If the robot is not yet localized, it is about to be, so we need to
    // update pose origins so that anything it has seen so far becomes rooted
    // to this mat's origin (whether mat is fixed or not)
    PRINT_NAMED_INFO("Robot.LocalizeToMat.LocalizingRobotFirstTime",
                     "Localizing robot %d for the first time (to %s mat).",
                     GetID(), ObjectTypeToString(existingMatPiece->GetType()));
        
    if((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
      PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                        "Failed to update robot %d's pose origin when (re-)localizing it.", GetID());
      return lastResult;
    }
        
    if(!existingMatPiece->IsMoveable()) {
      // If this also happens to be a fixed mat, then we have now localized
      // to a fixed mat
      _localizedToFixedObject = true;
    }
  }
      
  /*
  // Don't snap to horizontal or discrete Z levels when we see a mat marker
  // while on a ramp
  if(IsOnRamp() == false)
  {
  // If there is any significant rotation, make sure that it is roughly
  // around the Z axis
  Radians rotAngle;
  Vec3f rotAxis;
  robotPoseWrtMat.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
  const float dotProduct = DotProduct(rotAxis, Z_AXIS_3D());
  const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
  if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
  PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.RobotNotOnHorizontalPlane",
  "Robot's Z axis is not well aligned with the world Z axis. "
  "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)\n",
  rotAngle.getDegrees(), rotAxis.x(), rotAxis.y(), rotAxis.z());
  }
        
  // Snap to purely horizontal rotation and surface of the mat
  if(existingMatPiece->IsPoseOn(robotPoseWrtMat, 0, 10.f)) {
  Vec3f robotPoseWrtMat_trans = robotPoseWrtMat.GetTranslation();
  robotPoseWrtMat_trans.z() = existingMatPiece->GetDrivingSurfaceHeight();
  robotPoseWrtMat.SetTranslation(robotPoseWrtMat_trans);
  }
  robotPoseWrtMat.SetRotation( robotPoseWrtMat.GetRotationAngle<'Z'>(), Z_AXIS_3D() );
        
  } // if robot is on ramp
  */
      
  // Add the new vision-based pose to the robot's history. Note that we use
  // the pose w.r.t. the origin for storing poses in history.
  // HistRobotState p(robot->GetPoseFrameID(),
  //                  robotPoseWrtMat.GetWithRespectToOrigin(),
  //                  posePtr->GetHeadAngle(),
  //                  posePtr->GetLiftAngle());
  Pose3d robotPoseWrtOrigin = robotPoseWrtMat.GetWithRespectToOrigin();
      
  if((lastResult = AddVisionOnlyStateToHistory(existingMatPiece->GetLastObservedTime(),
                                              robotPoseWrtOrigin,
                                              histStatePtr->GetHeadAngle_rad(),
                                              histStatePtr->GetLiftAngle_rad())) != RESULT_OK)
  {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedAddingVisionOnlyPoseToHistory", "");
    return lastResult;
  }
      
      
  // Update the computed historical pose as well so that subsequent block
  // pose updates use obsMarkers whose camera's parent pose is correct.
  // Note again that we store the pose w.r.t. the origin in history.
  // TODO: Should SetPose() do the flattening w.r.t. origin?
  histStatePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, histStatePtr->GetHeadAngle_rad(), histStatePtr->GetLiftAngle_rad());
      
  // Compute the new "current" pose from history which uses the
  // past vision-based "ground truth" pose we just computed.
  if(UpdateCurrPoseFromHistory() == false) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedUpdateCurrPoseFromHistory", "");
    return RESULT_FAIL;
  }
      
  // Mark the robot as now being localized to this mat
  // NOTE: this should be _after_ calling AddVisionOnlyStateToHistory, since
  //    that function checks whether the robot is already localized
  lastResult = SetLocalizedTo(existingMatPiece);
  if(RESULT_OK != lastResult) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetLocalizedToFail", "");
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
    
Result Robot::SetOnRamp(bool t)
{
  ANKI_CPU_PROFILE("Robot::SetOnRamp");
  
  if(t == _onRamp) {
    // Nothing to do
    return RESULT_OK;
  }
      
  // We are either transition onto or off of a ramp
      
  Ramp* ramp = dynamic_cast<Ramp*>(GetBlockWorld().GetLocatedObjectByID(_rampID, ObjectFamily::Ramp));
  if(ramp == nullptr) {
    PRINT_NAMED_WARNING("Robot.SetOnRamp.NoRampWithID",
                        "Robot %d is transitioning on/off of a ramp, but Ramp object with ID=%d not found in the world",
                        _ID, _rampID.GetValue());
    return RESULT_FAIL;
  }
      
  assert(_rampDirection == Ramp::ASCENDING || _rampDirection == Ramp::DESCENDING);
      
  const bool transitioningOnto = (t == true);
      
  if(transitioningOnto) {
    // Record start (x,y) position coming from robot so basestation can
    // compute actual (x,y,z) position from upcoming odometry updates
    // coming from robot (which do not take slope of ramp into account)
    _rampStartPosition = {_pose.GetTranslation().x(), _pose.GetTranslation().y()};
    _rampStartHeight   = _pose.GetTranslation().z();
        
    PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOntoRamp",
                     "Robot %d transitioning onto ramp %d, using start (%.1f,%.1f,%.1f)",
                     _ID, ramp->GetID().GetValue(), _rampStartPosition.x(), _rampStartPosition.y(), _rampStartHeight);
        
  } else {
    Result res;
    // Just do an absolute pose update, setting the robot's position to
    // where we "know" he should be when he finishes ascending or
    // descending the ramp
    switch(_rampDirection)
    {
      case Ramp::ASCENDING:
        res = SetNewPose(ramp->GetPostAscentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
        break;
            
      case Ramp::DESCENDING:
        res = SetNewPose(ramp->GetPostDescentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
        break;
            
      default:
        PRINT_NAMED_WARNING("Robot.SetOnRamp.UnexpectedRampDirection",
                            "When transitioning on/off ramp, expecting the ramp direction to be either "
                            "ASCENDING or DESCENDING, not %d.", _rampDirection);
        return RESULT_FAIL;
    }
    
    if(res != RESULT_OK) {
      PRINT_NAMED_WARNING("Robot.SetOnRamp.SetNewPose",
                          "Robot %d failed to set new pose", _ID);
      return res;
    }
        
    _rampDirection = Ramp::UNKNOWN;
        
    const TimeStamp_t timeStamp = _stateHistory->GetNewestTimeStamp();
        
    PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOffRamp",
                     "Robot %d transitioning off of ramp %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d",
                     _ID, ramp->GetID().GetValue(),
                     _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
                     _pose.GetRotationAngle<'Z'>().getDegrees(),
                     timeStamp);
  } // if/else transitioning onto ramp
      
  _onRamp = t;
      
  return RESULT_OK;
      
} // SetOnPose()
    
    
Result Robot::SetPoseOnCharger()
{
  ANKI_CPU_PROFILE("Robot::SetPoseOnCharger");
  
  Charger* charger = dynamic_cast<Charger*>(GetBlockWorld().GetLocatedObjectByID(_chargerID));
  if(charger == nullptr) {
    PRINT_NAMED_WARNING("Robot.SetPoseOnCharger.NoChargerWithID",
                        "Robot %d has docked to charger, but Charger object with ID=%d not found in the world.",
                        _ID, _chargerID.GetValue());
    return RESULT_FAIL;
  }
      
  // Just do an absolute pose update, setting the robot's position to
  // where we "know" he should be when he finishes ascending the charger.
  Result lastResult = SetNewPose(charger->GetRobotDockedPose().GetWithRespectToOrigin());
  if(lastResult != RESULT_OK) {
    PRINT_NAMED_WARNING("Robot.SetPoseOnCharger.SetNewPose",
                        "Robot %d failed to set new pose", _ID);
    return lastResult;
  }

  const TimeStamp_t timeStamp = _stateHistory->GetNewestTimeStamp();
    
  PRINT_NAMED_INFO("Robot.SetPoseOnCharger.SetPose",
                   "Robot %d now on charger %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d",
                   _ID, charger->GetID().GetValue(),
                   _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
                   _pose.GetRotationAngle<'Z'>().getDegrees(),
                   timeStamp);
      
  return RESULT_OK;
      
} // SetOnPose()
    
    
Result Robot::DockWithObject(const ObjectID objectID,
                             const f32 speed_mmps,
                             const f32 accel_mmps2,
                             const f32 decel_mmps2,
                             const Vision::KnownMarker::Code markerCode,
                             const Vision::KnownMarker::Code markerCode2,
                             const DockAction dockAction,
                             const f32 placementOffsetX_mm,
                             const f32 placementOffsetY_mm,
                             const f32 placementOffsetAngle_rad,
                             const bool useManualSpeed,
                             const u8 numRetries,
                             const DockingMethod dockingMethod,
                             const bool doLiftLoadCheck)
{
  return DockWithObject(objectID,
                        speed_mmps,
                        accel_mmps2,
                        decel_mmps2,
                        markerCode,
                        markerCode2,
                        dockAction,
                        0, 0, std::numeric_limits<u8>::max(),
                        placementOffsetX_mm, placementOffsetY_mm, placementOffsetAngle_rad,
                        useManualSpeed,
                        numRetries,
                        dockingMethod,
                        doLiftLoadCheck);
}
    
Result Robot::DockWithObject(const ObjectID objectID,
                             const f32 speed_mmps,
                             const f32 accel_mmps2,
                             const f32 decel_mmps2,
                             const Vision::KnownMarker::Code markerCode,
                             const Vision::KnownMarker::Code markerCode2,
                             const DockAction dockAction,
                             const u16 image_pixel_x,
                             const u16 image_pixel_y,
                             const u8 pixel_radius,
                             const f32 placementOffsetX_mm,
                             const f32 placementOffsetY_mm,
                             const f32 placementOffsetAngle_rad,
                             const bool useManualSpeed,
                             const u8 numRetries,
                             const DockingMethod dockingMethod,
                             const bool doLiftLoadCheck)
{
  ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld->GetLocatedObjectByID(objectID));
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.ObjectDoesNotExist",
                      "Object with ID=%d no longer exists for docking.", objectID.GetValue());
    return RESULT_FAIL;
  }
      
  // Need to store these so that when we receive notice from the physical
  // robot that it has picked up an object we can transition the docking
  // object to being carried, using PickUpDockObject()
  _dockObjectID   = objectID;
  _dockMarkerCode = markerCode;

  // get the marker from the code
  const auto& markersWithCode = object->GetMarkersWithCode(markerCode);
  if ( markersWithCode.empty() ) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.NoMarkerWithCode",
                      "No marker found with that code.");
    return RESULT_FAIL;
  }
  
  // notify if we have more than one, since we are going to assume that any is fine to use its pose
  // we currently don't have this, so treat as warning. But if we ever allow, make sure that they are
  // simetrical with respect to the object
  const bool hasMultipleMarkers = (markersWithCode.size() > 1);
  if(hasMultipleMarkers) {
    PRINT_NAMED_WARNING("Robot.DockWithObject.MultipleMarkersForCode",
                        "Multiple markers found for code '%d'. Using first for lift attachment", markerCode);
  }
  
  const Vision::KnownMarker* dockMarker = markersWithCode[0];
  DEV_ASSERT(dockMarker != nullptr, "Robot.DockWithObject.InvalidMarker");
  
  // Dock marker has to be a child of the dock block
  if(dockMarker->GetPose().GetParent() != &object->GetPose()) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.MarkerNotOnObject",
                      "Specified dock marker must be a child of the specified dock object.");
    return RESULT_FAIL;
  }

  // Mark as dirty so that the robot no longer localizes to this object
  const bool propagateStack = false;
  GetObjectPoseConfirmer().MarkObjectDirty(object, propagateStack);

  _lastPickOrPlaceSucceeded = false;
      
  // Sends a message to the robot to dock with the specified marker
  // that it should currently be seeing. If pixel_radius == std::numeric_limits<u8>::max(),
  // the marker can be seen anywhere in the image (same as above function), otherwise the
  // marker's center must be seen at the specified image coordinates
  // with pixel_radius pixels.
  Result sendResult = SendRobotMessage<::Anki::Cozmo::DockWithObject>(0.0f,
                                                                      speed_mmps,
                                                                      accel_mmps2,
                                                                      decel_mmps2,
                                                                      dockAction,
                                                                      useManualSpeed,
                                                                      numRetries,
                                                                      dockingMethod,
                                                                      doLiftLoadCheck);
  if(sendResult == RESULT_OK) {
        
    // When we are "docking" with a ramp or crossing a bridge, we
    // don't want to worry about the X angle being large (since we
    // _expect_ it to be large, since the markers are facing upward).
    const bool checkAngleX = !(dockAction == DockAction::DA_RAMP_ASCEND  ||
                               dockAction == DockAction::DA_RAMP_DESCEND ||
                               dockAction == DockAction::DA_CROSS_BRIDGE);
        
    // Tell the VisionSystem to start tracking this marker:
    _visionComponent->SetMarkerToTrack(dockMarker->GetCode(), dockMarker->GetSize(),
                                          image_pixel_x, image_pixel_y, checkAngleX,
                                          placementOffsetX_mm, placementOffsetY_mm,
                                          placementOffsetAngle_rad);
  }
      
  return sendResult;
}
    
    
const std::set<ObjectID> Robot::GetCarryingObjects() const
{
  std::set<ObjectID> objects;
  if (_carryingObjectID.IsSet()) {
    objects.insert(_carryingObjectID);
  }
  if (_carryingObjectOnTopID.IsSet()) {
    objects.insert(_carryingObjectOnTopID);
  }
  return objects;
}

bool Robot::IsCarryingObject(const ObjectID& objectID) const
{
  return _carryingObjectID == objectID || _carryingObjectOnTopID == objectID;
}

void Robot::SetCarryingObject(ObjectID carryObjectID, Vision::Marker::Code atMarkerCode)
{
  ObservableObject* object = _blockWorld->GetLocatedObjectByID(carryObjectID);
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.SetCarryingObject.NullCarryObject",
                      "Object %d no longer exists in the world. Can't set it as robot's carried object.",
                      carryObjectID.GetValue());
  }
  else
  {
    _carryingObjectID = carryObjectID;
    _carryingMarkerCode = atMarkerCode;
    
    // Don't remain localized to an object if we are now carrying it
    if(_carryingObjectID == GetLocalizedTo())
    {
      // Note that the robot may still remaing localized (based on its
      // odometry), but just not *to an object*
      SetLocalizedTo(nullptr);
    } // if(_carryingObjectID == GetLocalizedTo())
    
    // Tell the robot it's carrying something
    // TODO: This is probably not the right way/place to do this (should we pass in carryObjectOnTopID?)
    if(_carryingObjectOnTopID.IsSet()) {
      SendSetCarryState(CarryState::CARRY_2_BLOCK);
    } else {
      SendSetCarryState(CarryState::CARRY_1_BLOCK);
    }
  }
}
  
void Robot::UnSetCarryingObjects(bool topOnly)
{
  // Note this loop doesn't actually _do_ anything. It's just sanity checks.
  std::set<ObjectID> carriedObjectIDs = GetCarryingObjects();
  for (auto& objID : carriedObjectIDs)
  {
    if (topOnly && objID != _carryingObjectOnTopID) {
      continue;
    }
        
    ObservableObject* carriedObject = _blockWorld->GetLocatedObjectByID(objID);
    if(carriedObject == nullptr) {
      PRINT_NAMED_ERROR("Robot.UnSetCarryingObjects.NullObject",
                        "Object %d robot %d thought it was carrying no longer exists in the world.",
                        objID.GetValue(), GetID());
      continue;
    }
    
    if ( carriedObject->GetPose().GetParent() == &_liftPose) {
      // if the carried object is still attached to the lift it can cause issues. We had a bug
      // in which we delocalized and unset as carrying, but would not dettach from lift, causing
      // the cube to accidentally inherit the new origin via its parent, the lift, since the robot is always
      // in the current origin. It would still be the a copy in the old origin in blockworld, but its pose
      // would be pointing to the current one, which caused issues with relocalization.
      // It's a warning because I think there are legit cases (like ClearObject), where it would be fine to
      // ignore the current pose, since it won't be used again.
      PRINT_NAMED_WARNING("Robot.UnSetCarryingObjects.StillAttached",
                          "Setting carried object '%d' as not being carried, but the pose is still attached to the lift", objID.GetValue());
      continue;
    }
    
    if ( !carriedObject->GetPose().GetParent()->IsOrigin() ) {
      // this happened as a bug when we had a stack of 2 cubes in the lift. The top one was not being detached properly,
      // so its pose was left attached to the bottom cube, which could cause issues if we ever deleted the bottom
      // object without seeing the top one ever again, since the pose for the bottom one (which is still top's pose's
      // parent) is deleted.
      // Also like &_liftPose check above, I believe it can happen when we delete the object, so downgraded to warning
      // instead of ANKI_VERIFY (which would be my ideal choice if delete took care of also detaching)
      PRINT_NAMED_WARNING("Robot.UnSetCarryingObjects.StillAttachedToSomething",
                          "Setting carried object '%d' as not being carried, but the pose is still attached to something (other cube?!)",
                          objID.GetValue());
    }
  }

  // this method should not affect the objects pose or pose state; just clear the IDs

  if (!topOnly) {
    // Tell the robot it's not carrying anything
    if (_carryingObjectID.IsSet()) {
      SendSetCarryState(CarryState::CARRY_NONE);
    }

    // Even if the above failed, still mark the robot's carry ID as unset
    _carryingObjectID.UnSet();
    _carryingMarkerCode = Vision::MARKER_INVALID;
  }
  _carryingObjectOnTopID.UnSet();
}
    
void Robot::UnSetCarryObject(ObjectID objID)
{
  // If it's the bottom object in the stack, unset all carried objects.
  if (_carryingObjectID == objID) {
    UnSetCarryingObjects(false);
  } else if (_carryingObjectOnTopID == objID) {
    UnSetCarryingObjects(true);
  }
}
    
    
Result Robot::SetObjectAsAttachedToLift(const ObjectID& objectID, const Vision::KnownMarker::Code atMarkerCode)
{
  if(!objectID.IsSet()) {
    // This can happen if the robot is picked up after/right at the end of completing a dock
    // The dock object gets cleared from the world but the robot ends up sending a
    // pickAndPlaceResult success
    PRINT_NAMED_WARNING("Robot.PickUpDockObject.ObjectIDNotSet",
                        "No docking object ID set, but told to pick one up.");
    return RESULT_FAIL;
  }
  
  if(IsCarryingObject()) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.AlreadyCarryingObject",
                      "Already carrying an object, but told to pick one up.");
    return RESULT_FAIL;
  }
      
  ObservableObject* object = _blockWorld->GetLocatedObjectByID(objectID);
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectDoesNotExist",
                      "Dock object with ID=%d no longer exists for picking up.", objectID.GetValue());
    return RESULT_FAIL;
  }
  
  // get the marker from the code
  const auto& markersWithCode = object->GetMarkersWithCode(atMarkerCode);
  if ( markersWithCode.empty() ) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoMarkerWithCode",
                      "No marker found with that code.");
    return RESULT_FAIL;
  }
  
  // notify if we have more than one, since we are going to assume that any is fine to use its pose
  // we currently don't have this, so treat as warning. But if we ever allow, make sure that they are
  // simetrical with respect to the object
  const bool hasMultipleMarkers = (markersWithCode.size() > 1);
  if(hasMultipleMarkers) {
    PRINT_NAMED_WARNING("Robot.PickUpDockObject.MultipleMarkersForCode",
                        "Multiple markers found for code '%d'. Using first for lift attachment", atMarkerCode);
  }
  
  const Vision::KnownMarker* attachmentMarker = markersWithCode[0];
  
  // Base the object's pose relative to the lift on how far away the dock
  // marker is from the center of the block
  // TODO: compute the height adjustment per object or at least use values from cozmoConfig.h
  Pose3d objectPoseWrtLiftPose;
  if(object->GetPose().GetWithRespectTo(_liftPose, objectPoseWrtLiftPose) == false) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectAndLiftPoseHaveDifferentOrigins",
                      "Object robot is picking up and robot's lift must share a common origin.");
    return RESULT_FAIL;
  }
      
  objectPoseWrtLiftPose.SetTranslation({attachmentMarker->GetPose().GetTranslation().Length() +
        LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f});

  // If we know there's an object on top of the object we are picking up,
  // mark it as being carried too
  // TODO: Do we need to be able to handle non-actionable objects on top of actionable ones?

  ObservableObject* objectOnTop = _blockWorld->FindLocatedObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM);
  if(objectOnTop != nullptr)
  {
    Pose3d onTopPoseWrtCarriedPose;
    if(objectOnTop->GetPose().GetWithRespectTo(object->GetPose(), onTopPoseWrtCarriedPose) == false)
    {
      PRINT_NAMED_WARNING("Robot.SetObjectAsAttachedToLift",
                          "Found object on top of carried object, but could not get its "
                          "pose w.r.t. the carried object.");
    } else {
      PRINT_NAMED_INFO("Robot.SetObjectAsAttachedToLift",
                       "Setting object %d on top of carried object as also being carried.",
                       objectOnTop->GetID().GetValue());
      
      onTopPoseWrtCarriedPose.SetParent(&object->GetPose());
      
      // Related to COZMO-3384: Consider whether top cubes (in a stack) should notify memory map
      // Notify blockworld of the change in pose for the object on top, but pretend the new pose is unknown since
      // we are not dropping the cube yet
      Result poseResult = GetObjectPoseConfirmer().AddObjectRelativeObservation(objectOnTop, onTopPoseWrtCarriedPose, object);
      if(RESULT_OK != poseResult)
      {
        PRINT_NAMED_WARNING("Robot.SetObjectAsAttachedToLift.AddObjectRelativeObservationFailed",
                            "objectID:%d", object->GetID().GetValue());
        return poseResult;
      }
      
      _carryingObjectOnTopID = objectOnTop->GetID();
    }
    
  } else {
    _carryingObjectOnTopID.UnSet();
  }
      
  SetCarryingObject(objectID, atMarkerCode); // also marks the object as carried
  
  // Robot may have just destroyed a configuration
  // update the configuration manager
  GetBlockWorld().GetBlockConfigurationManager().FlagForRebuild();
  
  // Don't actually change the object's pose until we've checked for objects on top
  Result poseResult = GetObjectPoseConfirmer().AddLiftRelativeObservation(object, objectPoseWrtLiftPose);
  if(RESULT_OK != poseResult)
  {
    // TODO: warn
    return poseResult;
  }
  
  return RESULT_OK;
      
} // AttachObjectToLift()
    
    
Result Robot::SetCarriedObjectAsUnattached(bool deleteLocatedObjects)
{
  if(IsCarryingObject() == false) {
    PRINT_NAMED_WARNING("Robot.SetCarriedObjectAsUnattached.CarryingObjectNotSpecified",
                        "Robot not carrying object, but told to place one. "
                        "(Possibly actually rolling or balancing or popping a wheelie.");
    return RESULT_FAIL;
  }
  
  // we currently only support attaching/detaching two objects
  DEV_ASSERT(GetCarryingObjects().size()<=2,"Robot.SetCarriedObjectAsUnattached.CountNotSupported");
  
  
  ObservableObject* object = _blockWorld->GetLocatedObjectByID(_carryingObjectID);
      
  if(object == nullptr)
  {
    // This really should not happen.  How can a object being carried get deleted?
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.CarryingObjectDoesNotExist",
                      "Carrying object with ID=%d no longer exists.", _carryingObjectID.GetValue());
    return RESULT_FAIL;
  }
     
  Pose3d placedPoseWrtRobot;
  if(object->GetPose().GetWithRespectTo(_pose, placedPoseWrtRobot) == false) {
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.OriginMisMatch",
                      "Could not get carrying object's pose relative to robot's origin.");
    return RESULT_FAIL;
  }
  
  // Initially just mark the pose as Dirty. Iff deleteLocatedObjects=true, then the ClearObject
  // call at the end will mark as Unknown. This is necessary because there are some
  // unfortunate ordering dependencies with how we set the pose, set the pose state, and
  // unset the carrying objects below. It's safer to do all of that, and _then_
  // clear the objects at the end.
  const Result poseResult = GetObjectPoseConfirmer().AddRobotRelativeObservation(object, placedPoseWrtRobot, PoseState::Dirty);
  if(RESULT_OK != poseResult)
  {
    PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopRobotRelativeObservationFailed",
                      "AddRobotRealtiveObservation failed for %d", object->GetID().GetValue());
    return poseResult;
  }
    
  PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.ObjectPlaced",
                   "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).",
                   _ID, object->GetID().GetValue(),
                   object->GetPose().GetTranslation().x(),
                   object->GetPose().GetTranslation().y(),
                   object->GetPose().GetTranslation().z());
  
  // if we have a top one, we expect it to currently be attached to the bottom one (pose-wise)
  // recalculate its pose right where we think it is, but detach from the block, and hook directly to the origin
  if ( _carryingObjectOnTopID.IsSet() )
  {
    ObservableObject* topObject = _blockWorld->GetLocatedObjectByID(_carryingObjectOnTopID);
    if ( nullptr != topObject )
    {
      // get wrt robot so that we can add a robot observation (handy way to modify a pose)
      Pose3d topPlacedPoseWrtRobot;
      if(topObject->GetPose().GetWithRespectTo(_pose, topPlacedPoseWrtRobot) == true)
      {
        const Result topPoseResult = GetObjectPoseConfirmer().AddRobotRelativeObservation(topObject, topPlacedPoseWrtRobot, PoseState::Dirty);
        if(RESULT_OK == topPoseResult)
        {
          PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.TopObjectPlaced",
                            "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).",
                            _ID,
                            topObject->GetID().GetValue(),
                            topObject->GetPose().GetTranslation().x(),
                            topObject->GetPose().GetTranslation().y(),
                            topObject->GetPose().GetTranslation().z());
        }
        else
        {
          PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopRobotRelativeObservationFailed",
                            "AddRobotRealtiveObservation failed for %d", topObject->GetID().GetValue());
        }
      }
      else
      {
        PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopOriginMisMatch",
                          "Could not get top carrying object's pose relative to robot's origin.");
      }
    }
    else
    {
      PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.TopCarryingObjectDoesNotExist",
                        "Top carrying object with ID=%d no longer exists.", _carryingObjectOnTopID.GetValue());
    }
  }

  // Store the object IDs we were carrying before we unset them so we can clear them later if needed
  auto const& carriedObjectIDs = GetCarryingObjects();
  
  UnSetCarryingObjects();  
      
  if(deleteLocatedObjects)
  {
    BlockWorldFilter filter;
    filter.AddAllowedIDs(carriedObjectIDs);
    GetBlockWorld().DeleteLocatedObjects(filter);
  }
    
  return RESULT_OK;
      
} // UnattachCarriedObject()
    
    
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool Robot::CanInteractWithObjectHelper(const ObservableObject& object, Pose3d& relPose) const
{
  // TODO:(bn) maybe there should be some central logic for which object families are valid here
  if( object.GetFamily() != ObjectFamily::Block &&
      object.GetFamily() != ObjectFamily::LightCube ) {
    return false;
  }

  // check that the object is ready to place on top of
  if( !object.IsRestingFlat() ||
      (IsCarryingObject() && GetCarryingObject() == object.GetID()) ) {
    return false;
  }

  // check if we can transform to robot space
  if ( !object.GetPose().GetWithRespectTo(GetPose(), relPose) ) {
    return false;
  }

  // check if it has something on top
  const ObservableObject* objectOnTop = GetBlockWorld().FindLocatedObjectOnTopOf(object, STACKED_HEIGHT_TOL_MM);
  if ( nullptr != objectOnTop ) {
    return false;
  }

  return true;
}
    
bool Robot::CanStackOnTopOfObject(const ObservableObject& objectToStackOn) const
{
  // Note rsam/kevin: this only works currently for original cubes. Doing height checks would require more
  // comparison of sizes, checks for I can stack but not pick up due to slack required to pick up, etc. In order
  // to simplify just cover the most basic case here (for the moment)

  Pose3d relPos;
  if( ! CanInteractWithObjectHelper(objectToStackOn, relPos) ) {
    return false;
  }
            
  // check if it's too high to stack on
  if ( objectToStackOn.IsPoseTooHigh(relPos, 1.f, STACKED_HEIGHT_TOL_MM, 0.5f) ) {
    return false;
  }
    
  // all checks clear
  return true;
}

bool Robot::CanPickUpObject(const ObservableObject& objectToPickUp) const
{
  Pose3d relPos;
  if( ! CanInteractWithObjectHelper(objectToPickUp, relPos) ) {
    return false;
  }
      
  // check if it's too high to pick up
  if ( objectToPickUp.IsPoseTooHigh(relPos, 2.f, STACKED_HEIGHT_TOL_MM, 0.5f) ) {
    return false;
  }
    
  // all checks clear
  return true;
}

bool Robot::CanPickUpObjectFromGround(const ObservableObject& objectToPickUp) const
{
  Pose3d relPos;
  if( ! CanInteractWithObjectHelper(objectToPickUp, relPos) ) {
    return false;
  }
      
  // check if it's too high to pick up
  if ( objectToPickUp.IsPoseTooHigh(relPos, 0.5f, ON_GROUND_HEIGHT_TOL_MM, 0.f) ) {
    return false;
  }
    
  // all checks clear
  return true;
}
    
// ============ Messaging ================
    
Result Robot::SendMessage(const RobotInterface::EngineToRobot& msg, bool reliable, bool hot) const
{
  Result sendResult = _context->GetRobotManager()->GetMsgHandler()->SendMessage(_ID, msg, reliable, hot);
  if(sendResult != RESULT_OK) {
    const char* msgTypeName = EngineToRobotTagToString(msg.GetTag());
    Util::sWarningF("Robot.SendMessage", { {DDATA, msgTypeName} }, "Robot %d failed to send a message type %s", _ID, msgTypeName);
  }
  return sendResult;
}
      
// Sync time with physical robot and trigger it robot to send back camera calibration
Result Robot::SendSyncTime() const
{
  Result result = SendMessage(RobotInterface::EngineToRobot(
                                RobotInterface::SyncTime(_ID,
                                                         #ifdef COZMO_V2
                                                         AndroidHAL::getInstance()->GetTimeStamp(),
                                                         #else
                                                         BaseStationTimer::getInstance()->GetCurrentTimeStamp(),
                                                         #endif
                                                         DRIVE_CENTER_OFFSET)));
  if (result == RESULT_OK) {
    result = SendMessage(RobotInterface::EngineToRobot(AnimKeyFrame::InitController()));
  }
  if(result == RESULT_OK) {
    result = SendMessage(RobotInterface::EngineToRobot(
                           RobotInterface::ImageRequest(ImageSendMode::Stream, ImageResolution::QVGA)));
        
    // Reset pose on connect
    PRINT_NAMED_INFO("Robot.SendSyncTime", "Setting pose to (0,0,0)");
    Pose3d zeroPose(0, Z_AXIS_3D(), {0,0,0}, _worldOrigin);
    return SendAbsLocalizationUpdate(zeroPose, 0, GetPoseFrameID());
  }
  
  if (result != RESULT_OK) {
    PRINT_NAMED_WARNING("Robot.SendSyncTime.FailedToSend","");
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
  Pose3d poseWrtOrigin = pose.GetWithRespectToOrigin();
  
  const Pose3d* origin = &poseWrtOrigin.FindOrigin();

  const PoseOriginID_t originID = GetPoseOriginList().GetOriginID(origin);
  if(originID == PoseOriginList::UnknownOriginID)
  {
    PRINT_NAMED_WARNING("Robot.SendAbsLocalizationUpdate.NoPoseOriginIndex",
                        "Origin %s (%p)",
                        origin->GetName().c_str(), origin);
    return RESULT_FAIL;
  }
  
  // Sanity check: if we grab the origin the index we just got, it should be the one we searched for
  DEV_ASSERT(GetPoseOriginList().GetOriginByID(originID) == origin,
             "Robot.SendAbsLocalizationUpdate.OriginIndexLookupFailed");
  
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
  if (_stateHistory->GetLatestVisionOnlyState(t, histState) == RESULT_FAIL) {
    PRINT_NAMED_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
    return RESULT_FAIL;
  }

  return SendAbsLocalizationUpdate(histState.GetPose().GetWithRespectToOrigin(), t, histState.GetFrameId());
}
    
Result Robot::SendHeadAngleUpdate() const
{
  return SendMessage(RobotInterface::EngineToRobot(
                       RobotInterface::HeadAngleUpdate(_currentHeadAngle)));
}

Result Robot::SendIMURequest(const u32 length_ms) const
{
  return SendRobotMessage<IMURequest>(length_ms);
}
  

bool Robot::HasExternalInterface() const
{
  return _context->GetExternalInterface() != nullptr;
}

IExternalInterface* Robot::GetExternalInterface()
{
  DEV_ASSERT(_context->GetExternalInterface() != nullptr, "Robot.ExternalInterface.nullptr");
  return _context->GetExternalInterface();
}

Util::Data::DataPlatform* Robot::GetContextDataPlatform()
{
  return _context->GetDataPlatform();
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
  const Vision::CameraCalibration& cameraCalibration = visionComponent.GetCameraCalibration();
  
  ExternalInterface::CameraConfig cameraConfig(cameraCalibration.GetFocalLength_x(),
                                               cameraCalibration.GetFocalLength_y(),
                                               cameraCalibration.GetCenter_x(),
                                               cameraCalibration.GetCenter_y(),
                                               cameraCalibration.ComputeHorizontalFOV().getDegrees(),
                                               cameraCalibration.ComputeVerticalFOV().getDegrees(),
                                               visionComponent.GetMinCameraExposureTime_ms(),
                                               visionComponent.GetMaxCameraExposureTime_ms(),
                                               visionComponent.GetMinCameraGain(),
                                               visionComponent.GetMaxCameraGain());
  
  ExternalInterface::PerRobotSettings robotSettings(GetID(),
                                                    GetHeadSerialNumber(),
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
  return GetBoundingQuadXY(_pose, padding_mm);
}
    
Quad2f Robot::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
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
  for(CornerName iCorner = FirstCorner; iCorner < NumCorners; ++iCorner) {
    // Rotate to given pose
    boundingQuad[iCorner] = R * boundingQuad[iCorner];
  }
      
  // Re-center
  Point2f center(atPose.GetTranslation().x(), atPose.GetTranslation().y());
  boundingQuad += center;
      
  return boundingQuad;
      
} // GetBoundingBoxXY()
    
  
    
    
f32 Robot::GetHeight() const
{
  return std::max(ROBOT_BOUNDING_Z, GetLiftHeight() + LIFT_HEIGHT_ABOVE_GRIPPER);
}
    
f32 Robot::GetLiftHeight() const
{
  return ConvertLiftAngleToLiftHeightMM(GetLiftAngle());
}
    
Pose3d Robot::GetLiftPoseWrtCamera(f32 atLiftAngle, f32 atHeadAngle) const
{
  Pose3d liftPose(_liftPose);
  ComputeLiftPose(atLiftAngle, liftPose);
      
  Pose3d camPose = GetCameraPose(atHeadAngle);
      
  Pose3d liftPoseWrtCam;
  bool result = liftPose.GetWithRespectTo(camPose, liftPoseWrtCam);
  
  DEV_ASSERT(result, "Lift and camera poses should be in same pose tree");
      
  return liftPoseWrtCam;
}
    
f32 Robot::ConvertLiftHeightToLiftAngleRad(f32 height_mm)
{
  height_mm = CLIP(height_mm, LIFT_HEIGHT_LOWDOCK, LIFT_HEIGHT_CARRY);
  return asinf((height_mm - LIFT_BASE_POSITION[2] - LIFT_FORK_HEIGHT_REL_TO_ARM_END)/LIFT_ARM_LENGTH);
}

f32 Robot::ConvertLiftAngleToLiftHeightMM(f32 angle_rad)
{
  return (sinf(angle_rad) * LIFT_ARM_LENGTH) + LIFT_BASE_POSITION[2] + LIFT_FORK_HEIGHT_REL_TO_ARM_END;
}
    
Result Robot::RequestIMU(const u32 length_ms) const
{
  return SendIMURequest(length_ms);
}
    
    
// ============ Pose history ===============

Result Robot::AddRobotStateToHistory(const Pose3d& pose, const RobotState& state)
{
  const HistRobotState histState(pose, state);
  return _stateHistory->AddRawOdomState(state.timestamp, histState);
}
    
    
Result Robot::UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin)
{
  // Reverse the connection between origin and robot, and connect the new
  // reversed connection
  //ASSERT_NAMED(p.GetPose().GetParent() == _poseOrigin, "Robot.UpdateWorldOrigin.InvalidPose");
  //Pose3d originWrtRobot = _pose.GetInverse();
  //originWrtRobot.SetParent(&newPoseOrigin);
      
  // TODO: get rid of nasty const_cast somehow
  Pose3d* newOrigin = const_cast<Pose3d*>(newPoseWrtNewOrigin.GetParent());
  newOrigin->SetParent(nullptr);
      
  // TODO: We should only be doing this (modifying what _worldOrigin points to) when it is one of the
  // placeHolder poseOrigins, not if it is a mat!
  std::string origName(_worldOrigin->GetName());
  *_worldOrigin = _pose.GetInverse();
  _worldOrigin->SetParent(&newPoseWrtNewOrigin);
      
      
  // Connect the old origin's pose to the same root the robot now has.
  // It is no longer the robot's origin, but for any of its children,
  // it is now in the right coordinates.
  if(_worldOrigin->GetWithRespectTo(*newOrigin, *_worldOrigin) == false) {
    PRINT_NAMED_ERROR("Robot.UpdateWorldOrigin.NewLocalizationOriginProblem",
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
  return _stateHistory->AddVisionOnlyState(t, histState);
}

Result Robot::GetComputedStateAt(const TimeStamp_t t_request, Pose3d& pose) const
{
  HistStateKey histStateKey;
  const HistRobotState* histStatePtr = nullptr;
  Result lastResult = _stateHistory->GetComputedStateAt(t_request, &histStatePtr, &histStateKey);
  if(lastResult == RESULT_OK) {
    // Grab the pose stored in the pose stamp we just found, and hook up
    // its parent to the robot's current world origin (since pose history
    // doesn't keep track of pose parent chains)
    pose = histStatePtr->GetPose();
    pose.SetParent(_worldOrigin);
  }
  return lastResult;
}
bool Robot::UpdateCurrPoseFromHistory()
{
  bool poseUpdated = false;
      
  TimeStamp_t t;
  HistRobotState histState;
  if (_stateHistory->ComputeStateAt(_stateHistory->GetNewestTimeStamp(), t, histState) == RESULT_OK)
  {

    Pose3d newPose;
    if((histState.GetPose().GetWithRespectTo(*_worldOrigin, newPose))==false)
    {
      // This is not necessarily an error anymore: it's possible we've received an
      // odometry update from the robot w.r.t. an old origin (before being delocalized),
      // in which case we can't use it to update the current pose of the robot
      // in its new frame.
      PRINT_NAMED_INFO("Robot.UpdateCurrPoseFromHistory.GetWrtParentFailed",
                        "Could not update robot %d's current pose using historical pose w.r.t. %s because we are now in frame %s.",
                        _ID, histState.GetPose().FindOrigin().GetName().c_str(),
                       _worldOrigin->GetName().c_str());
    }
    else
    {
      SetPose(newPose);
      poseUpdated = true;
    }

  }
      
  return poseUpdated;
}
  
Result Robot::ConnectToObjects(const FactoryIDArray& factory_ids)
{
  DEV_ASSERT_MSG(factory_ids.size() == _objectsToConnectTo.size(),
                 "Robot.ConnectToObjects.InvalidArrayLength",
                 "%zu slots requested. Max %zu",
                 factory_ids.size(), _objectsToConnectTo.size());
      
  std::stringstream strs;
  for (auto it = factory_ids.begin(); it != factory_ids.end(); ++it)
  {
    strs << "0x" << std::hex << *it << ", ";
  }
  std::stringstream strs2;
  for (auto it = _objectsToConnectTo.begin(); it != _objectsToConnectTo.end(); ++it)
  {
    strs2 << "0x" << std::hex << it->factoryID << ", pending = " << it->pending << ", ";
  }
  PRINT_CH_INFO("BlockPool", "Robot.ConnectToObjects",
                "Before processing factory_ids = %s. _objectsToConnectTo = %s",
                strs.str().c_str(), strs2.str().c_str());
      
  // Save the new list so we process it during the update loop. Note that we compare
  // against the list of current connected objects but we store it in the list of
  // objects to connect to.
  for (int i = 0; i < _connectedObjects.size(); ++i)
  {
    if (factory_ids[i] != _connectedObjects[i].factoryID) {
      _objectsToConnectTo[i].factoryID = factory_ids[i];
      _objectsToConnectTo[i].pending = true;
    }
  }
      
  return RESULT_OK;
}
  
bool Robot::IsConnectedToObject(FactoryID factoryID) const
{
  for (const ActiveObjectInfo& objectInfo : _connectedObjects)
  {
    if (objectInfo.factoryID == factoryID)
    {
      return true;
    }
  }
  
  return false;
}
  
void Robot::HandleConnectedToObject(uint32_t activeID, FactoryID factoryID, ObjectType objectType)
{
  ActiveObjectInfo& objectInfo = _connectedObjects[activeID];
  
  if (_connectedObjects[activeID].factoryID != factoryID)
  {
    PRINT_CH_INFO("BlockPool",
                  "Robot.HandleConnectedToObject",
                  "Ignoring connection to object 0x%x of type %s with active ID %d because expecting connection to 0x%x of type %s",
                  factoryID,
                  EnumToString(objectType),
                  activeID,
                  objectInfo.factoryID,
                  EnumToString(objectInfo.objectType));
    return;
  }
  
  if ((objectInfo.connectionState != ActiveObjectInfo::ConnectionState::PendingConnection) &&
      (objectInfo.connectionState != ActiveObjectInfo::ConnectionState::Disconnected))
  {
    PRINT_NAMED_ERROR("Robot.HandleConnectedToObject.InvalidState",
                      "Invalid state %d when connected to object 0x%x with active ID %d",
                      (int)objectInfo.connectionState, factoryID, activeID);
  }

  PRINT_CH_INFO("BlockPool", "Robot.HandleConnectToObject",
                "Connected to active Id %d with factory Id 0x%x of type %s. Connection State = %d",
                activeID, factoryID, EnumToString(objectType), objectInfo.connectionState);
  
  // Remove from the list of discovered objects since we are connecting to it
  RemoveDiscoveredObjects(factoryID);
  
  _connectedObjects[activeID].connectionState = ActiveObjectInfo::ConnectionState::Connected;
  _connectedObjects[activeID].lastDisconnectionTime = 0.0f;
}

void Robot::HandleDisconnectedFromObject(uint32_t activeID, FactoryID factoryID, ObjectType objectType)
{
  ActiveObjectInfo& objectInfo = _connectedObjects[activeID];

  if (objectInfo.factoryID != factoryID)
  {
    PRINT_CH_INFO("BlockPool",
                  "Robot.HandleDisconnectedFromObject",
                  "Ignoring disconnection from object 0x%x of type %s with active ID %d because expecting connection to 0x%x of type %s",
                  factoryID,
                  EnumToString(objectType),
                  activeID,
                  objectInfo.factoryID,
                  EnumToString(objectInfo.objectType));
    return;
  }

  if ((objectInfo.connectionState != ActiveObjectInfo::ConnectionState::PendingDisconnection) &&
      (objectInfo.connectionState != ActiveObjectInfo::ConnectionState::Connected))
  {
    PRINT_NAMED_ERROR("Robot.HandleDisconnectedFromObject.InvalidState",
                      "Invalid state %d when disconnected from object 0x%x with active ID %d",
                      (int)objectInfo.connectionState, factoryID, activeID);
  }
  
  PRINT_CH_INFO("BlockPool", "Robot.HandleDisconnectedFromObject",
                "Disconnected from active Id %d with factory Id 0x%x of type %s. Connection State = %d",
                activeID, factoryID, EnumToString(objectType), (int)objectInfo.connectionState);
  
  if (objectInfo.connectionState == ActiveObjectInfo::ConnectionState::PendingDisconnection)
  {
    // If we wanted to disconnect from this object, clear all the data now that we have done it
    objectInfo.Reset();
  }
  else
  {
    // We have disconnected without requesting it. Anotate that we are in that state
    objectInfo.connectionState = ActiveObjectInfo::ConnectionState::Disconnected;
    objectInfo.lastDisconnectionTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}

void Robot::ConnectToRequestedObjects()
{
  // Check if there is any new petition to connect to a new block
  auto it = std::find_if(_objectsToConnectTo.begin(), _objectsToConnectTo.end(), [](const ObjectToConnectToInfo& obj) {
      return obj.pending;
    });
      
  if (it == _objectsToConnectTo.end()) {
    return;
  }
      
//	std::stringstream strs;
//	for (auto it = _objectsToConnectTo.begin(); it != _objectsToConnectTo.end(); ++it) {
//		strs << "0x" << std::hex << it->factoryID << ", pending = " << it->pending << ", ";
//	}
//	std::stringstream strs2;
//	for (auto it = _connectedObjects.begin(); it != _connectedObjects.end(); ++it) {
//		strs2 << "0x" << std::hex << it->factoryID << ", ";
//	}
     
  // PRINT_NAMED_INFO("Robot.ConnectToRequestedObjects.BeforeProcessing",
//                "_objectsToConnectTo = %s. _connectedObjects = %s",
//                strs.str().c_str(), strs2.str().c_str());

  // Iterate over the connected objects and the new factory IDs to see what we need to send in the
  // message for every slot.
  DEV_ASSERT(_objectsToConnectTo.size() == _connectedObjects.size(),
             "Robot.ConnectToRequestedObjects.InvalidArraySize");
  
  for (int i = 0; i < _objectsToConnectTo.size(); ++i) {
        
    ObjectToConnectToInfo& newObjectToConnectTo = _objectsToConnectTo[i];
    ActiveObjectInfo& activeObjectInfo = _connectedObjects[i];
        
    // If there is nothing to do with this slot, continue
    if (!newObjectToConnectTo.pending) {
      continue;
    }
        
    // If we have already connected to the object in the given slot, we don't have to do anything
    if (newObjectToConnectTo.factoryID == activeObjectInfo.factoryID) {
      newObjectToConnectTo.Reset();
      continue;
    }
        
    // If the new factory ID is 0 then we want to disconnect from the object
    if (newObjectToConnectTo.factoryID == ActiveObject::InvalidFactoryID) {
      PRINT_CH_INFO("BlockPool", "Robot.ConnectToRequestedObjects.Sending",
                    "Sending message for slot %d with factory ID = %d",
                    i, ActiveObject::InvalidFactoryID);
      activeObjectInfo.connectionState = ActiveObjectInfo::ConnectionState::PendingDisconnection;
      newObjectToConnectTo.Reset();
      SendMessage(RobotInterface::EngineToRobot(SetPropSlot((FactoryID)ActiveObject::InvalidFactoryID, i)));
      continue;
    }
        
    // We are connecting to a new object. Check if the object is discovered yet
    // If it is not, don't clear it from the list of requested objects in case
    // we find it in the next execution of this loop
    auto discoveredObjIt = _discoveredObjects.find(newObjectToConnectTo.factoryID);
    if (discoveredObjIt == _discoveredObjects.end()) {
      continue;
    }
        
    for (const auto & connectedObj : _connectedObjects) {
      if ((connectedObj.connectionState == ActiveObjectInfo::ConnectionState::Connected) && (connectedObj.objectType == discoveredObjIt->second.objectType)) {
        PRINT_NAMED_WARNING("Robot.ConnectToRequestedObjects.SameTypeAlreadyConnected",
                            "Object with factory ID 0x%x matches type (%s) of another connected object. "
                            "Only one of each type may be connected.",
                            newObjectToConnectTo.factoryID, EnumToString(connectedObj.objectType));

        // If we can't connect to the new object we keep the one we have now
        newObjectToConnectTo.Reset();
        continue;
      }
    }
        
    // This is valid object to connect to.
    PRINT_CH_INFO("BlockPool", "Robot.ConnectToRequestedObjects.Sending",
                  "Sending message for slot %d with factory ID = 0x%x",
                  i, newObjectToConnectTo.factoryID);
    SendMessage(RobotInterface::EngineToRobot(SetPropSlot(newObjectToConnectTo.factoryID, i)));
        
    // We are done with this slot
    activeObjectInfo = discoveredObjIt->second;
    activeObjectInfo.connectionState = ActiveObjectInfo::ConnectionState::PendingConnection;
    newObjectToConnectTo.Reset();
  }
      
//   std::stringstream strs3;
//   for (auto it = _objectsToConnectTo.begin(); it != _objectsToConnectTo.end(); ++it) {
//     strs3 << "0x" << std::hex << it->factoryID << ", pending = " << it->pending << ", ";
//   }
//   std::stringstream strs4;
//   for (auto it = _connectedObjects.begin(); it != _connectedObjects.end(); ++it) {
//     strs4 << "0x" << std::hex << it->factoryID << ", ";
//   }
     
  // PRINT_NAMED_INFO("Robot.ConnectToRequestedObjects.AfterProcessing",
  //                  "_objectsToConnectTo = %s. _connectedObjects = %s", strs3.str().c_str(), strs4.str().c_str());

  return;
}
  
void Robot::CheckDisconnectedObjects()
{
  // Check for objects that have been disconnected long enough to consider them gone. Note the object has to be in
  // the Disconnected state which is the state we get when the disconnection wasn't requested.
  const float time = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((_lastDisconnectedCheckTime <= 0.0f) || (time >= (_lastDisconnectedCheckTime + kDisconnectedCheckDelay)))
  {
    for (int i = 0; i < _connectedObjects.size(); ++i)
    {
      ActiveObjectInfo& objectInfo = _connectedObjects[i];
      if ((objectInfo.connectionState == ActiveObjectInfo::ConnectionState::Disconnected) && (time > (objectInfo.lastDisconnectionTime + kDisconnectedDelay)))
      {
        PRINT_CH_INFO("BlockPool", "Robot.CheckDisconnectedObjects",
                      "Resetting slot %d with factory ID 0x%x, connection state %d. Object disconnected at %f, current time is %f with max delay %f seconds",
                      i, objectInfo.factoryID, objectInfo.connectionState, objectInfo.lastDisconnectionTime, time, kDisconnectedDelay);
        objectInfo.Reset();
      }
    }
    
    _lastDisconnectedCheckTime = time;
  }
}

void Robot::BroadcastAvailableObjects(bool enable)
{
  _enableDiscoveredObjectsBroadcasting = enable;
}

Result Robot::AbortAll()
{
  bool anyFailures = false;
      
  _actionList->Cancel();
      
  if(_pathComponent->Abort() != RESULT_OK) {
    anyFailures = true;
  }
      
  if(AbortDocking() != RESULT_OK) {
    anyFailures = true;
  }
      
  if(AbortAnimation() != RESULT_OK) {
    anyFailures = true;
  }
  
  _movementComponent->StopAllMotors();
      
  if(anyFailures) {
    return RESULT_FAIL;
  } else {
    return RESULT_OK;
  }
      
}
      
Result Robot::AbortDocking()
{
  return SendAbortDocking();
}
      
Result Robot::AbortAnimation()
{
  return SendAbortAnimation();
}

Result Robot::SendAbortAnimation()
{
  return SendMessage(RobotInterface::EngineToRobot(RobotInterface::AbortAnimation()));
}
    
Result Robot::SendAbortDocking()
{
  return SendMessage(RobotInterface::EngineToRobot(Anki::Cozmo::AbortDocking()));
}
 
Result Robot::SendSetCarryState(CarryState state)
{
  return SendMessage(RobotInterface::EngineToRobot(Anki::Cozmo::CarryStateUpdate(state)));
}
      
Result Robot::SendFlashObjectIDs()
{
  return SendMessage(RobotInterface::EngineToRobot(FlashObjectIDs()));
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
      
  // Send message to viz
  GetContext()->GetVizManager()->SetText(VizManager::DEBUG_STRING,
                                         NamedColors::ORANGE,
                                         "%s", text);
      
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

void Robot::MoveRobotPoseForward(const Pose3d &startPose, f32 distance, Pose3d &movedPose) {
  movedPose = startPose;
  f32 angle = startPose.GetRotationAngle<'Z'>().ToFloat();
  Vec3f trans;
  trans.x() = startPose.GetTranslation().x() + distance * cosf(angle);
  trans.y() = startPose.GetTranslation().y() + distance * sinf(angle);
  movedPose.SetTranslation(trans);
}
      
f32 Robot::GetDriveCenterOffset() const {
  f32 driveCenterOffset = DRIVE_CENTER_OFFSET;
  if (IsCarryingObject()) {
    driveCenterOffset = 0;
  }
  return driveCenterOffset;
}
    
bool Robot::Broadcast(ExternalInterface::MessageEngineToGame&& event)
{
  if(HasExternalInterface()) {
    GetExternalInterface()->Broadcast(event);
    return true;
  } else {
    return false;
  }
}

bool Robot::Broadcast(VizInterface::MessageViz&& event)
{
  auto* vizMgr = _context->GetVizManager();
  if (vizMgr != nullptr) {
    vizMgr->SendVizMessage(std::move(event));
    return true;
  }
  return false;
}

void Robot::BroadcastEngineErrorCode(EngineErrorCode error)
{
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::EngineErrorCodeMessage(error)));
  PRINT_NAMED_ERROR("Robot.BroadcastEngineErrorCode",
                    "Engine failing with error code %s[%hhu]",
                    EnumToString(error),
                    error);
}
    
ExternalInterface::RobotState Robot::GetRobotState() const
{
  ExternalInterface::RobotState msg;
      
  msg.robotID = GetID();
      
  msg.pose = GetPose().ToPoseStruct3d(GetPoseOriginList());
  if(msg.pose.originID == PoseOriginList::UnknownOriginID)
  {
    PRINT_NAMED_WARNING("Robot.GetRobotState.BadOriginID", "");
  }
  
  msg.poseAngle_rad = GetPose().GetRotationAngle<'Z'>().ToFloat();
  msg.posePitch_rad = GetPitchAngle().ToFloat();
      
  msg.leftWheelSpeed_mmps  = GetLeftWheelSpeed();
  msg.rightWheelSpeed_mmps = GetRightWheelSpeed();
      
  msg.headAngle_rad = GetHeadAngle();
  msg.liftHeight_mm = GetLiftHeight();
  
  msg.accel = GetHeadAccelData();
  msg.gyro = GetHeadGyroData();
  
  msg.status = _lastStatusFlags;
  if(IsAnimating())        { msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING; }
  if(IsIdleAnimating())    { msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING_IDLE; }
  if(IsCarryingObject())   {
    msg.status |= (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK;
    msg.carryingObjectID = GetCarryingObject();
    msg.carryingObjectOnTopID = GetCarryingObjectOnTop();
  } else {
    msg.carryingObjectID = -1;
    msg.carryingObjectOnTopID = -1;
  }
  
  msg.gameStatus = 0;
  if (IsLocalized() && _offTreadsState == OffTreadsState::OnTreads) { msg.gameStatus |= (uint8_t)GameStatusFlag::IsLocalized; }
      
  msg.headTrackingObjectID = GetMoveComponent().GetTrackToObject();
      
  msg.localizedToObjectID = GetLocalizedTo();

  msg.batteryVoltage = GetBatteryVoltage();
      
  msg.lastImageTimeStamp = GetVisionComponent().GetLastProcessedImageTimeStamp();
      
  return msg;
}
  
RobotState Robot::GetDefaultRobotState()
{
  const auto kDefaultStatus = (Util::EnumToUnderlying(RobotStatusFlag::HEAD_IN_POS) |
                               Util::EnumToUnderlying(RobotStatusFlag::LIFT_IN_POS));
  
  const RobotPose kDefaultPose(0.f, 0.f, 0.f, 0.f, 0.f);
  
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
                         5.f, //float batteryVoltage,
                         kDefaultStatus, //uint32_t status,
                         0, //uint16_t lastPathID,
                         0, //uint16_t cliffDataRaw,
                         -1); //int8_t currPathSegment
  
  return state;
}

RobotInterface::MessageHandler* Robot::GetRobotMessageHandler()
{
  if (!_context->GetRobotManager())
  {
    DEV_ASSERT(false, "Robot.GetRobotMessageHandler.nullptr");
    return nullptr;
  }
        
  return _context->GetRobotManager()->GetMsgHandler();
}
    
ObjectType Robot::GetDiscoveredObjectType(FactoryID id)
{
  auto it = _discoveredObjects.find(id);
  if (it != _discoveredObjects.end()) {
    return it->second.objectType;
  }
  return ObjectType::UnknownObject;
}
  
FactoryID Robot::GetClosestDiscoveredObjectsOfType(ObjectType type, uint8_t maxRSSI) const
{
  FactoryID closest = ActiveObject::InvalidFactoryID;
  uint8_t closestRSSI = maxRSSI;

//  std::stringstream str;
//  str << "Search for objects of type = " << EnumToString(type) << ":" << std::endl;
//  std::for_each(_discoveredObjects.cbegin(), _discoveredObjects.cend(), [&](const std::pair<FactoryID, ActiveObjectInfo>& pair)
//  {
//    const ActiveObjectInfo& object = pair.second;
//    if (object.objectType == type)
//    {
//      str << "Factory ID = 0x" << std::hex << object.factoryID << ", RSSI = " << std::dec << (int)object.rssi << std::endl;
//    }
//  });
//  PRINT_CH_INFO("BlockPool", "Robot.GetClosestDiscoveredObjectsOfType", "Total # of objects = %zu\n%s", _discoveredObjects.size(), str.str().c_str());
  
  std::for_each(_discoveredObjects.cbegin(), _discoveredObjects.cend(), [&](const auto& pair)
  {
    const ActiveObjectInfo& object = pair.second;
    if ((object.objectType == type) && (object.rssi <= closestRSSI))
    {
      closest = object.factoryID;
      closestRSSI = object.rssi;
    }
  });
  
  return closest;
}

  
const BehaviorFactory& Robot::GetBehaviorFactory() const
{
  return _behaviorMgr->GetBehaviorFactory();
}

BehaviorFactory& Robot::GetBehaviorFactory()
{
  return _behaviorMgr->GetBehaviorFactory();
}
  
Result Robot::ComputeHeadAngleToSeePose(const Pose3d& pose, Radians& headAngle, f32 yTolFrac) const
{
  Pose3d poseWrtNeck;
  const bool success = pose.GetWithRespectTo(_neckPose, poseWrtNeck);
  if(!success)
  {
    PRINT_NAMED_WARNING("Robot.ComputeHeadAngleToSeePose.OriginMismatch", "");
    return RESULT_FAIL_ORIGIN_MISMATCH;
  }
  
  // Assume the given point is in the XZ plane in front of the camera (i.e. so
  // if we were to turn and face it with the robot's body, we then just need to
  // find the right head angle)
  const Point3f pointWrtNeck(Point2f(poseWrtNeck.GetTranslation()).Length(), // Drop z and get length in XY plane
                             0.f,
                             poseWrtNeck.GetTranslation().z());
  
  Vision::Camera camera(_visionComponent->GetCamera());
  
  const Vision::CameraCalibration* calib = camera.GetCalibration();
  if(nullptr == calib)
  {
    PRINT_NAMED_ERROR("Robot.ComputeHeadAngleToSeePose.NullCamera", "");
    return RESULT_FAIL;
  }
  
  const f32 dampening = 0.8f;
  const f32 kYTol = yTolFrac * calib->GetNrows();
  
  f32 searchAngle_rad = 0.f;
  s32 iteration = 0;
  const s32 kMaxIterations = 25;
  
# define DEBUG_HEAD_ANGLE_ITERATIONS 0
  while(iteration++ < kMaxIterations)
  {
    if(DEBUG_HEAD_ANGLE_ITERATIONS) {
      PRINT_NAMED_DEBUG("ComputeHeadAngle", "%d: %.1fdeg", iteration, RAD_TO_DEG(searchAngle_rad));
    }
    
    // Get point w.r.t. camera at current search angle
    const Pose3d& cameraPoseWrtNeck = GetCameraPose(searchAngle_rad);
    const Point3f& pointWrtCam = cameraPoseWrtNeck.GetInverse() * pointWrtNeck;
    
    // Project point into the camera
    // Note: not using camera's Project3dPoint() method because it does special handling
    //  for points not in the image limits, which we don't want here. We also don't need
    //  to add ycen, because we'll just subtract it right back off to see how far from
    //  centered we are. And we're only using y, so we'll just special-case this here.
    if(Util::IsFltLE(pointWrtCam.z(), 0.f))
    {
      PRINT_NAMED_WARNING("Robot.ComputeHeadAngleToSeePose.BadProjectedZ", "");
      return RESULT_FAIL;
    }
    const f32 y = calib->GetFocalLength_y() * (pointWrtCam.y() / pointWrtCam.z());
    
    // See if the projection is close enough to center
    if(Util::IsFltLE(std::abs(y), kYTol))
    {
      if(DEBUG_HEAD_ANGLE_ITERATIONS) {
        PRINT_NAMED_DEBUG("ComputeHeadAngle", "CONVERGED: %.1fdeg", RAD_TO_DEG(searchAngle_rad));
      }
      
      headAngle = searchAngle_rad;
      break;
    }
    
    // Nope: keep searching. Adjust angle proportionally to how far off we are.
    const f32 angleInc = std::atan2f(y, calib->GetFocalLength_y());
    searchAngle_rad -= dampening*angleInc;
  }

  if(iteration == kMaxIterations)
  {
    PRINT_NAMED_WARNING("Robot.ComputeHeadAngleToSeePose.MaxIterations", "");
    return RESULT_FAIL;
  }
  
  return RESULT_OK;
}
  
Result Robot::ComputeTurnTowardsImagePointAngles(const Point2f& imgPoint, const TimeStamp_t timestamp,
                                                 Radians& absPanAngle, Radians& absTiltAngle) const
{
  const Vision::CameraCalibration* calib = GetVisionComponent().GetCamera().GetCalibration();
  const Point2f pt = imgPoint - calib->GetCenter();
  
  HistRobotState histState;
  TimeStamp_t t;
  Result result = GetStateHistory()->ComputeStateAt(timestamp, t, histState);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_WARNING("Robot.ComputeTurnTowardsImagePointAngles.ComputeHistPoseFailed", "t=%u", timestamp);
    absPanAngle = GetPose().GetRotation().GetAngleAroundZaxis();
    absTiltAngle = GetHeadAngle();
    return result;
  }
  
  absTiltAngle = std::atan2f(-pt.y(), calib->GetFocalLength_y()) + histState.GetHeadAngle_rad();
  absPanAngle  = std::atan2f(-pt.x(), calib->GetFocalLength_x()) + histState.GetPose().GetRotation().GetAngleAroundZaxis();
  
  return RESULT_OK;
}

void Robot::SetBodyColor(const s32 color)
{
  const BodyColor bodyColor = static_cast<BodyColor>(color);
  if(bodyColor <= BodyColor::UNKNOWN ||
     bodyColor >= BodyColor::COUNT ||
     bodyColor == BodyColor::RESERVED)
  {
    PRINT_NAMED_ERROR("Robot.SetBodyColor.InvalidColor",
                      "Robot has invalid body color %d", color);
    return;
  }
  
  _bodyColor = bodyColor;
}

void Robot::ObjectToConnectToInfo::Reset()
{
  factoryID = ActiveObject::InvalidFactoryID;
  pending = false;
}

void Robot::ActiveObjectInfo::Reset()
{
  factoryID = ActiveObject::InvalidFactoryID;
  objectType = ObjectType::InvalidObject;
  connectionState = ConnectionState::Invalid;
  rssi = 0;
  lastDiscoveredTimeStamp = 0;
  lastDisconnectionTime = 0.0f;
}

} // namespace Cozmo
} // namespace Anki
