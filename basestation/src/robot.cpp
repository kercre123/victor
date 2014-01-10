//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/robot.h"

// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/messages.h"

namespace Anki {
  namespace Cozmo {
    
    Robot::Robot()
    : addedToWorld(false),
      pose(0.f, Z_AXIS_3D, {{0.f, 0.f, WHEEL_RAD_TO_MM}}),
      camDownCalibSet(false), camHeadCalibSet(false),
    neckPose(0.f,Y_AXIS_3D, {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &pose),
      headCamPose(2.094395102393196, // Rotate -90deg around Y, then 90deg around Z
                  {{sqrtf(3.f)/3.f, -sqrtf(3.f)/3.f, sqrtf(3.f)/3.f}},
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &neckPose),
    liftBasePose(0.f, Y_AXIS_3D, {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &pose),
      matCamPose(M_PI, // Rotate 180deg around Y, then 90deg around Z
                 {{sqrtf(2.f)/2.f, -sqrtf(2.f)/2.f, 0.f}},
                 {{MAT_CAM_POSITION[0], MAT_CAM_POSITION[1], MAT_CAM_POSITION[2]}}, &pose),
      currentHeadAngle(0.f),
      isCarryingBlock(false),
      matMarker(NULL)
    {
      this->set_headAngle(currentHeadAngle);
      
      this->camDown.set_pose(this->matCamPose);
      
    } // Constructor: Robot
    
    void Robot::addToWorld(const u32 withID)
    {
      this->ID = withID;
      
      Messages::RobotAddedToWorld msg;
      msg.robotID = this->ID;
      
      const u8 *msgData = (const u8 *) &msg;
      messagesOut.emplace(msgData, msgData+sizeof(msg));
      
      this->addedToWorld = true;
      
    } // addToWorld()

    
    /*
    Robot::Robot( BlockWorld &theWorld )
    : world(theWorld)
    {
      // TODO: Compute this from down-camera calibration and position:
      this->downCamPixPerMM = 15.f;
      
    } // Constructor: Robot
    */
    
    /*
    template<>
    void Robot::processMessage<BlockObservationMessage>(const BlockObservationMessage *msg)
    {
      
    }
     */
    
    void Robot::updatePose()
    {
      if(not this->addedToWorld) {
        fprintf(stdout, "Robot::updatePose() called before robot was "
                "added to a world!\n");
        return;
      }
      
      if(not this->camDownCalibSet) {
        fprintf(stdout, "Robot::updatePose() called before mat camera "
                "calibration was set.\n");
        return;
      }
      
      bool poseUpdated = false;
      
      // TODO: add motion simulation (for in between messages)

      if(this->matMarker != NULL) {
        // Convert the MatMarker's image coordinates to a World
        // coordinates position
        Point2f centerVec(matMarker->get_imagePose().get_translation());
        
        const CameraCalibration& camCalib = camDown.get_calibration();
        
        const Point2f& imageCenterPt = camCalib.get_center();
        /*
        if(BlockWorld::ZAxisPointsUp) {
          // xvec = imgCen(1)-xcenIndex;
          // yvec = imgCen(2)-ycenIndex;
          
          centerVec *= -1.f;
          centerVec += imageCenterPt;
          
        } else {*/
          // xvec = xcenIndex - imgCen(1);
          // yvec = ycenIndex - imgCen(2);
          
          centerVec -= imageCenterPt;
        //}
        
        //xvecRot =  xvec*cos(-marker.upAngle) + yvec*sin(-marker.upAngle);
        //yvecRot = -xvec*sin(-marker.upAngle) + yvec*cos(-marker.upAngle);
        // xMat = xvecRot/pixPerMM + marker.X*squareWidth - squareWidth/2 -
        //          matSize(1)/2;
        // yMat = yvecRot/pixPerMM + marker.Y*squareWidth - squareWidth/2 -
        //          matSize(2)/2;
        RotationMatrix2d R(matMarker->get_upAngle());
        Point2f matPoint(R*centerVec);
        
        // This should have been computed by now
        CORETECH_ASSERT(this->downCamPixPerMM > 0.f);
        matPoint *= 1.f / this->downCamPixPerMM;
        matPoint.x() += matMarker->get_xSquare() * MatMarker2d::SquareWidth;
        matPoint.y() += matMarker->get_ySquare() * MatMarker2d::SquareWidth;
        matPoint -= MatMarker2d::SquareWidth * .5f;
        matPoint.x() -= MatSection::Size.x() * .5f;
        matPoint.y() -= MatSection::Size.y() * .5f;
        
        
        
        //fprintf(stdout, "MatMarker image angle = %.1f, upAngle = %.1f\n",
        //        matMarker->get_imagePose().get_angle().getDegrees(),
        //        matMarker->get_upAngle().getDegrees());
        
        // Using negative angle to switch between Z axis that goes into
        // the ground (mat camera's Z axis) and one that points up, out
        // of the ground (our world).
        Radians angle(matMarker->get_imagePose().get_angle());
/*
        // Subtract 90 degrees because the matMarker currently indicates the
        // direction towards the front of the robot, but in our world
        // coordinate system, that's in the y direction and we want 0 degrees
        // not to represent the world y axis direction, but rather the world
        // x axis direction.
        angle += M_PI_2;
 
        if(not BlockWorld::ZAxisPointsUp) {
          // TODO: is this correct??
          matPoint.x() *= -1.f;
          //matPoint.y() *= -1.f;
          //angle = -angle;
        }
    */
        // TODO: embed 2D pose in 3D space using its plane
        //this->pose = Pose3d( Pose2d(angle, matPoint) ); // bad! need to preserve original pose b/c other poses point to it!
        this->pose.set_rotation(angle, Z_AXIS_3D);
        this->pose.set_translation({{matPoint.y(), matPoint.x(), WHEEL_RAD_TO_MM}});
        
        // Delete the matMarker once we're done with it
        delete matMarker;
        matMarker = NULL;
        
        poseUpdated = true;
      } // if we have a matMarker2d
      
      if(poseUpdated)
      {
        // Send our updated pose to the physical robot:
        Messages::AbsLocalizationUpdate msg;
        msg.xPosition = pose.get_translation().x();
        msg.yPosition = pose.get_translation().y();
        
        Radians headingAngle;
        Vec3f   rotationAxis;
        pose.get_rotationVector().get_angleAndAxis(headingAngle, rotationAxis);
        
        // Angle will always be positive in a rotationVector.  We have to
        // take the axis into account here because we are using this as a
        // 2D rotation angle around the *positive* Z axis. So if the rotation
        // vector is rotating us around the *negative* Z axis, we need to use
        // the -angle in our message to the robot.
        if(headingAngle > 0.f) {
          // TODO: Just assuming axis is in Z direction here...
          CORETECH_ASSERT(NEAR_ZERO(rotationAxis.x()) &&
                          NEAR_ZERO(rotationAxis.y()) &&
                          NEAR(ABS(rotationAxis.z()), 1.f, 1e-6f));
          if(rotationAxis.z() < 0) {
            headingAngle = -headingAngle;
          }
        }
        
        msg.headingAngle = headingAngle.ToFloat();
        
        const u8 *msgData = (const u8 *) &msg;
        messagesOut.emplace(msgData, msgData+sizeof(msg));
        
        fprintf(stdout, "Basestation sending updated pose to robot: "
                "(%.1f, %.1f) at %.1f degrees\n",
                msg.xPosition, msg.yPosition,
                msg.headingAngle * (180.f/M_PI));
      } // if pose was updated
      
    } // updatePose()
    
    
    void Robot::checkMessages()
    {
      
      //
      // Parse Messages
      //
      //   Loop over all available messages from the physical robot
      //   and update any markers we saw
      //
      while(not this->messagesIn.empty())
      {
        MessageType& msg = messagesIn.front();
        
        //const u8 msgSize = msg[0];
        const Messages::ID msgID = static_cast<Messages::ID>(msg[0]);
        
        // TODO: Update to use dispatch functions instead of a switch
        switch(msgID)
        {
          case Messages::HeadCameraCalibration_ID:
          {
            const Messages::HeadCameraCalibration *calibMsg = reinterpret_cast<const Messages::HeadCameraCalibration*>(&(msg[0]));
            
            Anki::CameraCalibration calib(calibMsg->nrows,
                                          calibMsg->ncols,
                                          calibMsg->focalLength_x,
                                          calibMsg->focalLength_y,
                                          calibMsg->center_x,
                                          calibMsg->center_y,
                                          calibMsg->skew);
            
            fprintf(stdout,
                    "Basestation robot received head camera calibration.\n");
            this->camHead.set_calibration(calib);
            this->camHeadCalibSet = true;

            break;
          }
          case Messages::MatCameraCalibration_ID:
          {
            const Messages::MatCameraCalibration *calibMsg = reinterpret_cast<const Messages::MatCameraCalibration*>(&(msg[0]));
            
            Anki::CameraCalibration calib(calibMsg->nrows,
                                          calibMsg->ncols,
                                          calibMsg->focalLength_x,
                                          calibMsg->focalLength_y,
                                          calibMsg->center_x,
                                          calibMsg->center_y,
                                          calibMsg->skew);
            
            fprintf(stdout,
                    "Basestation robot received mat camera calibration.\n");
            this->camDown.set_calibration(calib);
            this->camDownCalibSet = true;
            
            // Compute the resolution of the mat camera from its FOV and height
            // off the mat:
            f32 matCamHeightInPix = ((static_cast<f32>(calibMsg->nrows)*.5f) /
                                     tanf(calibMsg->fov * .5f));
            this->downCamPixPerMM = matCamHeightInPix / MAT_CAM_HEIGHT_FROM_GROUND_MM;
            fprintf(stdout, "Computed mat cam's pixPerMM = %.1f\n",
                    this->downCamPixPerMM);
            
            
            break;
          } // camera calibration case
            
          case Messages::BlockMarkerObserved_ID:
          {
            // TODO: store observations from the same frame together
            
            // Translate the raw message into a BlockMarker2d object:
            const Messages::BlockMarkerObserved* blockMsg = reinterpret_cast<const Messages::BlockMarkerObserved*>(&(msg[0]));
            
            fprintf(stdout,
                    "Basestation Robot received ObservedBlockMarker message: "
                    "saw Block %d, Face %d at [(%.1f,%.1f), (%.1f,%.1f), "
                    "(%.1f,%.1f), (%.1f,%.1f)] with upDirection=%d and "
                    "headAngle=%.1fdeg\n",
                    blockMsg->blockType,       blockMsg->faceType,
                    blockMsg->x_imgUpperLeft,  blockMsg->y_imgUpperLeft,
                    blockMsg->x_imgLowerLeft,  blockMsg->y_imgLowerLeft,
                    blockMsg->x_imgUpperRight, blockMsg->y_imgUpperRight,
                    blockMsg->x_imgLowerRight, blockMsg->y_imgLowerRight,
                    blockMsg->upDirection,     blockMsg->headAngle*180.f/M_PI);
            
            Quad2f corners;
            
            corners[Quad::TopLeft].x()     = blockMsg->x_imgUpperLeft;
            corners[Quad::TopLeft].y()     = blockMsg->y_imgUpperLeft;
            
            corners[Quad::BottomLeft].x()  = blockMsg->x_imgLowerLeft;
            corners[Quad::BottomLeft].y()  = blockMsg->y_imgLowerLeft;
            
            corners[Quad::TopRight].x()    = blockMsg->x_imgUpperRight;
            corners[Quad::TopRight].y()    = blockMsg->y_imgUpperRight;
            
            corners[Quad::BottomRight].x() = blockMsg->x_imgLowerRight;
            corners[Quad::BottomRight].y() = blockMsg->y_imgLowerRight;
            
            MarkerUpDirection upDir = static_cast<MarkerUpDirection>(blockMsg->upDirection);
            
            // Construct a new BlockMarker2d at the end of the list
            this->visibleBlockMarkers2d.emplace_back(blockMsg->blockType,
                                                     blockMsg->faceType,
                                                     corners, upDir,
                                                     blockMsg->headAngle,
                                                     *this);
            
            break;
          }
          case Messages::MatMarkerObserved_ID:
          {
            // Translate the raw message into a MatMarker2d object:
            const Messages::MatMarkerObserved* matMsg = reinterpret_cast<const Messages::MatMarkerObserved*>(&(msg[0]));
            
            fprintf(stdout,
                    "Basestation robot received ObservedMatMarker message: "
                    "at mat square (%d,%d) seen at (%.1f,%.1f) "
                    "with orientation %.1f and upDirection=%d\n",
                    matMsg->x_MatSquare, matMsg->y_MatSquare,
                    matMsg->x_imgCenter, matMsg->y_imgCenter,
                    matMsg->angle * (180.f/M_PI), matMsg->upDirection);
            
            if(this->matMarker != NULL) {
              // We have two messages indicating a MatMarker was observed,
              // just use the last one?
              delete this->matMarker;
              
              // TODO: Issue a warning?
            }
            
            Pose2d imgPose(matMsg->angle, matMsg->x_imgCenter, matMsg->y_imgCenter);
            MarkerUpDirection upDir = static_cast<MarkerUpDirection>(matMsg->upDirection);
            
            this->matMarker = new MatMarker2d(matMsg->x_MatSquare,
                                              matMsg->y_MatSquare,
                                              imgPose, upDir);
            
            break;
          }
          default:
            CORETECH_THROW("Unrecognized RobotMessage type.");
            
        } // switch(msg type)
        
        this->messagesIn.pop();
        
      } // while message queue not empty
      
    } // checkMessages()
    
    
    void Robot::step(void)
    {
      // Reset what we have available from the physical robot
      if(this->matMarker != NULL) {
        delete this->matMarker;
        this->matMarker = NULL;
      }
      this->visibleBlockMarkers2d.clear();
      
      checkMessages();
      
      updatePose();
      
    } // step()

    void Robot::queueIncomingMessage(const u8 *msg, const u8 msgSize)
    {
      // Copy the incoming message into our queue
      this->messagesIn.emplace(msg, msg+msgSize); // C++11: no copy
    }
    
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
    
          /*
    void Robot::checkMessages(std::queue<RobotMessage> &messageQueue)
    {
      // Translate available messages and push onto the given queue.
      
      // TODO: add code to talk to Webots / Matlab / a real robot (BTLE)
      // TODO: much of this should probably live in coretech-communications
      
      
      // TEST!!!

      // "BlockType:FaceType:upperLeftX,upperLeftY:lowerLeftX,lowerLeftY:
      //   upperRightX,upperRightY:lowerRightX,lowerRightY"
      messageQueue.emplace(RobotMessage::BlockMarkerObservation,
                           "65:1:100,100:110,200:205,105:200,215");
      
      // "XsquareID:YsquareID:imgPosX,imgPosY,imgAngleDegrees:UpSide"
      messageQueue.emplace(RobotMessage::MatMarkerObservation,
                           "42:57:150,200:34:TOP");
      
    } // checkMessages()
         */
    
    
    /*
    
    void Robot::getVisibleBlockMarkers3d(std::multimap<BlockType, BlockMarker3d>& markers3d) const
    {
      if(not this->camHeadCalibSet) {
        fprintf(stdout, "Robot::getVisibleBlockMarkers3d() called before "
                "head camera calibration was set.\n");
        return;
      }
      
      // Estimate the pose of each observed BlockMarkers
      for(const BlockMarker2d& marker2d : this->visibleBlockMarkers2d)
      {
        // Create a BlockMarker3d from this marker2d, estimating its
        // Pose from the robot's head camera, and place in the map according
        // to its block type:
        markers3d.emplace(marker2d.get_blockType(),
                          BlockMarker3d(marker2d, this->camHead));
        
      } // FOR each marker2d
      
    } // getVisibleBlockMarkers3d()
    */
    
    void Robot::set_pose(const Pose3d &newPose)
    {
      // Update our current pose and let the physical robot know where it is:
      this->pose = newPose;
      
    } // set_pose()
    
    void Robot::set_headAngle(const Radians& angle)
    {
      if(angle < MIN_HEAD_ANGLE) {
        fprintf(stdout, "Requested head angle too small. Clipping.\n");
        currentHeadAngle = MIN_HEAD_ANGLE;
      }
      else if(angle > MAX_HEAD_ANGLE) {
        fprintf(stdout, "Requested head angle too large. Clipping.\n");
        currentHeadAngle = MAX_HEAD_ANGLE;
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
    
    
    void Robot::dockWithBlock(const Block& block)
    {
      
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
          
          this->camHead.project3dPoints(dots3D_goal, dots2D_goal);
          
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
        this->camHead.project3dPoints(searchWin3D, searchWin2D);
        
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
      
    } // dockWithBlock()
    
  } // namespace Cozmo
} // namespace Anki