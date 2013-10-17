
#include "anki/cozmo/messageProtocol.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki
{
  namespace Cozmo
  {
    
    /*
    BlockWorld::BlockWorld( MessagingInterface* msgInterfaceIn )
    : msgInterface(msgInterfaceIn), blocks(MaxBlockTypes), zAxisPointsUp(true)
    {
      
    }
     */
    
    BlockWorld::BlockWorld( )
    : blocks(MaxBlockTypes), zAxisPointsUp(true)
    {
      
    }
    
    BlockWorld::~BlockWorld(void)
    {
      // Delete all instantiated blocks:
      
      // For each vector of block types
      for( const std::vector<Block*>& typeList : this->blocks )
      {
        // For each block in the vector of this type of blocks"
        for( Block* block : typeList )
        {
          if(block != NULL)
          {
            delete block;
          }
        } // for each block
      } // for each block type
      
      // TODO: Should BlockWorld delete its Robots? Or were we given pointers
      //       to Robots from the outside someone else is managing?
      
      // Delete all instantiated robots:
      for( Robot* robot : this->robots ) {
      /*for(auto it = this->robots.begin(); it != this->robots.end(); ++it)
      {
        Robot *robot = it->second;*/
        if(robot != NULL)
        {
          delete robot;
        }
      }
      
    } // BlockWorld Destructor
    
    
    void BlockWorld::addRobot(Robot *robot)
    {
      /*
      const u8& ID = robot->get_ID();
      
      if(this->robots.count(ID) > 0) {
        CORETECH_THROW("Robot with ID already in BlockWorld.");
      }
      
      this->robots[ID] = robot;
      */
      robots.push_back(robot);
    } // addRobot()
    
    void BlockWorld::queueMessage(const u8 *msg)
    {
      this->messages.push(msg);
    }
    
    void BlockWorld::update(void)
    {
      /*
      // Go through messages in the queue and update blocks' and robots' poses
      // accordingly
      
      std::vector<BlockMarker3d> blockMarkers;
      
      while( not this->messages.empty() )
      {
        const u8 *msg = messages.front();
        messages.pop();
        
        const u8 msgSize = msg[0];
        const CozmoMsg_Command msgType = static_cast<CozmoMsg_Command>(msg[1]);
        
        switch(msgType)
        {
          case MSG_V2B_CORE_BLOCK_MARKER_OBSERVED:
          {
            // Create a BlockMarker2d from the message:
            const CozmoMsg_ObservedBlockMarker* blockMsg = reinterpret_cast<const CozmoMsg_ObservedBlockMarker*>(msg);
            
            Quad2f corners;
            
            corners[Quad2f::TopLeft].x() = blockMsg->x_imgUpperLeft;
            corners[Quad2f::TopLeft].y() = blockMsg->y_imgUpperLeft;
            
            corners[Quad2f::BottomLeft].x() = blockMsg->x_imgLowerLeft;
            corners[Quad2f::BottomLeft].y() = blockMsg->y_imgLowerLeft;

            corners[Quad2f::TopRight].x() = blockMsg->x_imgUpperRight;
            corners[Quad2f::TopRight].y() = blockMsg->y_imgUpperRight;

            corners[Quad2f::BottomRight].x() = blockMsg->x_imgLowerRight;
            corners[Quad2f::BottomRight].y() = blockMsg->y_imgLowerRight;
            
            BlockMarker2d blockMarker2d(blockMsg->blockType,
                                        blockMsg->faceType,
                                        corners);
            
            // Instantiate a BlockMarker3d in the list using the BlockMarker2d
            // and the camera that saw it.
            blockMarkers.emplace_back(blockMarker2d, this->robots[blockMsg->robotId]);
            
            break;
          }
            
          case MSG_V2B_CORE_MAT_MARKER_OBSERVED:
          {
            break;
          }
            
          default:
          {
            // TODO: send this message somewhere reasonable (log?)
            fprintf(stdout, "Unknown message type. Skipping.\n");
          }
        } // switch(msgType)
        
      } // while there are still messages
       */
      
      // Get updated observations from each robot and update blocks' and robots'
      // poses accordingly
      std::vector<BlockMarker3d> blockMarkers;
      
      for(Robot* robot : this->robots)
      {
        // Tell each robot to take a step, which will have it
        // check and parse any messages received from the physical
        // robot.
        robot->step();
        
        // Tell each robot where it is, based on its current mat observation:
        //updateRobotPose(robot);
                          
        // Each robot adds the 3d markers it saw to the list.
        //robot->getVisibleBlockMarkers3d(blockMarkers);
        
      } // for each robot
      
      // If any robots saw any markers, update/add the corresponding blocks:
      if(not blockMarkers.empty())
      {
        
      } // IF we saw any block markers
      
    } // update()
    
    void BlockWorld::updateRobotPose(Robot *robot)
    {
      // TODO: Can the robot do this for itself?
      
      // Use the robot's current MatMarker (if there is one) to update its
      // pose
      const MatMarker2d* matMarker = robot->get_matMarker2d();
      if(matMarker != NULL)
      {
        // Convert the MatMarker's image coordinates to a World
        // coordinates position
        Point2f centerVec(matMarker->get_imagePose().get_translation());
        
        const CameraCalibration& camCalib = robot->get_camDown().get_calibration();
        
        const Point2f imageCenterPt(camCalib.get_center_pt());
        
        if(this->zAxisPointsUp) {
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
        matPoint *= 1.f / robot->get_downCamPixPerMM();
        matPoint.x() += matMarker->get_xSquare() * MatMarker2d::SquareWidth;
        matPoint.y() += matMarker->get_ySquare() * MatMarker2d::SquareWidth;
        matPoint -= MatMarker2d::SquareWidth * .5f;
        matPoint.x() -= MatSection::Size.x() * .5f;
        matPoint.x() -= MatSection::Size.y() * .5f;
        
        if(not this->zAxisPointsUp) {
          matPoint.x() *= -1.f;
        }
        
        // orient1 = orient1 + marker.upAngle;
        Radians angle( matMarker->get_imagePose().get_angle()
                       + matMarker->get_upAngle() );
        
        Pose3d robotPose( Pose2d(angle, matPoint) );
        
        robot->set_pose( robotPose );
        
      } // if matMarker not NULL
      
    } // updateRobotPose()
    
    
  } // namespace Cozmo
} // namespace Anki
