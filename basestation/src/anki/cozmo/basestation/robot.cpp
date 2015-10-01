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
#include "anki/cozmo/basestation/faceAndApproachPlanner.h"
#include "anki/cozmo/basestation/pathDolerOuter.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/robot.h"
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
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "opencv2/highgui/highgui.hpp" // For imwrite() in ProcessImage
#include "anki/cozmo/basestation/soundManager.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/robotStatusAndActions.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/vision/basestation/visionMarker.h"
#include "anki/vision/basestation/observableObjectLibrary_impl.h"
#include "util/fileUtils/fileUtils.h"
#include "anki/vision/basestation/image.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#include <fstream>
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
    , _isPhysical(false)
    , _newStateMsgAvailable(false)
    , _msgHandler(msgHandler)
    , _blockWorld(this)
    , _faceWorld(*this)
    , _visionProcessor(dataPlatform)
    , _behaviorMgr(*this)
    , _isBehaviorMgrEnabled(false)
#   if !ASYNC_VISION_PROCESSING
    , _haveNewImage(false)
#   endif
    , _wheelsLocked(false)
    , _headLocked(false)
    , _liftLocked(false)
    , _currPathSegment(-1)
    , _lastSentPathID(0)
    , _lastRecvdPathID(0)
    , _usingManualPathSpeed(false)
    , _camera(robotID)
    , _visionWhileMovingEnabled(false)
    /*
    , _proxVals{{0}}
    , _proxBlocked{{false}}
    */
    , _poseOrigins(1)
    , _worldOrigin(&_poseOrigins.front())
    , _pose(-M_PI_2, Z_AXIS_3D(), {{0.f, 0.f, 0.f}}, _worldOrigin, "Robot_" + std::to_string(_ID))
    , _driveCenterPose(-M_PI_2, Z_AXIS_3D(), {{0.f, 0.f, 0.f}}, _worldOrigin, "Robot_" + std::to_string(_ID))
    , _frameId(0)
    , _neckPose(0.f,Y_AXIS_3D(), {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &_pose, "RobotNeck")
    , _headCamPose(RotationMatrix3d({0,0,1,  -1,0,0,  0,-1,0}),
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &_neckPose, "RobotHeadCam")
    , _liftBasePose(0.f, Y_AXIS_3D(), {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &_pose, "RobotLiftBase")
    , _liftPose(0.f, Y_AXIS_3D(), {{LIFT_ARM_LENGTH, 0.f, 0.f}}, &_liftBasePose, "RobotLift")
    , _currentHeadAngle(MIN_HEAD_ANGLE)
    , _currentLiftAngle(0)
    , _onRamp(false)
    , _isPickingOrPlacing(false)
    , _isPickedUp(false)
    , _isMoving(false)
    , _battVoltage(5)
    , _imageSendMode(ImageSendMode::Off)
    , _carryingMarker(nullptr)
    , _lastPickOrPlaceSucceeded(false)
    , _stateSaveMode(SAVE_OFF)
    , _imageSaveMode(SAVE_OFF)
    , _imgFramePeriod(0)
    , _lastImgTimeStamp(0)
    , _animationStreamer(_cannedAnimations)
    , _numAnimationBytesPlayed(0)
    , _numAnimationBytesStreamed(0)
    , _animationTag(0)
    , _emotionMgr(*this)
    , _imageDeChunker(*(new ImageDeChunker()))
    {
      _poseHistory = new RobotPoseHistory();
      PRINT_NAMED_INFO("Robot.Robot", "Created");
      _pose.SetName("Robot_" + std::to_string(_ID));
      _driveCenterPose.SetName("RobotDriveCenter_" + std::to_string(_ID));
      
      // Initializes _pose, _poseOrigins, and _worldOrigin:
      Delocalize();
      InitRobotMessageComponent(_msgHandler,robotID);
      
      _proceduralFace.MarkAsSentToRobot(false);
      _proceduralFace.SetTimeStamp(1); // Make greater than lastFace's timestamp, so it gets streamed
      _lastProceduralFace.MarkAsSentToRobot(true);
      
      // The call to Delocalize() will increment frameID, but we want it to be
      // initialzied to 0, to match the physical robot's initialization
      _frameId = 0;

      ReadAnimationDir();
      
      // Read in emotion and behavior manager Json
      Json::Value emotionConfig;
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
        
        jsonFilename = "config/basestation/config/emotion_config.json";
        success = _dataPlatform->readAsJson(Util::Data::Scope::Resources, jsonFilename, emotionConfig);
        if (!success)
        {
          PRINT_NAMED_ERROR("Robot.EmotionConfigJsonNotFound",
                            "Emotion Json config file %s not found.",
                            jsonFilename.c_str());
        }
      }
      _emotionMgr.Init(emotionConfig);
      _behaviorMgr.Init(behaviorConfig);
      
      SetHeadAngle(_currentHeadAngle);
      _pdo = new PathDolerOuter(msgHandler, robotID);
      _longPathPlanner  = new LatticePlanner(this, _dataPlatform);
      _shortPathPlanner = new FaceAndApproachPlanner;
      _selectedPathPlanner = _longPathPlanner;
      
      _poseOrigins.front().SetName("Robot" + std::to_string(_ID) + "_PoseOrigin0");
      
    } // Constructor: Robot

    Robot::~Robot()
    {
      delete &_imageDeChunker;
      delete _poseHistory;
      _poseHistory = nullptr;
      
      delete _pdo;
      _pdo = nullptr;

      delete _longPathPlanner;
      _longPathPlanner = nullptr;
      
      delete _shortPathPlanner;
      _shortPathPlanner = nullptr;
      
      _selectedPathPlanner = nullptr;
      
    }
    
    void Robot::SetPickedUp(bool t)
    {
      if(_isPickedUp == false && t == true) {
        // Robot is being picked up: de-localize it and clear all known objects
        Delocalize();
        _blockWorld.ClearAllExistingObjects();
        
        if (_externalInterface != nullptr) {
          _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPickedUp(GetID())));
        }
      }
      else if (true == _isPickedUp && false == t) {
        if (_externalInterface != nullptr) {
          _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotPutDown(GetID())));
        }
      }
      _isPickedUp = t;
    }
    
    void Robot::Delocalize()
    {
      PRINT_NAMED_INFO("Robot.Delocalize", "Delocalizing robot %d.\n", GetID());
      
      _localizedToID.UnSet();
      _localizedToFixedMat = false;

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
      VizManager::getInstance()->SetText(VizManager::LOCALIZED_TO, ::Anki::NamedColors::YELLOW,
                                         "LocalizedTo: <nothing>");
    }

    Result Robot::UpdateFullRobotState(const RobotState& msg)
    {
      Result lastResult = RESULT_OK;

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
                              "Failed to get last pose from history with frame ID=%d.\n", msg.pose_frame_id);
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
                                                KEYFRAME_BUFFER_SIZE - (_numAnimationBytesStreamed - _numAnimationBytesPlayed),
                                                (u8)MIN(1000.f/GetAverageImagePeriodMS(), u8_MAX));
      
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

    Vision::Camera Robot::GetHistoricalCamera(RobotPoseStamp* p, TimeStamp_t t)
    {
      Vision::Camera camera(GetCamera());
      
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
   
      if(!GetCamera().IsCalibrated()) {
        PRINT_NAMED_WARNING("MessageHandler::CalibrationNotSet",
                            "Received VisionMarker message from robot before "
                            "camera calibration was set on Basestation.");
        return RESULT_FAIL;
      }
      
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
                VizManager::getInstance()->DrawMatMarker(quadID++, matMarker->Get3dCorners(markerPose), ::Anki::NamedColors::RED);
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
      
      /* DEBUG
       const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
       static double lastUpdateTime = currentTime_sec;
       
       const double updateTimeDiff = currentTime_sec - lastUpdateTime;
       if(updateTimeDiff > 1.0) {
       PRINT_NAMED_WARNING("Robot.Update", "Gap between robot update calls = %f\n", updateTimeDiff);
       }
       lastUpdateTime = currentTime_sec;
       */
      
      
      if (GetCamera().IsCalibrated()) {
      
#     if !ASYNC_VISION_PROCESSING
      if(_haveNewImage)
        
        _visionProcessor.Update(_image, _robotStateForImage);
        _haveNewImage = false;
#     endif
      {
        
        // Signal the availability of an image
        //CozmoEngineSignals::RobotImageAvailableSignal().emit(GetID());
        
        ////////// Check for any messages from the Vision Thread ////////////
        
        Vision::ObservedMarker visionMarker;
        while(true == _visionProcessor.CheckMailbox(visionMarker)) {
          
          Result lastResult = QueueObservedMarker(visionMarker);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_ERROR("Robot.Update.FailedToQueueVisionMarker",
                              "Got VisionMarker message from vision processing thread but failed to queue it.");
            return lastResult;
          }
          
          const Quad2f& corners = visionMarker.GetImageCorners();
          VizManager::getInstance()->SendVisionMarker(corners[Quad::TopLeft].x(),  corners[Quad::TopLeft].y(),
                                                      corners[Quad::TopRight].x(),  corners[Quad::TopRight].y(),
                                                      corners[Quad::BottomRight].x(),  corners[Quad::BottomRight].y(),
                                                      corners[Quad::BottomLeft].x(),  corners[Quad::BottomLeft].y(),
                                                      visionMarker.GetCode() != Vision::MARKER_UNKNOWN);
        }
        
        Vision::TrackedFace faceDetection;
        while(true == _visionProcessor.CheckMailbox(faceDetection)) {
          /*
          PRINT_NAMED_INFO("Robot.Update",
                           "Robot %d reported seeing a face at (x,y,w,h)=(%d,%d,%d,%d).",
                           GetID(), faceDetection.x_upperLeft, faceDetection.y_upperLeft, faceDetection.width, faceDetection.height);
          */
          
          Result lastResult;
          
          // Get historical robot pose at specified timestamp to make sure we've
          // got a robot/camera in pose history
          TimeStamp_t t;
          RobotPoseStamp* p = nullptr;
          HistPoseKey poseKey;
          lastResult = ComputeAndInsertPoseIntoHistory(faceDetection.GetTimeStamp(),
                                                       t, &p, &poseKey, true);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_WARNING("Robot.Update.HistoricalPoseNotFound",
                                "For face seen at time: %d, hist: %d to %d\n",
                                faceDetection.GetTimeStamp(),
                                _poseHistory->GetOldestTimeStamp(),
                                _poseHistory->GetNewestTimeStamp());
            return lastResult;
          }

          // Use a camera from the robot's pose history to estimate the head's
          // 3D translation, w.r.t. that camera. Also puts the face's pose in
          // the camera's pose chain.
          faceDetection.UpdateTranslation(GetHistoricalCamera(p, t));
          
          // Now use the faceDetection to update FaceWorld:
          lastResult = _faceWorld.AddOrUpdateFace(faceDetection);
          if(lastResult != RESULT_OK) {
            PRINT_NAMED_ERROR("Robot.Update.FailedToUpdateFace",
                              "Got FaceDetection from vision processing but failed to update it.");
          }
          
          VizManager::getInstance()->DrawCameraFace(faceDetection,
            faceDetection.IsBeingTracked() ? ::Anki::NamedColors::GREEN : ::Anki::NamedColors::RED);

        }
        
        VizInterface::TrackerQuad trackerQuad;
        if(true == _visionProcessor.CheckMailbox(trackerQuad)) {
          // Send tracker quad info to viz
          VizManager::getInstance()->SendTrackerQuad(trackerQuad.topLeft_x, trackerQuad.topLeft_y,
                                                     trackerQuad.topRight_x, trackerQuad.topRight_y,
                                                     trackerQuad.bottomRight_x, trackerQuad.bottomRight_y,
                                                     trackerQuad.bottomLeft_x, trackerQuad.bottomLeft_y);
        }
        
        //MessageDockingErrorSignal dockingErrorSignal;
        std::pair<Pose3d, TimeStamp_t> markerPoseWrtCamera;
        if(true == _visionProcessor.CheckMailbox(markerPoseWrtCamera)) {
          
          // Convert from camera frame to robot frame
          Anki::Cozmo::RobotPoseStamp p;
          TimeStamp_t t;
          _poseHistory->GetRawPoseAt(markerPoseWrtCamera.second, t, p);
          
          // Hook the pose coming out of the vision system up to the historical
          // camera at that timestamp
          Vision::Camera histCamera(GetHistoricalCamera(&p, t));
          markerPoseWrtCamera.first.SetParent(&histCamera.GetPose());
          /*
          // Get the pose w.r.t. the (historical) robot pose instead of the camera pose
          Pose3d markerPoseWrtRobot;
          if(false == markerPoseWrtCamera.first.GetWithRespectTo(p.GetPose(), markerPoseWrtRobot)) {
            PRINT_NAMED_ERROR("Robot.Update.PoseOriginFail",
                              "Could not get marker pose w.r.t. robot.");
            return RESULT_FAIL;
          }
          */
          //Pose3d poseWrtRobot = poseWrtCam;
          //poseWrtRobot.PreComposeWith(camWrtRobotPose);
          Pose3d markerPoseWrtRobot(markerPoseWrtCamera.first);
          markerPoseWrtRobot.PreComposeWith(histCamera.GetPose());
          
          DockingErrorSignal dockErrMsg;
          dockErrMsg.timestamp = markerPoseWrtCamera.second;
          dockErrMsg.x_distErr = markerPoseWrtRobot.GetTranslation().x();
          dockErrMsg.y_horErr  = markerPoseWrtRobot.GetTranslation().y();
          dockErrMsg.z_height  = markerPoseWrtRobot.GetTranslation().z();
          dockErrMsg.angleErr  = markerPoseWrtRobot.GetRotation().GetAngleAroundZaxis().ToFloat() + M_PI_2;
                    
          // Visualize docking error signal
          VizManager::getInstance()->SetDockingError(dockErrMsg.x_distErr,
                                                     dockErrMsg.y_horErr,
                                                     dockErrMsg.angleErr);
          
          // Try to use this for closed-loop control by sending it on to the robot
          SendMessage(RobotInterface::EngineToRobot(std::move(dockErrMsg)));
        }
        {
          RobotInterface::PanAndTilt panTiltHead;
          if (true == _visionProcessor.CheckMailbox(panTiltHead)) {
            SendMessage(RobotInterface::EngineToRobot(std::move(panTiltHead)));
          }
        }
        
        ////////// Update the robot's blockworld //////////
        // (Note that we're only doing this if the vision processor completed)
        
        uint32_t numBlocksObserved = 0;

        if(RESULT_OK != _blockWorld.Update(numBlocksObserved)) {
          PRINT_NAMED_WARNING("Robot.Update.BlockWorldUpdateFailed", "");
        }
        
        if(RESULT_OK != _faceWorld.Update()) {
          PRINT_NAMED_WARNING("Robot.Update.FaceWorldUpdateFailed", "");
        }

      } // if(_visionProcessor.WasLastImageProcessed())
      } // if (GetCamera().IsCalibrated())
      
      ///////// Update the behavior manager ///////////
      
      // TODO: This object encompasses, for the time-being, what some higher level
      // module(s) would do.  e.g. Some combination of game state, build planner,
      // personality planner, etc.
      
      const double currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      
      _emotionMgr.Update(currentTime);
      
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
      }
      
      VizManager::getInstance()->SetText(VizManager::BEHAVIOR_STATE, ::Anki::NamedColors::MAGENTA,
                                         "Behavior: %s", behaviorName.c_str());

      
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
      using namespace Quad;
      Quad2f quadOnGround2d = GetBoundingQuadXY(robotPoseWrtOrigin);
      const f32 zHeight = robotPoseWrtOrigin.GetTranslation().z() + 0.5f;
      Quad3f quadOnGround3d(Point3f(quadOnGround2d[TopLeft].x(),     quadOnGround2d[TopLeft].y(),     zHeight),
                            Point3f(quadOnGround2d[BottomLeft].x(),  quadOnGround2d[BottomLeft].y(),  zHeight),
                            Point3f(quadOnGround2d[TopRight].x(),    quadOnGround2d[TopRight].y(),    zHeight),
                            Point3f(quadOnGround2d[BottomRight].x(), quadOnGround2d[BottomRight].y(), zHeight));
      
      static const ColorRGBA ROBOT_BOUNDING_QUAD_COLOR(0.0f, 0.8f, 0.0f, 0.75f);
      VizManager::getInstance()->DrawRobotBoundingBox(GetID(), quadOnGround3d, ROBOT_BOUNDING_QUAD_COLOR);

      return RESULT_OK;
      
    } // Update()
    
    bool Robot::GetCurrentImage(Vision::Image& img, TimeStamp_t newerThan)
    {
#     if ASYNC_VISION_PROCESSING
      return _visionProcessor.GetCurrentImage(img, newerThan);
#     else
      if(!_image.IsEmpty() && _image.GetTimestamp() > newerThan ) {
        _image.CopyDataTo(img);
        img.SetTimestamp(_image.GetTimestamp());
        return true;
      } else {
        return false;
      }
#     endif
    }
    
    u32 Robot::GetAverageImagePeriodMS()
    {
      return _imgFramePeriod;
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
      _camera.SetPose(newHeadPose);
      
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
        
    Result Robot::GetPathToPose(const Pose3d& targetPose, Planning::Path& path)
    {
      Pose3d targetPoseWrtOrigin;
      if(targetPose.GetWithRespectTo(*GetWorldOrigin(), targetPoseWrtOrigin) == false) {
        PRINT_NAMED_ERROR("Robot.GetPathToPose.OriginMisMatch",
                          "Could not get target pose w.r.t. robot %d's origin.\n", GetID());
        return RESULT_FAIL;
      }
      
      SelectPlanner(targetPoseWrtOrigin);
      
      // Compute drive center pose of given target robot pose
      Pose3d targetDriveCenterPose;
      ComputeDriveCenterPose(targetPoseWrtOrigin, targetDriveCenterPose);
      
      // Compute drive center pose for start pose
      IPathPlanner::EPlanStatus status = _selectedPathPlanner->GetPlan(path, GetDriveCenterPose(), targetDriveCenterPose);

      if(status == IPathPlanner::PLAN_NOT_NEEDED || status == IPathPlanner::DID_PLAN)
        return RESULT_OK;
      else
        return RESULT_FAIL;
    }

    
    void Robot::SelectPlanner(const Pose3d& targetPose)
    {
      Pose2d target2d(targetPose);
      Pose2d start2d(GetPose());

      float distSquared = pow(target2d.GetX() - start2d.GetX(), 2) + pow(target2d.GetY() - start2d.GetY(), 2);

      if(distSquared < MAX_DISTANCE_FOR_SHORT_PLANNER * MAX_DISTANCE_FOR_SHORT_PLANNER) {
        PRINT_NAMED_INFO("Robot.SelectPlanner", "distance^2 is %f, selecting short planner\n", distSquared);
        _selectedPathPlanner = _shortPathPlanner;
      }
      else {
        PRINT_NAMED_INFO("Robot.SelectPlanner", "distance^2 is %f, selecting long planner\n", distSquared);
        _selectedPathPlanner = _longPathPlanner;
      }
    }
    
    Result Robot::GetPathToPose(const std::vector<Pose3d>& poses, size_t& selectedIndex, Planning::Path& path)
    {
      // Let the long path (lattice) planner do its thing and choose a target
      _selectedPathPlanner = _longPathPlanner;
      
      // Compute drive center pose for start pose and goal poses
      std::vector<Pose3d> targetDriveCenterPoses(poses.size());
      for (int i=0; i< poses.size(); ++i) {
        ComputeDriveCenterPose(poses[i], targetDriveCenterPoses[i]);
      }
      IPathPlanner::EPlanStatus status = _selectedPathPlanner->GetPlan(path, GetDriveCenterPose(), targetDriveCenterPoses, selectedIndex);
      
      if(status == IPathPlanner::PLAN_NOT_NEEDED || status == IPathPlanner::DID_PLAN)
      {
        // See if SelectPlanner selects the long path planner based on the pose it
        // selected
        SelectPlanner(poses[selectedIndex]);
        
        // If SelectPlanner would rather use the short path planner, let it get a
        // plan and use that one instead.
        if(_selectedPathPlanner != _longPathPlanner) {

          // Compute drive center pose for start pose and goal pose
          Pose3d targetDriveCenterPose;
          ComputeDriveCenterPose(poses[selectedIndex], targetDriveCenterPose);
          status = _selectedPathPlanner->GetPlan(path, GetDriveCenterPose(), targetDriveCenterPose);
        }
        
        if(status == IPathPlanner::PLAN_NOT_NEEDED || status == IPathPlanner::DID_PLAN) {
          return RESULT_OK;
        } else {
          return RESULT_FAIL;
        }
      } else {
        return RESULT_FAIL;
      }
      
    } // GetPathToPose(multiple poses)
    
  
    void Robot::ExecuteTestPath()
    {
      Planning::Path p;
      _longPathPlanner->GetTestPath(GetPose(), p);
      ExecutePath(p);
    }
    
    // =========== Motor commands ============
    
    // Sends message to move lift at specified speed
    Result Robot::MoveLift(const f32 speed_rad_per_sec)
    {
      return SendMoveLift(speed_rad_per_sec);
    }
    
    // Sends message to move head at specified speed
    Result Robot::MoveHead(const f32 speed_rad_per_sec)
    {
      return SendMoveHead(speed_rad_per_sec);
    }
    
    // Sends a message to the robot to move the lift to the specified height
    Result Robot::MoveLiftToHeight(const f32 height_mm,
                                   const f32 max_speed_rad_per_sec,
                                   const f32 accel_rad_per_sec2,
                                   const f32 duration_sec)
    {
      return SendSetLiftHeight(height_mm, max_speed_rad_per_sec, accel_rad_per_sec2, duration_sec);
    }
    
    // Sends a message to the robot to move the head to the specified angle
    Result Robot::MoveHeadToAngle(const f32 angle_rad,
                                  const f32 max_speed_rad_per_sec,
                                  const f32 accel_rad_per_sec2,
                                  const f32 duration_sec)
    {
      return SendSetHeadAngle(angle_rad, max_speed_rad_per_sec, accel_rad_per_sec2, duration_sec);
    }

    Result Robot::TurnInPlaceAtSpeed(const f32 speed_rad_per_sec,
                                     const f32 accel_rad_per_sec2)
    {
      return SendTurnInPlaceAtSpeed(speed_rad_per_sec, accel_rad_per_sec2);
    }
    
    
    
    Result Robot::TapBlockOnGround(const u8 numTaps)
    {
      return SendTapBlockOnGround(numTaps);
    }
      
    Result Robot::EnableTrackToObject(const u32 objectID, bool headOnly)
    {
      _trackToObjectID = objectID;
      
      if(_blockWorld.GetObjectByID(_trackToObjectID) != nullptr) {
        _trackWithHeadOnly = headOnly;
        _trackToFaceID = Vision::TrackedFace::UnknownFace;

        // Store whether head/wheels were locked before tracking so we can
        // return them to this state when we disable tracking
        _headLockedBeforeTracking = IsHeadLocked();
        _wheelsLockedBeforeTracking = AreWheelsLocked();

        LockHead(true);
        if(!headOnly) {
          LockWheels(true);
        }
        
        return RESULT_OK;
      } else {
        PRINT_NAMED_ERROR("Robot.EnableTrackToObject.UnknownObject",
                          "Cannot track to object ID=%d, which does not exist.\n",
                          objectID);
        _trackToObjectID.UnSet();
        return RESULT_FAIL;
      }
    }
    
    Result Robot::EnableTrackToFace(Vision::TrackedFace::ID_t faceID, bool headOnly)
    {
      _trackToFaceID = faceID;
      if(_faceWorld.GetFace(_trackToFaceID) != nullptr) {
        _trackWithHeadOnly = headOnly;
        _trackToObjectID.UnSet();
        
        // Store whether head/wheels were locked before tracking so we can
        // return them to this state when we disable tracking        // Store whether head/wheels were locked before tracking so we can
        // return them to this state when we disable tracking
        _headLockedBeforeTracking = IsHeadLocked();
        _wheelsLockedBeforeTracking = AreWheelsLocked();
        
        LockHead(true);
        if(!headOnly) {
          LockWheels(true);
        }
        
        return RESULT_OK;
      } else {
        PRINT_NAMED_ERROR("Robot.EnableTrackToFace.UnknownFace",
                          "Cannot track to face ID=%lld, which does not exist.",
                          faceID);
        _trackToFaceID = Vision::TrackedFace::UnknownFace;
        return RESULT_FAIL;
      }
    }
    
    Result Robot::DisableTrackToObject()
    {
      if(_trackToObjectID.IsSet()) {
        _trackToObjectID.UnSet();
        // Restore lock state to whatever it was when we enabled tracking
        LockHead(_headLockedBeforeTracking);
        LockWheels(_wheelsLockedBeforeTracking);
      }
      return RESULT_OK;
    }
    
    Result Robot::DisableTrackToFace()
    {
      if(_trackToFaceID != Vision::TrackedFace::UnknownFace) {
        _trackToFaceID = Vision::TrackedFace::UnknownFace;
        // Restore lock state to whatever it was when we enabled tracking
        LockHead(_headLockedBeforeTracking);
        LockWheels(_wheelsLockedBeforeTracking);
      }
      return RESULT_OK;
    }
      
    Result Robot::DriveWheels(const f32 lwheel_speed_mmps,
                              const f32 rwheel_speed_mmps)
    {
      return SendDriveWheels(lwheel_speed_mmps, rwheel_speed_mmps);
    }
    
    Result Robot::StopAllMotors()
    {
      return SendStopAllMotors();
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
      
      return SendPlaceObjectOnGround(0, 0, 0, useManualSpeed);
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
      if (_externalInterface != nullptr) {
        _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::PlaySound(soundName, numLoops, volume)));
      }
      //CozmoEngineSignals::PlaySoundForRobotSignal().emit(GetID(), soundName, numLoops, volume);
      return RESULT_OK;
    } // PlaySound()
      
      
    void Robot::StopSound()
    {
      if (_externalInterface != nullptr) {
        _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::StopSound()));
      }
    } // StopSound()
      
      
    Result Robot::StopAnimation()
    {
      return SendAbortAnimation();
    }

    void Robot::ReplayLastAnimation(const s32 loopCount)
    {
      _animationStreamer.SetStreamingAnimation(_lastPlayedAnimationId, loopCount);
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
          if (ent->d_type == DT_REG && ent->d_name[0] != '.') {
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
      if (_externalInterface != nullptr) {
        std::vector<std::string> animNames(_cannedAnimations.GetAnimationNames());
        for (std::vector<std::string>::iterator i=animNames.begin(); i != animNames.end(); ++i) {
          _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::AnimationAvailable(*i)));
        }
      }
    }

    Result Robot::SyncTime()
    {
      return SendSyncTime();
    }
    
    Result Robot::TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments)
    {
      return SendMessage(RobotInterface::EngineToRobot(RobotInterface::TrimPath(numPopFrontSegments, numPopBackSegments)));
    }
    
    
    
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
      
      if(!_localizedToFixedMat && !existingMatPiece->IsMoveable()) {
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
        
        _localizedToFixedMat = true;
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
          _localizedToFixedMat = true;
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
      SetLocalizedTo(existingMatPiece->GetID());
      
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
      
      // Update VizText
      VizManager::getInstance()->SetText(VizManager::LOCALIZED_TO, ::Anki::NamedColors::YELLOW,
                                         "LocalizedTo: %s", existingMatPiece->GetPose().GetName().c_str());
      
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
    
    Result Robot::StopDocking()
    {
      _visionProcessor.StopMarkerTracking();
      return RESULT_OK;
    }
    
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
      
      Result sendResult = SendDockWithObject(dockAction, useManualSpeed);
      
      if(sendResult == RESULT_OK) {
        
        // When we are "docking" with a ramp or crossing a bridge, we
        // don't want to worry about the X angle being large (since we
        // _expect_ it to be large, since the markers are facing upward).
        const bool checkAngleX = !(dockAction == DockAction::DA_RAMP_ASCEND  ||
                                   dockAction == DockAction::DA_RAMP_DESCEND ||
                                   dockAction == DockAction::DA_CROSS_BRIDGE);
        
        // Tell the VisionSystem to start tracking this marker:
        _visionProcessor.SetMarkerToTrack(marker->GetCode(), marker->GetSize(), image_pixel_x, image_pixel_y, checkAngleX,
                                          placementOffsetX_mm, placementOffsetY_mm, placementOffsetAngle_rad);
      }
      
      return sendResult;
    }
    
    
    // Sends a message to the robot to dock with the specified marker
    // that it should currently be seeing. If pixel_radius == u8_MAX,
    // the marker can be seen anywhere in the image (same as above function), otherwise the
    // marker's center must be seen at the specified image coordinates
    // with pixel_radius pixels.
    Result Robot::SendDockWithObject(const DockAction dockAction,
                                     const bool useManualSpeed)
    {
      return SendMessage(RobotInterface::EngineToRobot(::Anki::Cozmo::DockWithObject(0.0f, dockAction, useManualSpeed)));
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
          
          // Tell the robot it's carrying something
          // TODO: This is probably not the right way/place to do this (should we pass in carryObjectOnTopID?)
          if(_carryingObjectOnTopID.IsSet()) {
            SendSetCarryState(CarryState::CARRY_2_BLOCK);
          } else {
            SendSetCarryState(CarryState::CARRY_1_BLOCK);
          }
        }
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
                            "Robot not carrying object, but told to place one. (Possibly actually rolling.\n");
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
    

    void Robot::StartBehavior(const std::string& behaviorName)
    {
      if(behaviorName == "NONE") {
        PRINT_NAMED_INFO("Robot.StartBehavior.DisablingBehaviorManager", "\n");
        _isBehaviorMgrEnabled = false;
      } else {
        _isBehaviorMgrEnabled = true;
        if(behaviorName == "AUTO") {
          PRINT_NAMED_INFO("Robot.StartBehavior.EnablingAutoBehaviorSelection", "\n");
        } else {
          if(RESULT_OK != _behaviorMgr.SelectNextBehavior(behaviorName, BaseStationTimer::getInstance()->GetCurrentTimeInSeconds())) {
            PRINT_NAMED_ERROR("Robot.StartBehavior.Fail", "\n");
          } else {
            PRINT_NAMED_INFO("Robot.StartBehavior.Success",
                             "Switching to behavior '%s'\n",
                             behaviorName.c_str());
          }
        }
      }
      
    }
    
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
    
    Result Robot::SendPlaceObjectOnGround(const f32 rel_x, const f32 rel_y, const f32 rel_angle, const bool useManualSpeed)
    {
      return SendMessage(RobotInterface::EngineToRobot(
        Anki::Cozmo::PlaceObjectOnGround(rel_x, rel_y, rel_angle, useManualSpeed)));
    }
    
    Result Robot::SendMoveLift(const f32 speed_rad_per_sec) const
    {
      return SendMessage(RobotInterface::EngineToRobot(RobotInterface::MoveLift(speed_rad_per_sec)));
    }
    
    Result Robot::SendMoveHead(const f32 speed_rad_per_sec) const
    {
      return SendMessage(RobotInterface::EngineToRobot(RobotInterface::MoveHead(speed_rad_per_sec)));
    }

    Result Robot::SendSetLiftHeight(const f32 height_mm,
                                    const f32 max_speed_rad_per_sec,
                                    const f32 accel_rad_per_sec2,
                                    const f32 duration_sec) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::SetLiftHeight(height_mm,max_speed_rad_per_sec, accel_rad_per_sec2, duration_sec)));
    }
    
    Result Robot::SendSetHeadAngle(const f32 angle_rad,
                                   const f32 max_speed_rad_per_sec,
                                   const f32 accel_rad_per_sec2,
                                   const f32 duration_sec) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::SetHeadAngle(angle_rad, max_speed_rad_per_sec, accel_rad_per_sec2, duration_sec)));
    }

    Result Robot::SendTurnInPlaceAtSpeed(const f32 speed_rad_per_sec,
                                         const f32 accel_rad_per_sec2) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::TurnInPlaceAtSpeed(speed_rad_per_sec, accel_rad_per_sec2)));
    }
    
    Result Robot::SendTapBlockOnGround(const u8 numTaps) const
    {
      /*
      MessageTapBlockOnGround m;
      m.numTaps = numTaps;
      
      return _msgHandler->SendMessage(_ID,m);
      */
      return Result::RESULT_OK;
    }
    
    Result Robot::SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::DriveWheels(lwheel_speed_mmps, rwheel_speed_mmps)));
    }
    
    Result Robot::SendStopAllMotors() const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::StopAllMotors()));
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
      /*
      MessageIMURequest m;
      m.length_ms = length_ms;

      return SendMessage(m);
      */
      return Result::RESULT_OK;
    }

    void Robot::SetSaveStateMode(const SaveMode_t mode)
    {
      _stateSaveMode = mode;
    }

      
    void Robot::SetSaveImageMode(const SaveMode_t mode)
    {
      _imageSaveMode = mode;
    }
    
    Result Robot::ProcessImage(const Vision::Image& image)
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
      robotState.liftAngle = p.GetLiftAngle();
      robotState.pose.x    = p.GetPose().GetTranslation().x();
      robotState.pose.y    = p.GetPose().GetTranslation().y();
      robotState.pose.z    = p.GetPose().GetTranslation().z();
      robotState.pose.angle= p.GetPose().GetRotationAngle<'Z'>().ToFloat();
      
#     if ASYNC_VISION_PROCESSING
      
      // Note this copies the image
      _visionProcessor.SetNextImage(image, robotState);
      
#     else
      
      image.CopyDataTo(_image);
      _image.SetTimestamp(image.GetTimestamp());
      _robotStateForImage = robotState;
      _haveNewImage = true;
      
#     endif
      
      return lastResult;
    }
    
    Result Robot::StartFaceTracking(u8 timeout_sec)
    {
      _visionProcessor.EnableFaceDetection(true);
      return RESULT_OK;
    }
    
    Result Robot::StopFaceTracking()
    {
      _visionProcessor.EnableFaceDetection(false);
      return RESULT_OK;
    }
    
    Result Robot::StartLookingForMarkers()
    {
      _visionProcessor.EnableMarkerDetection(true);
      return RESULT_OK;
    }

    Result Robot::StopLookingForMarkers()
    {
      _visionProcessor.EnableMarkerDetection(false);
      return RESULT_OK;
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

    Result Robot::SetWheelControllerGains(const f32 kpLeft, const f32 kiLeft, const f32 maxIntegralErrorLeft,
                                          const f32 kpRight, const f32 kiRight, const f32 maxIntegralErrorRight)
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::ControllerGains(kpLeft, kiLeft, 0.0f, maxIntegralErrorLeft,
          Anki::Cozmo::RobotInterface::ControllerChannel::controller_wheel
        )));
    }
      
    Result Robot::SetHeadControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError)
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::ControllerGains(kp, ki, kd, maxIntegralError,
          Anki::Cozmo::RobotInterface::ControllerChannel::controller_head
        )));
    }
    
    Result Robot::SetLiftControllerGains(const f32 kp, const f32 ki, const f32 kd, const f32 maxIntegralError)
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::ControllerGains(kp, ki, kd, maxIntegralError,
          Anki::Cozmo::RobotInterface::ControllerChannel::controller_lift
        )));
    }
    
    Result Robot::SetSteeringControllerGains(const f32 k1, const f32 k2)
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::ControllerGains(k1, k2, 0.0f, 0.0f,
          Anki::Cozmo::RobotInterface::ControllerChannel::controller_stearing
        )));
    }

    Result Robot::StartTestMode(const TestMode mode, s32 p1, s32 p2, s32 p3) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        StartControllerTestMode(p1, p2, p3, mode)));
    }
    
    Result Robot::RequestImage(const ImageSendMode mode, const ImageResolution resolution) const
    {
      return SendMessage(RobotInterface::EngineToRobot(
        RobotInterface::ImageRequest(mode, resolution)));
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
    

    
      
    ActiveCube* Robot::GetActiveObject(const ObjectID objectID)
    {
      ObservableObject* object = GetBlockWorld().GetObjectByIDandFamily(objectID,ObjectFamily::LightCube);
     
      if(object == nullptr) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d does not exist in the ACTIVE_OBJECT family in the world.\n",
                          objectID.GetValue());
        return nullptr;
      }
      
      if(!object->IsActive()) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d does not appear to be an active object.\n",
                          objectID.GetValue());
        return nullptr;
      }
      
      if(!object->IsIdentified()) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d is active but has not been identified.\n",
                          objectID.GetValue());
        return nullptr;
      }
      
      ActiveCube* activeCube = dynamic_cast<ActiveCube*>(object);
      if(activeCube == nullptr) {
        PRINT_NAMED_ERROR("Robot.GetActiveObject",
                          "Object %d could not be cast to an ActiveCube.\n",
                          objectID.GetValue());
        return nullptr;
      }
      
      return activeCube;
    } // GetActiveObject()
      
    Result Robot::SetObjectLights(const ObjectID& objectID,
                                  const WhichCubeLEDs whichLEDs,
                                  const u32 onColor, const u32 offColor,
                                  const u32 onPeriod_ms, const u32 offPeriod_ms,
                                  const u32 transitionOnPeriod_ms, const u32 transitionOffPeriod_ms,
                                  const bool turnOffUnspecifiedLEDs,
                                  const MakeRelativeMode makeRelative,
                                  const Point2f& relativeToPoint)
    {
      ActiveCube* activeCube = GetActiveObject(objectID);
      if(activeCube == nullptr) {
        PRINT_NAMED_ERROR("Robot.SetObjectLights", "Null active object pointer.\n");
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
          lights[i].onColor = activeCube->_ledState[i].onColor;
          lights[i].offColor = activeCube->_ledState[i].offColor;
          lights[i].onPeriod_ms = activeCube->_ledState[i].onPeriod_ms;
          lights[i].offPeriod_ms = activeCube->_ledState[i].offPeriod_ms;
          lights[i].transitionOnPeriod_ms = activeCube->_ledState[i].transitionOnPeriod_ms;
          lights[i].transitionOffPeriod_ms = activeCube->_ledState[i].transitionOffPeriod_ms;
        }
        return SendMessage(RobotInterface::EngineToRobot(CubeLights(lights, (uint32_t)activeCube->GetID())));
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
      ActiveCube* activeCube = GetActiveObject(objectID);
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

        return SendMessage(RobotInterface::EngineToRobot(CubeLights(lights, (uint32_t)activeCube->GetID())));
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
      
      if(ClearPath() != RESULT_OK) {
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
        lights[i].onColor = activeCube->_ledState[i].onColor;
        lights[i].offColor = activeCube->_ledState[i].offColor;
        lights[i].onPeriod_ms = activeCube->_ledState[i].onPeriod_ms;
        lights[i].offPeriod_ms = activeCube->_ledState[i].offPeriod_ms;
        lights[i].transitionOnPeriod_ms = activeCube->_ledState[i].transitionOnPeriod_ms;
        lights[i].transitionOffPeriod_ms = activeCube->_ledState[i].transitionOffPeriod_ms;
      }
      return SendMessage(RobotInterface::EngineToRobot(CubeLights(lights, (uint32_t)activeCube->GetID())));
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
    
  } // namespace Cozmo
} // namespace Anki
