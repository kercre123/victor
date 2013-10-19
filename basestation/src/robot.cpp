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

#include "anki/cozmo/messageProtocol.h"


namespace Anki {
  namespace Cozmo {
    
    Robot::Robot()
    {
      
    } // Constructor: Robot
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
      // TODO: add motion simulation (for in between messages)

      if(this->matMarker != NULL) {
        // Convert the MatMarker's image coordinates to a World
        // coordinates position
        Point2f centerVec(matMarker->get_imagePose().get_translation());
        
        const CameraCalibration& camCalib = camDown.get_calibration();
        
        // TODO: get center point from camCalib
        //const Point2f imageCenterPt(camCalib.get_center_pt());
        const Point2f imageCenterPt(640.f/2.f, 480.f/2.f);
        
        if(BlockWorld::ZAxisPointsUp) {
          // xvec = imgCen(1)-xcenIndex;
          // yvec = imgCen(2)-ycenIndex;
          
          centerVec *= -1.f;
          centerVec += imageCenterPt;
          
        } else {
          // xvec = xcenIndex - imgCen(1);
          // yvec = ycenIndex - imgCen(2);
          
          centerVec -= imageCenterPt;
        }
        
        //xvecRot =  xvec*cos(-marker.upAngle) + yvec*sin(-marker.upAngle);
        //yvecRot = -xvec*sin(-marker.upAngle) + yvec*cos(-marker.upAngle);
        // xMat = xvecRot/pixPerMM + marker.X*squareWidth - squareWidth/2 -
        //          matSize(1)/2;
        // yMat = yvecRot/pixPerMM + marker.Y*squareWidth - squareWidth/2 -
        //          matSize(2)/2;
        RotationMatrix2d R(matMarker->get_upAngle());
        Point2f matPoint(R*centerVec);
        // TODO: get downCamPixPerMM from camCalib!
        // matPoint *= 1.f / this->downCamPixPerMM;
        matPoint *= 1.f/24.f;
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
        // of the ground (our world)
        Radians angle(-matMarker->get_imagePose().get_angle());
        
        if(not BlockWorld::ZAxisPointsUp) {
          // TODO: is this correct??
          //matPoint.x() *= -1.f;
          matPoint.y() *= -1.f;
          angle = -angle;
        }
        
        // TODO: embed 2D pose in 3D space using its plane
        //this->pose = Pose3d( Pose2d(angle, matPoint) );
        this->pose.set_rotation(angle, {{0.f, 0.f, 1.f}});
        this->pose.set_translation({{matPoint.x(), matPoint.y(), WHEEL_DIAMETER_MM * .5f}});
        
        // Delete the matMarker once we're done with it
        delete matMarker;
        matMarker = NULL;
        
      } // if we have a matMarker2d
      
      // Send our (updated) pose to the physical robot:
      CozmoMsg_AbsLocalizationUpdate msg;
      msg.size = SIZE_MSG_B2V_CORE_ABS_LOCALIZATION_UPDATE;
      msg.msgID = MSG_B2V_CORE_ABS_LOCALIZATION_UPDATE;
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
        const CozmoMsg_Command msgType = static_cast<CozmoMsg_Command>(msg[1]);
        
        switch(msgType)
        {
          case MSG_V2B_CORE_BLOCK_MARKER_OBSERVED:
          {
            // TODO: store observations from the same frame together
            
            // Translate the raw message into a BlockMarker2d object:
            const CozmoMsg_ObservedBlockMarker* blockMsg = reinterpret_cast<const CozmoMsg_ObservedBlockMarker*>(&(msg[0]));
            
            fprintf(stdout,
                    "Basestation Robot received ObservedBlockMarker message: "
                    "saw Block %d, Face %d at [(%.1f,%.1f), (%.1f,%.1f), "
                    "(%.1f,%.1f), (%.1f,%.1f)] with upDirection=%d\n",
                    blockMsg->blockType,       blockMsg->faceType,
                    blockMsg->x_imgUpperLeft,  blockMsg->y_imgUpperLeft,
                    blockMsg->x_imgLowerLeft,  blockMsg->y_imgLowerLeft,
                    blockMsg->x_imgUpperRight, blockMsg->y_imgUpperRight,
                    blockMsg->x_imgLowerRight, blockMsg->y_imgLowerRight,
                    blockMsg->upDirection);
            
            Quad2f corners;
            
            corners[Quad2f::TopLeft].x()     = blockMsg->x_imgUpperLeft;
            corners[Quad2f::TopLeft].y()     = blockMsg->y_imgUpperLeft;
            
            corners[Quad2f::BottomLeft].x()  = blockMsg->x_imgLowerLeft;
            corners[Quad2f::BottomLeft].y()  = blockMsg->y_imgLowerLeft;
            
            corners[Quad2f::TopRight].x()    = blockMsg->x_imgUpperRight;
            corners[Quad2f::TopRight].y()    = blockMsg->y_imgUpperRight;
            
            corners[Quad2f::BottomRight].x() = blockMsg->x_imgLowerRight;
            corners[Quad2f::BottomRight].y() = blockMsg->y_imgLowerRight;
            
            MarkerUpDirection upDir = static_cast<MarkerUpDirection>(blockMsg->upDirection);
            
            // Construct a new BlockMarker2d at the end of the list
            this->visibleBlockMarkers2d.emplace_back(blockMsg->blockType,
                                                     blockMsg->faceType,
                                                     corners, upDir);
            
            break;
          }
          case MSG_V2B_CORE_MAT_MARKER_OBSERVED:
          {
            // Translate the raw message into a MatMarker2d object:
            const CozmoMsg_ObservedMatMarker* matMsg = reinterpret_cast<const CozmoMsg_ObservedMatMarker*>(&(msg[0]));
            
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
    
    void Robot::getVisibleBlockMarkers3d(std::vector<BlockMarker3d> &markers3d) const
    {
      // Estimate the pose of each observed BlockMarkers
      for(const BlockMarker2d& marker2d : this->visibleBlockMarkers2d)
      {
        // Create a BlockMarker3d from this marker2d, estimating its
        // Pose from the robot's head camera:
        markers3d.push_back( BlockMarker3d(marker2d, this->camHead) );
        
      } // FOR each marker2d
      
    } // getVisibleBlockMarkers3d()
    
    void Robot::set_pose(const Pose3d &newPose)
    {
      // Update our current pose and let the physical robot know where it is:
      this->pose = newPose;
      
      
    } // set_pose()
    
  } // namespace Cozmo
} // namespace Anki