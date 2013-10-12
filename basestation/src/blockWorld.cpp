
#include "anki/cozmo/basestation/blockWorld.h"
#include "block.h"
#include "mat.h"
#include "robot.h"

namespace Anki
{
  namespace Cozmo
  {
    
    BlockWorld::BlockWorld( void )
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
      for( Robot* robot : this->robots )
      {
        if(robot != NULL)
        {
          delete robot;
        }
      }
      
    } // BlockWorld Destructor
    
    
    void BlockWorld::addRobot(Robot *robot)
    {
      robots.push_back(robot);
    } // addRobot()
    
    void BlockWorld::update(void)
    {
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
        updateRobotPose(robot);
                          
        // Each robot adds the 3d markers it saw to the list.
        robot->getVisibleBlockMarkers3d(blockMarkers);
        
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
        matPoint.x() -= Mat::Size.x() * .5f;
        matPoint.x() -= Mat::Size.y() * .5f;
        
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
