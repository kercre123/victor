
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
    
    bool BlockWorld::ZAxisPointsUp = true;
    
    
    BlockWorld::BlockWorld( )
    : blocks(BlockWorld::MaxBlockTypes)
    {
      
    }
    
    BlockWorld::~BlockWorld(void)
    {
            
    } // BlockWorld Destructor
    
    
    void BlockWorld::addRobot(const u32 withID)
    {
      /*
      const u8& ID = robot->get_ID();
      
      if(this->robots.count(ID) > 0) {
        CORETECH_THROW("Robot with ID already in BlockWorld.");
      }
      
      this->robots[ID] = robot;
      */
      //robots.push_back(robot);
      robots.emplace_back();
      robots.back().addToWorld(withID);
      
    } // addRobot()
    
    void BlockWorld::queueMessage(const u8 *msg)
    {
      this->messages.push(msg);
    }
    
    void BlockWorld::update(void)
    {
      // Get updated observations from each robot and update blocks' and robots'
      // poses accordingly
      typedef std::multimap<BlockType, BlockMarker2d> BlockMarker2dMultiMap;
      BlockMarker2dMultiMap blockMarkers2d;
      
      for(Robot& robot : this->robots)
      {
        // Tell each robot to take a step, which will have it
        // check and parse any messages received from the physical
        // robot, and update its pose.
        robot.step();
        
        // Adds the 2d markers each robot saw to the multimap container,
        // grouped by the block type of the marker
        const std::vector<BlockMarker2d>& markers2d = robot.getVisibleBlockMarkers2d();
        for(const BlockMarker2d& marker2d : markers2d)
        {
          blockMarkers2d.emplace(marker2d.get_blockType(), marker2d);
        }
        
      } // for each robot
      
      // If any robots saw any markers, update/add the corresponding blocks:
      if(not blockMarkers2d.empty())
      {
        fprintf(stdout, "Saw %lu total BlockMarker2d's from all robots.\n",
                blockMarkers2d.size());
        
        // First for loop iterates over each unique BlockType in the multimap,
        // using upper_bound call.
        for(auto blockTypeIter = blockMarkers2d.begin(), end = blockMarkers2d.end();
            blockTypeIter != end;
            blockTypeIter = blockMarkers2d.upper_bound(blockTypeIter->first) )
        {
          // Instantiate a block of this type (at canonical location).  This in
          // turn will instantiate its faces' 3D markers.
          Block B_init(blockTypeIter->first);
          
          const std::vector<Block>::size_type blockTypeIndex = static_cast<std::vector<Block>::size_type>(B_init.get_type());
          
          // Get the range of iterators into the multimap with the current BlockType
          std::pair <BlockMarker2dMultiMap::iterator, BlockMarker2dMultiMap::iterator> range;
          range = blockMarkers2d.equal_range(blockTypeIter->first);
          
          // We will store a computed block pose for each 2d marker, with a
          // reference to that 2d marker.  (This is so we can recompute
          // Block pose from clusters of 2d markers later, without having to
          // reassociate them.)
          std::vector<std::pair<BlockMarker2d&, Pose3d> > blockPoses;
          
          // Second FOR loop iterates over each marker with this BlockType,
          // using the range we just got from equal_range.
          for(auto markerIter=range.first; markerIter != range.second; ++markerIter)
          {
            BlockMarker2d& marker2d = markerIter->second;
            
            // Inside this loop, all markers we consider should have the same
            // type as the Block we instantiated above.
            CORETECH_ASSERT(marker2d.get_blockType() == B_init.get_type());
            
            // Get the face from the current block type that has this marker's
            // face type.
            const Block::FaceName whichFace = Block::FaceType_to_FaceName(marker2d.get_faceType());
            const BlockMarker3d& marker3d = B_init.get_faceMarker(whichFace);
            
            // First update the head pose according to the blockMarker
            // TODO: this should probably not actually rotate the head but instead
            //       use the head angle from the correct timestamped state history?
            //       (doing it this way prevents this method from being const since
            //        it changes the robot for each marker)
            Robot& robot = marker2d.get_seenBy();
            robot.set_headAngle(marker2d.get_headAngle());
            
            // Figure out where the marker we saw is in 3D space, in camera's
            // coordinate frame. (Since the marker's coordinates relative to
            // the block are used here, the computed pose will be the blockPose
            // we want -- still relative to the camera however.)
            Quad3f corners3d;
            marker3d.getSquareCorners(corners3d, &(B_init.get_pose())); // corners3d relative to (canonical) Block pose
            Pose3d blockPose(robot.get_camHead().computeObjectPose(marker2d.get_quad(),
                                                                   corners3d) );
            
            // Now get the block pose in World coordinates using the pose tree,
            // instead of being in camera-centric coordinates, and add it to the
            // list of computed poses for this block type
            blockPoses.emplace_back(marker2d, blockPose.getWithRespectTo(Pose3d::World));
            
          } // for each marker with current blockType
          
          std::vector<Block> blocksSeen;
          
          if(blockPoses.size() > 1) {
            // TODO: cluster poses, recompute pose of each cluster, and a block to blocksSeenf for each
            
            fprintf(stdout, "Saw %lu markers with type %d, need to implement "
                    "clustering.\n", blockPoses.size(), B_init.get_type());
            
          } else {
            CORETECH_ASSERT(not blockPoses.empty());
            
            // Just have one marker of this block type
            B_init.set_pose(blockPoses[0].second);
            blocksSeen.push_back(B_init);
          }
          
          // Now go through all the blocks we saw, check if they overlap with a
          // block we already know about, or if they are a new one
          // TODO: be smarter about merge decisions
          // TODO: need a way to decide when to delete a block we've seen!!
          for(Block& blockSeen : blocksSeen) {
            
            const float minDimSeen = blockSeen.get_minDim();
            
            // Store pointers to any existing blocks that overlap with this one
            std::vector<Block*> overlapping;
            
            for(auto & blockExist : this->blocks[blockTypeIndex])
            {
              // TODO: smarter block pose comparison
              const float minDist = 0.5f*std::min(minDimSeen, blockExist.get_minDim());
              if( computeDistanceBetween(blockSeen.get_pose(),
                                         blockExist.get_pose()) < minDist )
              {
                overlapping.push_back(&blockExist);
              }
              
            } // for each existing block of this type
            
            if(overlapping.empty()) {
              // no existing blocks overlapped with the block we saw, so add it
              // as a new block
              
              fprintf(stdout, "Adding new block %lu at (%.1f, %.1f, %.1f)\n",
                      blockTypeIndex,
                      blockSeen.get_pose().get_translation().x(),
                      blockSeen.get_pose().get_translation().y(),
                      blockSeen.get_pose().get_translation().z());
              
              this->blocks[blockTypeIndex].push_back(blockSeen);
              
            } else {
              if(overlapping.size() > 1) {
                fprintf(stdout, "More than one overlapping block found -- will use first.\n");
                // TODO: do something smarter here?
              }
              
              fprintf(stdout, "Merging observation of block %lu at (%.1f, %.1f, %.1f)\n",
                      blockTypeIndex,
                      blockSeen.get_pose().get_translation().x(),
                      blockSeen.get_pose().get_translation().y(),
                      blockSeen.get_pose().get_translation().z());
              
              // TODO: better way of merging existing/observed block pose
              overlapping[0]->set_pose( blockSeen.get_pose() );
              
            } // if/else overlapping existing blocks found
            
          } // for each block seen
          
        } // for each blockType
        
      } // if we saw any block markers
      
    } // update()
    
    /*
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
    */
    
  } // namespace Cozmo
} // namespace Anki
