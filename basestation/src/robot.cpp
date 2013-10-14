//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/blockWorld.h"
#include "block.h"
#include "mat.h"
#include "robot.h"

namespace Anki {
  namespace Cozmo {
    
    Robot::Robot( BlockWorld &theWorld )
    : world(theWorld)
    {
      // TODO: Compute this from down-camera calibration and position:
      this->downCamPixPerMM = 15.f;
      
    } // Constructor: Robot
    
    
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
      std::queue<RobotMessage> messages;
      
      // TODO: get messages from physical robot and populate queue
      this->checkMessages(messages);
      
      while(not messages.empty())
      {
        const RobotMessage &msg = messages.front();
        
        switch(msg.get_type())
        {
          case RobotMessage::BlockMarkerObservation:
            
            // Construct a new BlockMarker2d at the end of the list
            this->visibleBlockMarkers2d.emplace_back(msg);
            
            break;
            
          case RobotMessage::MatMarkerObservation:
            if(this->matMarker2d != NULL) {
              // We have two messages indicating a MatMarker was observed,
              // just use the last one?
              delete this->matMarker2d;
              
              // TODO: Issue a warning?
            }
            this->matMarker2d = new MatMarker2d(msg);
            
            break;
            
          default:
            CORETECH_THROW("Unrecognized RobotMessage type.");
            
        } // switch(msg type)
        
        messages.pop();
        
      } // while message queue not empty
      
    } // step()

                
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