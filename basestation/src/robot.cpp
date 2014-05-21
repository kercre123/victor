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
      msgHandler_ = msgHandler;
      blockWorld_ = blockWorld;
      pathPlanner_ = pathPlanner;
      
      return RESULT_OK;
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      robots_[withID] = new Robot(withID, msgHandler_, blockWorld_, pathPlanner_);
      ids_.push_back(withID);
    }
    
    std::vector<RobotID_t> const& RobotManager::GetRobotIDList() const
    {
      return ids_;
    }
    
    // Get a pointer to a robot by ID
    Robot* RobotManager::GetRobotByID(const RobotID_t robotID)
    {
      if (robots_.find(robotID) != robots_.end()) {
        return robots_[robotID];
      }
      
      return NULL;
    }
    
    size_t RobotManager::GetNumRobots() const
    {
      return robots_.size();
    }
    
    bool RobotManager::DoesRobotExist(const RobotID_t withID) const
    {
      return robots_.count(withID) > 0;
    }

    
    void RobotManager::UpdateAllRobots()
    {
      for (auto &r : robots_) {
        // Call update
        r.second->Update();
      }
    }
    
    
#pragma mark --- Robot Class Implementations ---
    
    Robot::Robot(const RobotID_t robotID, IMessageHandler* msgHandler, BlockWorld* world, IPathPlanner* pathPlanner)
    : ID_(robotID), msgHandler_(msgHandler), world_(world), pathPlanner_(pathPlanner),
      pose(-M_PI_2, Z_AXIS_3D, {{0.f, 0.f, 0.f}}),
      neckPose(0.f,Y_AXIS_3D, {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &pose),
      headCamPose({0,0,1,  -1,0,0,  0,-1,0},
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &neckPose),
      liftBasePose(0.f, Y_AXIS_3D, {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &pose),
      currentHeadAngle(0), currentLiftAngle(0),
      isCarryingBlock_(false), isTraversingPath_(false), isPickingOrPlacing_(false)
    {
      this->set_headAngle(currentHeadAngle);
      
    } // Constructor: Robot
   
    
    void Robot::updatePose()
    {
      
      
    } // updatePose()
  
    
    
    void Robot::Update(void)
    {
      // TODO: State update
      // ...
      
      
     
      // Visualize path if robot has just started traversing it.
      // Clear the path when it has stopped.
      static bool wasTraversingPath = false;
      if (!wasTraversingPath && IsTraversingPath() && path_.GetNumSegments() > 0) {
        VizManager::getInstance()->DrawPath(ID_,path_,VIZ_COLOR_EXECUTED_PATH);
        wasTraversingPath = true;
      } else if (wasTraversingPath && !IsTraversingPath()){
        path_.Clear();
        VizManager::getInstance()->ErasePath(ID_);
        wasTraversingPath = false;
      }
      
    } // step()

    
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

    
    void Robot::set_pose(const Pose3d &newPose)
    {
      // Update our current pose and let the physical robot know where it is:
      this->pose = newPose;
      
    } // set_pose()
    
    void Robot::set_headAngle(const f32& angle)
    {
      if(angle < MIN_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Requested head angle (%f rad) too small. Clipping.\n", angle);
        currentHeadAngle = MIN_HEAD_ANGLE;
        SendHeadAngleUpdate();
      }
      else if(angle > MAX_HEAD_ANGLE) {
        PRINT_NAMED_WARNING("Robot.HeadAngleOOB", "Requested head angle (%f rad) too large. Clipping.\n", angle);
        currentHeadAngle = MAX_HEAD_ANGLE;
        SendHeadAngleUpdate();
      }
      else {
        currentHeadAngle = angle;
      }
      
      // Start with canonical (untilted) headPose
      Pose3d newHeadPose(this->headCamPose);
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-currentHeadAngle, Y_AXIS_3D);
      newHeadPose.rotateBy(Rvec);
      
      // Update the head camera's pose
      this->camHead.set_pose(newHeadPose);
      
    } // set_headAngle()

    void Robot::set_liftAngle(const f32& angle)
    {
      currentLiftAngle = angle;
    }
    
    
    void Robot::dockWithBlock(const Block& block)
    {
     /*
      // Compute the necessary head angle and docking target position for
      // this block
      
      // Get the face closest to us
      const BlockMarker3d* closestFace = NULL;
      f32 minDistance = 0.f;
      
      for(Block::FaceName face=Block::FIRST_FACE; face < Block::NUM_FACES; ++face)
      {
        const BlockMarker3d& currentFace = block.get_faceMarker(face);
        Pose3d facePose = currentFace.get_pose().getWithRespectTo(this->pose.get_parent());
        f32 thisDistance = computeDistanceBetween(this->pose.get_translation(),
                                                  facePose.get_translation());
        if(closestFace==NULL || thisDistance < minDistance) {
          closestFace = &currentFace;
          minDistance = thisDistance;
        }
      }
      
      Quad2f dots2D_goal, searchWin2D;
      f32 desiredLiftHeight = 0.f;
      if(this->isCarryingBlock)
      {
        fprintf(stdout, "dockWithBlock() not yet implemented when carrying a block.\n");
        CORETECH_ASSERT(false);
        
      } else {
        // We are not carrying a block, so we figure out the lifter height we
        // need and where the docking face should end up
        // (This is assuming we are trying to align the origin (center?) of the
        //  lifter mechanism with the origin (center) of the face.)
        
        // Desired lifter height in world coordinates
        Pose3d face_wrt_world = closestFace->get_pose().getWithRespectTo(Pose3d::World);
        desiredLiftHeight = face_wrt_world.get_translation().z();
        
        // Compute lift angle to achieve that height
        Radians desiredLiftAngle = asinf((desiredLiftHeight - LIFT_JOINT_HEIGHT) / LIFT_ARM_LENGTH);
        RotationMatrix3d R(desiredLiftAngle, X_AXIS_3D);
        Vec3f liftVec(0.f, LIFT_ARM_LENGTH, 0.f);
        liftVec = R * liftVec;
      
        Pose3d desiredLiftPose(0.f, X_AXIS_3D, liftVec, &(this->liftBasePose));

        Pose3d desiredDockingFacePose = desiredLiftPose.getWithRespectTo(&(this->pose));
      
        BlockMarker3d virtualBlockFace(closestFace->get_blockType(),
                                       closestFace->get_faceType(),
                                       desiredDockingFacePose);
        
        bool withinImage = true;
        do {
          
          // Project the virtual docking face's target into our image
          Quad3f dots3D_goal;
          virtualBlockFace.getDockingTarget(dots3D_goal, &(this->camHead.get_pose()));
          
          this->camHead.Project3dPoints(dots3D_goal, dots2D_goal);
          
          if(this->camHead.isBehind(dots2D_goal[Quad::TopLeft]) ||
             this->camHead.isBehind(dots2D_goal[Quad::TopRight]) ||
             this->camHead.isBehind(dots2D_goal[Quad::BottomLeft]) ||
             this->camHead.isBehind(dots2D_goal[Quad::BottomRight]))
          {
            fprintf(stdout, "Projected virtual docking dots should not be "
                    "behind camera!\n");
            CORETECH_ASSERT(false);
          }
          
          // Tilt head until we find an angle that puts the docking dots well
          // within the image
          // TODO: actually do the math and compute this angle directly?
          
          // If it's above/below the camera, tilt the head up/down until we can see it
          const f32 nrows = static_cast<float>(this->camHead.get_calibration().get_nrows());
          if(dots2D_goal[Quad::BottomLeft].y()  > 0.9f*nrows ||
             dots2D_goal[Quad::BottomRight].y() > 0.9f*nrows)
          {
            fprintf(stdout, "Virtual docking dots too low for current "
                    "head pose.  Tilting head down...\n");
            
            this->set_headAngle(this->currentHeadAngle - 3.f*M_PI/180.f);
            
            withinImage = false;
          }
          else if(dots2D_goal[Quad::TopLeft].y()  < 0.1f*nrows ||
                  dots2D_goal[Quad::TopRight].y() < 0.1f*nrows)
          {
            fprintf(stdout, "Virtual docking dots too high for current "
                    "head pose.  Tilting head up...\n");
            
            this->set_headAngle(this->currentHeadAngle + 3.f*M_PI/180.f);
            
            withinImage = false;
          } else {
            withinImage = true;
          }
        } while(not withinImage);
        
        // At the final head angle, figure out where the closest face's
        // docking target window will appear so we can tell the robot
        // to start its search there
        Quad3f searchWin3D;
        closestFace->getDockingBoundingBox(searchWin3D,
                                           &(this->camHead.get_pose()));
        this->camHead.Project3dPoints(searchWin3D, searchWin2D);
        
      } // if carrying a block

      // TODO: Update this
#if 0
      // Create a dock initiation message
      CozmoMsg_InitiateDock msg;
      msg.size = sizeof(CozmoMsg_InitiateDock);
      msg.msgID = MSG_B2V_CORE_INITIATE_DOCK;
      msg.headAngle  = this->currentHeadAngle.ToFloat();
      msg.liftHeight = desiredLiftHeight;
      
      for(Quad::CornerName i_corner=Quad::FirstCorner;
          i_corner < Quad::NumCorners; ++i_corner)
      {
        msg.dotX[i_corner]     = dots2D_goal[i_corner].x();
        msg.dotY[i_corner]     = dots2D_goal[i_corner].y();
      }
      
      const s16 left  = static_cast<s16>(searchWin2D.get_minX());
      const s16 right = static_cast<s16>(searchWin2D.get_maxX());
      const s16 top   = static_cast<s16>(searchWin2D.get_minY());
      const s16 btm   = static_cast<s16>(searchWin2D.get_maxY());
      
      CORETECH_ASSERT(left < right);
      CORETECH_ASSERT(top < btm);
      
      msg.winX = left;
      msg.winY = top;
      msg.winWidth = right - left;
      msg.winHeight = btm - top;
      
      // Queue the message for sending to the robot
      const u8 *msgData = (const u8 *) &msg;
      messagesOut.emplace(msgData, msgData+sizeof(msg));
#endif
      */
      
    } // dockWithBlock()

    
    Result Robot::GetPathToPose(const Pose3d& targetPose, Planning::Path& path)
    {
      
      Result res = pathPlanner_->GetPlan(path, get_pose(), targetPose);
      
      // TODO: Make some sort of ApplySpeedProfile() function.
      //       Currently, we just set the speed of last segment to something slow.
      if (res == RESULT_OK) {
        path[path.GetNumSegments()-1].SetTargetSpeed(20);
        path[path.GetNumSegments()-1].SetDecel(100);
      }
      
      return res;
    }
    
    Result Robot::ExecutePathToPose(const Pose3d& pose)
    {
      Planning::Path p;
      if (GetPathToPose(pose, p) == RESULT_OK) {
        path_ = p;
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
    
    Result Robot::SendRequestCamCalib() const
    {
      MessageRequestCamCalib m;
      return msgHandler_->SendMessage(ID_, m);
    }
    
    // Clears the path that the robot is executing which also stops the robot
    Result Robot::SendClearPath() const
    {
      MessageClearPath m;
      m.pathID = 0;
      
      return msgHandler_->SendMessage(ID_, m);
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
            
            if (msgHandler_->SendMessage(ID_, m) == RESULT_FAIL)
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
            
            if (msgHandler_->SendMessage(ID_, m) == RESULT_FAIL)
              return RESULT_FAIL;
            break;
          }
          case Planning::PST_POINT_TURN:
            PRINT_NAMED_ERROR("PointTurnNotImplemented", "Point turns not working yet");
            return RESULT_FAIL;
          default:
            PRINT_NAMED_ERROR("Invalid path segment", "Can't send path segment of unknown type");
            return RESULT_FAIL;
            
        }
      }
      
      // Send start path execution message
      MessageExecutePath m;
      m.pathID = 0;
      return msgHandler_->SendMessage(ID_, m);
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
      return msgHandler_->SendMessage(ID_, m);
    }
    
    Result Robot::SendMoveLift(const f32 speed_rad_per_sec) const
    {
      MessageMoveLift m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      
      return msgHandler_->SendMessage(ID_,m);
    }
    
    Result Robot::SendMoveHead(const f32 speed_rad_per_sec) const
    {
      MessageMoveHead m;
      m.speed_rad_per_sec = speed_rad_per_sec;
      
      return msgHandler_->SendMessage(ID_,m);
    }

    Result Robot::SendSetLiftHeight(const f32 height_mm,
                                    const f32 max_speed_rad_per_sec,
                                    const f32 accel_rad_per_sec2) const
    {
      MessageSetLiftHeight m;
      m.height_mm = height_mm;
      m.max_speed_rad_per_sec = max_speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      
      return msgHandler_->SendMessage(ID_,m);
    }
    
    Result Robot::SendSetHeadAngle(const f32 angle_rad,
                                   const f32 max_speed_rad_per_sec,
                                   const f32 accel_rad_per_sec2) const
    {
      MessageSetHeadAngle m;
      m.angle_rad = angle_rad;
      m.max_speed_rad_per_sec = max_speed_rad_per_sec;
      m.accel_rad_per_sec2 = accel_rad_per_sec2;
      
      return msgHandler_->SendMessage(ID_,m);
    }
    
    Result Robot::SendDriveWheels(const f32 lwheel_speed_mmps, const f32 rwheel_speed_mmps) const
    {
      MessageDriveWheels m;
      m.lwheel_speed_mmps = lwheel_speed_mmps;
      m.rwheel_speed_mmps = rwheel_speed_mmps;
      
      return msgHandler_->SendMessage(ID_,m);
    }
    
    Result Robot::SendStopAllMotors() const
    {
      MessageStopAllMotors m;
      return msgHandler_->SendMessage(ID_,m);
    }
    
    Result Robot::SendAbsLocalizationUpdate() const
    {
      // TODO: Add z?
      MessageAbsLocalizationUpdate m;
      
      // TODO: need sync'd timestamps!
      m.timestamp = 0;
      
      m.xPosition = pose.get_translation().x();
      m.yPosition = pose.get_translation().y();
      
      m.headingAngle = atan2(pose.get_rotationMatrix()(1,0),
                             pose.get_rotationMatrix()(0,0));
      
      return msgHandler_->SendMessage(ID_, m);
    }
    
    Result Robot::SendHeadAngleUpdate() const
    {
      MessageHeadAngleUpdate m;
      
      m.newAngle = currentHeadAngle;
      
      return msgHandler_->SendMessage(ID_, m);
    }

    Result Robot::SendImageRequest(const ImageSendMode_t mode) const
    {
      MessageImageRequest m;
      
      m.imageSendMode = mode;
      m.resolution = Vision::CAMERA_RES_QQVGA;
      
      return msgHandler_->SendMessage(ID_, m);
    }
    
    Result Robot::SendStartTestMode(const TestMode mode) const
    {
      MessageStartTestMode m;
      
      m.mode = mode;
      
      return msgHandler_->SendMessage(ID_, m);
    }
    
    Result Robot::SendHeadlight(u8 intensity)
    {
      MessageSetHeadlight m;
      m.intensity = intensity;
      return msgHandler_->SendMessage(ID_, m);
    }
    
  } // namespace Cozmo
} // namespace Anki
