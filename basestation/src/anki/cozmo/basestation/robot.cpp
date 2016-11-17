
//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/actions/actionContainers.h"
#include "anki/cozmo/basestation/animations/engineAnimationController.h"
#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "anki/cozmo/basestation/minimalAnglePlanner.h"
#include "anki/cozmo/basestation/faceAndApproachPlanner.h"
#include "anki/cozmo/basestation/pathDolerOuter.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/ledEncoding.h"
#include "anki/cozmo/basestation/petWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/robotIdleTimeoutComponent.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/robotToEngineImplMessaging.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/vision/CameraSettings.h"
// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/components/visionComponent.h"
#include "anki/cozmo/basestation/objectPoseConfirmer.h"
#include "anki/cozmo/basestation/blocks/blockFilter.h"
#include "anki/cozmo/basestation/components/blockTapFilterComponent.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/speedChooser.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"
#include "anki/vision/basestation/image.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/robotStatusAndActions.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/types/gameStatusFlag.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/helpers/ankiDefines.h"
#include "util/fileUtils/fileUtils.h"
#include "util/transport/reliableConnection.h"

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp" // For imwrite() in ProcessImage

#include <fstream>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_DISTANCE_FOR_SHORT_PLANNER 40.0f
#define MAX_DISTANCE_TO_PREDOCK_POSE 20.0f
#define MIN_DISTANCE_FOR_MINANGLE_PLANNER 1.0f

#define BUILD_NEW_ANIMATION_CODE 0

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

////////
// Consts for robot offtreadsState
///////

// timeToConsiderOfftreads is tuned based on the fact that we have to wait half a second from the time the cliff sensor detects
// ground to when the robot state message updates to the fact that it is no longer picked up
static const float kRobotTimeToConsiderOfftreads_ms = 250.0f;
static const float kRobotTimeToConsiderOfftreadsOnBack_ms = kRobotTimeToConsiderOfftreads_ms * 5.0f;

// Laying flat angles
static const float kPitchAngleOntreads_rads = DEG_TO_RAD(0);
static const float kPitchAngleOntreadsTolerence_rads = DEG_TO_RAD(45);

//Constants for on back
static const float kPitchAngleOnBack_rads = DEG_TO_RAD(74.5f);
static const float kPitchAngleOnBack_sim_rads = DEG_TO_RAD(96.4f);
static const float kPitchAngleOnBackTolerance_deg = 15.0f;

//Constants for on side
static const float kOnLeftSide_rawYAccel = -9000.0;
static const float kOnRightSide_rawYAccel = 10500.0;
static const float kOnSideTolerance_rawYAccel =3000.0f;

// On face angles
static const float kPitchAngleOnFacePlantMin_rads = DEG_TO_RAD(110.f);
static const float kPitchAngleOnFacePlantMax_rads = DEG_TO_RAD(-110.f);
static const float kPitchAngleOnFacePlantMin_sim_rads = DEG_TO_RAD(110.f); //This has not been tested
static const float kPitchAngleOnFacePlantMax_sim_rads = DEG_TO_RAD(-110.f); //This has not been tested

// For gyro drift check
static const float kDriftCheckMaxRate_rad_per_sec = DEG_TO_RAD_F32(10.f);
static const float kDriftCheckPeriod_ms = 5000.f;
static const float kDriftCheckGyroZMotionThresh_rad_per_sec = DEG_TO_RAD_F32(1.f);
static const float kDriftCheckMaxAngleChangeRate_rad_per_sec = DEG_TO_RAD_F32(0.1f);

// For tool code reading
// 4-degree look down: (Make sure to update cozmoBot.proto to match!)
const RotationMatrix3d Robot::_kDefaultHeadCamRotation = RotationMatrix3d({
  0,             -0.0698,    0.9976,
 -1.0000,         0,         0,
  0,             -0.9976,   -0.0698,
});


Robot::Robot(const RobotID_t robotID, const CozmoContext* context)
  : _context(context)
  , _ID(robotID)
  , _timeSynced(false)
  , _lastMsgTimestamp(0)
  , _blockWorld(new BlockWorld(this))
  , _faceWorld(new FaceWorld(*this))
  , _petWorld(new PetWorld(*this))
  , _behaviorMgr(new BehaviorManager(*this))
  , _aiInformationAnalyzer(new AIInformationAnalyzer())
  , _audioClient(new Audio::RobotAudioClient(this))
  , _animationStreamer(_context, *_audioClient)
  , _drivingAnimationHandler(new DrivingAnimationHandler(*this))
#if BUILD_NEW_ANIMATION_CODE
  , _animationController(new RobotAnimation::EngineAnimationController(_context, _audioClient.get()))
#endif
  , _actionList(new ActionList())
  , _movementComponent(*this)
  , _visionComponentPtr( new VisionComponent(*this, VisionComponent::RunMode::Asynchronous, _context))
  , _nvStorageComponent(*this, _context)
  , _textToSpeechComponent(_context)
  , _objectPoseConfirmerPtr(new ObjectPoseConfirmer(*this))
  , _lightsComponent( new LightsComponent( *this ) )
  , _neckPose(0.f,Y_AXIS_3D(),
              {NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}, &_pose, "RobotNeck")
  , _headCamPose(_kDefaultHeadCamRotation,
                 {HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}, &_neckPose, "RobotHeadCam")
  , _liftBasePose(0.f, Y_AXIS_3D(),
                  {LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}, &_pose, "RobotLiftBase")
  , _liftPose(0.f, Y_AXIS_3D(), {LIFT_ARM_LENGTH, 0.f, 0.f}, &_liftBasePose, "RobotLift")
  , _currentHeadAngle(MIN_HEAD_ANGLE)
  , _gyroDriftReported(false)
  , _driftCheckStartPoseFrameId(0)
  , _driftCheckStartAngle_rad(0)
  , _driftCheckStartGyroZ_rad_per_sec(0)
  , _driftCheckStartTime_ms(0)
  , _driftCheckCumSumGyroZ_rad_per_sec(0)
  , _driftCheckMinGyroZ_rad_per_sec(0)
  , _driftCheckMaxGyroZ_rad_per_sec(0)
  , _driftCheckNumReadings(0)
  , _poseHistory(nullptr)
  , _moodManager(new MoodManager(this))
  , _progressionUnlockComponent(new ProgressionUnlockComponent(*this))
  , _speedChooser(new SpeedChooser(*this))
  , _blockFilter(new BlockFilter(this, context->GetExternalInterface()))
  , _tapFilterComponent(new BlockTapFilterComponent(*this))
  , _lastDiconnectedCheckTime(0)
  , _robotToEngineImplMessaging(new RobotToEngineImplMessaging(this))
  , _robotIdleTimeoutComponent(new RobotIdleTimeoutComponent(*this))
{
  _poseHistory = new RobotPoseHistory();
  PRINT_NAMED_INFO("Robot.Robot", "Created");
      
  _pose.SetName("Robot_" + std::to_string(_ID));
  _driveCenterPose.SetName("RobotDriveCenter_" + std::to_string(_ID));
      
  // Initializes _pose, _poseOrigins, and _worldOrigin:
  Delocalize(false);
      
  // Delocalize will mark isLocalized as false, but we are going to consider
  // the robot localized (by odometry alone) to start, until he gets picked up.
  _isLocalized = true;
  SetLocalizedTo(nullptr);

  _robotToEngineImplMessaging->InitRobotMessageComponent(_context->GetRobotManager()->GetMsgHandler(),robotID, this);
  
  
  // The call to Delocalize() will increment frameID, but we want it to be
  // initialzied to 0, to match the physical robot's initialization
  _frameId = 0;
      
  _lastDebugStringHash = 0;
      
  // Read in Mood Manager Json
  if (nullptr != _context->GetDataPlatform())
  {
    _moodManager->Init(_context->GetDataLoader()->GetRobotMoodConfig());
    LoadEmotionEvents();
  }

  // Initialize progression
  if (nullptr != _context->GetDataPlatform())
  {
    Json::Value progressionUnlockConfig;
    std::string jsonFilename = "config/basestation/config/unlock_config.json";
    bool success = _context->GetDataPlatform()->readAsJson(Util::Data::Scope::Resources,
                                                           jsonFilename,
                                                           progressionUnlockConfig);
    if (!success)
    {
      PRINT_NAMED_ERROR("Robot.UnlockConfigJsonNotFound",
                        "Unlock Json config file %s not found.",
                        jsonFilename.c_str());
    }
        
    _progressionUnlockComponent->Init(progressionUnlockConfig);
  }
  else {
    Json::Value empty;
    _progressionUnlockComponent->Init(empty);
  }
      
  // load available behaviors into the behavior factory
  LoadBehaviors();

  _behaviorMgr->InitConfiguration(_context->GetDataLoader()->GetRobotBehaviorConfig());
  
  // Setting camera pose according to current head angle.
  // (Not using SetHeadAngle() because _isHeadCalibrated is initially false making the function do nothing.)
  _visionComponentPtr->GetCamera().SetPose(GetCameraPose(_currentHeadAngle));
  
  _pdo = new PathDolerOuter(_context->GetRobotManager()->GetMsgHandler(), robotID);

  if (nullptr != _context->GetDataPlatform()) {
    _longPathPlanner  = new LatticePlanner(this, _context->GetDataPlatform());
  }
  else {
    // For unit tests, or cases where we don't have data, use the short planner in it's place
    PRINT_NAMED_WARNING("Robot.NoDataPlatform.WrongPlanner",
                        "Using short planner as the long planner, since we dont have a data platform");
    _longPathPlanner = new FaceAndApproachPlanner;
  }

  _shortPathPlanner = new FaceAndApproachPlanner;
  _shortMinAnglePathPlanner = new MinimalAnglePlanner;
  _selectedPathPlanner = _longPathPlanner;
  _fallbackPathPlanner = nullptr;
      
  if (nullptr != _context->GetDataPlatform())
  {
    _visionComponentPtr->Init(_context->GetDataLoader()->GetRobotVisionConfig());
  }
      
  // Read all neccessary data off the robot and back it up
  // Potentially duplicates some reads like FaceAlbumData
  _nvStorageComponent.GetRobotDataBackupManager().ReadAllBackupDataFromRobot();
      
} // Constructor: Robot
    
Robot::~Robot()
{
  AbortAll();
  
  // This needs to happen before ActionList is destroyed, because otherwise behaviors will try to respond
  // to actions shutting down
  _behaviorMgr.reset();
  
  // Destroy our actionList before things like the path planner, since actions often rely on those
  // things existing
  Util::SafeDelete(_actionList);
      
  // destroy vision component first because its thread might be using things from Robot. This fixes a crash
  // caused by the vision thread using _poseHistory when it was destroyed here
  Util::SafeDelete(_visionComponentPtr);
  
  Util::SafeDelete(_poseHistory);
  Util::SafeDelete(_pdo);
  Util::SafeDelete(_longPathPlanner);
  Util::SafeDelete(_shortPathPlanner);
  Util::SafeDelete(_shortMinAnglePathPlanner);
  Util::SafeDelete(_moodManager);
  Util::SafeDelete(_progressionUnlockComponent);
  Util::SafeDelete(_tapFilterComponent);
  Util::SafeDelete(_blockFilter);
  Util::SafeDelete(_drivingAnimationHandler);
  Util::SafeDelete(_speedChooser);

  _selectedPathPlanner = nullptr;
  _fallbackPathPlanner = nullptr;
      
}
    
void Robot::SetOnCharger(bool onCharger)
{
  ObservableObject* object = GetBlockWorld().GetObjectByID(_chargerID, ObjectFamily::Charger);
  Charger* charger = dynamic_cast<Charger*>(object);
  if (onCharger && !_isOnCharger)
  {
    // If we don't actually have a charger, add an unconnected one now
    if (nullptr == charger)
    {
      ObjectID newObjID = AddUnconnectedCharger();
      charger = dynamic_cast<Charger*>(GetBlockWorld().GetObjectByID(newObjID));
      if(nullptr == charger)
      {
        PRINT_NAMED_ERROR("Robot.SetOnCharger.FailedToAddUnconnectedCharger", "NewID=%d",
                          newObjID.GetValue());
      }
    }
          
    PRINT_NAMED_EVENT("robot.on_charger", "");
    Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ChargerEvent(true)));
        
  }
  else if (!onCharger && _isOnCharger)
  {
    PRINT_NAMED_EVENT("robot.off_charger", "");
    Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::ChargerEvent(false)));
  }
      
  if( onCharger && nullptr != charger )
  {
    charger->SetPoseRelativeToRobot(*this);
  }
      
  _isOnCharger = onCharger;
}
    
ObjectID Robot::AddUnconnectedCharger()
{
  // ChargerId is unknown because it exists in a previous frame but not this one

  BlockWorldFilter filter;
  filter.SetOriginMode(BlockWorldFilter::OriginMode::InAnyFrame);
  filter.SetFilterFcn(nullptr);
  filter.AddAllowedType(ObjectType::Charger_Basic);
  
  std::vector<ObservableObject *> matchingObjects;
  GetBlockWorld().FindMatchingObjects(filter, matchingObjects);
  
  ObservableObject* obj = nullptr;
  for(auto object : matchingObjects)
  {
    if(obj != nullptr)
    {
      ASSERT_NAMED(object->GetID() == obj->GetID(), "Matching charger ids not equal");
    }
    obj = object;
  }
  ObjectID objID;
  s32 activeId = -1;
  if(obj != nullptr)
  {
    activeId = obj->GetActiveID();
  }
  // Copying existing object's activeId and objectId if an existing object was found
  objID = GetBlockWorld().AddActiveObject(activeId, 0, ActiveObjectType::OBJECT_CHARGER, obj);
  
  SetCharger(objID);
  
  return _chargerID;
}

    
bool Robot::CheckAndUpdateTreadsState(const RobotState& msg)
{
  if (!IsHeadCalibrated()) {
    return false;
  }
  
  const bool isOfftreads = IS_STATUS_FLAG_SET(IS_PICKED_UP);
  const bool isFalling = IS_STATUS_FLAG_SET(IS_FALLING);
  TimeStamp_t currentTimestamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  //////////
  // Check the robot's orientation
  //////////
  
  //// COZMO_UP_RIGHT
  const bool currOntreads = std::abs(GetPitchAngle().ToDouble() - kPitchAngleOntreads_rads) <= kPitchAngleOntreadsTolerence_rads;
  
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
  if(_awaitingConfirmationTreadState == OffTreadsState::OnTreads && isOfftreads == true) {
    // Robot is being picked up from not being picked up, notify systems
    _awaitingConfirmationTreadState = OffTreadsState::InAir;
    // Allows this to be called instantly
    _timeOffTreadStateChanged_ms = currentTimestamp - kRobotTimeToConsiderOfftreads_ms;
  }
  
  // Transition from inAir to Ontreads
  // there is a delay for the cliff sensor to confirm the robot is no longer picked up
  if (_awaitingConfirmationTreadState != OffTreadsState::OnTreads && isOfftreads != true
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
      _visionComponentPtr->Pause(true);
    }
    
    // Falling seems worthy of a DAS event
    if (_awaitingConfirmationTreadState == OffTreadsState::Falling) {
      _fallingStartedTime_ms = GetLastMsgTimestamp();
      PRINT_NAMED_EVENT("Robot.CheckAndUpdateTreadsState.FallingStarted",
                        "t=%dms",
                        _fallingStartedTime_ms);
      
      // Stop all actions
      GetActionList().Cancel();
      
    } else if (_offTreadsState == OffTreadsState::Falling) {
      // This is not an exact measurement of fall time since it includes some detection delays on the robot side
      // It may also include kRobotTimeToConsiderOfftreads_ms depending on how the robot lands
      PRINT_NAMED_EVENT("Robot.CheckAndUpdateTreadsState.FallingStopped",
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
        _visionComponentPtr->Pause(false);
      }
      
      ASSERT_NAMED(!IsLocalized(), "Robot should be delocalized when first put back down!");
      
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
    if(_isOnChargerPlatform && _offTreadsState != OffTreadsState::OnTreads)
    {
      _isOnChargerPlatform = false;
      Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotOnChargerPlatformEvent(_isOnChargerPlatform)));
    }
    
    return true;
  }
  
  return false;
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
      
  // Add a new pose origin to use until the robot gets localized again
  const Pose3d* oldOrigin = _worldOrigin;
  _worldOrigin = new Pose3d();
  PoseOriginID_t originID = _poseOriginList.AddOrigin(_worldOrigin);
  _worldOrigin->SetName("Robot" + std::to_string(_ID) + "_PoseOrigin" + std::to_string(originID));
  
  // Log delocalization, new origin name, and num origins to DAS
  PRINT_NAMED_EVENT("Robot.Delocalize", "Delocalizing robot %d. New origin: %s. NumOrigins=%zu",
                    GetID(), _worldOrigin->GetName().c_str(), _poseOriginList.GetSize());
  
  _pose.SetRotation(0, Z_AXIS_3D());
  _pose.SetTranslation({0.f, 0.f, 0.f});
  _pose.SetParent(_worldOrigin);
      
  _driveCenterPose.SetRotation(0, Z_AXIS_3D());
  _driveCenterPose.SetTranslation({0.f, 0.f, 0.f});
  _driveCenterPose.SetParent(_worldOrigin);
  
  // Create a new pose frame so that we can't get pose history entries with the same pose
  // frame that have different origins (Not 100% sure this is totally necessary but seems
  // like the cleaner / safer thing to do.)
  AddVisionOnlyPoseToHistory(GetLastMsgTimestamp(), _pose, GetHeadAngle(), GetLiftAngle());
  AddRawOdomPoseToHistory(GetLastMsgTimestamp(),GetPoseFrameID(), _pose, GetHeadAngle(), GetLiftAngle(), isCarryingObject);
  
  if(_timeSynced)
  {
    // Need to update the robot's pose history with our new origin and pose frame IDs
    PRINT_NAMED_INFO("Robot.Delocalize.SendingNewOriginID",
                     "Sending new localization update at t=%u, with pose frame %u and origin ID=%u",
                     GetLastMsgTimestamp(),
                     GetPoseFrameID(),
                     _poseOriginList.GetOriginID(_worldOrigin));
    SendAbsLocalizationUpdate(_pose, GetLastMsgTimestamp(), GetPoseFrameID());
  }
  
  // Update VizText
  GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: <nothing>");
  GetContext()->GetVizManager()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOriginList.GetSize(),
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

  // delete objects that have become useless since we delocalized last time
  _blockWorld->DeleteObjectsFromZombieOrigins();
  
  // create a new memory map for this origin
  _blockWorld->CreateLocalizedMemoryMap(_worldOrigin);
  
  // deselect blockworld's selected object, if it has one
  _blockWorld->DeselectCurrentObject();
      
  // notify behavior whiteboard
  _behaviorMgr->GetWhiteboard().OnRobotDelocalized();
  
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
      if(false == marker.GetPose().GetWithRespectTo(_visionComponentPtr->GetCamera().GetPose(), markerPoseWrtCamera)) {
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
  _behaviorMgr->GetWhiteboard().OnRobotRelocalized();
  
  // Update VizText
  GetContext()->GetVizManager()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: %s_%d",
                                         ObjectTypeToString(object->GetType()), _localizedToID.GetValue());
  GetContext()->GetVizManager()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOriginList.GetSize(),
                                         _worldOrigin->GetName().c_str());
      
  return RESULT_OK;
      
} // SetLocalizedTo()
    

void Robot::DetectGyroDrift(const RobotState& msg)
{
  f32 gyroZ = msg.gyro.z;
  
  if (!_gyroDriftReported) {
    
    // Reset gyro drift detector if
    // 1) Wheels are moving
    // 2) Raw gyro reading exceeds possible drift value
    // 3) Cliff is detected
    // 4) Head isn't calibrated
    // 5) Drift detector started but the raw gyro reading deviated too much from starting values, indicating motion.
    if (GetMoveComponent().IsMoving() ||
        (std::fabsf(gyroZ) > kDriftCheckMaxRate_rad_per_sec) ||
        IsCliffDetected() ||
        !_isHeadCalibrated ||
        
        ((_driftCheckStartTime_ms != 0) &&
         ((std::fabsf(_driftCheckStartGyroZ_rad_per_sec - gyroZ) > kDriftCheckGyroZMotionThresh_rad_per_sec) ||
          (_driftCheckStartPoseFrameId != GetPoseFrameID())))
        
        ) {
      _driftCheckStartTime_ms = 0;
    }
    
    // Robot's not moving. Initialize drift detection.
    else if (_driftCheckStartTime_ms == 0) {
      _driftCheckStartPoseFrameId        = GetPoseFrameID();
      _driftCheckStartAngle_rad          = GetPose().GetRotation().GetAngleAroundZaxis();
      _driftCheckStartGyroZ_rad_per_sec  = gyroZ;
      _driftCheckStartTime_ms            = msg.timestamp;
      _driftCheckCumSumGyroZ_rad_per_sec = gyroZ;
      _driftCheckMinGyroZ_rad_per_sec    = gyroZ;
      _driftCheckMaxGyroZ_rad_per_sec    = gyroZ;
      _driftCheckNumReadings             = 1;
    }
    
    // If gyro readings have been accumulating for long enough...
    else if (msg.timestamp - _driftCheckStartTime_ms > kDriftCheckPeriod_ms) {
      
      // ...check if there was a sufficient change in heading angle or pitch. Otherwise, reset detector.
      const f32 headingAngleChange = std::fabsf((_driftCheckStartAngle_rad - GetPose().GetRotation().GetAngleAroundZaxis()).ToFloat());
      const f32 angleChangeThresh = kDriftCheckMaxAngleChangeRate_rad_per_sec * MILLIS_TO_SEC(kDriftCheckPeriod_ms);
      
      if (headingAngleChange > angleChangeThresh) {
        // Report drift detected just one time during a session
        Util::sWarningF("robot.detect_gyro_drift.drift_detected",
                        {{DDATA, TO_DDATA_STR(RAD_TO_DEG_F32(headingAngleChange))}},
                        "mean: %f, min: %f, max: %f",
                        RAD_TO_DEG_F32(_driftCheckCumSumGyroZ_rad_per_sec / _driftCheckNumReadings),
                        RAD_TO_DEG_F32(_driftCheckMinGyroZ_rad_per_sec),
                        RAD_TO_DEG_F32(_driftCheckMaxGyroZ_rad_per_sec));
        _gyroDriftReported = true;
      }

      _driftCheckStartTime_ms = 0;
    }
    
    // Record min and max observed gyro readings and cumulative sum for later mean computation
    else {
      if (gyroZ > _driftCheckMaxGyroZ_rad_per_sec) {
        _driftCheckMaxGyroZ_rad_per_sec = gyroZ;
      }
      if (gyroZ < _driftCheckMinGyroZ_rad_per_sec) {
        _driftCheckMinGyroZ_rad_per_sec = gyroZ;
      }
      _driftCheckCumSumGyroZ_rad_per_sec += gyroZ;
      ++_driftCheckNumReadings;
    }
  }
}
  
  
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
      
  // Get ID of last/current path that the robot executed
  SetLastRecvdPathID(msg.lastPathID);
      
  // Raw cliff data
  _cliffDataRaw = msg.cliffDataRaw;
  
  // Update other state vars
  SetCurrPathSegment( msg.currPathSegment );
  SetNumFreeSegmentSlots(msg.numFreeSegmentSlots);
      
  // Dole out more path segments to the physical robot if needed:
  if (IsTraversingPath() && GetLastRecvdPathID() == GetLastSentPathID()) {
    _pdo->Update(_currPathSegment, _numFreeSegmentSlots);
  }
  
  // Update IMU data
  _robotAccel = msg.accel;
  _robotGyro = msg.gyro;
  
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
  SetOnCharger(IS_STATUS_FLAG_SET(IS_ON_CHARGER));
  SetIsCharging(IS_STATUS_FLAG_SET(IS_CHARGING));
  _isCliffSensorOn = IS_STATUS_FLAG_SET(CLIFF_DETECTED);
  _chargerOOS = IS_STATUS_FLAG_SET(IS_CHARGER_OOS);
  _isBodyInAccessoryMode = IS_STATUS_FLAG_SET(IS_BODY_ACC_MODE);

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
  else if(msg.pose_frame_id >= GetPoseFrameID()) // ignore state messages with old frame ID
  {
    _numMismatchedFrameIDs = 0;
    
    Pose3d newPose;
        
    if(IsOnRamp()) {
          
      // Sanity check:
      CORETECH_ASSERT(_rampID.IsSet());
          
      // Don't update pose history while on a ramp.
      // Instead, just compute how far the robot thinks it has gone (in the plane)
      // and compare that to where it was when it started traversing the ramp.
      // Adjust according to the angle of the ramp we know it's on.
          
      const f32 distanceTraveled = (Point2f(msg.pose.x, msg.pose.y) - _rampStartPosition).Length();
          
      Ramp* ramp = dynamic_cast<Ramp*>(_blockWorld->GetObjectByID(_rampID, ObjectFamily::Ramp));
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.NoRampWithID",
                          "Updating robot %d's state while on a ramp, but Ramp object with ID=%d not found in the world.",
                          _ID, _rampID.GetValue());
        return RESULT_FAIL;
      }
          
      // Progress must be along ramp's direction (init assuming ascent)
      Radians headingAngle = ramp->GetPose().GetRotationAngle<'Z'>();
          
      // Initialize tilt angle assuming we are ascending
      Radians tiltAngle = ramp->GetAngle();
          
      switch(_rampDirection)
      {
        case Ramp::DESCENDING:
          tiltAngle    *= -1.f;
          headingAngle += M_PI;
          break;
        case Ramp::ASCENDING:
          break;
              
        default:
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.UnexpectedRampDirection",
                            "Robot is on a ramp, expecting the ramp direction to be either "
                            "ASCEND or DESCENDING, not %d", _rampDirection);
          return RESULT_FAIL;
      }

      const f32 heightAdjust = distanceTraveled*sin(tiltAngle.ToFloat());
      const Point3f newTranslation(_rampStartPosition.x() + distanceTraveled*cos(headingAngle.ToFloat()),
                                   _rampStartPosition.y() + distanceTraveled*sin(headingAngle.ToFloat()),
                                   _rampStartHeight + heightAdjust);
          
      const RotationMatrix3d R_heading(headingAngle, Z_AXIS_3D());
      const RotationMatrix3d R_tilt(tiltAngle, Y_AXIS_3D());
          
      newPose = Pose3d(R_tilt*R_heading, newTranslation, _pose.GetParent());
      //SetPose(newPose); // Done by UpdateCurrPoseFromHistory() below
          
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
        RobotPoseStamp p;
        lastResult = _poseHistory->GetLastPoseWithFrameID(msg.pose_frame_id, p);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.GetLastPoseWithFrameIdError",
                            "Failed to get last pose from history with frame ID=%d",
                            msg.pose_frame_id);
          return lastResult;
        }
        pose_z = p.GetPose().GetWithRespectToOrigin().GetTranslation().z();
      }
      
      newPose.SetTranslation({newPose.GetTranslation().x(), newPose.GetTranslation().y(), pose_z});
      
    } // if/else on ramp
    
    // Add to history
    lastResult = AddRawOdomPoseToHistory(msg.timestamp,
                                         msg.pose_frame_id,
                                         newPose,
                                         msg.headAngle,
                                         msg.liftAngle,
                                         isCarryingObject);
    
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_WARNING("Robot.UpdateFullRobotState.AddPoseError",
                          "AddRawOdomPoseToHistory failed for timestamp=%d", msg.timestamp);
      return lastResult;
    }
    
    if(UpdateCurrPoseFromHistory() == false) {
      lastResult = RESULT_FAIL;
    }
  }
  else // Robot frameID is less than engine frameID
  {
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
  
  DetectGyroDrift(msg);
  
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

  const float imageFrameRate = 1000.0f / _visionComponentPtr->GetFramePeriod_ms();
  const float imageProcRate = 1000.0f / _visionComponentPtr->GetProcessingPeriod_ms();
            
  // Send state to visualizer for displaying
  GetContext()->GetVizManager()->SendRobotState(
    stateMsg,
    static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (_numAnimationBytesStreamed - _numAnimationBytesPlayed),
    AnimationStreamer::NUM_AUDIO_FRAMES_LEAD-(_numAnimationAudioFramesStreamed - _numAnimationAudioFramesPlayed),
    (u8)MIN(((u8)imageFrameRate), u8_MAX),
    (u8)MIN(((u8)imageProcRate), u8_MAX),
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
  RobotPoseStamp p;
  TimeStamp_t t;
  Result result = _poseHistory->GetRawPoseAt(t_request, t, p);
  if(RESULT_OK != result)
  {
    return result;
  }
  
  camera = GetHistoricalCamera(p, t);
  return RESULT_OK;
}
    
Pose3d Robot::GetHistoricalCameraPose(const RobotPoseStamp& histPoseStamp, TimeStamp_t t) const
{
  // Compute pose from robot body to camera
  // Start with canonical (untilted) headPose
  Pose3d camPose(_headCamPose);
      
  // Rotate that by the given angle
  RotationVector3d Rvec(-histPoseStamp.GetHeadAngle(), Y_AXIS_3D());
  camPose.RotateBy(Rvec);
      
  // Precompose with robot body to neck pose
  camPose.PreComposeWith(_neckPose);
      
  // Set parent pose to be the historical robot pose
  camPose.SetParent(&(histPoseStamp.GetPose()));
      
  camPose.SetName("PoseHistoryCamera_" + std::to_string(t));
      
  return camPose;
}
    
Vision::Camera Robot::GetHistoricalCamera(const RobotPoseStamp& p, TimeStamp_t t) const
{
  Vision::Camera camera(_visionComponentPtr->GetCamera());
      
  // Update the head camera's pose
  camera.SetPose(GetHistoricalCameraPose(p, t));
      
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
  
  const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  _robotIdleTimeoutComponent->Update(currentTime);
  
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
     const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
     static double lastUpdateTime = currentTime_sec;
       
     const double updateTimeDiff = currentTime_sec - lastUpdateTime;
     if(updateTimeDiff > 1.0) {
     PRINT_NAMED_WARNING("Robot.Update", "Gap between robot update calls = %f\n", updateTimeDiff);
     }
     lastUpdateTime = currentTime_sec;
  */
      
      
  if(_visionComponentPtr->GetCamera().IsCalibrated())
  {
    // NOTE: Also updates BlockWorld and FaceWorld using markers/faces that were detected
    Result visionResult = _visionComponentPtr->UpdateAllResults();
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
  
  
  
  // Update ChargerPlatform - this has to happen before the behaviors which might need this information
  ObservableObject* charger = GetBlockWorld().GetObjectByID(_chargerID, ObjectFamily::Charger);
  if( nullptr != charger && charger->IsPoseStateKnown() && _offTreadsState == OffTreadsState::OnTreads)
  {
    // This state is useful for knowing not to play a cliff react when just driving off the charger.
    bool isOnChargerPlatform = charger->GetBoundingQuadXY().Intersects(GetBoundingQuadXY());
    if( isOnChargerPlatform != _isOnChargerPlatform)
    {
      _isOnChargerPlatform = isOnChargerPlatform;
      Broadcast(
         ExternalInterface::MessageEngineToGame(
           ExternalInterface::RobotOnChargerPlatformEvent(_isOnChargerPlatform))
                );
    }
  }
  
      
  ///////// Update the behavior manager ///////////
      
  // TODO: This object encompasses, for the time-being, what some higher level
  // module(s) would do.  e.g. Some combination of game state, build planner,
  // personality planner, etc.
      
  _moodManager->Update(currentTime);
      
  _progressionUnlockComponent->Update();
  
  _tapFilterComponent->Update();
  
  // information analyzer should run before behaviors so that they can feed off its findings
  _aiInformationAnalyzer->Update(*this);
      
  const char* behaviorChooserName = "";
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
      behaviorDebugStr += behavior->GetName();
      const std::string& stateName = behavior->GetDebugStateName();
      if (!stateName.empty())
      {
        behaviorDebugStr += "-" + stateName;
      }
    }
        
    const IBehaviorChooser* behaviorChooser = _behaviorMgr->GetBehaviorChooser();
    if (behaviorChooser)
    {
      behaviorChooserName = behaviorChooser->GetName();
    }
  } else {
    --ticksToPreventBehaviorManagerFromRotatingTooEarly_Jira_1242;
  }
      
  GetContext()->GetVizManager()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::MAGENTA,
                                         "%s", behaviorDebugStr.c_str());
  
  GetContext()->SetSdkStatus(SdkStatusType::Behavior,
                                 std::string(behaviorChooserName) + std::string(":") + behaviorDebugStr);
  
  //////// Update Robot's State Machine /////////////
  Result actionResult = _actionList->Update();
  if(actionResult != RESULT_OK) {
    PRINT_NAMED_INFO("Robot.Update", "Robot %d had an action fail.", GetID());
  }        
  //////// Stream Animations /////////
  if(_timeSynced) { // Don't stream anything before we've connected
    Result animStreamResult = _animationStreamer.Update(*this);
    if(animStreamResult != RESULT_OK) {
      PRINT_NAMED_WARNING("Robot.Update",
                          "Robot %d had an animation streaming failure.", GetID());
    }
    
    // NEW Animations!
    if (BUILD_NEW_ANIMATION_CODE) {
      Result result = _animationController->Update(*this);
      if (result != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.Update",
                            "Robot %d had an animation streaming failure.", GetID());
      }
    }
    
  }

  /////////// Update NVStorage //////////
  _nvStorageComponent.Update();

  /////////// Update path planning / following ////////////


  if( _driveToPoseStatus != ERobotDriveToPoseStatus::Waiting ) {

    bool forceReplan = _driveToPoseStatus == ERobotDriveToPoseStatus::Error;

    if( _numPlansFinished == _numPlansStarted ) {
      // nothing to do with the planners, so just update the status based on the path following
      if( IsTraversingPath() ) {
        _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;

        // haveOriginsChanged: note that we check if the parent of the currentPlannerGoal is different than the world
        // origin. With origin rejiggering it's possible that our origin is the current world origin, as long as we
        // could rejigger our parent to the new origin. If we could not rejigger, then our parent is also not the
        // origin, but moreover we can't get the goal with respect to the new origin
        const bool haveOriginsChanged = (_currentPlannerGoal.GetParent() != _worldOrigin);
        const bool canAdjustOrigin = _currentPlannerGoal.GetWithRespectTo(*_worldOrigin, _currentPlannerGoal);
        if ( haveOriginsChanged && !canAdjustOrigin )
        {
          // the origins changed and we can't rejigger our goal to the new origin (we probably delocalized),
          // completely abort
          _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
          AbortDrivingToPose();
          PRINT_NAMED_INFO("Robot.Update.Replan.NotPossible", "Our goal is in another coordinate frame that we can't get wrt current, we can't replan");
        }
        else
        {
          // check if we need adjusting origins
          if (haveOriginsChanged && canAdjustOrigin)
          {
            // our origin changed, but we were able to recompute _currentPlannerGoal wrt current origin. Abort the current
            // plan and start driving to the updated _currentPlannerGoal. Note this can discard other goals for multiple goal requests,
            // but it's likely that the closest goal is still the closest one, and this is a faster fix (rsam 2016)
            AbortDrivingToPose();
            PRINT_NAMED_INFO("Robot.Update.Replan.RejiggeringPlanner", "Our goal is in another coordinate frame, but we are updating to current frame");
            const Result planningResult = StartDrivingToPose( _currentPlannerGoal, _pathMotionProfile );
            if ( planningResult != RESULT_OK ) {
              PRINT_NAMED_WARNING("Robot.Update.Replan.RejiggeringPlanner", "We could not start driving to rejiggered pose.");
              // We failed to replan, abort
              AbortDrivingToPose();
              _driveToPoseStatus = ERobotDriveToPoseStatus::Error; // reset in case StartDriving didn't set it
            }
          }
          else if( GetBlockWorld().DidObjectsChange() || forceReplan )
          {
            // we didn't need to adjust origins
            // see if we need to replan, but only bother checking if the world objects changed
            _fallbackShouldForceReplan = forceReplan;
            RestartPlannerIfNeeded(forceReplan);
          }
        }
      }
      else {
        _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
      }
    }
    else {
      // we are waiting on a plan to currently compute
      // TODO:(bn) timeout logic might fit well here?
      switch( _selectedPathPlanner->CheckPlanningStatus() ) {
        case EPlannerStatus::Error:
          if( nullptr != _fallbackPathPlanner ) {
            _selectedPathPlanner = _fallbackPathPlanner;
            _fallbackPathPlanner = nullptr;
            _numPlansFinished = _numPlansStarted;
            PRINT_NAMED_INFO("Robot.Update.Planner.Error",
                             "Running planner returned error status, using fallback planner instead");
            if( !StartPlanner() ) {
              _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
              AbortDrivingToPose();
              _numPlansFinished = _numPlansStarted;
            } else {
              if( IsTraversingPath() ) {
                _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
              }
              else {
                _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
              }
              _numPlansStarted++;
            }
          } else {
            _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
            PRINT_NAMED_INFO("Robot.Update.Planner.Error", "Running planner returned error status");
            AbortDrivingToPose();
            _numPlansFinished = _numPlansStarted;
          }
          break;

        case EPlannerStatus::Running:
          // status should stay the same, but double check it
          if( _driveToPoseStatus != ERobotDriveToPoseStatus::ComputingPath &&
              _driveToPoseStatus != ERobotDriveToPoseStatus::Replanning) {
            PRINT_NAMED_WARNING("Robot.Planning.StatusError.Running",
                                "Status was invalid, setting to ComputePath");
            _driveToPoseStatus =  ERobotDriveToPoseStatus::ComputingPath;
          }
          break;

        case EPlannerStatus::CompleteWithPlan: {
          // get the path
          Planning::GoalID selectedPoseIdx;
          Planning::Path newPath;

          _selectedPathPlanner->GetCompletePath(GetDriveCenterPose(), newPath, selectedPoseIdx, &_pathMotionProfile);
          
          // check for collisions
          bool collisionsAcceptable = _selectedPathPlanner->ChecksForCollisions()
                                      || (newPath.GetNumSegments()==0);
          // Some children of IPathPlanner may return a path that hasn't been checked for obstacles.
          // Here, check if the planner used to compute that path considers obstacles. If it doesnt,
          // check for an obstacle penalty. If there is one, re-run with the lattice planner,
          // which considers obstacles in its search.
          if( (!collisionsAcceptable) && (nullptr != _longPathPlanner) ) {
            const float startPoseAngle = GetPose().GetRotationAngle<'Z'>().ToFloat();
            const bool __attribute__((unused)) obstaclesLoaded = _longPathPlanner->PreloadObstacles();
            ASSERT_NAMED(obstaclesLoaded, "Lattice planner didnt preload obstacles.");
            if( !_longPathPlanner->CheckIsPathSafe(newPath, startPoseAngle) ) {
              // bad path. try with the fallback planner if possible
              if( nullptr != _fallbackPathPlanner ) {
                _selectedPathPlanner = _fallbackPathPlanner;
                _fallbackPathPlanner = nullptr;
                _numPlansFinished = _numPlansStarted;
                PRINT_NAMED_INFO("Robot.Update.Planner.Collisions",
                                 "Planner returned a path with obstacles, using fallback planner instead");
                if( !StartPlanner() ) {
                  _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
                  AbortDrivingToPose();
                  _numPlansFinished = _numPlansStarted;
                } else {
                   if( IsTraversingPath() ) {
                     _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
                   }
                   else {
                     _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
                   }
                  _numPlansStarted++;
                }
              } else {
                // we only have a path with collisions, abort
                PRINT_NAMED_INFO("Robot.Update.Planner.CompleteNoCollisionFreePlan", "A plan was found, but it contains collisions");
                _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
                _numPlansFinished = _numPlansStarted;
              }
            } else {
              collisionsAcceptable = true;
            }
          }
          
          if( collisionsAcceptable ) {
            _numPlansFinished = _numPlansStarted;
            if( newPath.GetNumSegments()==0 ) {
              _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
              PRINT_NAMED_INFO("Robot.Update.Planner.CompleteWithPlan.EmptyPlan", "Planner completed but with an empty plan");
            } else {
              _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
              PRINT_NAMED_INFO("Robot.Update.Planner.CompleteWithPlan", "Running planner complete with a plan");
            
              ExecutePath(newPath, _usingManualPathSpeed);

              if( _plannerSelectedPoseIndexPtr != nullptr ) {
                // When someone called StartDrivingToPose with multiple possible poses, they had an option to pass
                // in a pointer to be set when we know which pose was selected by the planner. If that pointer is
                // non-null, set it now, then clear the pointer so we won't set it again

                // TODO:(bn) think about re-planning, here, what if replanning wanted to switch targets? For now,
                // replanning will always chose the same target pose, which should be OK for now
                *_plannerSelectedPoseIndexPtr = selectedPoseIdx;
                _plannerSelectedPoseIndexPtr = nullptr;
              }
            }
          }
          break;
        }


        case EPlannerStatus::CompleteNoPlan:
          PRINT_NAMED_INFO("Robot.Update.Planner.CompleteNoPlan", "Running planner complete with no plan");
          _driveToPoseStatus = ERobotDriveToPoseStatus::Waiting;
          _numPlansFinished = _numPlansStarted;
          break;
      }
    }
  }
      
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

  const float imageProcRate = 1000.0f / _visionComponentPtr->GetProcessingPeriod_ms();
      
  // So we can have an arbitrary number of data here that is likely to change want just hash it all
  // together if anything changes without spamming
  snprintf(buffer, sizeof(buffer),
           "%c%c%c%c %2dHz %s%s ",
           GetMoveComponent().IsLiftMoving() ? 'L' : ' ',
           GetMoveComponent().IsHeadMoving() ? 'H' : ' ',
           GetMoveComponent().IsMoving() ? 'B' : ' ',
           IsCarryingObject() ? 'C' : ' ',
           // SimpleMoodTypeToString(GetMoodManager().GetSimpleMood()),
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::LIFT_TRACK) ? 'L' : ' ',
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::HEAD_TRACK) ? 'H' : ' ',
           // _movementComponent.AreAnyTracksLocked((u8)AnimTrackFlag::BODY_TRACK) ? 'B' : ' ',
           (u8)MIN(((u8)imageProcRate), u8_MAX),
           behaviorChooserName,
           behaviorDebugStr.c_str());
      
  std::hash<std::string> hasher;
  size_t curr_hash = hasher(std::string(buffer));
  if( _lastDebugStringHash != curr_hash )
  {
    SendDebugString(buffer);
    _lastDebugStringHash = curr_hash;
  }

  _lightsComponent->Update();


  if( kDebugPossibleBlockInteraction ) {
    // print a bunch of info helpful for debugging block states
    BlockWorldFilter filter;
    filter.SetAllowedFamilies({ObjectFamily::LightCube});
    std::vector<ObservableObject*> matchingObjects;
    GetBlockWorld().FindMatchingObjects(filter, matchingObjects);
    for( const auto obj : matchingObjects ) {
        const ObservableObject* topObj = GetBlockWorld().FindObjectOnTopOf(*obj, STACKED_HEIGHT_TOL_MM);
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
                          obj->PoseStateToString( obj->GetPoseState() ),
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

    
void Robot::SetNewPose(const Pose3d& newPose)
{
  SetPose(newPose.GetWithRespectToOrigin());
  ++_frameId;
  
  // Note: using last message timestamp instead of newest timestamp in history
  //  because it's possible we did not put the last-received state message into
  //  history (if it had old frame ID), but we still want the latest time we
  //  can get.
  const TimeStamp_t timeStamp = GetLastMsgTimestamp();
  
  Result addResult = AddRawOdomPoseToHistory(timeStamp, _frameId, _pose, GetHeadAngle(), GetLiftAngle(), IsCarryingObject());
  if(RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("Robot.SetNewPose.AddRawOdomPoseFailed",
                      "t=%d FrameID=%d", timeStamp, _frameId);
    return;
  }
    
  SendAbsLocalizationUpdate(_pose, timeStamp, _frameId);
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
                          angle, RAD_TO_DEG_F32(angle));
    }
        
    _visionComponentPtr->GetCamera().SetPose(GetCameraPose(_currentHeadAngle));
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
  
bool Robot::IsHeadCalibrated()
{
  return _isHeadCalibrated;
}

bool Robot::IsLiftCalibrated()
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

  CORETECH_ASSERT(_liftPose.GetParent() == &_liftBasePose);
}
    
Radians Robot::GetPitchAngle() const
{
  return _pitchAngle;
}
        
void Robot::SelectPlanner(const Pose3d& targetPose)
{
  // set our current planner goal to the given pose flattened out, so that we can later compare if the origin
  // has changed since we started planning towards this pose. Also because the planner doesn't know about
  // pose origins, so this pose should be wrt origin always
  _currentPlannerGoal = targetPose.GetWithRespectToOrigin();

  Pose2d target2d(_currentPlannerGoal);
  Pose2d start2d(GetPose().GetWithRespectToOrigin());

  float distSquared = pow(target2d.GetX() - start2d.GetX(), 2) + pow(target2d.GetY() - start2d.GetY(), 2);

  if(distSquared < MAX_DISTANCE_FOR_SHORT_PLANNER * MAX_DISTANCE_FOR_SHORT_PLANNER) {

    Radians finalAngleDelta = _currentPlannerGoal.GetRotationAngle<'Z'>() - GetDriveCenterPose().GetRotationAngle<'Z'>();
    const bool withinFinalAngleTolerance = finalAngleDelta.getAbsoluteVal().ToFloat() <=
      2 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

    Radians initialTurnAngle = atan2( target2d.GetY() - GetDriveCenterPose().GetTranslation().y(),
                                      target2d.GetX() - GetDriveCenterPose().GetTranslation().x()) -
      GetDriveCenterPose().GetRotationAngle<'Z'>();

    const bool initialTurnAngleLarge = initialTurnAngle.getAbsoluteVal().ToFloat() >
      0.5 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

    const bool farEnoughAwayForMinAngle = distSquared > std::pow( MIN_DISTANCE_FOR_MINANGLE_PLANNER, 2);

    // if we would need to turn fairly far, but our current angle is fairly close to the goal, use the
    // planner which backs up first to minimize the turn
    if( withinFinalAngleTolerance && initialTurnAngleLarge && farEnoughAwayForMinAngle ) {
      PRINT_NAMED_INFO("Robot.SelectPlanner.ShortMinAngle",
                       "distance^2 is %f, angleDelta is %f, intiialTurnAngle is %f, selecting short min_angle planner",
                       distSquared,
                       finalAngleDelta.getAbsoluteVal().ToFloat(),
                       initialTurnAngle.getAbsoluteVal().ToFloat());
      _selectedPathPlanner = _shortMinAnglePathPlanner;
    }
    else {
      PRINT_NAMED_INFO("Robot.SelectPlanner.Short",
                       "distance^2 is %f, angleDelta is %f, intiialTurnAngle is %f, selecting short planner",
                       distSquared,
                       finalAngleDelta.getAbsoluteVal().ToFloat(),
                       initialTurnAngle.getAbsoluteVal().ToFloat());
      _selectedPathPlanner = _shortPathPlanner;
    }
  }
  else {
    PRINT_NAMED_INFO("Robot.SelectPlanner.Long", "distance^2 is %f, selecting long planner", distSquared);
    _selectedPathPlanner = _longPathPlanner;
  }
  
  if( _selectedPathPlanner != _longPathPlanner ) {
    _fallbackPathPlanner = _longPathPlanner;
  } else {
    _fallbackPathPlanner = nullptr;
  }
}

void Robot::SelectPlanner(const std::vector<Pose3d>& targetPoses)
{
  if( ! targetPoses.empty() ) {
    Planning::GoalID closest = IPathPlanner::ComputeClosestGoalPose(GetDriveCenterPose(), targetPoses);
    SelectPlanner(targetPoses[closest]);
  }
}

Result Robot::StartDrivingToPose(const Pose3d& targetPose,
                                 const PathMotionProfile motionProfile,
                                 bool useManualSpeed)
{
  _usingManualPathSpeed = useManualSpeed;

  Pose3d targetPoseWrtOrigin;
  if(targetPose.GetWithRespectTo(*GetWorldOrigin(), targetPoseWrtOrigin) == false) {
    PRINT_NAMED_WARNING("Robot.StartDrivingToPose.OriginMisMatch",
                        "Could not get target pose w.r.t. robot %d's origin.", GetID());
    _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
    return RESULT_FAIL;
  }

  SelectPlanner(targetPoseWrtOrigin);

  // Compute drive center pose of given target robot pose
  Pose3d targetDriveCenterPose;
  ComputeDriveCenterPose(targetPoseWrtOrigin, targetDriveCenterPose);

  const bool somePlannerSucceeded = StartPlanner(GetDriveCenterPose(), targetDriveCenterPose);
  if( !somePlannerSucceeded ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
    return RESULT_FAIL;
  }

  if( IsTraversingPath() ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
  }
  else {
    _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
  }

  _numPlansStarted++;
      
  _pathMotionProfile = motionProfile;

  return RESULT_OK;
}

Result Robot::StartDrivingToPose(const std::vector<Pose3d>& poses,
                                 const PathMotionProfile motionProfile,
                                 Planning::GoalID* selectedPoseIndexPtr,
                                 bool useManualSpeed)
{
  _usingManualPathSpeed = useManualSpeed;
  _plannerSelectedPoseIndexPtr = selectedPoseIndexPtr;

  SelectPlanner(poses);

  // Compute drive center pose for start pose and goal poses
  std::vector<Pose3d> targetDriveCenterPoses(poses.size());
  for (int i=0; i< poses.size(); ++i) {
    ComputeDriveCenterPose(poses[i], targetDriveCenterPoses[i]);
  }

  const bool somePlannerSucceeded = StartPlanner(GetDriveCenterPose(), targetDriveCenterPoses);
  if( !somePlannerSucceeded ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
    return RESULT_FAIL;
  }

  if( IsTraversingPath() ) {
    _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
  }
  else {
    _driveToPoseStatus = ERobotDriveToPoseStatus::ComputingPath;
  }

  _numPlansStarted++;

  _pathMotionProfile = motionProfile;
      
  return RESULT_OK;
}

ERobotDriveToPoseStatus Robot::CheckDriveToPoseStatus() const
{
  return _driveToPoseStatus;
}

bool Robot::StartPlanner()
{
  if( _fallbackPlannerTargetPoses.size() == 1 ) {
    return StartPlanner(_fallbackPlannerDriveCenterPose, _fallbackPlannerTargetPoses.back());
  } else if ( _fallbackPlannerTargetPoses.size() > 1 ){
    return StartPlanner(_fallbackPlannerDriveCenterPose, _fallbackPlannerTargetPoses);
  } else { // treat it as an error
    PRINT_NAMED_ERROR("Robot.Update.Planner.Error", "Could not restart planner, missing target poses");
    return false;
  }
}
  
bool Robot::StartPlanner(const Pose3d& driveCenterPose, const std::vector<Pose3d>& targetDriveCenterPoses)
{
  // save start and target poses in case this run fails and we need to try again
  _fallbackPlannerDriveCenterPose = driveCenterPose;
  _fallbackPlannerTargetPoses = targetDriveCenterPoses;
  
  EComputePathStatus status = _selectedPathPlanner->ComputePath(driveCenterPose, targetDriveCenterPoses);
  if( status == EComputePathStatus::Error ) {
    // try again with the fallback, if it exists
    if( nullptr != _fallbackPathPlanner ) {
      _selectedPathPlanner = _fallbackPathPlanner;
      _fallbackPathPlanner = nullptr;
      if( EComputePathStatus::Error != _selectedPathPlanner->ComputePath(driveCenterPose, targetDriveCenterPoses) ) {
        return true;
      }
    }
    return false;
  }
  return true;
}
  
bool Robot::StartPlanner(const Pose3d& driveCenterPose, const Pose3d& targetDriveCenterPose)
{
  // save start and target poses in case this run fails and we need to try again
  _fallbackPlannerDriveCenterPose = driveCenterPose;
  _fallbackPlannerTargetPoses.clear();
  _fallbackPlannerTargetPoses.push_back(targetDriveCenterPose);
  
  EComputePathStatus status = _selectedPathPlanner->ComputePath(driveCenterPose, targetDriveCenterPose);
  if( status == EComputePathStatus::Error ) {
    // try again with the fallback, if it hasnt been used already
    if( nullptr != _fallbackPathPlanner ) {
      _selectedPathPlanner = _fallbackPathPlanner;
      _fallbackPathPlanner = nullptr;
      if( EComputePathStatus::Error != _selectedPathPlanner->ComputePath(driveCenterPose, targetDriveCenterPose) ) {
        return true;
      }
    }
    return false;
  }
  return true;
}

void Robot::RestartPlannerIfNeeded(bool forceReplan)
{
  assert(nullptr != _selectedPathPlanner);
  assert(_numPlansStarted == _numPlansFinished);
  
  switch( _selectedPathPlanner->ComputeNewPathIfNeeded( GetDriveCenterPose(), forceReplan ) ) {
    case EComputePathStatus::Error:
      _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
      AbortDrivingToPose();
      PRINT_NAMED_INFO("Robot.Update.Replan.Fail", "ComputeNewPathIfNeeded returned failure!");
      break;
      
    case EComputePathStatus::Running:
      _numPlansStarted++;
      PRINT_NAMED_INFO("Robot.Update.Replan.Running", "ComputeNewPathIfNeeded running");
      _driveToPoseStatus = ERobotDriveToPoseStatus::Replanning;
      break;
      
    case EComputePathStatus::NoPlanNeeded:
      // leave status as following, don't update plan attempts since no new planning is needed
      break;
  }
}
  
Result Robot::PlaceObjectOnGround(const bool useManualSpeed)
{
  if(!IsCarryingObject()) {
    PRINT_NAMED_ERROR("Robot.PlaceObjectOnGround.NotCarryingObject",
                      "Robot told to place object on ground, but is not carrying an object.");
    return RESULT_FAIL;
  }
      
  _usingManualPathSpeed = useManualSpeed;
  _lastPickOrPlaceSucceeded = false;
      
  return SendRobotMessage<Anki::Cozmo::PlaceObjectOnGround>(0, 0, 0,
                                                            DEFAULT_PATH_MOTION_PROFILE.speed_mmps,
                                                            DEFAULT_PATH_MOTION_PROFILE.accel_mmps2,
                                                            DEFAULT_PATH_MOTION_PROFILE.decel_mmps2,
                                                            useManualSpeed);
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

void Robot::LoadBehaviors()
{
  const auto& behaviorData = _context->GetDataLoader()->GetBehaviorJsons();
  for( const auto& fileJsonPair : behaviorData )
  {
    const auto& filename = fileJsonPair.first;
    const auto& behaviorJson = fileJsonPair.second;
    if (!behaviorJson.empty())
    {
      // PRINT_NAMED_DEBUG("Robot.LoadBehavior", "Loading '%s'", fullFileName.c_str());
      const Result ret = _behaviorMgr->CreateBehaviorFromConfiguration(behaviorJson);
      if ( ret != RESULT_OK ) {
        PRINT_NAMED_ERROR("Robot.LoadBehavior.CreateFailed", "Failed to create behavior from '%s'", filename.c_str());
      }
    }
    else
    {
      PRINT_NAMED_WARNING("Robot.LoadBehavior", "Failed to read '%s'", filename.c_str());
    }
    // don't print anything if we read an empty json
  }
}

Result Robot::SyncTime()
{
  _timeSynced = false;
  _poseHistory->Clear();
      
  return SendSyncTime();
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
      
  if(!existingObject->CanBeUsedForLocalization()) {
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
      
  RobotPoseStamp* posePtr = nullptr;
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
    // Get computed RobotPoseStamp at the time the object was observed.
    if ((lastResult = GetComputedPoseAt(seenObject->GetLastObservedTime(), &posePtr)) != RESULT_OK) {
      PRINT_NAMED_ERROR("Robot.LocalizeToObject.CouldNotFindHistoricalPose",
                        "Time %d", seenObject->GetLastObservedTime());
      return lastResult;
    }
        
    // The computed historical pose is always stored w.r.t. the robot's world
    // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
    // will work correctly
    Pose3d robotPoseAtObsTime = posePtr->GetPose();
    robotPoseAtObsTime.SetParent(_worldOrigin);
        
    // Get the pose of the robot with respect to the observed object
    if(robotPoseAtObsTime.GetWithRespectTo(seenObject->GetPose(), robotPoseWrtObject) == false) {
      PRINT_NAMED_ERROR("Robot.LocalizeToObject.ObjectPoseOriginMisMatch",
                        "Could not get RobotPoseStamp w.r.t. seen object pose.");
      return RESULT_FAIL;
    }
        
    liftAngle = posePtr->GetLiftAngle();
    headAngle = posePtr->GetHeadAngle();
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
    if((lastResult = AddVisionOnlyPoseToHistory(seenObject->GetLastObservedTime(),
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
    PRINT_NAMED_EVENT("Robot.LocalizeToObject.RejiggeringOrigins",
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
        
    // Now we need to go through all objects whose poses have been adjusted
    // by this origin switch and notify the outside world of the change.
    _blockWorld->UpdateObjectOrigins(oldOrigin, _worldOrigin);

    // after updating all block world objects, flatten out origins to remove grandparents
    _poseOriginList.Flatten(_worldOrigin);
        
  } // if(_worldOrigin != &existingObject->GetPose().FindOrigin())
      
      
  if(nullptr != posePtr)
  {
    // Update the computed historical pose as well so that subsequent block
    // pose updates use obsMarkers whose camera's parent pose is correct.
    posePtr->SetAll(GetPoseFrameID(), robotPoseWrtOrigin, liftAngle, liftAngle, posePtr->IsCarryingObject());
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
  // NOTE: this should be _after_ calling AddVisionOnlyPoseToHistory, since
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
      
  // Get computed RobotPoseStamp at the time the mat was observed.
  RobotPoseStamp* posePtr = nullptr;
  if ((lastResult = GetComputedPoseAt(matSeen->GetLastObservedTime(), &posePtr)) != RESULT_OK) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.CouldNotFindHistoricalPose", "Time %d", matSeen->GetLastObservedTime());
    return lastResult;
  }
      
  // The computed historical pose is always stored w.r.t. the robot's world
  // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
  // will work correctly
  Pose3d robotPoseAtObsTime = posePtr->GetPose();
  robotPoseAtObsTime.SetParent(_worldOrigin);
      
  /*
  // Get computed Robot pose at the time the mat was observed (note that this
  // also makes the pose have the robot's current world origin as its parent
  Pose3d robotPoseAtObsTime;
  if(robot->GetComputedPoseAt(matSeen->GetLastObservedTime(), robotPoseAtObsTime) != RESULT_OK) {
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
                      "Could not get RobotPoseStamp w.r.t. matPose.");
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
  // RobotPoseStamp p(robot->GetPoseFrameID(),
  //                  robotPoseWrtMat.GetWithRespectToOrigin(),
  //                  posePtr->GetHeadAngle(),
  //                  posePtr->GetLiftAngle());
  Pose3d robotPoseWrtOrigin = robotPoseWrtMat.GetWithRespectToOrigin();
      
  if((lastResult = AddVisionOnlyPoseToHistory(existingMatPiece->GetLastObservedTime(),
                                              robotPoseWrtOrigin,
                                              posePtr->GetHeadAngle(),
                                              posePtr->GetLiftAngle())) != RESULT_OK)
  {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedAddingVisionOnlyPoseToHistory", "");
    return lastResult;
  }
      
      
  // Update the computed historical pose as well so that subsequent block
  // pose updates use obsMarkers whose camera's parent pose is correct.
  // Note again that we store the pose w.r.t. the origin in history.
  // TODO: Should SetPose() do the flattening w.r.t. origin?
  posePtr->SetAll(GetPoseFrameID(), robotPoseWrtOrigin, posePtr->GetHeadAngle(), posePtr->GetLiftAngle(), posePtr->IsCarryingObject());
      
  // Compute the new "current" pose from history which uses the
  // past vision-based "ground truth" pose we just computed.
  if(UpdateCurrPoseFromHistory() == false) {
    PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedUpdateCurrPoseFromHistory", "");
    return RESULT_FAIL;
  }
      
  // Mark the robot as now being localized to this mat
  // NOTE: this should be _after_ calling AddVisionOnlyPoseToHistory, since
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
    
    
// Clears the path that the robot is executing which also stops the robot
Result Robot::ClearPath()
{
  GetContext()->GetVizManager()->ErasePath(_ID);
  _pdo->ClearPath();
  return SendMessage(RobotInterface::EngineToRobot(RobotInterface::ClearPath(0)));
}
    
// Sends a path to the robot to be immediately executed
Result Robot::ExecutePath(const Planning::Path& path, const bool useManualSpeed)
{
  Result lastResult = RESULT_FAIL;
      
  if (path.GetNumSegments() == 0) {
    PRINT_NAMED_WARNING("Robot.ExecutePath.EmptyPath", "");
    lastResult = RESULT_OK;
  } else {
        
    // TODO: Clear currently executing path or write to buffered path?
    lastResult = ClearPath();
    if(lastResult == RESULT_OK) {
      ++_lastSentPathID;
      _pdo->SetPath(path);
      _usingManualPathSpeed = useManualSpeed;
      lastResult = SendExecutePath(path, useManualSpeed);
    }
        
    // Visualize path if robot has just started traversing it.
    GetContext()->GetVizManager()->DrawPath(_ID, path, NamedColors::EXECUTED_PATH);
        
  }
      
  return lastResult;
}
  
    
Result Robot::SetOnRamp(bool t)
{
  ANKI_CPU_PROFILE("Robot::SetOnRamp");
  
  if(t == _onRamp) {
    // Nothing to do
    return RESULT_OK;
  }
      
  // We are either transition onto or off of a ramp
      
  Ramp* ramp = dynamic_cast<Ramp*>(GetBlockWorld().GetObjectByID(_rampID, ObjectFamily::Ramp));
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
    // Just do an absolute pose update, setting the robot's position to
    // where we "know" he should be when he finishes ascending or
    // descending the ramp
    switch(_rampDirection)
    {
      case Ramp::ASCENDING:
        SetPose(ramp->GetPostAscentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
        break;
            
      case Ramp::DESCENDING:
        SetPose(ramp->GetPostDescentPose(WHEEL_BASE_MM).GetWithRespectToOrigin());
        break;
            
      default:
        PRINT_NAMED_WARNING("Robot.SetOnRamp.UnexpectedRampDirection",
                            "When transitioning on/off ramp, expecting the ramp direction to be either "
                            "ASCENDING or DESCENDING, not %d.", _rampDirection);
        return RESULT_FAIL;
    }
        
    _rampDirection = Ramp::UNKNOWN;
        
    const TimeStamp_t timeStamp = _poseHistory->GetNewestTimeStamp();
        
    PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOffRamp",
                     "Robot %d transitioning off of ramp %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d",
                     _ID, ramp->GetID().GetValue(),
                     _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
                     _pose.GetRotationAngle<'Z'>().getDegrees(),
                     timeStamp);
        
    // We are creating a new pose frame at the top of the ramp
    //IncrementPoseFrameID();
    ++_frameId;
    Result lastResult = SendAbsLocalizationUpdate(_pose,
                                                  timeStamp,
                                                  _frameId);
    if(lastResult != RESULT_OK) {
      PRINT_NAMED_WARNING("Robot.SetOnRamp.SendAbsLocUpdateFailed",
                          "Robot %d failed to send absolute localization update.", _ID);
      return lastResult;
    }

  } // if/else transitioning onto ramp
      
  _onRamp = t;
      
  return RESULT_OK;
      
} // SetOnPose()
    
    
Result Robot::SetPoseOnCharger()
{
  ANKI_CPU_PROFILE("Robot::SetPoseOnCharger");
  
  Charger* charger = dynamic_cast<Charger*>(GetBlockWorld().GetObjectByID(_chargerID, ObjectFamily::Charger));
  if(charger == nullptr) {
    PRINT_NAMED_WARNING("Robot.SetPoseOnCharger.NoChargerWithID",
                        "Robot %d has docked to charger, but Charger object with ID=%d not found in the world.",
                        _ID, _chargerID.GetValue());
    return RESULT_FAIL;
  }
      
  // Just do an absolute pose update, setting the robot's position to
  // where we "know" he should be when he finishes ascending the charger.
  SetPose(charger->GetDockedPose().GetWithRespectToOrigin());

  const TimeStamp_t timeStamp = _poseHistory->GetNewestTimeStamp();
    
  PRINT_NAMED_INFO("Robot.SetPoseOnCharger.SetPose",
                   "Robot %d now on charger %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d",
                   _ID, charger->GetID().GetValue(),
                   _pose.GetTranslation().x(), _pose.GetTranslation().y(), _pose.GetTranslation().z(),
                   _pose.GetRotationAngle<'Z'>().getDegrees(),
                   timeStamp);
      
  // We are creating a new pose frame at the top of the ramp
  //IncrementPoseFrameID();
  ++_frameId;
  Result lastResult = SendAbsLocalizationUpdate(_pose,
                                                timeStamp,
                                                _frameId);
  if(lastResult != RESULT_OK) {
    PRINT_NAMED_WARNING("Robot.SetPoseOnCharger.SendAbsLocUpdateFailed",
                        "Robot %d failed to send absolute localization update.", _ID);
    return lastResult;
  }
      
  return RESULT_OK;
      
} // SetOnPose()
    
    
Result Robot::DockWithObject(const ObjectID objectID,
                             const f32 speed_mmps,
                             const f32 accel_mmps2,
                             const f32 decel_mmps2,
                             const Vision::KnownMarker* marker,
                             const Vision::KnownMarker* marker2,
                             const DockAction dockAction,
                             const f32 placementOffsetX_mm,
                             const f32 placementOffsetY_mm,
                             const f32 placementOffsetAngle_rad,
                             const bool useManualSpeed,
                             const u8 numRetries,
                             const DockingMethod dockingMethod)
{
  return DockWithObject(objectID,
                        speed_mmps,
                        accel_mmps2,
                        decel_mmps2,
                        marker,
                        marker2,
                        dockAction,
                        0, 0, u8_MAX,
                        placementOffsetX_mm, placementOffsetY_mm, placementOffsetAngle_rad,
                        useManualSpeed,
                        numRetries,
                        dockingMethod);
}
    
Result Robot::DockWithObject(const ObjectID objectID,
                             const f32 speed_mmps,
                             const f32 accel_mmps2,
                             const f32 decel_mmps2,
                             const Vision::KnownMarker* marker,
                             const Vision::KnownMarker* marker2,
                             const DockAction dockAction,
                             const u16 image_pixel_x,
                             const u16 image_pixel_y,
                             const u8 pixel_radius,
                             const f32 placementOffsetX_mm,
                             const f32 placementOffsetY_mm,
                             const f32 placementOffsetAngle_rad,
                             const bool useManualSpeed,
                             const u8 numRetries,
                             const DockingMethod dockingMethod)
{
  ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld->GetObjectByID(objectID));
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.ObjectDoesNotExist",
                      "Object with ID=%d no longer exists for docking.", objectID.GetValue());
    return RESULT_FAIL;
  }
      
  CORETECH_ASSERT(marker != nullptr);
      
  // Need to store these so that when we receive notice from the physical
  // robot that it has picked up an object we can transition the docking
  // object to being carried, using PickUpDockObject()
  _dockObjectID = objectID;
  _dockMarker   = marker;
      
  // Dock marker has to be a child of the dock block
  if(marker->GetPose().GetParent() != &object->GetPose()) {
    PRINT_NAMED_ERROR("Robot.DockWithObject.MarkerNotOnObject",
                      "Specified dock marker must be a child of the specified dock object.");
    return RESULT_FAIL;
  }

  // Mark as dirty so that the robot no longer localizes to this object
  GetObjectPoseConfirmer().SetPoseState(object, PoseState::Dirty);
      
  _usingManualPathSpeed = useManualSpeed;
  _lastPickOrPlaceSucceeded = false;
      
  // Sends a message to the robot to dock with the specified marker
  // that it should currently be seeing. If pixel_radius == u8_MAX,
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
                                                                      dockingMethod);
  if(sendResult == RESULT_OK) {
        
    // When we are "docking" with a ramp or crossing a bridge, we
    // don't want to worry about the X angle being large (since we
    // _expect_ it to be large, since the markers are facing upward).
    const bool checkAngleX = !(dockAction == DockAction::DA_RAMP_ASCEND  ||
                               dockAction == DockAction::DA_RAMP_DESCEND ||
                               dockAction == DockAction::DA_CROSS_BRIDGE);
        
    // Tell the VisionSystem to start tracking this marker:
    _visionComponentPtr->SetMarkerToTrack(marker->GetCode(), marker->GetSize(),
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
    
void Robot::SetCarryingObject(ObjectID carryObjectID)
{
  ObservableObject* object = _blockWorld->GetObjectByID(carryObjectID);
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.SetCarryingObject.NullCarryObject",
                      "Object %d no longer exists in the world. Can't set it as robot's carried object.",
                      carryObjectID.GetValue());
  }
  else
  {
    _carryingObjectID = carryObjectID;
    
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
        
    ObservableObject* carriedObject = _blockWorld->GetObjectByID(objID);
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
  }

  if (!topOnly) {
    // Tell the robot it's not carrying anything
    if (_carryingObjectID.IsSet()) {
      SendSetCarryState(CarryState::CARRY_NONE);
    }

    // Even if the above failed, still mark the robot's carry ID as unset
    _carryingObjectID.UnSet();
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
    
    
Result Robot::SetObjectAsAttachedToLift(const ObjectID& objectID, const Vision::KnownMarker* objectMarker)
{
  if(!objectID.IsSet()) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectIDNotSet",
                      "No docking object ID set, but told to pick one up.");
    return RESULT_FAIL;
  }
      
  if(objectMarker == nullptr) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoDockMarkerSet",
                      "No docking marker set, but told to pick up object.");
    return RESULT_FAIL;
  }
      
  if(IsCarryingObject()) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.AlreadyCarryingObject",
                      "Already carrying an object, but told to pick one up.");
    return RESULT_FAIL;
  }
      
  ObservableObject* object = _blockWorld->GetObjectByID(objectID);
  if(object == nullptr) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectDoesNotExist",
                      "Dock object with ID=%d no longer exists for picking up.", objectID.GetValue());
    return RESULT_FAIL;
  }
      
  // Base the object's pose relative to the lift on how far away the dock
  // marker is from the center of the block
  // TODO: compute the height adjustment per object or at least use values from cozmoConfig.h
  Pose3d objectPoseWrtLiftPose;
  if(object->GetPose().GetWithRespectTo(_liftPose, objectPoseWrtLiftPose) == false) {
    PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectAndLiftPoseHaveDifferentOrigins",
                      "Object robot is picking up and robot's lift must share a common origin.");
    return RESULT_FAIL;
  }
      
  objectPoseWrtLiftPose.SetTranslation({objectMarker->GetPose().GetTranslation().Length() +
        LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f});

  // If we know there's an object on top of the object we are picking up,
  // mark it as being carried too
  // TODO: Do we need to be able to handle non-actionable objects on top of actionable ones?

  ObservableObject* objectOnTop = _blockWorld->FindObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM);
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
      
  SetCarryingObject(objectID); // also marks the object as carried
  _carryingMarker   = objectMarker;
  
  // Don't actually change the object's pose until we've checked for objects on top
  Result poseResult = GetObjectPoseConfirmer().AddLiftRelativeObservation(object, objectPoseWrtLiftPose);
  if(RESULT_OK != poseResult)
  {
    // TODO: warn
    return poseResult;
  }
  
  return RESULT_OK;
      
} // AttachObjectToLift()
    
    
Result Robot::SetCarriedObjectAsUnattached(bool clearObjects)
{
  if(IsCarryingObject() == false) {
    PRINT_NAMED_WARNING("Robot.SetCarriedObjectAsUnattached.CarryingObjectNotSpecified",
                        "Robot not carrying object, but told to place one. "
                        "(Possibly actually rolling or balancing or popping a wheelie.");
    return RESULT_FAIL;
  }
      
  ObservableObject* object = _blockWorld->GetObjectByID(_carryingObjectID);
      
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
  
  // Initially just mark the pose as Dirty. Iff clearObject=true, then the ClearObject
  // call at the end will mark as Unknown. This is necessary because there are some
  // unfortunate ordering dependencies with how we set the pose, set the pose state, and
  // unset the carrying objects below. It's safer to do all of that, and _then_
  // clear the objects at the end.
  Result poseResult = GetObjectPoseConfirmer().AddRobotRelativeObservation(object, placedPoseWrtRobot, PoseState::Dirty);
  if(RESULT_OK != poseResult)
  {
    // TODO: warn/error
    return poseResult;
  }
    
  PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.ObjectPlaced",
                   "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).",
                   _ID, object->GetID().GetValue(),
                   object->GetPose().GetTranslation().x(),
                   object->GetPose().GetTranslation().y(),
                   object->GetPose().GetTranslation().z());

  // Store the object IDs we were carrying before we unset them so we can clear them later if needed
  auto carriedObjectIDs = GetCarryingObjects();
  
  UnSetCarryingObjects(); 
  _carryingMarker = nullptr;
      
  if(_carryingObjectOnTopID.IsSet()) {
    ObservableObject* objectOnTop = _blockWorld->GetObjectByID(_carryingObjectOnTopID);
    if(objectOnTop == nullptr)
    {
      // This really should not happen.  How can a object being carried get deleted?
      PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached",
                        "Object on top of carrying object with ID=%d no longer exists.",
                        _carryingObjectOnTopID.GetValue());
      return RESULT_FAIL;
    }
        
    Pose3d placedPoseOnTop;
    if(objectOnTop->GetPose().GetWithRespectTo(_pose.FindOrigin(), placedPoseOnTop) == false) {
      PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.OriginMisMatch",
                        "Could not get carrying object's pose relative to robot's origin.");
      return RESULT_FAIL;
    }
    
    Result poseResult = GetObjectPoseConfirmer().AddObjectRelativeObservation(objectOnTop, placedPoseOnTop, object);
    if(RESULT_OK != poseResult)
    {
      // TODO: warn / error
      return poseResult;
    }

    _carryingObjectOnTopID.UnSet();
    PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached", "Updated object %d on top of carried object.",
                     objectOnTop->GetID().GetValue());
  }
  
  if(clearObjects)
  {
    for(auto const& objectID : carriedObjectIDs)
    {
      GetBlockWorld().ClearObject(objectID); // Marks as unknown
    }
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
  if( object.IsPoseStateUnknown() ||
      !object.IsRestingFlat() ||
      (IsCarryingObject() && GetCarryingObject() == object.GetID()) ) {
    return false;
  }

  // check if we can transform to robot space
  if ( !object.GetPose().GetWithRespectTo(GetPose(), relPose) ) {
    return false;
  }

  // check if it has something on top
  const ObservableObject* objectOnTop = GetBlockWorld().FindObjectOnTopOf(object, STACKED_HEIGHT_TOL_MM);
  if ( nullptr != objectOnTop ) {
    return false;
  }

  return true;
}
  
// Helper for the following functions to reason about the object's height (mid or top)
// relative to specified threshold
inline static bool IsTooHigh(const ObservableObject& object, const Pose3d& poseWrtRobot,
                             float heightMultiplier, float heightTol, bool useTop)
{
  const Point3f rotatedSize( poseWrtRobot.GetRotation() * object.GetSize() );
  const float rotatedHeight = std::abs( rotatedSize.z() );
  float z = poseWrtRobot.GetTranslation().z();
  if(useTop) {
    z += rotatedHeight*0.5f;
  }
  const bool isTooHigh = z > (heightMultiplier * rotatedHeight + heightTol);
  return isTooHigh;
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
  if ( IsTooHigh(objectToStackOn, relPos, 1.f, STACKED_HEIGHT_TOL_MM, true) ) {
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
  if ( IsTooHigh(objectToPickUp, relPos, 2.f, STACKED_HEIGHT_TOL_MM, true) ) {
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
  if ( IsTooHigh(objectToPickUp, relPos, 0.5f, ON_GROUND_HEIGHT_TOL_MM, false) ) {
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
                                                         BaseStationTimer::getInstance()->GetCurrentTimeStamp(),
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
    
// Sends a path to the robot to be immediately executed
Result Robot::SendExecutePath(const Planning::Path& path, const bool useManualSpeed) const
{
  // Send start path execution message
  PRINT_NAMED_INFO("Robot::SendExecutePath",
                   "sending start execution message (pathID = %d, manualSpeed == %d)",
                   _lastSentPathID, useManualSpeed);
  return SendMessage(RobotInterface::EngineToRobot(RobotInterface::ExecutePath(_lastSentPathID, useManualSpeed)));
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
  ASSERT_NAMED(GetPoseOriginList().GetOriginByID(originID) == origin,
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
  RobotPoseStamp p;
  if (_poseHistory->GetLatestVisionOnlyPose(t, p) == RESULT_FAIL) {
    PRINT_NAMED_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
    return RESULT_FAIL;
  }

  return SendAbsLocalizationUpdate(p.GetPose().GetWithRespectToOrigin(), t, p.GetFrameId());
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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void Robot::HandleMessage(const ExternalInterface::EnableDroneMode& msg)
{
  _isCliffReactionDisabled = msg.isStarted;
  SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableStopOnCliff(!msg.isStarted)));
}
  

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
  ASSERT_NAMED(result == true, "Lift and camera poses should be in same pose tree");
      
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
    
Result Robot::AddRawOdomPoseToHistory(const TimeStamp_t t,
                                      const PoseFrameID_t frameID,
                                      const Pose3d& pose,
                                      const f32 head_angle,
                                      const f32 lift_angle,
                                      const bool isCarryingObject)
{
  RobotPoseStamp poseStamp(frameID, pose, head_angle, lift_angle, isCarryingObject);
  return _poseHistory->AddRawOdomPose(t, poseStamp);
}
    
    
Result Robot::UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin)
{
  // Reverse the connection between origin and robot, and connect the new
  // reversed connection
  //CORETECH_ASSERT(p.GetPose().GetParent() == _poseOrigin);
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
    
    
Result Robot::AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                         const Pose3d& pose,
                                         const f32 head_angle,
                                         const f32 lift_angle)
{      
  // We have a new ("ground truth") key frame. Increment the pose frame!
  //IncrementPoseFrameID();
  ++_frameId;
  
  // vision poses do not care about whether you are carrying an object. This field has no meaning here, so we
  // set to false always
  // COZMO-3309 Need to change poseHistory to robot status history
  const bool isCarryingObject = false;
  
  RobotPoseStamp poseStamp(_frameId, pose, head_angle, lift_angle, isCarryingObject);
  return _poseHistory->AddVisionOnlyPose(t, poseStamp);
}

Result Robot::ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                              TimeStamp_t& t, RobotPoseStamp** p,
                                              HistPoseKey* key,
                                              bool withInterpolation)
{
  return _poseHistory->ComputeAndInsertPoseAt(t_request, t, p, key, withInterpolation);
}

Result Robot::GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p)
{
  return _poseHistory->GetVisionOnlyPoseAt(t_request, p);
}

Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, Pose3d& pose) const
{
  const RobotPoseStamp* poseStamp;
  Result lastResult = GetComputedPoseAt(t_request, &poseStamp);
  if(lastResult == RESULT_OK) {
    // Grab the pose stored in the pose stamp we just found, and hook up
    // its parent to the robot's current world origin (since pose history
    // doesn't keep track of pose parent chains)
    pose = poseStamp->GetPose();
    pose.SetParent(_worldOrigin);
  }
  return lastResult;
}
    
Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, const RobotPoseStamp** p, HistPoseKey* key) const
{
  return _poseHistory->GetComputedPoseAt(t_request, p, key);
}

Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key)
{
  return _poseHistory->GetComputedPoseAt(t_request, p, key);
}

bool Robot::IsValidPoseKey(const HistPoseKey key) const
{
  return _poseHistory->IsValidPoseKey(key);
}
    
bool Robot::UpdateCurrPoseFromHistory()
{
  bool poseUpdated = false;
      
  TimeStamp_t t;
  RobotPoseStamp p;
  if (_poseHistory->ComputePoseAt(_poseHistory->GetNewestTimeStamp(), t, p) == RESULT_OK)
  {

    Pose3d newPose;
    if((p.GetPose().GetWithRespectTo(*_worldOrigin, newPose))==false)
    {
      // This is not necessarily an error anymore: it's possible we've received an
      // odometry update from the robot w.r.t. an old origin (before being delocalized),
      // in which case we can't use it to update the current pose of the robot
      // in its new frame.
      PRINT_NAMED_INFO("Robot.UpdateCurrPoseFromHistory.GetWrtParentFailed",
                        "Could not update robot %d's current pose using historical pose w.r.t. %s because we are now in frame %s.",
                        _ID, p.GetPose().FindOrigin().GetName().c_str(),
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
  ASSERT_NAMED_EVENT(factory_ids.size() == _objectsToConnectTo.size(),
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
  _connectedObjects[activeID].lastDisconnectionTime = 0;
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
  ASSERT_NAMED(_objectsToConnectTo.size() == _connectedObjects.size(),
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
  double time = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if ((_lastDiconnectedCheckTime <= 0) || (time >= (_lastDiconnectedCheckTime + kDiconnectedCheckDelay)))
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
    
    _lastDiconnectedCheckTime = time;
  }
}

void Robot::BroadcastAvailableObjects(bool enable)
{
  _enableDiscoveredObjectsBroadcasting = enable;
}
    
      
Robot::ReactionCallbackIter Robot::AddReactionCallback(const Vision::Marker::Code code, ReactionCallback callback)
{
  //CoreTechPrint("_reactionCallbacks size = %lu\n", (unsigned long)_reactionCallbacks.size());
      
  _reactionCallbacks[code].emplace_front(callback);
      
  return _reactionCallbacks[code].cbegin();
      
} // AddReactionCallback()
    
    
// Remove a preivously-added callback using the iterator returned by
// AddReactionCallback above.
void Robot::RemoveReactionCallback(const Vision::Marker::Code code, ReactionCallbackIter callbackToRemove)
{
  _reactionCallbacks[code].erase(callbackToRemove);
  if(_reactionCallbacks[code].empty()) {
    _reactionCallbacks.erase(code);
  }
} // RemoveReactionCallback()
    
    
Result Robot::AbortAll()
{
  bool anyFailures = false;
      
  _actionList->Cancel();
      
  if(AbortDrivingToPose() != RESULT_OK) {
    anyFailures = true;
  }
      
  if(AbortDocking() != RESULT_OK) {
    anyFailures = true;
  }
      
  if(AbortAnimation() != RESULT_OK) {
    anyFailures = true;
  }
  
  _movementComponent.StopAllMotors();
      
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
    
Result Robot::AbortDrivingToPose()
{
  _selectedPathPlanner->StopPlanning();
  Result ret = ClearPath();
  _numPlansFinished = _numPlansStarted;

  return ret;
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
  const int kMaxDebugStringLen = u8_MAX;
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

void Robot::BroadcastEngineErrorCode(EngineErrorCode error)
{
  Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::EngineErrorCodeMessage(error)));
  PRINT_NAMED_ERROR("Robot.BroadcastEngineErrorCode",
                    "Engine failing with error code %s[%hhu]",
                    EnumToString(error),
                    error);
}
    
ExternalInterface::RobotState Robot::GetRobotState()
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
  
  msg.status = 0;
  if(GetMoveComponent().IsMoving()) { msg.status |= (uint32_t)RobotStatusFlag::IS_MOVING; }
  if(IsPickingOrPlacing()) { msg.status |= (uint32_t)RobotStatusFlag::IS_PICKING_OR_PLACING; }
  if(_offTreadsState != OffTreadsState::OnTreads) { msg.status |= (uint32_t)RobotStatusFlag::IS_PICKED_UP; }
  if(_offTreadsState == OffTreadsState::Falling)  { msg.status |= (uint32_t)RobotStatusFlag::IS_FALLING; }
  if(IsAnimating())        { msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING; }
  if(IsIdleAnimating())    { msg.status |= (uint32_t)RobotStatusFlag::IS_ANIMATING_IDLE; }
  if( IsOnCharger() )      { msg.status |= (uint32_t)RobotStatusFlag::IS_ON_CHARGER; }
  if(IsCarryingObject())   {
    msg.status |= (uint32_t)RobotStatusFlag::IS_CARRYING_BLOCK;
    msg.carryingObjectID = GetCarryingObject();
    msg.carryingObjectOnTopID = GetCarryingObjectOnTop();
  } else {
    msg.carryingObjectID = -1;
  }
  if(!GetActionList().IsEmpty()) {
    msg.status |= (uint32_t)RobotStatusFlag::IS_PATHING;
  }
  if(_chargerOOS) { msg.status |= (uint32_t)RobotStatusFlag::IS_CHARGER_OOS; }
  
  msg.gameStatus = 0;
  if (IsLocalized() && _offTreadsState == OffTreadsState::OnTreads) { msg.gameStatus |= (uint8_t)GameStatusFlag::IsLocalized; }
      
  msg.headTrackingObjectID = GetMoveComponent().GetTrackToObject();
      
  msg.localizedToObjectID = GetLocalizedTo();
      
  // TODO: Add proximity sensor data to state message
      
  msg.batteryVoltage = GetBatteryVoltage();
      
  msg.lastImageTimeStamp = GetVisionComponent().GetLastProcessedImageTimeStamp();
      
  return msg;
}
    
RobotInterface::MessageHandler* Robot::GetRobotMessageHandler()
{
  if (!_context->GetRobotManager())
  {
    ASSERT_NAMED(false, "Robot.GetRobotMessageHandler.nullptr");
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
  return ObjectType::Unknown;
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

bool Robot::IsIdle() const
{
  return !IsTraversingPath() && _actionList->IsEmpty();
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
  
  Vision::Camera camera(_visionComponentPtr->GetCamera());
  
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
      PRINT_NAMED_DEBUG("ComputeHeadAngle", "%d: %.1fdeg", iteration, RAD_TO_DEG_F32(searchAngle_rad));
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
        PRINT_NAMED_DEBUG("ComputeHeadAngle", "CONVERGED: %.1fdeg", RAD_TO_DEG_F32(searchAngle_rad));
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
  
  RobotPoseStamp poseStamp;
  TimeStamp_t t;
  Result result = GetPoseHistory()->ComputePoseAt(timestamp, t, poseStamp);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_WARNING("Robot.ComputeTurnTowardsImagePointAngles.ComputeHistPoseFailed", "t=%u", timestamp);
    absPanAngle = GetPose().GetRotation().GetAngleAroundZaxis();
    absTiltAngle = GetHeadAngle();
    return result;
  }
  
  absTiltAngle = std::atan2f(-pt.y(), calib->GetFocalLength_y()) + poseStamp.GetHeadAngle();
  absPanAngle  = std::atan2f(-pt.x(), calib->GetFocalLength_x()) + poseStamp.GetPose().GetRotation().GetAngleAroundZaxis();
  
  return RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
