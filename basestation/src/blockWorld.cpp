
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
    
    
    void updateSelectedBlock(Robot& robot, const Block* currentWorldBlock)
    {
      CORETECH_ASSERT(currentWorldBlock != NULL);
      
      const Camera& cam = robot.get_camHead();
      
      // Get the block's pose w.r.t. this robot's camera, so we can
      // project it
      Pose3d crntPose = currentWorldBlock->get_pose().getWithRespectTo(&(cam.get_pose()));
      
      Point2f imgPosCrnt;
      cam.project3dPoint(crntPose.get_translation(), imgPosCrnt);
      
      if(imgPosCrnt.x() >= 0.f && imgPosCrnt.y() >= 0.f)
      {
        const Point2f& imgCen = cam.get_calibration().get_center();
        
        const Block* blockSel = robot.get_selectedBlock();
        
        if(blockSel == NULL) {
          // If the robot does not have a selected block, use this one
          robot.set_selectedBlock(currentWorldBlock);
        }
        else if(blockSel != currentWorldBlock) { // selected not already this blockSeen
          
          // get the currently-selected block's origin projected into the
          // robot's image
          Pose3d selPose = blockSel->get_pose().getWithRespectTo(&(cam.get_pose()));
          
          Point2f imgPosSel;
          cam.project3dPoint(selPose.get_translation(), imgPosSel);
          
          const f32 crntDist = computeDistanceBetween(imgPosCrnt, imgCen);
          const f32 selDist  = computeDistanceBetween(imgPosSel,  imgCen);
          
          // if blockSeen's projected origin is closer to the image
          // center than the currently-selected one, use this
          // blockSeen as the new selection for this robot
          if(crntDist < selDist) {
            robot.set_selectedBlock(currentWorldBlock);
          }
          
        } // if/else blockSel==NULL
        
      } // if this blockSeen is within this robot's image
      
    } // updateSelectedBlock()

    
    void getCorrespondingCorners(const BlockMarker2d& marker2d,
                                 const Block&         block,
                                 Quad3f&              corners3d)
    {
      const Block::FaceName whichFace = Block::FaceType_to_FaceName(marker2d.get_faceType());
      const BlockMarker3d& marker3d = block.get_faceMarker(whichFace);
      
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
      marker3d.getSquareCorners(corners3d, &(block.get_pose())); // corners3d relative to (canonical) Block pose
      
    } // getCorrespondingCorners()
    
    void BlockWorld::clusterBlockPoses(const std::vector<MarkerPosePair>& blockPoses,
                                       const f32 distThreshold,
                                       std::vector<std::vector<const MarkerPosePair*> >& markerClusters)
    {
      std::vector<bool> assigned(blockPoses.size(), false);
      
      for(size_t i_pose=0; i_pose < blockPoses.size(); ++i_pose)
      {
        if(not assigned[i_pose])
        {
          // Start a new block with this marker and mark it as assigned
          // to a block
          markerClusters.emplace_back(); // adds new cluster
          markerClusters.back().emplace_back(&(blockPoses[i_pose])); // add this marker to that cluster
          assigned[i_pose] = true;
          
          // See how far unassigned marker's poses are from this one:
          for(size_t i_other=0; i_other < blockPoses.size(); ++i_other)
          {
            if(not assigned[i_other])
            {
              f32 dist = computeDistanceBetween(blockPoses[i_pose].second,
                                                blockPoses[i_other].second);
              
              // If close enough, add it to the current cluster
              if(dist < distThreshold) {
                markerClusters.back().emplace_back(&(blockPoses[i_other]));
              }
            } // if not assigned
          } // for each other blockPose
        } // if not assigned
      } // for each blockPose
      
      // Sanity check:
      for(bool each_assigned : assigned)
      {
        // All markers should have been assigned a block
        CORETECH_ASSERT(each_assigned);
      }
      
    } // clusterBlockPoses()
    
    
    void BlockWorld::addAndUpdateBlocks(BlockMarker2dMultiMap& blockMarkers2d)
    {
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
          std::vector<MarkerPosePair> blockPoses;
          
          // Second FOR loop iterates over each marker with this BlockType,
          // using the range we just got from equal_range.
          for(auto markerIter=range.first; markerIter != range.second; ++markerIter)
          {
            BlockMarker2d& marker2d = markerIter->second;
            
            // Inside this loop, all markers we consider should have the same
            // type as the Block we instantiated above.
            CORETECH_ASSERT(marker2d.get_blockType() == B_init.get_type());
            
            Quad3f corners3d;
            getCorrespondingCorners(marker2d, B_init, corners3d);
            const Camera& cam = marker2d.get_seenBy().get_camHead();
            Pose3d blockPose(cam.computeObjectPose(marker2d.get_quad(), corners3d));
            
            // Now get the block pose in World coordinates using the pose tree,
            // instead of being in camera-centric coordinates, and add it to the
            // list of computed poses for this block type
            blockPoses.emplace_back(marker2d, blockPose.getWithRespectTo(Pose3d::World));
            
          } // for each marker with current blockType
          
          std::vector<Block> blocksSeen;
          
          if(blockPoses.size() > 1) {
            
            // Group markers which could be part of the same block (i.e. those
            // whose poses are colocated, noting that the pose is relative to
            // the center of the block)
            std::vector<std::vector<const MarkerPosePair*> > markerClusters;
            clusterBlockPoses(blockPoses, 0.5f*B_init.get_minDim(),
                              markerClusters);
            
            fprintf(stdout, "Grouping %lu observed 2D markers of type %d into %lu blocks.\n",
                    blockPoses.size(), B_init.get_type(), markerClusters.size());

            
            // For each new block, re-estimate the pose based on all markers that
            // were assigned to it
            for(auto & cluster : markerClusters)
            {
              CORETECH_ASSERT(not cluster.empty());
              
              if(cluster.size() == 1) {
                // No need to re-estimate, just use the one marker we saw.
                B_init.set_pose(cluster[0]->second);
                blocksSeen.push_back(B_init);
              }
              else {
                std::vector<Point2f> imgPoints;
                std::vector<Point3f> objPoints;
                imgPoints.reserve(4*cluster.size());
                objPoints.reserve(4*cluster.size());
                
                // TODO: Implement ability to estimate block pose with markers seen by different robots
                const Robot* robot = &(cluster[0]->first.get_seenBy());
                
                for(auto & clusterMember : cluster)
                {
                  const BlockMarker2d& marker2d = clusterMember->first;
                  
                  if(&(marker2d.get_seenBy()) == robot)
                  {
                    Quad3f corners3d;
                    getCorrespondingCorners(marker2d, B_init, corners3d);
                    
                    for(Quad::CornerName i_face=Quad::FirstCorner;
                        i_face < Quad::NumCorners; ++i_face)
                    {
                      imgPoints.push_back(marker2d.get_quad()[i_face]);
                      objPoints.push_back(corners3d[i_face]);
                    }
                  }
                  else {
                    fprintf(stdout, "Ability to re-estimate single block's "
                            "pose from markers seen by two different robots "
                            "not yet implemented. Will just use markers from "
                            "first robot in the cluster.\n");
                  }
                  
                } // for each cluster member
                
                // Compute the block pose from all the corresponding 2d (image)
                // and 3d (object) points
                Pose3d blockPose(robot->get_camHead().computeObjectPose(imgPoints, objPoints));
                
                // Now get the block pose in World coordinates using the pose
                // tree, instead of being in camera-centric coordinates, and
                // assign that pose to the temporary Block of this type
                B_init.set_pose(blockPose.getWithRespectTo(Pose3d::World));
                
                // Add this to the list of blocks seen
                blocksSeen.push_back(B_init);
                
              } // if 1 or more members in this cluster
              
            } // for each cluster
            
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
            
            const Block* currentWorldBlock = NULL;
            if(overlapping.empty()) {
              // no existing blocks overlapped with the block we saw, so add it
              // as a new block
              
              fprintf(stdout, "Adding new block %lu at (%.1f, %.1f, %.1f)\n",
                      blockTypeIndex,
                      blockSeen.get_pose().get_translation().x(),
                      blockSeen.get_pose().get_translation().y(),
                      blockSeen.get_pose().get_translation().z());
              
              this->blocks[blockTypeIndex].push_back(blockSeen);
              currentWorldBlock = &(this->blocks[blockTypeIndex].back());
              
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
              
              currentWorldBlock = overlapping[0];
              
            } // if/else overlapping existing blocks found
            
            // Set the updated world block as the selected block for each robot for
            // which it's centroid projects closest to the robot's center of
            // field of view (and actually visible by that robot)
            if(currentWorldBlock != NULL)
            {
              for(Robot& robot : this->robots)
              {
                //updateSelectedBlock(robot, currentWorldBlock);
              } // for each robot
            } // if currentWorldBlock not NULL
            
          } // for each block seen
          
        } // for each blockType
        
      } // if we saw any block markers
      
    } // addAndUpdateBlocks()
    
    
    void BlockWorld::update(void)
    {
      // Get updated observations from each robot and update blocks' and robots'
      // poses accordingly
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
      
      addAndUpdateBlocks(blockMarkers2d);
      
    } // update()
    
    void BlockWorld::commandRobotToDock(const size_t whichRobot) 
    {
      if(whichRobot < this->robots.size())
      {
        
        this->robots[whichRobot].dockWithSelectedBlock();
        
      } else {
        fprintf(stdout, "Invalid robot commanded to Dock.\n");
      }
    } // commandRobotToDock()
    
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
