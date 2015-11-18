//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

// TODO:(bn) should these be a full path?
#include "anki/cozmo/basestation/pathPlanner.h"
#include "anki/cozmo/basestation/latticePlanner.h"
#include "anki/cozmo/basestation/minimalAnglePlanner.h"
#include "anki/cozmo/basestation/faceAndApproachPlanner.h"
#include "anki/cozmo/basestation/pathDolerOuter.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/activeCube.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/basestation/cozmoActions.h"
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
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "opencv2/highgui/highgui.hpp" // For imwrite() in ProcessImage
#include "anki/cozmo/basestation/soundManager.h"    // TODO: REMOVE ME
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"
#include "anki/vision/basestation/image.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/robotStatusAndActions.h"
#include "util/helpers/templateHelpers.h"
#include "util/fileUtils/fileUtils.h"

#include "opencv2/calib3d/calib3d.hpp"

#include <fstream>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_DISTANCE_FOR_SHORT_PLANNER 40.0f
#define MAX_DISTANCE_TO_PREDOCK_POSE 20.0f

namespace Anki {
  namespace Cozmo {
    
    Robot::Robot(const RobotID_t robotID, RobotInterface::MessageHandler* msgHandler,
                 IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform)
    : _externalInterface(externalInterface)
    , _dataPlatform(dataPlatform)
    , _ID(robotID)
    , _timeSynced(false)
    , _msgHandler(msgHandler)
    , _blockWorld(this)
    , _faceWorld(*this)
    , _behaviorMgr(*this)
    , _movementComponent(*this)
    , _visionComponent(robotID, VisionComponent::RunMode::Asynchronous, dataPlatform)
    , _neckPose(0.f,Y_AXIS_3D(), {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &_pose, "RobotNeck")
    , _headCamPose(RotationMatrix3d({0,0,1,  -1,0,0,  0,-1,0}),
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &_neckPose, "RobotHeadCam")
    , _liftBasePose(0.f, Y_AXIS_3D(), {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &_pose, "RobotLiftBase")
    , _liftPose(0.f, Y_AXIS_3D(), {{LIFT_ARM_LENGTH, 0.f, 0.f}}, &_liftBasePose, "RobotLift")
    , _currentHeadAngle(MIN_HEAD_ANGLE)
    , _audioClient( nullptr )
    , _animationStreamer(_externalInterface, _cannedAnimations)
    , _moodManager(this)
    , _progressionManager(this)
    , _imageDeChunker(new ImageDeChunker())
    {
      _poseHistory = new RobotPoseHistory();
      PRINT_NAMED_INFO("Robot.Robot", "Created");
      
      _pose.SetName("Robot_" + std::to_string(_ID));
      _driveCenterPose.SetName("RobotDriveCenter_" + std::to_string(_ID));
      
      // Initializes _pose, _poseOrigins, and _worldOrigin:
      Delocalize();
      
      // Delocalize will mark isLocalized as false, but we are going to consider
      // the robot localized (by odometry alone) to start, until he gets picked up.
      _isLocalized = true;
      SetLocalizedTo(nullptr);
      
      InitRobotMessageComponent(_msgHandler,robotID);
      
      if (HasExternalInterface())
      {
        SetupGainsHandlers(*_externalInterface);
        SetupVisionHandlers(*_externalInterface);
        SetupMiscHandlers(*_externalInterface);
      }
      
      _proceduralFace.MarkAsSentToRobot(false);
      _proceduralFace.SetTimeStamp(1); // Make greater than lastFace's timestamp, so it gets streamed
      _lastProceduralFace.MarkAsSentToRobot(true);
      
      // The call to Delocalize() will increment frameID, but we want it to be
      // initialzied to 0, to match the physical robot's initialization
      _frameId = 0;

      ReadAnimationDir();
      
      // Read in behavior manager Json
      Json::Value behaviorConfig;
      if (nullptr != _dataPlatform)
      {
        std::string jsonFilename = "config/basestation/config/behavior_config.json";
        bool success = _dataPlatform->readAsJson(Util::Data::Scope::Resources, jsonFilename, behaviorConfig);
        if (!success)
        {
          PRINT_NAMED_ERROR("Robot.BehaviorConfigJsonNotFound",
                            "Behavior Json config file %s not found.",
                            jsonFilename.c_str());
        }
      }
      //_moodManager.Init(moodConfig); // [MarkW:TODO] Replace emotion_config.json, also, wouldn't this be the same config for each robot? Load once earlier?
      _behaviorMgr.Init(behaviorConfig);
      
      SetHeadAngle(_currentHeadAngle);
      _pdo = new PathDolerOuter(msgHandler, robotID);

      if (nullptr != _dataPlatform) {
        _longPathPlanner  = new LatticePlanner(this, _dataPlatform);
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
      
    } // Constructor: Robot
    
    Robot::~Robot()
    {
      Util::SafeDelete(_imageDeChunker);
      Util::SafeDelete(_poseHistory);
      Util::SafeDelete(_pdo);
      Util::SafeDelete(_longPathPlanner);
      Util::SafeDelete(_shortPathPlanner);
      Util::SafeDelete(_shortMinAnglePathPlanner);
      
      _selectedPathPlanner = nullptr;
      
    }
    
    void Robot::SetPickedUp(bool t)
    {
      if(_isPickedUp == false && t == true) {
        // Robot is being picked up: de-localize it
        Delocalize();
        
        _visionComponent.Pause(true);
        
        Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPickedUp(GetID())));
      }
      else if (true == _isPickedUp && false == t) {
        // Robot just got put back down
        _visionComponent.Pause(false);
        
        ASSERT_NAMED(!IsLocalized(), "Robot should be delocalized when first put back down!");
        
        // If we are not localized and there is nothing else left in the world that
        // we could localize to, then go ahead and mark us as localized (via
        // odometry alone)
        if(false == _blockWorld.AnyRemainingLocalizableObjects()) {
          PRINT_NAMED_INFO("Robot.SetPickedUp.NoMoreRemainingLocalizableObjects",
                           "Marking previously-unlocalized robot %d as localized to odometry because "
                           "there are no more objects to localize to in the world.", GetID());
          SetLocalizedTo(nullptr); // marks us as localized to odometry only
        }
        
        Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPutDown(GetID())));
      }
      _isPickedUp = t;
    }
    
    void Robot::Delocalize()
    {
      PRINT_NAMED_INFO("Robot.Delocalize", "Delocalizing robot %d.\n", GetID());
      
      _isLocalized = false;
      _localizedToID.UnSet();
      _localizedToFixedObject = false;
      _localizedMarkerDistToCameraSq = -1.f;

      // NOTE: no longer doing this here because Delocalize() can be called by
      //  BlockWorld::ClearAllExistingObjects, resulting in a weird loop...
      //_blockWorld.ClearAllExistingObjects();
      
      // Add a new pose origin to use until the robot gets localized again
      _poseOrigins.emplace_back();
      _poseOrigins.back().SetName("Robot" + std::to_string(_ID) + "_PoseOrigin" + std::to_string(_poseOrigins.size() - 1));
      _worldOrigin = &_poseOrigins.back();
      
      _pose.SetRotation(0, Z_AXIS_3D());
      _pose.SetTranslation({{0.f, 0.f, 0.f}});
      _pose.SetParent(_worldOrigin);
      
      _driveCenterPose.SetRotation(0, Z_AXIS_3D());
      _driveCenterPose.SetTranslation({{0.f, 0.f, 0.f}});
      _driveCenterPose.SetParent(_worldOrigin);
      
      _poseHistory->Clear();
      //++_frameId;
     
      // Update VizText
      VizManager::getInstance()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: <nothing>");
      VizManager::getInstance()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOrigins.size(),
                                         _worldOrigin->GetName().c_str());
    } // Delocalize()
    
    Result Robot::SetLocalizedTo(const ObservableObject* object)
    {
      if(object == nullptr) {
        VizManager::getInstance()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                           "LocalizedTo: Odometry");
        _localizedToID.UnSet();
        _isLocalized = true;
        return RESULT_OK;
      }
      
      if(object->GetID().IsUnknown()) {
        PRINT_NAMED_ERROR("Robot.SetLocalizedTo.IdNotSet",
                          "Cannot localize to an object with no ID set.\n");
        return RESULT_FAIL;
      }
      
      // Find the closest, most recently observed marker on the object
      TimeStamp_t mostRecentObsTime = 0;
      for(auto marker : object->GetMarkers()) {
        if(marker.GetLastObservedTime() >= mostRecentObsTime) {
          Pose3d markerPoseWrtCamera;
          if(false == marker.GetPose().GetWithRespectTo(_visionComponent.GetCamera().GetPose(), markerPoseWrtCamera)) {
            PRINT_NAMED_ERROR("Robot.SetLocalizedTo.MarkerOriginProblem",
                              "Could not get pose of marker w.r.t. robot camera.\n");
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
      
      // Update VizText
      VizManager::getInstance()->SetText(VizManager::LOCALIZED_TO, NamedColors::YELLOW,
                                         "LocalizedTo: %s_%d",
                                         ObjectTypeToString(object->GetType()), _localizedToID.GetValue());
      VizManager::getInstance()->SetText(VizManager::WORLD_ORIGIN, NamedColors::YELLOW,
                                         "WorldOrigin[%lu]: %s",
                                         _poseOrigins.size(),
                                         _worldOrigin->GetName().c_str());
      
      return RESULT_OK;
      
    } // SetLocalizedTo()
    
    
    Result Robot::UpdateFullRobotState(const RobotState& msg)
    {
      Result lastResult = RESULT_OK;

      // Ignore state messages received before time sync
      if (!_timeSynced) {
        return lastResult;
      }
      
      // Save state to file
      if(_stateSaveMode != SAVE_OFF)
      {
        // Make sure image capture folder exists
        std::string robotStateCaptureDir = _dataPlatform->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_ROBOT_STATE_CAPTURE_DIR);
        if (!Util::FileUtils::CreateDirectory(robotStateCaptureDir, false, true)) {
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.CreateDirFailed","%s", robotStateCaptureDir.c_str());
        }
        
#if(0)
        // Compose line for entire state msg in hex
        char stateMsgLine[256];
        memset(stateMsgLine,0,256);
        
        u8 msgBytes[256];
        msg.GetBytes(msgBytes);
        for (int i=0; i< msg.GetSize(); i++){
          sprintf(&stateMsgLine[2*i], "%02x", (unsigned char)msgBytes[i]);
        }
        sprintf(&stateMsgLine[msg.GetSize() *2],"\n");
        FILE *stateFile;
        stateFile = fopen("RobotState.txt", "a");
        fputs(stateMsgLine, stateFile);
        fclose(stateFile);
#endif
        /*
        // clad functionality missing: ToJson()
        // Write state message to JSON file
        // TODO: (ds/as) use current game log folder instead?
        std::string msgFilename(std::string(AnkiUtil::kP_ROBOT_STATE_CAPTURE_DIR) + "/cozmo" + std::to_string(GetID()) + "_state_" + std::to_string(msg.timestamp) + ".json");
        
        Json::Value json = msg.CreateJson();
        PRINT_NAMED_INFO("Robot.UpdateFullRobotState", "Writing RobotState JSON to file %s", msgFilename.c_str());
        _dataPlatform->writeAsJson(Util::Data::Scope::Cache, msgFilename, json);
        */
#if(0)
        // Compose line for IMU output file.
        // Used for determining delay constant in image timestamp.
        char stateMsgLine[512];
        memset(stateMsgLine,0,512);
        sprintf(stateMsgLine, "%d, %f, %f\n", msg.timestamp, msg.rawGyroZ, msg.rawAccelY);
        
        FILE *stateFile;
        stateFile = fopen("cozmoIMUState.csv", "a");
        fputs(stateMsgLine, stateFile);
        fclose(stateFile);
#endif
        
        
        // Turn off save mode if we were in one-shot mode
        if (_stateSaveMode == SAVE_ONE_SHOT) {
          _stateSaveMode = SAVE_OFF;
        }
      }

      // Set flag indicating that robot state messages have been received
      _newStateMsgAvailable = true;
      
      // Update head angle
      SetHeadAngle(msg.headAngle);
      
      // Update lift angle
      SetLiftAngle(msg.liftAngle);
      
      // Update robot pitch angle
      _pitchAngle = msg.pose.pitch_angle;
      

      // TODO: (KY/DS) remove SetProxSensorData
      //// Update proximity sensor values
      //SetProxSensorData(PROX_LEFT, msg. proxLeft, msg.status & IS_PROX_SIDE_BLOCKED);
      //SetProxSensorData(PROX_FORWARD, msg.proxForward, msg.status & IS_PROX_FORWARD_BLOCKED);
      //SetProxSensorData(PROX_RIGHT, msg.proxRight, msg.status & IS_PROX_SIDE_BLOCKED);
      
      // Get ID of last/current path that the robot executed
      SetLastRecvdPathID(msg.lastPathID);
      
      // Update other state vars
      SetCurrPathSegment( msg.currPathSegment );
      SetNumFreeSegmentSlots(msg.numFreeSegmentSlots);
      
      // Dole out more path segments to the physical robot if needed:
      if (IsTraversingPath() && GetLastRecvdPathID() == GetLastSentPathID()) {
        _pdo->Update(_currPathSegment, _numFreeSegmentSlots);
      }
      
      //robot->SetCarryingBlock( msg.status & IS_CARRYING_BLOCK ); // Still needed?
      SetPickingOrPlacing((bool)( msg.status & (uint16_t)RobotStatusFlag::IS_PICKING_OR_PLACING ));
      
      SetPickedUp((bool)( msg.status & (uint16_t)RobotStatusFlag::IS_PICKED_UP ));
      
      _battVoltage = (f32)msg.battVolt10x * 0.1f;
      _isMoving = static_cast<bool>(msg.status & (uint16_t)RobotStatusFlag::IS_MOVING);
      _isHeadMoving = !static_cast<bool>(msg.status & (uint16_t)RobotStatusFlag::HEAD_IN_POS);
      _isLiftMoving = !static_cast<bool>(msg.status & (uint16_t)RobotStatusFlag::LIFT_IN_POS);
      _leftWheelSpeed_mmps = msg.lwheel_speed_mmps;
      _rightWheelSpeed_mmps = msg.rwheel_speed_mmps;
      
      _hasMovedSinceLocalization |= _isMoving || _isPickedUp;
      
      Pose3d newPose;
      
      if(IsOnRamp()) {
        
        // Sanity check:
        CORETECH_ASSERT(_rampID.IsSet());
        
        // Don't update pose history while on a ramp.
        // Instead, just compute how far the robot thinks it has gone (in the plane)
        // and compare that to where it was when it started traversing the ramp.
        // Adjust according to the angle of the ramp we know it's on.
        
        const f32 distanceTraveled = (Point2f(msg.pose.x, msg.pose.y) - _rampStartPosition).Length();
        
        Ramp* ramp = dynamic_cast<Ramp*>(_blockWorld.GetObjectByIDandFamily(_rampID, ObjectFamily::Ramp));
        if(ramp == nullptr) {
          PRINT_NAMED_ERROR("Robot.UpdateFullRobotState.NoRampWithID",
                            "Updating robot %d's state while on a ramp, but Ramp object with ID=%d not found in the world.\n",
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
                              "ASCEND or DESCENDING, not %d.\n", _rampDirection);
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
                              "Failed to get last pose from history with frame ID=%d.\n",
                              msg.pose_frame_id);
            return lastResult;
          }
          pose_z = p.GetPose().GetTranslation().z();
        }
        
        // Need to put the odometry update in terms of the current robot origin
        newPose = Pose3d(msg.pose.angle, Z_AXIS_3D(), {msg.pose.x, msg.pose.y, pose_z}, _worldOrigin);
        
      } // if/else on ramp
      
      // Add to history
      lastResult = AddRawOdomPoseToHistory(msg.timestamp,
                                           msg.pose_frame_id,
                                           newPose.GetTranslation().x(),
                                           newPose.GetTranslation().y(),
                                           newPose.GetTranslation().z(),
                                           newPose.GetRotationAngle<'Z'>().ToFloat(),
                                           msg.headAngle,
                                           msg.liftAngle);
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.UpdateFullRobotState.AddPoseError",
                            "AddRawOdomPoseToHistory failed for timestamp=%d\n", msg.timestamp);
        return lastResult;
      }
      
      if(UpdateCurrPoseFromHistory(*_worldOrigin) == false) {
        lastResult = RESULT_FAIL;
      }
      
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
      
      // Send state to visualizer for displaying
      VizManager::getInstance()->SendRobotState(stateMsg,
                                                static_cast<size_t>(AnimConstants::KEYFRAME_BUFFER_SIZE) - (_numAnimationBytesStreamed - _numAnimationBytesPlayed),
                                                (u8)MIN(1000.f/GetAverageImagePeriodMS(), u8_MAX),
                                                (u8)MIN(1000.f/GetAverageImageProcPeriodMS(), u8_MAX),
                                                _animationTag);
      
      return lastResult;
      
    } // UpdateFullRobotState()
    
    bool Robot::HasReceivedRobotState() const
    {
      return _newStateMsgAvailable;
    }
    
    void Robot::SetPhysicalRobot(bool isPhysical)
    {
      if (_isPhysical != isPhysical) {
        if (isPhysical) {
          // "Recalibrate" camera pose within head for physical robot
          // TODO: Do this properly!
          _headCamPose.RotateBy(RotationVector3d(HEAD_CAM_YAW_CORR, Z_AXIS_3D()));
          _headCamPose.RotateBy(RotationVector3d(-HEAD_CAM_PITCH_CORR, Y_AXIS_3D()));
          _headCamPose.RotateBy(RotationVector3d(HEAD_CAM_ROLL_CORR, X_AXIS_3D()));
          _headCamPose.SetTranslation({HEAD_CAM_POSITION[0] + HEAD_CAM_TRANS_X_CORR, HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]});
          PRINT_NAMED_INFO("Robot.SetPhysicalRobot", "Slop factor applied to head cam pose for physical robot: yaw_corr=%f, pitch_corr=%f, roll_corr=%f, x_trans_corr=%fmm", HEAD_CAM_YAW_CORR, HEAD_CAM_PITCH_CORR, HEAD_CAM_ROLL_CORR, HEAD_CAM_TRANS_X_CORR);
        } else {
          _headCamPose.SetRotation({0,0,1,  -1,0,0,  0,-1,0});
          _headCamPose.SetTranslation({HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]});
          PRINT_STREAM_INFO("Robot.SetPhysicalRobot", "Slop factor removed from head cam pose for simulated robot");
        }
        _isPhysical = isPhysical;
      }
      
    }
    
    f32 ComputePoseAngularSpeed(const RobotPoseStamp& p1, const RobotPoseStamp& p2, const f32 dt)
    {
      const Radians poseAngle1( p1.GetPose().GetRotationAngle<'Z'>() );
      const Radians poseAngle2( p2.GetPose().GetRotationAngle<'Z'>() );
      const f32 poseAngSpeed = std::abs((poseAngle1-poseAngle2).ToFloat()) / dt;
      
      return poseAngSpeed;
    }

    f32 ComputeHeadAngularSpeed(const RobotPoseStamp& p1, const RobotPoseStamp& p2, const f32 dt)
    {
      const f32 headAngSpeed = std::abs((Radians(p1.GetHeadAngle()) - Radians(p2.GetHeadAngle())).ToFloat()) / dt;
      return headAngSpeed;
    }

    Vision::Camera Robot::GetHistoricalCamera(TimeStamp_t t_request)
    {
      RobotPoseStamp p;
      TimeStamp_t t;
      _poseHistory->GetRawPoseAt(t_request, t, p);
      return GetHistoricalCamera(&p, t);
    }
    
    Vision::Camera Robot::GetHistoricalCamera(RobotPoseStamp* p, TimeStamp_t t)
    {
      Vision::Camera camera(_visionComponent.GetCamera());
      
      // Compute pose from robot body to camera
      // Start with canonical (untilted) headPose
      Pose3d camPose(_headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-p->GetHeadAngle(), Y_AXIS_3D());
      camPose.RotateBy(Rvec);
      
      // Precompose with robot body to neck pose
      camPose.PreComposeWith(_neckPose);
      
      // Set parent pose to be the historical robot pose
      camPose.SetParent(&(p->GetPose()));
      
      camPose.SetName("PoseHistoryCamera_" + std::to_string(t));
      
      // Update the head camera's pose
      camera.SetPose(camPose);
      
      return camera;
    }
    
    Result Robot::QueueObservedMarker(const Vision::ObservedMarker& markerOrig)
    {
      Result lastResult = RESULT_OK;
      
      // Get historical robot pose at specified timestamp to get
      // head angle and to attach as parent of the camera pose.
      TimeStamp_t t;
      RobotPoseStamp* p = nullptr;
      HistPoseKey poseKey;
      lastResult = ComputeAndInsertPoseIntoHistory(markerOrig.GetTimeStamp(), t, &p, &poseKey, true);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.QueueObservedMarker.HistoricalPoseNotFound",
                            "Time: %d, hist: %d to %d\n",
                            markerOrig.GetTimeStamp(),
                            _poseHistory->GetOldestTimeStamp(),
                            _poseHistory->GetNewestTimeStamp());
        return lastResult;
      }
      
      // If we get here, ComputeAndInsertPoseIntoHistory() should have succeeded
      // and this should be true
      assert(markerOrig.GetTimeStamp() == t);
      
      // If this is not a face marker and "Vision while moving" is disabled and
      // we aren't picking/placing, check to see if the robot's body or head are
      // moving too fast to queue this marker
      if(!_visionWhileMovingEnabled && !IsPickingOrPlacing())
      {
        TimeStamp_t t_prev, t_next;
        RobotPoseStamp p_prev, p_next;
        
        lastResult = _poseHistory->GetRawPoseBeforeAndAfter(t, t_prev, p_prev, t_next, p_next);
        if(lastResult != RESULT_OK) {
          PRINT_NAMED_WARNING("Robot.QueueObservedMarker.HistoricalPoseNotFound",
                              "Could not get next/previous poses for t = %d, so "
                              "cannot compute angular velocity. Ignoring marker.\n", t);
          
          // Don't return failure, but don't queue the marker either (since we
          // couldn't check the angular velocity while seeing it
          return RESULT_OK;
        }

        const f32 ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC = 135.f;
        const f32 HEAD_ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC = 10.f;
        
        assert(t_prev < t);
        assert(t_next > t);
        const f32 dtPrev_sec = static_cast<f32>(t - t_prev) * 0.001f;
        const f32 dtNext_sec = static_cast<f32>(t_next - t) * 0.001f;
        const f32 headSpeedPrev = ComputeHeadAngularSpeed(*p, p_prev, dtPrev_sec);
        const f32 headSpeedNext = ComputeHeadAngularSpeed(*p, p_next, dtNext_sec);
        const f32 turnSpeedPrev = ComputePoseAngularSpeed(*p, p_prev, dtPrev_sec);
        const f32 turnSpeedNext = ComputePoseAngularSpeed(*p, p_next, dtNext_sec);
        
        if(turnSpeedNext > DEG_TO_RAD(ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC) ||
           turnSpeedPrev > DEG_TO_RAD(ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC))
        {
          //          PRINT_NAMED_WARNING("Robot.QueueObservedMarker",
          //                              "Ignoring vision marker seen while turning with angular "
          //                              "velocity = %.1f/%.1f deg/sec (thresh = %.1fdeg)\n",
          //                              RAD_TO_DEG(turnSpeedPrev), RAD_TO_DEG(turnSpeedNext),
          //                              ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC);
          return RESULT_OK;
        } else if(headSpeedNext > DEG_TO_RAD(HEAD_ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC) ||
                  headSpeedPrev > DEG_TO_RAD(HEAD_ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC))
        {
          //          PRINT_NAMED_WARNING("Robot.QueueObservedMarker",
          //                              "Ignoring vision marker seen while head moving with angular "
          //                              "velocity = %.1f/%.1f deg/sec (thresh = %.1fdeg)\n",
          //                              RAD_TO_DEG(headSpeedPrev), RAD_TO_DEG(headSpeedNext),
          //                              HEAD_ANGULAR_VELOCITY_THRESHOLD_DEG_PER_SEC);
          return RESULT_OK;
        }
        
      } // if(!_visionWhileMovingEnabled)
      
      // Update the marker's camera to use a pose from pose history, and
      // create a new marker with the updated camera
      Vision::ObservedMarker marker(markerOrig.GetTimeStamp(), markerOrig.GetCode(),
                                    markerOrig.GetImageCorners(),
                                    GetHistoricalCamera(p, markerOrig.GetTimeStamp()),
                                    markerOrig.GetUserHandle());
      
      // Queue the marker for processing by the blockWorld
      _blockWorld.QueueObservedMarker(poseKey, marker);
      
      // React to the marker if there is a callback for it
      auto reactionIter = _reactionCallbacks.find(marker.GetCode());
      if(reactionIter != _reactionCallbacks.end()) {
        // Run each reaction for this code, in order:
        for(auto & reactionCallback : reactionIter->second) {
          lastResult = reactionCallback(this, &marker);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_WARNING("Robot.Update.ReactionCallbackFailed",
                                "Reaction callback failed for robot %d observing marker with code %d.\n",
                                GetID(), marker.GetCode());
          }
        }
      }
      
      // Visualize the marker in 3D
      // TODO: disable this block when not debugging / visualizing
      if(true){
        
        // Note that this incurs extra computation to compute the 3D pose of
        // each observed marker so that we can draw in the 3D world, but this is
        // purely for debug / visualization
        u32 quadID = 0;
        
        // When requesting the markers' 3D corners below, we want them
        // not to be relative to the object the marker is part of, so we
        // will request them at a "canonical" pose (no rotation/translation)
        const Pose3d canonicalPose;
        
        
        // Block Markers
        std::set<const ObservableObject*> const& blocks = _blockWorld.GetObjectLibrary(ObjectFamily::Block).GetObjectsWithMarker(marker);
        for(auto block : blocks) {
          std::vector<Vision::KnownMarker*> const& blockMarkers = block->GetMarkersWithCode(marker.GetCode());
          
          for(auto blockMarker : blockMarkers) {
            
            Pose3d markerPose;
            Result poseResult = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                     blockMarker->Get3dCorners(canonicalPose),
                                                                     markerPose);
            if(poseResult != RESULT_OK) {
              PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker",
                                  "Could not estimate pose of block marker. Not visualizing.\n");
            } else {
              if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
                VizManager::getInstance()->DrawGenericQuad(quadID++, blockMarker->Get3dCorners(markerPose), NamedColors::OBSERVED_QUAD);
              } else {
                PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker.MarkerOriginNotCameraOrigin",
                                    "Cannot visualize a Block marker whose pose origin is not the camera's origin that saw it.\n");
              }
            }
          }
        }
        
        
        // Mat Markers
        std::set<const ObservableObject*> const& mats = _blockWorld.GetObjectLibrary(ObjectFamily::Mat).GetObjectsWithMarker(marker);
        for(auto mat : mats) {
          std::vector<Vision::KnownMarker*> const& matMarkers = mat->GetMarkersWithCode(marker.GetCode());
          
          for(auto matMarker : matMarkers) {
            Pose3d markerPose;
            Result poseResult = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                     matMarker->Get3dCorners(canonicalPose),
                                                                     markerPose);
            if(poseResult != RESULT_OK) {
              PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker",
                                  "Could not estimate pose of mat marker. Not visualizing.\n");
            } else {
              if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
                VizManager::getInstance()->DrawMatMarker(quadID++, matMarker->Get3dCorners(markerPose), NamedColors::RED);
              } else {
                PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker.MarkerOriginNotCameraOrigin",
                                    "Cannot visualize a Mat marker whose pose origin is not the camera's origin that saw it.\n");
              }
            }
          }
        }
        
      } // 3D marker visualization
      
      return lastResult;
      
    } // QueueObservedMarker()

    
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
    
    
    
    Result Robot::Update(void)
    {
#if(0)
      ActiveBlockLightTest(1);
      return RESULT_OK;
#endif
      
      VizManager::getInstance()->SendStartRobotUpdate();
      
      /* DEBUG
       const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
       static double lastUpdateTime = currentTime_sec;
       
       const double updateTimeDiff = currentTime_sec - lastUpdateTime;
       if(updateTimeDiff > 1.0) {
       PRINT_NAMED_WARNING("Robot.Update", "Gap between robot update calls = %f\n", updateTimeDiff);
       }
       lastUpdateTime = currentTime_sec;
       */
      
      
      if(_visionComponent.GetCamera().IsCalibrated())
      {
        Result visionResult = RESULT_OK;

        // Helper macro for running a vision component, capturing result, and
        // printing error message / returning if that result was not RESULT_OK.
#       define TRY_AND_RETURN_ON_FAILURE(__NAME__) \
        do { if((visionResult = _visionComponent.__NAME__(*this)) != RESULT_OK) { \
          PRINT_NAMED_ERROR("Robot.Update." QUOTE(__NAME__) "Failed", ""); \
          return visionResult; } } while(0)
        
        TRY_AND_RETURN_ON_FAILURE(UpdateVisionMarkers);
        TRY_AND_RETURN_ON_FAILURE(UpdateFaces);
        TRY_AND_RETURN_ON_FAILURE(UpdateTrackingQuad);
        TRY_AND_RETURN_ON_FAILURE(UpdateDockingErrorSignal);
        TRY_AND_RETURN_ON_FAILURE(UpdateMotionCentroid);
        
#       undef TRY_AND_RETURN_ON_FAILURE
        
        // Update Block and Face Worlds
        uint32_t numBlocksObserved = 0;
        if(RESULT_OK != _blockWorld.Update(numBlocksObserved)) {
          PRINT_NAMED_WARNING("Robot.Update.BlockWorldUpdateFailed", "");
        }
        
        if(RESULT_OK != _faceWorld.Update()) {
          PRINT_NAMED_WARNING("Robot.Update.FaceWorldUpdateFailed", "");
        }
        
      } // if (GetCamera().IsCalibrated())
      
      ///////// Update the behavior manager ///////////
      
      // TODO: This object encompasses, for the time-being, what some higher level
      // module(s) would do.  e.g. Some combination of game state, build planner,
      // personality planner, etc.
      
      const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      _moodManager.Update(currentTime);
      
      _progressionManager.Update(currentTime);
      
      const char* behaviorChooserName = "";
      std::string behaviorName("<disabled>");
      if(_isBehaviorMgrEnabled) {
        _behaviorMgr.Update(currentTime);
        
        const IBehavior* behavior = _behaviorMgr.GetCurrentBehavior();
        if(behavior != nullptr) {
          behaviorName = behavior->GetName();
          const std::string& stateName = behavior->GetStateName();
          if (!stateName.empty())
          {
            behaviorName += "-" + stateName;
          }
        }
        
        const IBehaviorChooser* behaviorChooser = _behaviorMgr.GetBehaviorChooser();
        if (behaviorChooser)
        {
          behaviorChooserName = behaviorChooser->GetName();
        }
      }
      
      VizManager::getInstance()->SetText(VizManager::BEHAVIOR_STATE, NamedColors::MAGENTA,
                                         "Behavior:%s:%s", behaviorChooserName, behaviorName.c_str());

      
      //////// Update Robot's State Machine /////////////
      Result actionResult = _actionList.Update(*this);
      if(actionResult != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.Update", "Robot %d had an action fail.", GetID());
      }
        
      //////// Stream Animations /////////
      Result animStreamResult = _animationStreamer.Update(*this);
      if(animStreamResult != RESULT_OK) {
        PRINT_NAMED_WARNING("Robot.Update",
                            "Robot %d had an animation streaming failure.", GetID());
      }


      /////////// Update path planning / following ////////////


      if( _driveToPoseStatus != ERobotDriveToPoseStatus::Waiting ) {

        bool forceReplan = _driveToPoseStatus == ERobotDriveToPoseStatus::Error;

        if( _numPlansFinished == _numPlansStarted ) {
          // nothing to do with the planners, so just update the status based on the path following
          if( IsTraversingPath() ) {
            _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;

            if( GetBlockWorld().DidObjectsChange() || forceReplan ) {
              // see if we need to replan, but only bother checking if the world objects changed
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
            _driveToPoseStatus =  ERobotDriveToPoseStatus::Error;
            PRINT_NAMED_INFO("Robot.Update.Planner.Error", "Running planner returned error status");
            AbortDrivingToPose();
            _numPlansFinished = _numPlansStarted;
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
            PRINT_NAMED_INFO("Robot.Update.Planner.CompleteWithPlan", "Running planner complete with a plan");

            _driveToPoseStatus = ERobotDriveToPoseStatus::FollowingPath;
            _numPlansFinished = _numPlansStarted;

            size_t selectedPoseIdx;
            Planning::Path newPath;

            _selectedPathPlanner->GetCompletePath(GetDriveCenterPose(), newPath, selectedPoseIdx, &_pathMotionProfile);
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
        
      
      /////////// Update visualization ////////////
      
      // Draw observed markers, but only if images are being streamed
      _blockWorld.DrawObsMarkers();
      
      // Draw All Objects by calling their Visualize() methods.
      _blockWorld.DrawAllObjects();
      
      // Always draw robot w.r.t. the origin, not in its current frame
      Pose3d robotPoseWrtOrigin = GetPose().GetWithRespectToOrigin();
      
      // Triangle pose marker
      VizManager::getInstance()->DrawRobot(GetID(), robotPoseWrtOrigin);
      
      // Full Webots CozmoBot model
      VizManager::getInstance()->DrawRobot(GetID(), robotPoseWrtOrigin, GetHeadAngle(), GetLiftAngle());
      
      // Robot bounding box
      static const ColorRGBA ROBOT_BOUNDING_QUAD_COLOR(0.0f, 0.8f, 0.0f, 0.75f);
      
      using namespace Quad;
      Quad2f quadOnGround2d = GetBoundingQuadXY(robotPoseWrtOrigin);
      const f32 zHeight = robotPoseWrtOrigin.GetTranslation().z() + WHEEL_RAD_TO_MM;
      Quad3f quadOnGround3d(Point3f(quadOnGround2d[TopLeft].x(),     quadOnGround2d[TopLeft].y(),     zHeight),
                            Point3f(quadOnGround2d[BottomLeft].x(),  quadOnGround2d[BottomLeft].y(),  zHeight),
                            Point3f(quadOnGround2d[TopRight].x(),    quadOnGround2d[TopRight].y(),    zHeight),
                            Point3f(quadOnGround2d[BottomRight].x(), quadOnGround2d[BottomRight].y(), zHeight));
    
      VizManager::getInstance()->DrawRobotBoundingBox(GetID(), quadOnGround3d, ROBOT_BOUNDING_QUAD_COLOR);
      
      /*
      // Draw 3d bounding box
      Vec3f vizTranslation = GetPose().GetTranslation();
      vizTranslation.z() += 0.5f*ROBOT_BOUNDING_Z;
      Pose3d vizPose(GetPose().GetRotation(), vizTranslation);
      
      VizManager::getInstance()->DrawCuboid(999, {ROBOT_BOUNDING_X, ROBOT_BOUNDING_Y, ROBOT_BOUNDING_Z},
                                            vizPose, ROBOT_BOUNDING_QUAD_COLOR);
      */
      
      VizManager::getInstance()->SendEndRobotUpdate();
      
      return RESULT_OK;
      
    } // Update()
    
    bool Robot::GetCurrentImage(Vision::Image& img, TimeStamp_t newerThan)
    {
      PRINT_NAMED_ERROR("Robot.GetCurrentImage.Deprecated", "");
      return false;
    }
    
    u32 Robot::GetAverageImagePeriodMS() const
    {
      return _imgFramePeriod;
    }
    
    u32 Robot::GetAverageImageProcPeriodMS() const
    {
      return _imgProcPeriod;
    }
      
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

    
    void Robot::SetPose(const Pose3d &newPose)
    {
      // Update our current pose and keep the name consistent
      const std::string name = _pose.GetName();
      _pose = newPose;
      _pose.SetName(name);
      
      ComputeDriveCenterPose(_pose, _driveCenterPose);
      
    } // SetPose()
    
    void Robot::SetHeadAngle(const f32& angle)
    {
      if (!IsValidHeadAngle(angle, &_currentHeadAngle)) {
        PRINT_NAMED_WARNING("HeadAngleOOB","angle %f  (TODO: Send correction or just recalibrate?)\n", angle);
      }
      
      // Start with canonical (untilted) headPose
      Pose3d newHeadPose(_headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-_currentHeadAngle, Y_AXIS_3D());
      newHeadPose.RotateBy(Rvec);
      newHeadPose.SetName("Camera");
      
      // Update the head camera's pose
      _visionComponent.GetCamera().SetPose(newHeadPose);
      
    } // set_headAngle()

    void Robot::ComputeLiftPose(const f32 atAngle, Pose3d& liftPose)
    {
      // Reset to canonical position
      liftPose.SetRotation(atAngle, Y_AXIS_3D());
      liftPose.SetTranslation({{LIFT_ARM_LENGTH, 0.f, 0.f}});
      
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
    
    f32 Robot::GetPitchAngle()
    {
      return _pitchAngle;
    }
        
    void Robot::SelectPlanner(const Pose3d& targetPose)
    {
      Pose2d target2d(targetPose);
      Pose2d start2d(GetPose());

      float distSquared = pow(target2d.GetX() - start2d.GetX(), 2) + pow(target2d.GetY() - start2d.GetY(), 2);

      if(distSquared < MAX_DISTANCE_FOR_SHORT_PLANNER * MAX_DISTANCE_FOR_SHORT_PLANNER) {

        Radians finalAngleDelta = targetPose.GetRotationAngle<'Z'>() - GetDriveCenterPose().GetRotationAngle<'Z'>();
        const bool withinFinalAngleTolerance = finalAngleDelta.getAbsoluteVal().ToFloat() <=
          2 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

        Radians initialTurnAngle = atan2( target2d.GetY() - GetDriveCenterPose().GetTranslation().y(),
                                          target2d.GetX() - GetDriveCenterPose().GetTranslation().x()) -
                                   GetDriveCenterPose().GetRotationAngle<'Z'>();

        const bool initialTurnAngleLarge = initialTurnAngle.getAbsoluteVal().ToFloat() >
          0.5 * PLANNER_MAINTAIN_ANGLE_THRESHOLD;

        // if we would need to turn fairly far, but our current angle is fairly close to the goal, use the
        // planner which backs up first to minimize the turn
        if( withinFinalAngleTolerance && initialTurnAngleLarge ) {
          PRINT_NAMED_INFO("Robot.SelectPlanner.ShortMinAngle",
                           "distance^2 is %f, angleDelta is %f, intiialTurnAngle is %f, selecting short min_angle planner\n",
                           distSquared,
                           finalAngleDelta.getAbsoluteVal().ToFloat(),
                           initialTurnAngle.getAbsoluteVal().ToFloat());
          _selectedPathPlanner = _shortMinAnglePathPlanner;
        }
        else {
          PRINT_NAMED_INFO("Robot.SelectPlanner.Short",
                           "distance^2 is %f, angleDelta is %f, intiialTurnAngle is %f, selecting short planner\n",
                           distSquared,
                           finalAngleDelta.getAbsoluteVal().ToFloat(),
                           initialTurnAngle.getAbsoluteVal().ToFloat());
          _selectedPathPlanner = _shortPathPlanner;
        }
      }
      else {
        PRINT_NAMED_INFO("Robot.SelectPlanner.Long", "distance^2 is %f, selecting long planner\n", distSquared);
        _selectedPathPlanner = _longPathPlanner;
      }
    }

    void Robot::SelectPlanner(const std::vector<Pose3d>& targetPoses)
    {
      if( ! targetPoses.empty() ) {
        size_t closest = IPathPlanner::ComputeClosestGoalPose(GetDriveCenterPose(), targetPoses);
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
        PRINT_NAMED_ERROR("Robot.StartDrivingToPose.OriginMisMatch",
                          "Could not get target pose w.r.t. robot %d's origin.\n", GetID());
        _driveToPoseStatus = ERobotDriveToPoseStatus::Error;
        return RESULT_FAIL;
      }

      SelectPlanner(targetPoseWrtOrigin);

      // Compute drive center pose of given target robot pose
      Pose3d targetDriveCenterPose;
      ComputeDriveCenterPose(targetPoseWrtOrigin, targetDriveCenterPose);

      // Compute drive center pose for start pose
      EComputePathStatus status = _selectedPathPlanner->ComputePath(GetDriveCenterPose(), targetDriveCenterPose);
      if( status == EComputePathStatus::Error ) {
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
                                     size_t* selectedPoseIndexPtr,
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

      EComputePathStatus status = _selectedPathPlanner->ComputePath(GetDriveCenterPose(), targetDriveCenterPoses);
      if( status == EComputePathStatus::Error ) {
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
    
    Result Robot::PlaceObjectOnGround(const bool useManualSpeed)
    {
      if(!IsCarryingObject()) {
        PRINT_NAMED_ERROR("Robot.PlaceObjectOnGround.NotCarryingObject",
                          "Robot told to place object on ground, but is not carrying an object.\n");
        return RESULT_FAIL;
      }
      
      _usingManualPathSpeed = useManualSpeed;
      _lastPickOrPlaceSucceeded = false;
      
      return SendRobotMessage<Anki::Cozmo::PlaceObjectOnGround>(0, 0, 0, useManualSpeed);
    }
    
    u8 Robot::PlayAnimation(const std::string& animName, const u32 numLoops)
    {
      u8 tag = _animationStreamer.SetStreamingAnimation(animName, numLoops);
      _lastPlayedAnimationId = animName;
      return tag;
    }
    
    Result Robot::SetIdleAnimation(const std::string &animName)
    {
      return _animationStreamer.SetIdleAnimation(animName);
    }
    
    void Robot::SetProceduralFace(const ProceduralFace& face)
    {
      // First one
      if(_lastProceduralFace.GetTimeStamp() == 0) {
        _lastProceduralFace = face;
        _lastProceduralFace.MarkAsSentToRobot(true);
        _proceduralFace.MarkAsSentToRobot(true);
      } else {
        if(_proceduralFace.HasBeenSentToRobot()) {
          // If the current face has already been sent, make it the
          // last procedural face (sent). Otherwise, we'll just
          // replace the current face and leave "last" as is.
          std::swap(_lastProceduralFace, _proceduralFace);
        }
        _proceduralFace = face;
        _proceduralFace.MarkAsSentToRobot(false);
      }
    }
    
    void Robot::MarkProceduralFaceAsSent()
    {
      _proceduralFace.MarkAsSentToRobot(true);
    }
    
    const std::string Robot::GetStreamingAnimationName() const
    {
      return _animationStreamer.GetStreamingAnimationName();
    }
    
    Result Robot::PlaySound(const std::string& soundName, u8 numLoops, u8 volume)
    {
      Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::PlaySound(soundName, numLoops, volume)));
      
      //CozmoEngineSignals::PlaySoundForRobotSignal().emit(GetID(), soundName, numLoops, volume);
      return RESULT_OK;
    } // PlaySound()
      
      
    void Robot::StopSound()
    {
      Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::StopSound()));
    } // StopSound()
      
      
    Result Robot::StopAnimation()
    {
      return SendAbortAnimation();
    }

    // Read the animations in a dir
    void Robot::ReadAnimationFile(const char* filename, std::string& animationId)
    {
      Json::Value animDefs;
      const bool success = _dataPlatform->readAsJson(filename, animDefs);
      if (success && !animDefs.empty()) {
        PRINT_NAMED_INFO("Robot.ReadAnimationFile", "reading %s", filename);
        _cannedAnimations.DefineFromJson(animDefs, animationId);
      }

    }

    
    // Read the animations in a dir
    void Robot::ReadAnimationDir()
    {
      if (_dataPlatform == nullptr) { return; }
      static const std::regex jsonFilenameMatcher("[^.].*\\.json\0");
      SoundManager::getInstance()->LoadSounds(_dataPlatform);
      FaceAnimationManager::getInstance()->ReadFaceAnimationDir(_dataPlatform);
      
      const std::string animationFolder =
        _dataPlatform->pathToResource(Util::Data::Scope::Resources, "assets/animations/");
      std::string animationId;
      s32 loadedFileCount = 0;
      DIR* dir = opendir(animationFolder.c_str());
      if ( dir != nullptr) {
        dirent* ent = nullptr;
        while ( (ent = readdir(dir)) != nullptr) {
          
          if (ent->d_type == DT_REG && std::regex_match(ent->d_name, jsonFilenameMatcher)) {
            std::string fullFileName = animationFolder + ent->d_name;
            struct stat attrib{0};
            int result = stat(fullFileName.c_str(), &attrib);
            if (result == -1) {
              PRINT_NAMED_WARNING("Robot.ReadAnimationFile", "could not get mtime for %s", fullFileName.c_str());
              continue;
            }
            bool loadFile = false;
            auto mapIt = _loadedAnimationFiles.find(fullFileName);
            if (mapIt == _loadedAnimationFiles.end()) {
              _loadedAnimationFiles.insert({fullFileName, attrib.st_mtimespec.tv_sec});
              loadFile = true;
            } else {
              if (mapIt->second < attrib.st_mtimespec.tv_sec) {
                mapIt->second = attrib.st_mtimespec.tv_sec;
                loadFile = true;
              } else {
                //PRINT_NAMED_INFO("Robot.ReadAnimationFile", "old time stamp for %s", fullFileName.c_str());
              }
            }
            if (loadFile) {
              ReadAnimationFile(fullFileName.c_str(), animationId);
              ++loadedFileCount;
            }
          }
        }
        closedir(dir);
      } else {
        PRINT_NAMED_INFO("Robot.ReadAnimationFile", "folder not found %s", animationFolder.c_str());
      }

      // Tell UI about available animations
      if (HasExternalInterface()) {
        std::vector<std::string> animNames(_cannedAnimations.GetAnimationNames());
        for (std::vector<std::string>::iterator i=animNames.begin(); i != animNames.end(); ++i) {
          _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::AnimationAvailable(*i)));
        }
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
        PRINT_NAMED_ERROR("Robot.LocalizeToObject.ExistingObjectPieceNullPointer", "\n");
        return RESULT_FAIL;
      }
      
      if(!existingObject->CanBeUsedForLocalization()) {
        PRINT_NAMED_ERROR("Robot.LocalizeToObject.UnlocalizedObject",
                          "Refusing to localize to object %d, which claims not to be localizable.\n",
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
                            "Time %d\n", seenObject->GetLastObservedTime());
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
                            "Could not get RobotPoseStamp w.r.t. seen object pose.\n");
          return RESULT_FAIL;
        }
        
        liftAngle = posePtr->GetLiftAngle();
        headAngle = posePtr->GetHeadAngle();
      }
      
      // Make the computed robot pose use the existing mat piece as its parent
      robotPoseWrtObject.SetParent(&existingObject->GetPose());
      //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));
      
#     if 0
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
                              "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)\n",
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
#     endif
      
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
        if((lastResult = AddVisionOnlyPoseToHistory(existingObject->GetLastObservedTime(),
                                                    robotPoseWrtOrigin.GetTranslation().x(),
                                                    robotPoseWrtOrigin.GetTranslation().y(),
                                                    robotPoseWrtOrigin.GetTranslation().z(),
                                                    robotPoseWrtOrigin.GetRotationAngle<'Z'>().ToFloat(),
                                                    headAngle, liftAngle)) != RESULT_OK)
        {
          PRINT_NAMED_ERROR("Robot.LocalizeToObject.FailedAddingVisionOnlyPoseToHistory", "\n");
          return lastResult;
        }
      }
      
      // If the robot's world origin is about to change by virtue of being localized
      // to existingObject, rejigger things so anything seen while the robot was
      // rooted to this world origin will get updated to be w.r.t. the new origin.
      if(_worldOrigin != &existingObject->GetPose().FindOrigin())
      {
        PRINT_NAMED_INFO("Robot.LocalizeToObject.RejiggeringOrigins",
                         "Robot %d's current world origin is %s, about to "
                         "localize to world origin %s.",
                         GetID(),
                         _worldOrigin->GetName().c_str(),
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
        _worldOrigin->SetName("RejiggeredOrigin");
        
        assert(_worldOrigin->IsOrigin() == false);
        
        // Now that the previous origin is hooked up to the new one (which is
        // now the old one's parent), point the worldOrigin at the new one.
        _worldOrigin = const_cast<Pose3d*>(_worldOrigin->GetParent()); // TODO: Avoid const cast?
        
        // Now we need to go through all objects whose poses have been adjusted
        // by this origin switch and notify the outside world of the change.
        _blockWorld.UpdateObjectOrigins(oldOrigin, _worldOrigin);
        
      } // if(_worldOrigin != &existingObject->GetPose().FindOrigin())
      
      
      if(nullptr != posePtr)
      {
        // Update the computed historical pose as well so that subsequent block
        // pose updates use obsMarkers whose camera's parent pose is correct.
        // Note again that we store the pose w.r.t. the origin in history.
        // TODO: Should SetPose() do the flattening w.r.t. origin?
        posePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, liftAngle, liftAngle);
      }
      
      // Compute the new "current" pose from history which uses the
      // past vision-based "ground truth" pose we just computed.
      assert(&existingObject->GetPose().FindOrigin() == _worldOrigin);
      assert(_worldOrigin != nullptr);
      if(UpdateCurrPoseFromHistory(*_worldOrigin) == false) {
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
      
      // Send the ground truth pose that was computed instead of the new current
      // pose and let the robot deal with updating its current pose based on the
      // history that it keeps.
      SendAbsLocalizationUpdate();
      
      return RESULT_OK;
    } // LocalizeToObject()
    
    
    Result Robot::LocalizeToMat(const MatPiece* matSeen, MatPiece* existingMatPiece)
    {
      Result lastResult;
      
      if(matSeen == nullptr) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.MatSeenNullPointer", "\n");
        return RESULT_FAIL;
      } else if(existingMatPiece == nullptr) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.ExistingMatPieceNullPointer", "\n");
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
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.CouldNotFindHistoricalPose", "Time %d\n", matSeen->GetLastObservedTime());
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
       PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.CouldNotComputeHistoricalPose", "Time %d\n", matSeen->GetLastObservedTime());
       return false;
       }
       */
      
      // Get the pose of the robot with respect to the observed mat piece
      Pose3d robotPoseWrtMat;
      if(robotPoseAtObsTime.GetWithRespectTo(matSeen->GetPose(), robotPoseWrtMat) == false) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.MatPoseOriginMisMatch",
                          "Could not get RobotPoseStamp w.r.t. matPose.\n");
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
                              "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)\n",
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
                         "Localizing robot %d to fixed %s mat for the first time.\n",
                         GetID(), ObjectTypeToString(existingMatPiece->GetType()));
        
        if((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                            "Failed to update robot %d's pose origin when (re-)localizing it.\n", GetID());
          return lastResult;
        }
        
        _localizedToFixedObject = true;
      }
      else if(IsLocalized() == false) {
        // If the robot is not yet localized, it is about to be, so we need to
        // update pose origins so that anything it has seen so far becomes rooted
        // to this mat's origin (whether mat is fixed or not)
        PRINT_NAMED_INFO("Robot.LocalizeToMat.LocalizingRobotFirstTime",
                         "Localizing robot %d for the first time (to %s mat).\n",
                         GetID(), ObjectTypeToString(existingMatPiece->GetType()));
        
        if((lastResult = UpdateWorldOrigin(robotPoseWrtMat)) != RESULT_OK) {
          PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetPoseOriginFailure",
                            "Failed to update robot %d's pose origin when (re-)localizing it.\n", GetID());
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
      //RobotPoseStamp p(robot->GetPoseFrameID(), robotPoseWrtMat.GetWithRespectToOrigin(), posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
      Pose3d robotPoseWrtOrigin = robotPoseWrtMat.GetWithRespectToOrigin();
      
      if((lastResult = AddVisionOnlyPoseToHistory(existingMatPiece->GetLastObservedTime(),
                                                  robotPoseWrtOrigin.GetTranslation().x(),
                                                  robotPoseWrtOrigin.GetTranslation().y(),
                                                  robotPoseWrtOrigin.GetTranslation().z(),
                                                  robotPoseWrtOrigin.GetRotationAngle<'Z'>().ToFloat(),
                                                  posePtr->GetHeadAngle(),
                                                  posePtr->GetLiftAngle())) != RESULT_OK)
      {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedAddingVisionOnlyPoseToHistory", "\n");
        return lastResult;
      }
      
      
      // Update the computed historical pose as well so that subsequent block
      // pose updates use obsMarkers whose camera's parent pose is correct.
      // Note again that we store the pose w.r.t. the origin in history.
      // TODO: Should SetPose() do the flattening w.r.t. origin?
      posePtr->SetPose(GetPoseFrameID(), robotPoseWrtOrigin, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
      
      // Compute the new "current" pose from history which uses the
      // past vision-based "ground truth" pose we just computed.
      if(UpdateCurrPoseFromHistory(existingMatPiece->GetPose()) == false) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.FailedUpdateCurrPoseFromHistory", "\n");
        return RESULT_FAIL;
      }
      
      // Mark the robot as now being localized to this mat
      // NOTE: this should be _after_ calling AddVisionOnlyPoseToHistory, since
      //    that function checks whether the robot is already localized
      lastResult = SetLocalizedTo(existingMatPiece);
      if(RESULT_OK != lastResult) {
        PRINT_NAMED_ERROR("Robot.LocalizeToMat.SetLocalizedToFail", "\n");
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
      VizManager::getInstance()->ErasePath(_ID);
      _pdo->ClearPath();
      return SendMessage(RobotInterface::EngineToRobot(RobotInterface::ClearPath(0)));
    }
    
    // Sends a path to the robot to be immediately executed
    Result Robot::ExecutePath(const Planning::Path& path, const bool useManualSpeed)
    {
      Result lastResult = RESULT_FAIL;
      
      if (path.GetNumSegments() == 0) {
        PRINT_NAMED_WARNING("Robot.ExecutePath.EmptyPath", "\n");
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
        VizManager::getInstance()->DrawPath(_ID, path, NamedColors::EXECUTED_PATH);
        
      }
      
      return lastResult;
    }
  
    
    Result Robot::SetOnRamp(bool t)
    {
      if(t == _onRamp) {
        // Nothing to do
        return RESULT_OK;
      }
      
      // We are either transition onto or off of a ramp
      
      Ramp* ramp = dynamic_cast<Ramp*>(_blockWorld.GetObjectByIDandFamily(_rampID, ObjectFamily::Ramp));
      if(ramp == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetOnRamp.NoRampWithID",
                          "Robot %d is transitioning on/off of a ramp, but Ramp object with ID=%d not found in the world.\n",
                          _ID, _rampID.GetValue());
        return RESULT_FAIL;
      }
      
      assert(_rampDirection == Ramp::ASCENDING || _rampDirection == Ramp::DESCENDING);
      
      const bool transitioningOnto = (t == true);
      
      if(transitioningOnto) {
        // Record start (x,y) position coming from robot so basestation can
        // compute actual (x,y,z) position from upcoming odometry updates
        // coming from robot (which do not take slope of ramp into account)
        _rampStartPosition = {{_pose.GetTranslation().x(), _pose.GetTranslation().y()}};
        _rampStartHeight   = _pose.GetTranslation().z();
        
        PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOntoRamp",
                         "Robot %d transitioning onto ramp %d, using start (%.1f,%.1f,%.1f)\n",
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
            PRINT_NAMED_ERROR("Robot.SetOnRamp.UnexpectedRampDirection",
                              "When transitioning on/off ramp, expecting the ramp direction to be either "
                              "ASCENDING or DESCENDING, not %d.\n", _rampDirection);
            return RESULT_FAIL;
        }
        
        _rampDirection = Ramp::UNKNOWN;
        
        const TimeStamp_t timeStamp = _poseHistory->GetNewestTimeStamp();
        
        PRINT_NAMED_INFO("Robot.SetOnRamp.TransitionOffRamp",
                         "Robot %d transitioning off of ramp %d, at (%.1f,%.1f,%.1f) @ %.1fdeg, timeStamp = %d\n",
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
          PRINT_NAMED_ERROR("Robot.SetOnRamp.SendAbsLocUpdateFailed",
                            "Robot %d failed to send absolute localization update.\n", _ID);
          return lastResult;
        }

      } // if/else transitioning onto ramp
      
      _onRamp = t;
      
      return RESULT_OK;
      
    } // SetOnPose()
    
    Result Robot::DockWithObject(const ObjectID objectID,
                                 const Vision::KnownMarker* marker,
                                 const Vision::KnownMarker* marker2,
                                 const DockAction dockAction,
                                 const f32 placementOffsetX_mm,
                                 const f32 placementOffsetY_mm,
                                 const f32 placementOffsetAngle_rad,
                                 const bool useManualSpeed)
    {
      return DockWithObject(objectID,
                            marker,
                            marker2,
                            dockAction,
                            0, 0, u8_MAX,
                            placementOffsetX_mm, placementOffsetY_mm, placementOffsetAngle_rad,
                            useManualSpeed);
    }
    
    Result Robot::DockWithObject(const ObjectID objectID,
                                 const Vision::KnownMarker* marker,
                                 const Vision::KnownMarker* marker2,
                                 const DockAction dockAction,
                                 const u16 image_pixel_x,
                                 const u16 image_pixel_y,
                                 const u8 pixel_radius,
                                 const f32 placementOffsetX_mm,
                                 const f32 placementOffsetY_mm,
                                 const f32 placementOffsetAngle_rad,
                                 const bool useManualSpeed)
    {
      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(objectID));
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
                          "Specified dock marker must be a child of the specified dock object.\n");
        return RESULT_FAIL;
      }

      _usingManualPathSpeed = useManualSpeed;
      _lastPickOrPlaceSucceeded = false;
      
      // Sends a message to the robot to dock with the specified marker
      // that it should currently be seeing. If pixel_radius == u8_MAX,
      // the marker can be seen anywhere in the image (same as above function), otherwise the
      // marker's center must be seen at the specified image coordinates
      // with pixel_radius pixels.
      Result sendResult = SendRobotMessage<::Anki::Cozmo::DockWithObject>(0.0f, dockAction, useManualSpeed);
      
      
      if(sendResult == RESULT_OK) {
        
        // When we are "docking" with a ramp or crossing a bridge, we
        // don't want to worry about the X angle being large (since we
        // _expect_ it to be large, since the markers are facing upward).
        const bool checkAngleX = !(dockAction == DockAction::DA_RAMP_ASCEND  ||
                                   dockAction == DockAction::DA_RAMP_DESCEND ||
                                   dockAction == DockAction::DA_CROSS_BRIDGE);
        
        // Tell the VisionSystem to start tracking this marker:
        _visionComponent.SetMarkerToTrack(marker->GetCode(), marker->GetSize(),
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
      ObservableObject* object = _blockWorld.GetObjectByID(carryObjectID);
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetCarryingObject",
          "Object %d no longer exists in the world. Can't set it as robot's carried object.", carryObjectID.GetValue());
      } else {
        ActionableObject* carriedObject = dynamic_cast<ActionableObject*>(object);
        if(carriedObject == nullptr) {
          // This really should not happen
          PRINT_NAMED_ERROR("Robot.SetCarryingObject",
            "Object %d could not be cast as an ActionableObject, so cannot mark it as carried.", carryObjectID.GetValue());
        } else {
          if(carriedObject->IsBeingCarried()) {
            PRINT_NAMED_WARNING("Robot.SetCarryingObject",
              "Robot %d is about to mark object %d as carried but that object already thinks it is being carried.",
              GetID(), carryObjectID.GetValue());
          }
          carriedObject->SetBeingCarried(true);
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
        } // if/else (carriedObject == nullptr)
      }
    }
    
    void Robot::UnSetCarryingObjects()
    {
      std::set<ObjectID> carriedObjectIDs = GetCarryingObjects();
      for (auto& objID : carriedObjectIDs) {
        ObservableObject* object = _blockWorld.GetObjectByID(objID);
        if(object == nullptr) {
          PRINT_NAMED_ERROR("Robot.UnSetCarryingObjects",
                            "Object %d robot %d thought it was carrying no longer exists in the world.\n",
                            objID.GetValue(), GetID());
        } else {
          ActionableObject* carriedObject = dynamic_cast<ActionableObject*>(object);
          if(carriedObject == nullptr) {
            // This really should not happen
            PRINT_NAMED_ERROR("Robot.UnSetCarryingObjects",
                              "Carried object %d could not be cast as an ActionableObject.\n",
                              objID.GetValue());
          } else if(carriedObject->IsBeingCarried() == false) {
            PRINT_NAMED_WARNING("Robot.UnSetCarryingObjects",
                                "Robot %d thinks it is carrying object %d but that object "
                                "does not think it is being carried.\n", GetID(), objID.GetValue());
            
          } else {
            carriedObject->SetBeingCarried(false);
          }
        }
      }
      
      // Tell the robot it's not carrying anything
      if (_carryingObjectID.IsSet()) {
        SendSetCarryState(CarryState::CARRY_NONE);
      }

      // Even if the above failed, still mark the robot's carry ID as unset
      _carryingObjectID.UnSet();
      _carryingObjectOnTopID.UnSet();
    }
    
    Result Robot::SetObjectAsAttachedToLift(const ObjectID& objectID, const Vision::KnownMarker* objectMarker)
    {
      if(!objectID.IsSet()) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectIDNotSet",
                          "No docking object ID set, but told to pick one up.\n");
        return RESULT_FAIL;
      }
      
      if(objectMarker == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.NoDockMarkerSet",
                          "No docking marker set, but told to pick up object.\n");
        return RESULT_FAIL;
      }
      
      if(IsCarryingObject()) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.AlreadyCarryingObject",
                          "Already carrying an object, but told to pick one up.\n");
        return RESULT_FAIL;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(objectID));
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectDoesNotExist",
                          "Dock object with ID=%d no longer exists for picking up.\n", objectID.GetValue());
        return RESULT_FAIL;
      }
      
      // Base the object's pose relative to the lift on how far away the dock
      // marker is from the center of the block
      // TODO: compute the height adjustment per object or at least use values from cozmoConfig.h
      Pose3d objectPoseWrtLiftPose;
      if(object->GetPose().GetWithRespectTo(_liftPose, objectPoseWrtLiftPose) == false) {
        PRINT_NAMED_ERROR("Robot.PickUpDockObject.ObjectAndLiftPoseHaveDifferentOrigins",
                          "Object robot is picking up and robot's lift must share a common origin.\n");
        return RESULT_FAIL;
      }
      
      objectPoseWrtLiftPose.SetTranslation({{objectMarker->GetPose().GetTranslation().Length() +
        LIFT_FRONT_WRT_WRIST_JOINT, 0.f, -12.5f}});
      
      // make part of the lift's pose chain so the object will now be relative to
      // the lift and move with the robot
      objectPoseWrtLiftPose.SetParent(&_liftPose);
      

      // If we know there's an object on top of the object we are picking up,
      // mark it as being carried too
      // TODO: Do we need to be able to handle non-actionable objects on top of actionable ones?

      const f32 STACKED_HEIGHT_TOL_MM = 15.f; // TODO: make this a parameter somewhere
      ObservableObject* objectOnTop = _blockWorld.FindObjectOnTopOf(*object, STACKED_HEIGHT_TOL_MM);
      if(objectOnTop != nullptr) {
        ActionableObject* actionObjectOnTop = dynamic_cast<ActionableObject*>(objectOnTop);
        if(actionObjectOnTop != nullptr) {
          Pose3d onTopPoseWrtCarriedPose;
          if(actionObjectOnTop->GetPose().GetWithRespectTo(object->GetPose(), onTopPoseWrtCarriedPose) == false)
          {
            PRINT_NAMED_WARNING("Robot.SetObjectAsAttachedToLift",
                                "Found object on top of carried object, but could not get its "
                                "pose w.r.t. the carried object.\n");
          } else {
            PRINT_NAMED_INFO("Robot.SetObjectAsAttachedToLift",
                             "Setting object %d on top of carried object as also being carried.\n",
                             actionObjectOnTop->GetID().GetValue());
            onTopPoseWrtCarriedPose.SetParent(&object->GetPose());
            actionObjectOnTop->SetPose(onTopPoseWrtCarriedPose);
            _carryingObjectOnTopID = actionObjectOnTop->GetID();
            actionObjectOnTop->SetBeingCarried(true);
          }
        }
      } else {
        _carryingObjectOnTopID.UnSet();
      }
      
      SetCarryingObject(objectID); // also marks the object as carried
      _carryingMarker   = objectMarker;

      // Don't actually change the object's pose until we've checked for objects on top
      object->SetPose(objectPoseWrtLiftPose);

      return RESULT_OK;
      
    } // AttachObjectToLift()
    
    
    Result Robot::SetCarriedObjectAsUnattached()
    {
      if(IsCarryingObject() == false) {
        PRINT_NAMED_WARNING("Robot.SetCarriedObjectAsUnattached.CarryingObjectNotSpecified",
                            "Robot not carrying object, but told to place one. (Possibly actually rolling or balancing or popping a wheelie.\n");
        return RESULT_FAIL;
      }
      
      ActionableObject* object = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(_carryingObjectID));
      
      if(object == nullptr)
      {
        // This really should not happen.  How can a object being carried get deleted?
        PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.CarryingObjectDoesNotExist",
                          "Carrying object with ID=%d no longer exists.\n", _carryingObjectID.GetValue());
        return RESULT_FAIL;
      }
     
      Pose3d placedPose;
      if(object->GetPose().GetWithRespectTo(_pose.FindOrigin(), placedPose) == false) {
        PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.OriginMisMatch",
                          "Could not get carrying object's pose relative to robot's origin.\n");
        return RESULT_FAIL;
      }
      object->SetPose(placedPose);
      
      PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached.ObjectPlaced",
                       "Robot %d successfully placed object %d at (%.2f, %.2f, %.2f).\n",
                       _ID, object->GetID().GetValue(),
                       object->GetPose().GetTranslation().x(),
                       object->GetPose().GetTranslation().y(),
                       object->GetPose().GetTranslation().z());

      UnSetCarryingObjects(); // also sets carried objects as not being carried anymore
      _carryingMarker = nullptr;
      
      if(_carryingObjectOnTopID.IsSet()) {
        ActionableObject* objectOnTop = dynamic_cast<ActionableObject*>(_blockWorld.GetObjectByID(_carryingObjectOnTopID));
        if(objectOnTop == nullptr)
        {
          // This really should not happen.  How can a object being carried get deleted?
          PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached",
                            "Object on top of carrying object with ID=%d no longer exists.\n",
                            _carryingObjectOnTopID.GetValue());
          return RESULT_FAIL;
        }
        
        Pose3d placedPoseOnTop;
        if(objectOnTop->GetPose().GetWithRespectTo(_pose.FindOrigin(), placedPoseOnTop) == false) {
          PRINT_NAMED_ERROR("Robot.SetCarriedObjectAsUnattached.OriginMisMatch",
                            "Could not get carrying object's pose relative to robot's origin.\n");
          return RESULT_FAIL;
          
        }
        objectOnTop->SetPose(placedPoseOnTop);
        objectOnTop->SetBeingCarried(false);
        _carryingObjectOnTopID.UnSet();
        PRINT_NAMED_INFO("Robot.SetCarriedObjectAsUnattached", "Updated object %d on top of carried object.\n",
                         objectOnTop->GetID().GetValue());
      }
      
      return RESULT_OK;
      
    } // UnattachCarriedObject()
    
    // ============ Messaging ================
    
    Result Robot::SendMessage(const RobotInterface::EngineToRobot& msg, bool reliable, bool hot) const
    {
      Result sendResult = _msgHandler->SendMessage(_ID, msg, reliable, hot);
      if(sendResult != RESULT_OK) {
        PRINT_NAMED_ERROR("Robot.SendMessage", "Robot %d failed to send a message.", _ID);
      }
      return sendResult;
    }
      
    // Sync time with physical robot and trigger it robot to send back camera calibration
    Result Robot::SendSyncTime() const
    {

      Result result = SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::SyncTime(_ID, BaseStationTimer::getInstance()->GetCurrentTimeStamp())));
      
      if(result == RESULT_OK) {
        result = SendMessage(RobotInterface::EngineToRobot(
          RobotInterface::ImageRequest(ImageSendMode::Stream, ImageResolution::CVGA)));
        
        // Reset pose on connect
        PRINT_NAMED_INFO("Robot.SendSyncTime", "Setting pose to (0,0,0)");
        Pose3d zeroPose(0, Z_AXIS_3D(), {0,0,0});
        return SendAbsLocalizationUpdate(zeroPose, 0, GetPoseFrameID());
      } else {
        PRINT_NAMED_WARNING("Robot.SendSyncTime.FailedToSend","");
      }
      
      return result;
    }
    
    // Sends a path to the robot to be immediately executed
    Result Robot::SendExecutePath(const Planning::Path& path, const bool useManualSpeed) const
    {
      // Send start path execution message
      PRINT_NAMED_INFO("Robot::SendExecutePath", "sending start execution message (pathID = %d, manualSpeed == %d)", _lastSentPathID, useManualSpeed);
      return SendMessage(RobotInterface::EngineToRobot(RobotInterface::ExecutePath(_lastSentPathID, useManualSpeed)));
    }
    
    Result Robot::SendAbsLocalizationUpdate(const Pose3d&        pose,
                                            const TimeStamp_t&   t,
                                            const PoseFrameID_t& frameId) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::AbsoluteLocalizationUpdate(
          t,
          frameId,
          pose.GetTranslation().x(),
          pose.GetTranslation().y(),
          pose.GetRotation().GetAngleAroundZaxis().ToFloat()
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
      return SendRobotMessage<RobotInterface::ImuRequest>(length_ms);
    }

    Result Robot::SendEnablePickupDetect(const bool enable) const
    {
      return SendRobotMessage<RobotInterface::EnablePickupDetect>(enable);
    }
    
    void Robot::SetSaveStateMode(const SaveMode_t mode)
    {
      _stateSaveMode = mode;
    }

      
    void Robot::SetSaveImageMode(const SaveMode_t mode)
    {
      _imageSaveMode = mode;
    }
    
    Result Robot::ProcessImage(const Vision::ImageRGB& image)
    {
      Result lastResult = RESULT_OK;
      
      if (_imageSaveMode != SAVE_OFF) {
        
        // Make sure image capture folder exists
        std::string imageCaptureDir = _dataPlatform->pathToResource(Util::Data::Scope::Cache, AnkiUtil::kP_IMG_CAPTURE_DIR);
        if (!Util::FileUtils::CreateDirectory(imageCaptureDir, false, true)) {
          PRINT_NAMED_WARNING("Robot.ProcessImage.CreateDirFailed","%s",imageCaptureDir.c_str());
        }
        
        // Write image to file (recompressing as jpeg again!)
        static u32 imgCounter = 0;
        char imgFilename[256];
        std::vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
        compression_params.push_back(90);
        sprintf(imgFilename, "%s/cozmo%d_%dms_%d.jpg", imageCaptureDir.c_str(), GetID(), image.GetTimestamp(), imgCounter++);
        imwrite(imgFilename, image.get_CvMat_(), compression_params);
        
        if (_imageSaveMode == SAVE_ONE_SHOT) {
          _imageSaveMode = SAVE_OFF;
        }
      }
      
      // Compute framerate
      if (_lastImgTimeStamp > 0) {
        const f32 imgFramerateAvgCoeff = 0.25f;
        _imgFramePeriod = _imgFramePeriod * (1.f-imgFramerateAvgCoeff) + (image.GetTimestamp() - _lastImgTimeStamp) * imgFramerateAvgCoeff;
      }
      _lastImgTimeStamp = image.GetTimestamp();
      
      const f32 imgProcrateAvgCoeff = 0.9f;
      _imgProcPeriod = (_imgProcPeriod * (1.f-imgProcrateAvgCoeff) +
                        _visionComponent.GetProcessingPeriod() * imgProcrateAvgCoeff);
      
      // For now, we need to reassemble a RobotState message to provide the
      // vision system (because it is just copied from the embedded vision
      // implementation on the robot). We'll just reassemble that from
      // pose history, but this should not be necessary forever.
      // NOTE: only the info found in pose history will be valid in the state message!
      RobotState robotState;
      
      RobotPoseStamp p;
      TimeStamp_t actualTimestamp;
      //lastResult = _poseHistory->GetRawPoseAt(image.GetTimestamp(), actualTimestamp, p);
      lastResult = _poseHistory->ComputePoseAt(image.GetTimestamp(), actualTimestamp, p, true); // TODO: use interpolation??
      if(lastResult != RESULT_OK) {
      PRINT_NAMED_ERROR("Robot.ProcessImage.PoseHistoryFail",
                        "Unable to get computed pose at image timestamp of %d.\n", image.GetTimestamp());
        return lastResult;
      }
      
      robotState.timestamp     = actualTimestamp;
      robotState.pose_frame_id = p.GetFrameId();
      robotState.headAngle     = p.GetHeadAngle();
      robotState.liftAngle     = p.GetLiftAngle();
      robotState.pose.x        = p.GetPose().GetTranslation().x();
      robotState.pose.y        = p.GetPose().GetTranslation().y();
      robotState.pose.z        = p.GetPose().GetTranslation().z();
      robotState.pose.angle    = p.GetPose().GetRotationAngle<'Z'>().ToFloat();
      
      _visionComponent.SetNextImage(image, robotState);
      
      // TEST:
      // Draw ground plane
      {
        // Define ROI quad on ground plane, in robot-centric coordinates (origin is *)
        // The region is "d" mm long and starts "d0" mm from the robot origin.
        // It is "w_close" mm wide at the end close to the robot and "w_far" mm
        // wide at the opposite end
        //                              _____
        //  +---------+    _______------     |
        //  | Robot   |   |                  |
        //  |       * |   | w_close          | w_far
        //  |         |   |_______           |
        //  +---------+           ------_____|
        //
        //          |<--->|<---------------->|
        //            d0           d
        //
        const f32 d0       = 50.f;  // distance to close side of ground quad
        const f32 d        = 100.f; // size of quad along robot's X direction
        const f32 w_close  = 30.f;  // size of quad along robot's Y direction
        const f32 w_far    = 80.f;
        
        // Ground plane quad in robot coordinates
        // Note that the z coordinate is actually 0, but in the mapping to the
        // image plane below, we are actually doing K[R t]* [Px Py Pz 1]',
        // and Pz == 0 and we thus drop out the third column, making it
        // K[R t] * [Px Py 0 1]' or H * [Px Py 1]', so for convenience, we just
        // go ahead and fill in that 1 here:
        Quad3f groundQuad({d0 + d,  0.5f*w_far,   1.f},
                          {d0    ,  0.5f*w_close, 1.f},
                          {d0 + d, -0.5f*w_far,   1.f},
                          {d0    , -0.5f*w_close, 1.f});
        
        // Get the robot origin w.r.t. the camera position at the time the image
        // was captured (this takes into account the head angle at that time)
        // TODO: Use historical camera
        Pose3d robotPoseWrtCamera;
        bool result = GetPose().GetWithRespectTo(GetVisionComponent().GetCamera().GetPose(), robotPoseWrtCamera);
        assert(result == true); // this really shouldn't fail! camera has to be in the robot's pose tree
        const RotationMatrix3d& R = robotPoseWrtCamera.GetRotationMatrix();
        const Vec3f& T = robotPoseWrtCamera.GetTranslation();
        
        const Matrix_3x3f K = GetVisionComponent().GetCameraCalibration().GetCalibrationMatrix();
        
        // Construct the homography mapping points on the ground plane into the
        // image plane
        Matrix_3x3f H = K*Matrix_3x3f{R.GetColumn(0),R.GetColumn(1),T};
        
        // Project ground quad in camera image
        // (This could be done by Camera::ProjectPoints, but that would duplicate
        //  the computation of H we did above, which here we need to use below)
        Quad2f imgGroundQuad;
        for(Quad::CornerName iCorner = Quad::CornerName::FirstCorner;
            iCorner != Quad::CornerName::NumCorners; ++iCorner)
        {
          Point3f temp = H * groundQuad[iCorner];
          ASSERT_NAMED(temp.z() > 0.f, "Projected ground quad points should have z > 0.");
          const f32 divisor = 1.f / temp.z();
          imgGroundQuad[iCorner].x() = temp.x() * divisor;
          imgGroundQuad[iCorner].y() = temp.y() * divisor;
        }
        

        Vision::ImageRGB overheadImg;
        
        // Need to apply a shift after the homography to put things in image
        // coordinates with (0,0) at the upper left (since groundQuad's origin
        // is not upper left). Also mirror Y coordinates since we are looking
        // from above, not below
        Matrix_3x3f InvShift{
          1.f, 0.f, d0, // Negated b/c we're using inv(Shift)
          0.f,-1.f, w_far*0.5f,
          0.f, 0.f, 1.f};
        
        // Note that we're applying the inverse homography, so we're doing
        //  inv(Shift * inv(H)), which is the same as  (H * inv(Shift))
        cv::warpPerspective(image.get_CvMat_(), overheadImg.get_CvMat_(), (H*InvShift).get_CvMatx_(),
                            cv::Size(d, w_far), cv::INTER_LINEAR | cv::WARP_INVERSE_MAP);
        
        { // DEBUG
          Vision::ImageRGB dispImg;
          image.CopyTo(dispImg);
          dispImg.DrawQuad(imgGroundQuad, NamedColors::RED, 1);
          dispImg.Display("GroundQuad");
          overheadImg.Display("OverheadView");
        }
      }
                       
      return lastResult;
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
        (Point2f){{ROBOT_BOUNDING_X_FRONT, -0.5f*ROBOT_BOUNDING_Y}},
        {{ROBOT_BOUNDING_X_FRONT,  0.5f*ROBOT_BOUNDING_Y}},
        {{ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X, -0.5f*ROBOT_BOUNDING_Y}},
        {{ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X,  0.5f*ROBOT_BOUNDING_Y}});

      Quad2f boundingQuad(CanonicalBoundingBoxXY);
      if(padding_mm != 0.f) {
        Quad2f paddingQuad((const Point2f){{ padding_mm, -padding_mm}},
                           {{ padding_mm,  padding_mm}},
                           {{-padding_mm, -padding_mm}},
                           {{-padding_mm,  padding_mm}});
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
      return (std::sin(GetLiftAngle()) * LIFT_ARM_LENGTH) + LIFT_BASE_POSITION[2] + LIFT_FORK_HEIGHT_REL_TO_ARM_END;
    }
    
    Result Robot::RequestIMU(const u32 length_ms) const
    {
      return SendIMURequest(length_ms);
    }
    
    
    // ============ Pose history ===============
    
    Result Robot::AddRawOdomPoseToHistory(const TimeStamp_t t,
                                          const PoseFrameID_t frameID,
                                          const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                          const f32 pose_angle,
                                          const f32 head_angle,
                                          const f32 lift_angle)
    {
      return _poseHistory->AddRawOdomPose(t, frameID, pose_x, pose_y, pose_z, pose_angle, head_angle, lift_angle);
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
      
      // TODO: We should only be doing this (modifying what _worldOrigin points to) when it is one of the placeHolder poseOrigins, not if it is a mat!
      std::string origName(_worldOrigin->GetName());
      *_worldOrigin = _pose.GetInverse();
      _worldOrigin->SetParent(&newPoseWrtNewOrigin);
      
      
      // Connect the old origin's pose to the same root the robot now has.
      // It is no longer the robot's origin, but for any of its children,
      // it is now in the right coordinates.
      if(_worldOrigin->GetWithRespectTo(*newOrigin, *_worldOrigin) == false) {
        PRINT_NAMED_ERROR("Robot.UpdateWorldOrigin.NewLocalizationOriginProblem",
                          "Could not get pose origin w.r.t. new origin pose.\n");
        return RESULT_FAIL;
      }
      
      //_worldOrigin->PreComposeWith(*newOrigin);
      
      // Preserve the old world origin's name, despite updates above
      _worldOrigin->SetName(origName);
      
      // Now make the robot's world origin point to the new origin
      _worldOrigin = newOrigin;
      
      newOrigin->SetRotation(0, Z_AXIS_3D());
      newOrigin->SetTranslation({{0,0,0}});
      
      // Now make the robot's origin point to the new origin
      // TODO: avoid the icky const_cast here...
      _worldOrigin = const_cast<Pose3d*>(newPoseWrtNewOrigin.GetParent());

      _robotWorldOriginChangedSignal.emit(GetID());
      
      return RESULT_OK;
      
    } // UpdateWorldOrigin()
    
    
    Result Robot::AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                             const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                             const f32 pose_angle,
                                             const f32 head_angle,
                                             const f32 lift_angle)
    {      
      // We have a new ("ground truth") key frame. Increment the pose frame!
      //IncrementPoseFrameID();
      ++_frameId;
      
      return _poseHistory->AddVisionOnlyPose(t, _frameId,
                                            pose_x, pose_y, pose_z,
                                            pose_angle,
                                            head_angle,
                                            lift_angle);
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

    Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, Pose3d& pose)
    {
      RobotPoseStamp* poseStamp;
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
    
    Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key)
    {
      return _poseHistory->GetComputedPoseAt(t_request, p, key);
    }

    TimeStamp_t Robot::GetLastMsgTimestamp() const
    {
      return _poseHistory->GetNewestTimeStamp();
    }
    
    bool Robot::IsValidPoseKey(const HistPoseKey key) const
    {
      return _poseHistory->IsValidPoseKey(key);
    }
    
    bool Robot::UpdateCurrPoseFromHistory(const Pose3d& wrtParent)
    {
      bool poseUpdated = false;
      
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory->ComputePoseAt(_poseHistory->GetNewestTimeStamp(), t, p) == RESULT_OK) {
        if (p.GetFrameId() == GetPoseFrameID()) {
          
          // Grab a copy of the pose from history, which has been flattened (i.e.,
          // made with respect to whatever its origin was when it was stored).
          // We just assume for now that is the same as the _current_ world origin
          // (bad assumption? or will differing frame IDs help us?), and make that
          // chaining connection so that we can get the pose w.r.t. the requested
          // parent.
          Pose3d histPoseWrtCurrentWorld(p.GetPose());
          histPoseWrtCurrentWorld.SetParent(&wrtParent.FindOrigin());
          
          Pose3d newPose;
          if((histPoseWrtCurrentWorld.GetWithRespectTo(wrtParent, newPose))==false) {
            PRINT_NAMED_ERROR("Robot.UpdateCurrPoseFromHistory.GetWrtParentFailed",
                              "Could not update robot %d's current pose from history w.r.t. specified pose %s.",
                              _ID, wrtParent.GetName().c_str());
          } else {
            SetPose(newPose);
            poseUpdated = true;
          }
           
        }
      }
      
      return poseUpdated;
    }
    

    void Robot::SetBackpackLights(const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& onColor,
                                  const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& offColor,
                                  const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& onPeriod_ms,
                                  const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& offPeriod_ms,
                                  const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& transitionOnPeriod_ms,
                                  const std::array<u32,(size_t)LEDId::NUM_BACKPACK_LEDS>& transitionOffPeriod_ms)
    {
      ASSERT_NAMED((int)LEDId::NUM_BACKPACK_LEDS == 5, "Robot.wrong.number.of.backpack.ligths");
      std::array<Anki::Cozmo::LightState, 5> lights;
      for (int i = 0; i < (int)LEDId::NUM_BACKPACK_LEDS; ++i){
        lights[i].onColor = onColor[i];
        lights[i].offColor = offColor[i];
        lights[i].onPeriod_ms = onPeriod_ms[i];
        lights[i].offPeriod_ms = offPeriod_ms[i];
        lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms[i];
        lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms[i];
      }

      SendMessage(RobotInterface::EngineToRobot(RobotInterface::BackpackLights(lights)));
    }
    
    ObservableObject* Robot::GetActiveObject(const ObjectID     objectID,
                                             const ObjectFamily inFamily)
    {
      ObservableObject* object = nullptr;
      const char* familyStr = nullptr;
      if(inFamily == ObjectFamily::Unknown) {
        object = GetBlockWorld().GetObjectByID(objectID);
        familyStr = EnumToString(inFamily);
      } else {
        object = GetBlockWorld().GetObjectByIDandFamily(objectID, inFamily);
        familyStr = "any";
      }
      
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d does not exist in %s family.",
                          objectID.GetValue(), EnumToString(inFamily));
        return nullptr;
      }
      
      if(!object->IsActive()) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d does not appear to be an active object.",
                          objectID.GetValue());
        return nullptr;
      }
      
      if(object->GetIdentityState() != ActiveIdentityState::Identified) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d is active but has not been identified.",
                          objectID.GetValue());
        return nullptr;
      }
      
      return object;
    } // GetActiveObject()
    
    ObservableObject* Robot::GetActiveObjectByActiveID(const s32 activeID, const ObjectFamily inFamily)
    {
      for(auto objectsByType : GetBlockWorld().GetAllExistingObjects())
      {
        if(inFamily == ObjectFamily::Unknown || inFamily == objectsByType.first)
        {
          for(auto objectsByID : objectsByType.second)
          {
            for(auto objectWithID : objectsByID.second)
            {
              ObservableObject* object = objectWithID.second;
              if(object->IsActive() && object->GetActiveID() == activeID)
              {
                return object;
              }
            }
          }
        } // if(inFamily == ObjectFamily::Unknown || inFamily == objectsByFamily.first)
      } // for each family
      
      return nullptr;
    } // GetActiveObjectByActiveID()
    
    
    Result Robot::SetObjectLights(const ObjectID& objectID,
                                  const WhichCubeLEDs whichLEDs,
                                  const u32 onColor, const u32 offColor,
                                  const u32 onPeriod_ms, const u32 offPeriod_ms,
                                  const u32 transitionOnPeriod_ms, const u32 transitionOffPeriod_ms,
                                  const bool turnOffUnspecifiedLEDs,
                                  const MakeRelativeMode makeRelative,
                                  const Point2f& relativeToPoint)
    {
      ActiveCube* activeCube = dynamic_cast<ActiveCube*>(GetActiveObject(objectID, ObjectFamily::LightCube));
      if(activeCube == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetObjectLights", "Null active object pointer.");
        return RESULT_FAIL_INVALID_OBJECT;
      } else {
        
        // NOTE: if make relative mode is "off", this call doesn't do anything:
        const WhichCubeLEDs rotatedWhichLEDs = activeCube->MakeWhichLEDsRelativeToXY(whichLEDs,
                                                                                      relativeToPoint,
                                                                                      makeRelative);
        
        activeCube->SetLEDs(rotatedWhichLEDs, onColor, offColor, onPeriod_ms, offPeriod_ms,
                            transitionOnPeriod_ms, transitionOffPeriod_ms,
                            turnOffUnspecifiedLEDs);
        
        std::array<Anki::Cozmo::LightState, 4> lights;
        ASSERT_NAMED((int)ActiveObjectConstants::NUM_CUBE_LEDS == 4, "Robot.wrong.number.of.cube.ligths");
        for (int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i){
          const ActiveCube::LEDstate& ledState = activeCube->GetLEDState(i);
          lights[i].onColor = ledState.onColor;
          lights[i].offColor = ledState.offColor;
          lights[i].onPeriod_ms = ledState.onPeriod_ms;
          lights[i].offPeriod_ms = ledState.offPeriod_ms;
          lights[i].transitionOnPeriod_ms = ledState.transitionOnPeriod_ms;
          lights[i].transitionOffPeriod_ms = ledState.transitionOffPeriod_ms;
        }
        return SendMessage(RobotInterface::EngineToRobot(CubeLights(lights, (uint32_t)activeCube->GetActiveID())));
      }
    }
      
    Result Robot::SetObjectLights(const ObjectID& objectID,
                                  const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& onColor,
                                  const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& offColor,
                                  const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& onPeriod_ms,
                                  const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& offPeriod_ms,
                                  const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& transitionOnPeriod_ms,
                                  const std::array<u32,(size_t)ActiveObjectConstants::NUM_CUBE_LEDS>& transitionOffPeriod_ms,
                                  const MakeRelativeMode makeRelative,
                                  const Point2f& relativeToPoint)
    {
      ActiveCube* activeCube = dynamic_cast<ActiveCube*>(GetActiveObject(objectID, ObjectFamily::LightCube));
      if(activeCube == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetObjectLights", "Null active object pointer.\n");
        return RESULT_FAIL_INVALID_OBJECT;
      } else {
        
        activeCube->SetLEDs(onColor, offColor, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms);

        // NOTE: if make relative mode is "off", this call doesn't do anything:
        activeCube->MakeStateRelativeToXY(relativeToPoint, makeRelative);
        
        std::array<Anki::Cozmo::LightState, 4> lights;
        ASSERT_NAMED((int)ActiveObjectConstants::NUM_CUBE_LEDS == 4, "Robot.wrong.number.of.cube.ligths");
        for (int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i){
          lights[i].onColor = onColor[i];
          lights[i].offColor = offColor[i];
          lights[i].onPeriod_ms = onPeriod_ms[i];
          lights[i].offPeriod_ms = offPeriod_ms[i];
          lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms[i];
          lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms[i];
        }

        return SendMessage(RobotInterface::EngineToRobot(CubeLights(lights, (uint32_t)activeCube->GetActiveID())));
      }

    }
      
    Robot::ReactionCallbackIter Robot::AddReactionCallback(const Vision::Marker::Code code, ReactionCallback callback)
    {
      //CoreTechPrint("_reactionCallbacks size = %lu\n", _reactionCallbacks.size());
      
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
      
      _actionList.Cancel();
      
      if(AbortDrivingToPose() != RESULT_OK) {
        anyFailures = true;
      }
      
      if(AbortDocking() != RESULT_OK) {
        anyFailures = true;
      }
      
      if(AbortAnimation() != RESULT_OK) {
        anyFailures = true;
      }
      
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
      _animationStreamer.SetStreamingAnimation("");
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
      return SendMessage(RobotInterface::EngineToRobot(Anki::Cozmo::CarryState(state)));
    }
      
    Result Robot::SendFlashObjectIDs()
    {
      return SendMessage(RobotInterface::EngineToRobot(FlashObjectIDs()));
    }
     
      /*
    Result Robot::SendSetBlockLights(const u8 blockID,
                                     const std::array<u32,NUM_BLOCK_LEDS>& color,
                                     const std::array<u32,NUM_BLOCK_LEDS>& onPeriod_ms,
                                     const std::array<u32,NUM_BLOCK_LEDS>& offPeriod_ms)
    {
      MessageSetBlockLights m;
      m.blockID = blockID;
      m.color = color;
      m.onPeriod_ms = onPeriod_ms;
      m.offPeriod_ms = offPeriod_ms;

      return _msgHandler->SendMessage(GetID(), m);
    }
       */
      
    Result Robot::SendSetObjectLights(const ObjectID& objectID, const u32 onColor, const u32 offColor,
                                      const u32 onPeriod_ms, const u32 offPeriod_ms)
    {
      PRINT_NAMED_ERROR("Robot.SendSetObjectLights", "Deprecated.\n");
      return RESULT_FAIL;
      /*
      // Need to determing the blockID (meaning its internal "active" ID) from the
      // objectID known to the robot / UI
      Vision::ObservableObject* object = _blockWorld.GetObjectByIDandFamily(objectID, ObjectFamily::ACTIVE_BLOCKS);
      if(!object->IsActive()) {
        PRINT_NAMED_ERROR("Robot.SendSetObjectLights",
                          "Object %d does not appear to be an active object.\n", objectID.GetValue());
        return RESULT_FAIL;
      }
      
      if(!object->IsIdentified()) {
        PRINT_NAMED_ERROR("Robot.SendSetObjectLights",
                          "Object %d is active but has not been identified.\n", objectID.GetValue());
        return RESULT_FAIL;
      }
      
      // TODO: Get rid of the need for reinterpret_cast here (add virtual GetActiveID() to ObsObject?)
      ActiveCube* activeCube = reinterpret_cast<ActiveCube*>(object);
      if(activeCube == nullptr) {
        PRINT_NAMED_ERROR("Robot.SendSetObjectLights",
                          "Object %d could not be cast to an ActiveCube.\n", objectID.GetValue());
        return RESULT_FAIL;
      }
      
      activeCube->SetLEDs(ActiveCube::WhichCubeLEDs::ALL, color, onPeriod_ms, offPeriod_ms);
      
      return SendSetObjectLights(activeCube);
      */
    } // SendSetObjectLights()
      
    Result Robot::SendSetObjectLights(const ActiveCube* activeCube)
    {
      if(activeCube == nullptr) {
        PRINT_NAMED_ERROR("Robot.SendSetObjectLights", "Null active object pointer provided.\n");
        return RESULT_FAIL_INVALID_OBJECT;
      }
      std::array<Anki::Cozmo::LightState, 4> lights;
      ASSERT_NAMED((int)ActiveObjectConstants::NUM_CUBE_LEDS == 4, "Robot.wrong.number.of.cube.ligths");
      for (int i = 0; i < (int)ActiveObjectConstants::NUM_CUBE_LEDS; ++i){
        const ActiveCube::LEDstate& ledState = activeCube->GetLEDState(i);
        lights[i].onColor = ledState.onColor;
        lights[i].offColor = ledState.offColor;
        lights[i].onPeriod_ms = ledState.onPeriod_ms;
        lights[i].offPeriod_ms = ledState.offPeriod_ms;
        lights[i].transitionOnPeriod_ms = ledState.transitionOnPeriod_ms;
        lights[i].transitionOffPeriod_ms = ledState.transitionOffPeriod_ms;
      }
      return SendMessage(RobotInterface::EngineToRobot(CubeLights(lights, (uint32_t)activeCube->GetActiveID())));
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
  } // namespace Cozmo
} // namespace Anki
