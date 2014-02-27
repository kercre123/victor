//
//  robot.cpp
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/general.h"

// TODO: This is shared between basestation and robot and should be moved up
#include "anki/cozmo/robot/cozmoConfig.h"

#include "messageHandler.h"

namespace Anki {
  namespace Cozmo {
    
#pragma mark --- RobotManager Class Implementations ---

#if USE_SINGLETON_ROBOT_MANAGER
    RobotManager* RobotManager::singletonInstance_ = 0;
#endif
    
    RobotManager::RobotManager()
    {
      
    }
    
    ReturnCode RobotManager::Init(MessageHandler* msgHandler, BlockWorld* blockWorld)
    {
      msgHandler_ = msgHandler;
      blockWorld_ = blockWorld;
      
      return EXIT_SUCCESS;
    }
    
    void RobotManager::AddRobot(const RobotID_t withID)
    {
      robots_[withID] = new Robot(withID, msgHandler_, blockWorld_);
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
      // TODO
    }
    
    
#pragma mark --- Robot Class Implementations ---
    
    Robot::Robot(const RobotID_t robotID, MessageHandler* msgHandler, BlockWorld* world)
    : ID_(robotID), msgHandler_(msgHandler), world_(world),
      pose(-M_PI_2, Z_AXIS_3D, {{0.f, 0.f, WHEEL_RAD_TO_MM}}),
      neckPose(0.f,Y_AXIS_3D, {{NECK_JOINT_POSITION[0], NECK_JOINT_POSITION[1], NECK_JOINT_POSITION[2]}}, &pose),
      headCamPose({0,0,1,  -1,0,0,  0,-1,0},
                  {{HEAD_CAM_POSITION[0], HEAD_CAM_POSITION[1], HEAD_CAM_POSITION[2]}}, &neckPose),
      liftBasePose(0.f, Y_AXIS_3D, {{LIFT_BASE_POSITION[0], LIFT_BASE_POSITION[1], LIFT_BASE_POSITION[2]}}, &pose),
      currentHeadAngle(DEG_TO_RAD(-14.4f)),
      isCarryingBlock(false)
    {
      this->set_headAngle(currentHeadAngle);
      
    } // Constructor: Robot
   
    
    void Robot::updatePose()
    {
      
      
    } // updatePose()
  
    
    
    void Robot::Update(void)
    {
      
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
      */
      
    } // dockWithBlock()

    
    
  } // namespace Cozmo
} // namespace Anki