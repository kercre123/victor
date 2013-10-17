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
    
    
    void Robot::step(void)
    {
      // Reset what we have available from the physical robot
      if(this->matMarker2d != NULL) {
        delete this->matMarker2d;
        this->matMarker2d = NULL;
      }
      this->visibleBlockMarkers2d.clear();
      
      // Loop over all available messages from the robot and update any
      // markers we saw
      
      while(not this->messages.empty())
      {
        MessageType& msg = messages.front();
        
        const u8 msgSize = msg[0];
        const CozmoMsg_Command msgType = static_cast<CozmoMsg_Command>(msg[1]);
        
        switch(msgType)
        {
          case MSG_V2B_CORE_BLOCK_MARKER_OBSERVED:
          {
            // TODO: store observations from the same frame together
            
            // Translate the raw message into a BlockMarker2d object:
            const CozmoMsg_ObservedBlockMarker* blockMsg = reinterpret_cast<const CozmoMsg_ObservedBlockMarker*>(&(msg[0]));
            
            fprintf(stdout, "Received ObservedBlockMarker message: "
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
            
            fprintf(stdout, "Received ObservedMatMarker message: "
                    "at mat square (%d,%d) seen at (%.1f,%.1f) "
                    "with orientation %.1f and upDirection=%d\n",
                    matMsg->x_MatSquare, matMsg->y_MatSquare,
                    matMsg->x_imgCenter, matMsg->y_imgCenter,
                    matMsg->angle * (180.f/M_PI), matMsg->upDirection);
            
            if(this->matMarker2d != NULL) {
              // We have two messages indicating a MatMarker was observed,
              // just use the last one?
              delete this->matMarker2d;
              
              // TODO: Issue a warning?
            }
            
            Pose2d imgPose(matMsg->angle, matMsg->x_imgCenter, matMsg->y_imgCenter);
            MarkerUpDirection upDir = static_cast<MarkerUpDirection>(matMsg->upDirection);
            
            this->matMarker2d = new MatMarker2d(matMsg->x_MatSquare,
                                                matMsg->y_MatSquare,
                                                imgPose, upDir);
            break;
          }
          default:
            CORETECH_THROW("Unrecognized RobotMessage type.");
            
        } // switch(msg type)
        
        this->messages.pop();
        
      } // while message queue not empty
      
    } // step()

    void Robot::queueMessage(const u8 *msg, const u8 msgSize)
    {
      // Copy the incoming message into our queue
      this->messages.emplace(msg, msg+msgSize); // C++11: no copy
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