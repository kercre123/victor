//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "pathPlanner.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robot.h"


#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/vision/CameraSettings.h"

// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/robot/cozmoConfig.h"

#include "messageHandler.h"
#include "vizManager.h"

namespace Anki {
  namespace Cozmo {
    
#pragma mark --- RobotManager Class Implementations ---

#if USE_SINGLETON_ROBOT_MANAGER
    RobotManager* RobotManager::singletonInstance_ = 0;
#endif
    
    RobotManager::RobotManager()
    {
      
    }
    
    Result RobotManager::Init(IMessageHandler* msgHandler, BlockWorld* blockWorld, IPathPlanner* pathPlanner)
    {
      _msgHandler  = msgHandler;
      _blockWorld  = blockWorld;
      _pathPlanner = pathPlanner;
      
      return RESULT_OK;
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      _robots[withID] = new Robot(withID, _msgHandler, _blockWorld, _pathPlanner);
      _IDs.push_back(withID);
    }
    
    std::vector<RobotID_t> const& RobotManager::GetRobotIDList() const
    {
      return _IDs;
    }
    
    // Get a pointer to a robot by ID
    Robot* RobotManager::GetRobotByID(const RobotID_t robotID)
    {
      if (_robots.find(robotID) != _robots.end()) {
        return _robots[robotID];
      }
      
      return nullptr;
    }
    
    size_t RobotManager::GetNumRobots() const
    {
      return _robots.size();
    }
    
    bool RobotManager::DoesRobotExist(const RobotID_t withID) const
    {
      return _robots.count(withID) > 0;
    }

    
    void RobotManager::UpdateAllRobots()
    {
      for (auto &r : _robots) {
        // Call update
        r.second->Update();
      }
    }
    
    
#pragma mark --- Robot Class Implementations ---
    
    Robot::Robot(const RobotID_t robotID, IMessageHandler* msgHandler, BlockWorld* world, IPathPlanner* pathPlanner)
    : _ID(robotID)
    , _msgHandler(msgHandler)
    , _world(world)
    , _pathPlanner(pathPlanner)
    , _currPathSegment(-1)
    , _pose(-M_PI_2, Z_AXIS_3D, {{0.f, 0.f, 0.f}})
    , _frameId(0)
    , _neckPose(0.f,Y_AXIS_3D, {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &_pose)
    , _headCamPose({0,0,1,  -1,0,0,  0,-1,0},
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &_neckPose)
    , _liftBasePose(0.f, Y_AXIS_3D, {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &_pose)
    , _currentHeadAngle(0)
    , _currentLiftAngle(0)
    , _isCarryingBlock(false)
    , _isPickingOrPlacing(false)
    {
      SetHeadAngle(_currentHeadAngle);
      
    } // Constructor: Robot
     
    
    void Robot::Update(void)
    {
      // TODO: State update
      // ...
      
      static bool wasTraversingPath = false;
      
      // If the robot is traversing a path, consider replanning it
      // TODO:(bn) only check this if the blocks have been updated
      if(IsTraversingPath()) {
        Planning::Path newPath;
        if(_pathPlanner->ReplanIfNeeded(newPath, GetPose())) {
          _path.Clear();
          VizManager::getInstance()->ErasePath(_ID);
          wasTraversingPath = false;

          PRINT_NAMED_INFO("Robot.Update.ClearPath", "sending message to clear old path");
          MessageClearPath clearMessage;
          _msgHandler->SendMessage(_ID, clearMessage);

          _path = newPath;
          PRINT_NAMED_INFO("Robot.Update.UpdatePath", "sending new path to robot");
          SendExecutePath(_path);
        }
      }
     
      // Visualize path if robot has just started traversing it.
      // Clear the path when it has stopped.
      if (!wasTraversingPath && IsTraversingPath() && _path.GetNumSegments() > 0) {
        VizManager::getInstance()->DrawPath(_ID,_path,VIZ_COLOR_EXECUTED_PATH);
        wasTraversingPath = true;
      } else if (wasTraversingPath && !IsTraversingPath()){
        _path.Clear();
        VizManager::getInstance()->ErasePath(_ID);
        wasTraversingPath = false;
      }
      
    } // step()

    /*
    void Robot::getOutgoingMessage(u8 *msgOut, u8 &msgSize)
    {
      MessageType& nextMessage = this->messagesOut.front();
      if(msgSize < nextMessage.size()) {
        CORETECH_THROW("Next outgoing message too large for given buffer.");
        msgSize = 0;
        return;
      }
        
      std::copy(nextMessage.begin(), nextMessage.end(), msgOut);
      msgSize = nextMessage.size();
      
      this->messagesOut.pop();
    }
     */

    
    void Robot::SetPose(const Pose3d &newPose)
    {
      // Update our current pose and let the physical robot know where it is:
      _pose = newPose;
      
    } // set_pose()
    
    void Robot::SetHeadAngle(const f32& angle)
    {
      if(angle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Requested head angle (%f rad) too small. Clipping.\n", angle);
        _currentHeadAngle = MIN_HEAD_ANGLE;
        SendHeadAngleUpdate();
      }
      else if(angle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Requested head angle (%f rad) too large. Clipping.\n", angle);
        _currentHeadAngle = MAX_HEAD_ANGLE;
        SendHeadAngleUpdate();
      }
      else {
        _currentHeadAngle = angle;
      }
      
      // Start with canonical (untilted) headPose
      Pose3d newHeadPose(_headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-_currentHeadAngle, Y_AXIS_3D);
      newHeadPose.rotateBy(Rvec);
      
      // Update the head camera's pose
      _camera.SetPose(newHeadPose);
      
    } // set_headAngle()

    void Robot::SetLiftAngle(const f32& angle)
    {
      _currentLiftAngle = angle;
    }
    

    Result Robot::GetPathToPose(const Pose3d& targetPose, Planning::Path& path)
    {
      
      Result res = _pathPlanner->GetPlan(path, GetPose(), targetPose);
      
      // // TODO: Make some sort of ApplySpeedProfile() function.
      // //       Currently, we just set the speed of last segment to something slow.
      // if (res == RESULT_OK) {
      //   path[path.GetNumSegments()-1].SetTargetSpeed(20);
      //   path[path.GetNumSegments()-1].SetDecel(100);
      // }
      
      return res;
    }
    
    Result Robot::ExecutePathToPose(const Pose3d& pose)
    {
      Planning::Path p;
      if (GetPathToPose(pose, p) == RESULT_OK) {
        _path = p;
        return ExecutePath(p);
      }
        
      return RESULT_FAIL;
      
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
                                   const f32 accel_rad_per_sec2)
    {
      return SendSetLiftHeight(height_mm, max_speed_rad_per_sec, accel_rad_per_sec2);
    }
    
    // Sends a message to the robot to move the head to the specified angle
    Result Robot::MoveHeadToAngle(const f32 angle_rad,
                                  const f32 max_speed_rad_per_sec,
                                  const f32 accel_rad_per_sec2)
    {
      return SendSetHeadAngle(angle_rad, max_speed_rad_per_sec, accel_rad_per_sec2);
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

    Result Robot::TrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments)
    {
      return SendTrimPath(numPopFrontSegments, numPopBackSegments);
    }
    
    // Clears the path that the robot is executing which also stops the robot
    Result Robot::ClearPath()
    {
      return SendClearPath();
    }
    
    // Sends a path to the robot to be immediately executed
    Result Robot::ExecutePath(const Planning::Path& path)
    {
      // TODO: Clear currently executing path or write to buffered path?
      if (ClearPath() == RESULT_FAIL)
        return RESULT_FAIL;

      return SendExecutePath(path);
    }
    
    // Sends a message to the robot to dock with the specified block
    // that it should currently be seeing.
    Result Robot::DockWithBlock(const u8 markerType,
                                const f32 markerWidth_mm,
                                const DockAction_t dockAction)
    {
      return SendDockWithBlock(markerType, markerWidth_mm, dockAction);
    }
    
    // Sends a message to the robot to dock with the specified block
    // that it should currently be seeing. If pixel_radius == u8_MAX,
    // the marker can be seen anywhere in the image (same as above function), otherwise the
    // marker's center must be seen at the specified image coordinates
    // with pixel_radius pixels.
    Result Robot::DockWithBlock(const u8 markerType,
                                const f32 markerWidth_mm,
                                const DockAction_t dockAction,
                                const u16 image_pixel_x,
                                const u16 image_pixel_y,
                                const u8 pixel_radius)
    {
      return SendDockWithBlock(markerWidth_mm, markerWidth_mm, dockAction, image_pixel_x, image_pixel_y, pixel_radius);
    }

    Result Robot::SetHeadlight(u8 intensity)
    {
      return SendHeadlight(intensity);
    }
    
    // ============ Messaging ================
    
    // Sync time with physical robot and trigger it robot to send back camera calibration
    Result Robot::SendInit() const
    {
      MessageRobotInit m;
      m.robotID  = _ID;
      m.syncTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    // Clears the path that the robot is executing which also stops the robot
    Result Robot::SendClearPath() const
    {
      MessageClearPath m;
      m.pathID = 0;
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    // Removes the specified number of segments from the front and back of the path
    Result Robot::SendTrimPath(const u8 numPopFrontSegments, const u8 numPopBackSegments) const
    {
      MessageTrimPath m;
      m.numPopFrontSegments = numPopFrontSegments;
      m.numPopBackSegments = numPopBackSegments;

      return _msgHandler->SendMessage(_ID, m);
    }
    
    // Sends a path to the robot to be immediately executed
    Result Robot::SendExecutePath(const Planning::Path& path) const
    {
      // Send path segments
      for (u8 i=0; i<path.GetNumSegments(); i++)
      {
        switch (path.GetSegmentConstRef(i).GetType())
        {
          case Planning::PST_LINE:
          {
            MessageAppendPathSegmentLine m;
            const Planning::PathSegmentDef::s_line* l = &(path.GetSegmentConstRef(i).GetDef().line);
            m.x_start_mm = l->startPt_x;
            m.y_start_mm = l->startPt_y;
            m.x_end_mm = l->endPt_x;
            m.y_end_mm = l->endPt_y;
            m.pathID = 0;
            m.segmentID = i;
            
            m.targetSpeed = path.GetSegmentConstRef(i).GetTargetSpeed();
            m.accel = path.GetSegmentConstRef(i).GetAccel();
            m.decel = path.GetSegmentConstRef(i).GetDecel();
            
            if (_msgHandler->SendMessage(_ID, m) == RESULT_FAIL)
              return RESULT_FAIL;
            break;
          }
          case Planning::PST_ARC:
          {
            MessageAppendPathSegmentArc m;
            const Planning::PathSegmentDef::s_arc* a = &(path.GetSegmentConstRef(i).GetDef().arc);
            m.x_center_mm = a->centerPt_x;
            m.y_center_mm = a->centerPt_y;
            m.radius_mm = a->radius;
            m.startRad = a->startRad;
            m.sweepRad = a->sweepRad;
            m.pathID = 0;
            m.segmentID = i;
            
            m.targetSpeed = path.GetSegmentConstRef(i).GetTargetSpeed();
            m.accel = path.GetSegmentConstRef(i).GetAccel();
            m.decel = path.GetSegmentConstRef(i).GetDecel();
            
            if (_msgHandler->SendMessage(_ID, m) == RESULT_FAIL)
              return RESULT_FAIL;
            break;
          }
          case Planning::PST_POINT_TURN:
          {
            MessageAppendPathSegmentPointTurn m;
            const Planning::PathSegmentDef::s_turn* t = &(path.GetSegmentConstRef(i).GetDef().turn);
            m.x_center_mm = t->x;
            m.y_center_mm = t->y;
            m.targetRad = t->targetAngle;
            m.pathID = 0;
            m.segmentID = i;
            
            m.targetSpeed = path.GetSegmentConstRef(i).GetTargetSpeed();
            m.accel = path.GetSegmentConstRef(i).GetAccel();
            m.decel = path.GetSegmentConstRef(i).GetDecel();

            if (_msgHandler->SendMessage(_ID, m) == RESULT_FAIL)
              return RESULT_FAIL;
            break;
          }
          default:
            PRINT_NAMED_ERROR("Invalid path segment", "Can't send path segment of unknown type");
            return RESULT_FAIL;
            
        }
      }
      
      // Send start path execution message
      MessageExecutePath m;
      m.pathID = 0;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    
    Result Robot::SendDockWithBlock(const u8 markerType, const f32 markerWidth_mm, const DockAction_t dockAction) const
    {
      return SendDockWithBlock(markerType, markerWidth_mm, dockAction, 0, 0, u8_MAX);
    }

    
    Result Robot::SendDockWithBlock(const u8 markerType,
                                    const f32 markerWidth_mm,
                                    const DockAction_t dockAction,
                                    const u16 image_pixel_x,
                                    const u16 image_pixel_y,
                                    const u8 pixel_radius) const
    {
      MessageDockWithBlock m;
      m.markerWidth_mm = markerWidth_mm;
      m.markerType = markerType;
      m.dockAction = dockAction;
      m.image_pixel_x = image_pixel_x;
      m.image_pixel_y = image_pixel_y;
      m.pixel_radius = pixel_radius;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendMoveLift(const f32 speed_rad_per_sec) const
    {
      MessageMoveLift m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendMoveHead(const f32 speed_rad_per_sec) const
    {
      MessageMoveHead m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      
      return _msgHandler->SendMessage(_ID,m);
    }

    Result Robot::SendSetLiftHeight(const f32 height_mm,
                                    const f32 max_speed_rad_per_sec,
                                    const f32 accel_rad_per_sec2) const
    {
      MessageSetLiftHeight m;
      m.height_mm = height_mm;
      m.max_speed_rad_per_sec = max_speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendSetHeadAngle(const f32 angle_rad,
                                   const f32 max_speed_rad_per_sec,
                                   const f32 accel_rad_per_sec2) const
    {
      MessageSetHeadAngle m;
      m.angle_rad = angle_rad;
      m.max_speed_rad_per_sec = max_speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps) const
    {
      MessageDriveWheels m;
      m.lwheel_speed_mmps = lwheel_speed_mmps;
      m.rwheel_speed_mmps = rwheel_speed_mmps;
      
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendStopAllMotors() const
    {
      MessageStopAllMotors m;
      return _msgHandler->SendMessage(_ID,m);
    }
    
    Result Robot::SendAbsLocalizationUpdate() const
    {
      // TODO: Add z?
      MessageAbsLocalizationUpdate m;
      
      // Look in history for the last vis pose and send it.
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory.GetLatestVisionOnlyPose(t, p) == RESULT_FAIL) {
        PRINT_NAMED_WARNING("Robot.SendAbsLocUpdate.NoVizPoseFound", "");
        return RESULT_FAIL;
      }

      m.timestamp = t;
      
      m.pose_frame_id = p.GetFrameId();
      
      m.xPosition = p.GetPose().get_translation().x();
      m.yPosition = p.GetPose().get_translation().y();

      m.headingAngle = p.GetPose().get_rotationMatrix().GetAngleAroundZaxis().ToFloat();
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendHeadAngleUpdate() const
    {
      MessageHeadAngleUpdate m;
      
      m.newAngle = _currentHeadAngle;
      
      return _msgHandler->SendMessage(_ID, m);
    }

    Result Robot::SendImageRequest(const ImageSendMode_t mode) const
    {
      MessageImageRequest m;
      
      m.imageSendMode = mode;
      m.resolution = IMG_STREAM_RES;
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendStartTestMode(const TestMode mode) const
    {
      MessageStartTestMode m;
      
      m.mode = mode;
      
      return _msgHandler->SendMessage(_ID, m);
    }
    
    Result Robot::SendHeadlight(u8 intensity)
    {
      MessageSetHeadlight m;
      m.intensity = intensity;
      return _msgHandler->SendMessage(_ID, m);
    }
    
    const Quad2f Robot::CanonicalBoundingBoxXY({{ROBOT_BOUNDING_X_FRONT, -0.5f*ROBOT_BOUNDING_Y}},
                                               {{ROBOT_BOUNDING_X_FRONT,  0.5f*ROBOT_BOUNDING_Y}},
                                               {{ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X, -0.5f*ROBOT_BOUNDING_Y}},
                                               {{ROBOT_BOUNDING_X_FRONT - ROBOT_BOUNDING_X,  0.5f*ROBOT_BOUNDING_Y}});
    
    Quad2f Robot::GetBoundingQuadXY(const f32 padding_mm) const
    {
      return GetBoundingQuadXY(_pose, padding_mm);
    }
    
    Quad2f Robot::GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm) const
    {
      const RotationMatrix2d R(atPose.get_rotationMatrix().GetAngleAroundZaxis());
      
      Quad2f boundingQuad(Robot::CanonicalBoundingBoxXY);
      if(padding_mm != 0.f) {
        Quad2f paddingQuad({{ padding_mm, -padding_mm}},
                           {{ padding_mm,  padding_mm}},
                           {{-padding_mm, -padding_mm}},
                           {{-padding_mm,  padding_mm}});
        boundingQuad += paddingQuad;
      }
      
      using namespace Quad;
      for(CornerName iCorner = FirstCorner; iCorner < NumCorners; ++iCorner) {
        // Rotate to given pose
        boundingQuad[iCorner] = R*Robot::CanonicalBoundingBoxXY[iCorner];
      }
      
      // Re-center
      Point2f center(atPose.get_translation().x(), atPose.get_translation().y());
      boundingQuad += center;
      
      return boundingQuad;
      
    } // GetBoundingBoxXY()
    
    
    // ============ Pose history ===============
    
    Result Robot::AddRawOdomPoseToHistory(const TimeStamp_t t,
                                          const PoseFrameID_t frameID,
                                          const f32 pose_x, const f32 pose_y, const f32 pose_z,
                                          const f32 pose_angle,
                                          const f32 head_angle)
    {
      return _poseHistory.AddRawOdomPose(t, frameID, pose_x, pose_y, pose_z, pose_angle, head_angle);
    }
    
    Result Robot::AddVisionOnlyPoseToHistory(const TimeStamp_t t,
                                             const RobotPoseStamp& p)
    {
      return _poseHistory.AddVisionOnlyPose(t, p);
    }

    Result Robot::ComputeAndInsertPoseIntoHistory(const TimeStamp_t t_request,
                                                  TimeStamp_t& t, RobotPoseStamp** p,
                                                  HistPoseKey* key,
                                                  bool withInterpolation)
    {
      return _poseHistory.ComputeAndInsertPoseAt(t_request, t, p, key, withInterpolation);
    }

    Result Robot::GetVisionOnlyPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p)
    {
      return _poseHistory.GetVisionOnlyPoseAt(t_request, p);
    }

    Result Robot::GetComputedPoseAt(const TimeStamp_t t_request, RobotPoseStamp** p, HistPoseKey* key)
    {
      return _poseHistory.GetComputedPoseAt(t_request, p, key);
    }

    bool Robot::IsValidPoseKey(const HistPoseKey key) const
    {
      return _poseHistory.IsValidPoseKey(key);
    }
    
    bool Robot::UpdateCurrPoseFromHistory()
    {
      bool poseUpdated = false;
      
      TimeStamp_t t;
      RobotPoseStamp p;
      if (_poseHistory.ComputePoseAt(_poseHistory.GetNewestTimeStamp(), t, p) == RESULT_OK) {
        if (p.GetFrameId() == GetPoseFrameID()) {
          _pose = p.GetPose();
          poseUpdated = true;
        }
      }
      
      return poseUpdated;
    }
    
    
  } // namespace Cozmo
} // namespace Anki
