
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/messages.h"

#include "messageHandler.h"

namespace Anki
{
  namespace Cozmo
  {
    BlockWorld* BlockWorld::singletonInstance_ = 0;
    
    /*
    BlockWorld::BlockWorld( MessagingInterface* msgInterfaceIn )
    : msgInterface(msgInterfaceIn), blocks(MaxBlockTypes), zAxisPointsUp(true)
    {
      
    }
     */
    
    bool BlockWorld::ZAxisPointsUp = true;
    
    
    BlockWorld::BlockWorld( )
    : robotMgr_(RobotManager::getInstance()),
      msgHandler_(MessageHandler::getInstance())
    {
      // TODO: Create each known block / matpiece from a configuration/definitions file
      
      // Increment the ID counter each time we define a block
      ObjectID_t blockID=0;
      
      //
      // 1x1 Cubes
      //
      
      // "FUEL"
      {
        Block_Cube1x1* block = new Block_Cube1x1(++blockID);
        block->AddFace(Block::FRONT_FACE,
                       {115, 117, 167, 238, 206, 221, 156, 168,  58, 114, 118},
                       32.f);
        block->SetColor(1.f, 0.f, 0.f);
        block->SetSize(60.f, 60.f, 60.f);
        block->SetName("FUEL");
        
        //float temp = block->GetMinDim();
        blockLibrary_.AddObject(block);
      }
      
      
      //
      // 2x1
      //
      
      // "TEMP2x1"
      {/*
        Block_2x1* block = new Block_2x1(++blockID);
        block->AddFace(Block::FRONT_FACE,
                       {115, 117, 167, 238, 206, 221, 156, 168,  58, 114, 118},
                       32.f);
        block->AddFace(Block::LEFT_FACE,
                       {32, 115, 117, 167, 238, 206, 221, 156, 168,  58, 114, 118},
                       32.f);
        block->SetColor(0.f, 0.f, 1.f);
        block->SetSize(120.f, 60.f, 60.f);
        block->SetName("TEMP2x1");
        blockLibrary_.AddObject(block);
        */
      }
      
      //
      //
      //
      ObjectID_t matID=0;
      
      // Webots mat:
      {
        MatPiece* mat = new MatPiece(++matID);
        mat->AddMarker({24, 24, 24, 16, 25, 49, 56, 9, 56, 48, 32},
                       Pose3d(2.094395, {-0.577350,0.577350,-0.577350}, {-250.000000,250.000000,0.000000}),
                       90.000000);
        mat->AddMarker({35, 64, 70, 196, 140, 137, 8, 1, 48, 32, 34},
                       Pose3d(-3.141593, {1.000000,0.000000,0.000000}, {0.000000,250.000000,0.000000}),
                       90.000000);
        mat->AddMarker({35, 64, 70, 196, 141, 137, 8, 1, 48, 32, 34},
                       Pose3d(-3.141593, {1.000000,0.000000,0.000000}, {250.000000,250.000000,0.000000}),
                       90.000000);
        mat->AddMarker({24, 24, 24, 16, 24, 49, 56, 9, 56, 48, 32},
                       Pose3d(2.094395, {-0.577350,0.577350,-0.577350}, {-250.000000,0.000000,0.000000}),
                       90.000000);
        mat->AddMarker({35, 98, 2, 196, 136, 9, 24, 0, 32, 34, 98},
                       Pose3d(1.570796, {-1.000000,0.000000,0.000000}, {0.000000,0.000000,0.000000}),
                       90.000000);
        mat->AddMarker({8, 24, 24, 24, 8, 25, 56, 33, 56, 24, 48},
                       Pose3d(2.094395, {-0.577350,-0.577350,0.577350}, {250.000000,0.000000,0.000000}),
                       90.000000);
        mat->AddMarker({35, 98, 2, 196, 137, 9, 24, 1, 32, 34, 98},
                       Pose3d(1.570796, {-1.000000,0.000000,0.000000}, {-250.000000,-250.000000,0.000000}),
                       90.000000);
        mat->AddMarker({35, 98, 2, 196, 136, 9, 24, 1, 32, 34, 98},
                       Pose3d(1.570796, {-1.000000,0.000000,0.000000}, {0.000000,-250.000000,0.000000}),
                       90.000000);
        mat->AddMarker({8, 24, 24, 24, 9, 25, 56, 33, 56, 24, 48},
                       Pose3d(2.094395, {-0.577350,-0.577350,0.577350}, {250.000000,-250.000000,0.000000}),
                       90.000000);

        matLibrary_.AddObject(mat);
      }

    }
    
    /*
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
    
    void BlockWorld::computeIndividualBlockPoses(const VisionMarkerMultiMap&      markers,
                                                 const Vision::ObservableObject&  objInit,
                                                 std::vector<MarkerPosePair>&     objPoses)
    {
      // Get the range of iterators into the multimap with the current BlockType
      auto range = markers.equal_range(objInit.GetID());
      
      // This FOR loop iterates over each marker with this BlockType,
      // using the range we just got from equal_range.
      for(auto markerIter=range.first; markerIter != range.second; ++markerIter)
      {
        const Vision::Marker& marker2d = markerIter->second;
        
        // Inside this loop, all markers we consider should have the same
        // type as the Block we instantiated above.
//        CORETECH_ASSERT(marker2d() == B_init.get_type());
        
        Quad3f corners3d;
        //Pose3d blockPose( marker2d.EstimatePose(<#const Anki::Vision::Camera &wrtCamera#>))
        getCorrespondingCorners(marker2d, objInit, corners3d);
        const Vision::Camera* cam = marker2d.GetSeenBy();
        Pose3d objPose(cam->computeObjectPose(marker2d.GetCorners(), corners3d));
        
        // Now get the block pose in World coordinates using the pose tree,
        // instead of being in camera-centric coordinates, and add it to the
        // list of computed poses for this block type
        objPoses.emplace_back(marker2d, objPose.getWithRespectTo(Pose3d::World));
        
      } // for each marker with current blockType
    } // computeIndividualBlockPoses
    
    
    void BlockWorld::GroupPosesIntoObjects(const std::vector<MarkerPosePair>&      objPoses,
                                           const Vision::ObservableObject*         objInit,
                                           std::vector<Vision::ObservableObject*>& objectsSeen)
    {
      
      if(objPoses.size() > 1) {
        
        // Group markers which could be part of the same block (i.e. those
        // whose poses are colocated, noting that the pose is relative to
        // the center of the block)
        std::vector<std::vector<const MarkerPosePair*> > markerClusters;
        clusterBlockPoses(objPoses, 0.5f*objInit->GetMinDim(),
                          markerClusters);
        
        fprintf(stdout, "Grouping %lu observed 2D markers of type %d into %lu blocks.\n",
                objPoses.size(), objInit->GetID(), markerClusters.size());
        
        
        // For each new block, re-estimate the pose based on all markers that
        // were assigned to it
        for(auto & cluster : markerClusters)
        {
          CORETECH_ASSERT(not cluster.empty());
          
          if(cluster.size() == 1) {
            fprintf(stdout, "Singleton cluster: using already-computed pose.\n");
            
            // No need to re-estimate, just use the one marker we saw.
            objectsSeen.push_back(objInit->clone());
            objectsSeen.back()->SetPose(cluster[0]->second);
          }
          else {
            fprintf(stdout, "Re-computing pose from all memers of cluster.\n");
            
            std::vector<Point2f> imgPoints;
            std::vector<Point3f> objPoints;
            imgPoints.reserve(4*cluster.size());
            objPoints.reserve(4*cluster.size());
            
            // TODO: Implement ability to estimate block pose with markers seen by different robots
            const Vision::Camera* camera = cluster[0]->first.GetSeenBy();
            
            for(auto & clusterMember : cluster)
            {
              const Vision::Marker& marker2d = clusterMember->first;
              
              if(marker2d.GetSeenBy() == camera)
              {
                Quad3f corners3d;
                getCorrespondingCorners(marker2d, objInit, corners3d);
                
                for(Quad::CornerName i_face=Quad::FirstCorner;
                    i_face < Quad::NumCorners; ++i_face)
                {
                  imgPoints.push_back(marker2d.GetCorners()[i_face]);
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
            Pose3d objPose(camera->computeObjectPose(imgPoints, objPoints));
            
            // Now get the block pose in World coordinates using the pose
            // tree, instead of being in camera-centric coordinates, and
            // assign that pose to the temporary Block of this type
            objectsSeen.push_back(objInit->clone());
            objectsSeen.back()->SetPose(objPose.getWithRespectTo(Pose3d::World));
            
          } // if 1 or more members in this cluster
          
        } // for each cluster
        
      } else {
        CORETECH_ASSERT(not objPoses.empty());
        
        // Just have one marker of this block type
        objectsSeen.push_back(objInit->clone());
        objectsSeen.back()->SetPose(objPoses[0].second);
      }
    } // groupBlockPoses()
    */
    
    //template<class ObjectType>
    void FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                const std::map<ObjectID_t, std::vector<Vision::ObservableObject*> >& objectsExisting,
                                std::vector<Vision::ObservableObject*>& overlappingExistingObjects)
    {
      auto objectsExistingIter = objectsExisting.find(objectSeen->GetID());
      if(objectsExistingIter != objectsExisting.end())
      {
        for(auto objExist : objectsExistingIter->second)
        {
          // TODO: smarter block pose comparison
          const float minDist = 5.f; // TODO: make parameter ... 0.5f*std::min(minDimSeen, objExist->GetMinDim());
          const Radians angleThresh( 5.f*M_PI/180.f ); // TODO: make parameter
          Pose3d P_diff;
          if( objExist->IsSameAs(*objectSeen, minDist, angleThresh, P_diff) ) {
            overlappingExistingObjects.push_back(objExist);
          }
          
        } // for each existing object of this type
      }
      
    } // FindOverlappingObjects()
    
    
    void BlockWorld::AddAndUpdateObjects(const std::vector<Vision::ObservableObject*> objectsSeen,
                                         std::map<ObjectID_t, std::vector<Vision::ObservableObject*> >& objectsExisting)
    {
      for(auto objSeen : objectsSeen) {
        
        //const float minDimSeen = objSeen->GetMinDim();
        
        // Store pointers to any existing blocks that overlap with this one
        std::vector<Vision::ObservableObject*> overlappingObjects;
        FindOverlappingObjects(objSeen, objectsExisting, overlappingObjects);
        
        if(overlappingObjects.empty()) {
          // no existing blocks overlapped with the block we saw, so add it
          // as a new block
          
          fprintf(stdout, "Adding new block %hu at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetID(),
                  objSeen->GetPose().get_translation().x(),
                  objSeen->GetPose().get_translation().y(),
                  objSeen->GetPose().get_translation().z());
          
          objectsExisting[objSeen->GetID()].push_back(objSeen);
          
        }
        else {
          if(overlappingObjects.size() > 1) {
            fprintf(stdout, "More than one overlapping block found -- will use first.\n");
            // TODO: do something smarter here?
          }
          
          fprintf(stdout, "Merging observation of block %hu at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetID(),
                  objSeen->GetPose().get_translation().x(),
                  objSeen->GetPose().get_translation().y(),
                  objSeen->GetPose().get_translation().z());
          
          // TODO: better way of merging existing/observed block pose
          overlappingObjects[0]->SetPose( objSeen->GetPose() );
          
        } // if/else overlapping existing blocks found
        
      } // for each block seen

    } // AddAndUpdateObjects()
    
    
    void BlockWorld::Update(void)
    {
      robotMgr_->UpdateAllRobots();
      
      //
      // Localize robots using mat observations
      //
      std::vector<Vision::ObservableObject*> matsSeen;
      for(auto robotID : robotMgr_->GetRobotIDList())
      {
        Robot* robot = robotMgr_->GetRobotByID(robotID);
        
        // Get all mat objects *seen by this robot's camera*
        matsSeen.clear();
        matLibrary_.CreateObjectsFromMarkers(obsMarkers_, matsSeen,
                                             &robot->get_camHead());
        
        // TODO: what to do when a robot sees multiple mat pieces at the same time
        if(not matsSeen.empty()) {
          if(matsSeen.size() > 1) {
            PRINT_NAMED_WARNING("MultipleMatPiecesObserved",
                                "Robot %d observed more than one mat pieces at "
                                "the same time; will only use first for now.");
          }
          
          // At this point the mat's pose should be relative to the robot's
          // camera's pose
          const Pose3d* matWrtCamera = &(matsSeen[0]->GetPose());
          CORETECH_ASSERT(matWrtCamera->get_parent() ==
                          &(robot->get_camHead().get_pose())); // MatPose's parent is camera
          /*
          PRINT_INFO("Observed mat w.r.t. camera is (%f,%f,%f)\n",
                     matWrtCamera->get_translation().x(),
                     matWrtCamera->get_translation().y(),
                     matWrtCamera->get_translation().z());
          */
          
          // Now get the pose of the robot relative to the mat, using the pose
          // tree
          CORETECH_ASSERT(matWrtCamera->get_parent()->get_parent()->get_parent() ==
                          &(robot->get_pose())); // Robot pose is just a couple more up the pose chain
          
          Pose3d newPose( robot->get_pose().getWithRespectTo(matWrtCamera) );
          
          Pose3d P_diff;
          CORETECH_ASSERT( newPose.IsSameAs((*(robot->get_camHead().get_pose().get_parent()) *
                                             robot->get_camHead().get_pose() *
                                             (*matWrtCamera)).getInverse(),
                                            5.f, 5*M_PI/180.f, P_diff) );
          
          
          /*
          Pose3d newPose( (*(robot->get_camHead().get_pose().get_parent()) *
                           robot->get_camHead().get_pose() *
                           (*matWrtCamera)).getInverse() );
          */
          newPose.set_parent( Pose3d::World); // robot->get_pose().get_parent() );
          robot->set_pose(newPose);
          
          PRINT_INFO("Using mat %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f)\n",
                     matsSeen[0]->GetID(), robotID,
                     robot->get_pose().get_translation().x(),
                     robot->get_pose().get_translation().y(),
                     robot->get_pose().get_translation().z(),
                     robot->get_pose().get_rotationAngle().getDegrees(),
                     robot->get_pose().get_rotationAxis().x(),
                     robot->get_pose().get_rotationAxis().y(),
                     robot->get_pose().get_rotationAxis().z());

        } // IF any mat piece was seen
        
      } // FOR each robotID
      
      
      //
      // Find any observed blocks from the remaining markers
      //
      std::vector<Vision::ObservableObject*> blocksSeen;
      blockLibrary_.CreateObjectsFromMarkers(obsMarkers_, blocksSeen);
      
      // Use them to add or update existing blocks in our world
      AddAndUpdateObjects(blocksSeen, existingBlocks_);
      
      // TODO: Deal with unknown markers?
      
      // Toss any remaining markers?
      if(not obsMarkers_.empty()) {
        PRINT_INFO("%lu messages did not match any known objects and went unused.\n",
                   obsMarkers_.size());
        obsMarkers_.clear();
      }
      
    } // Update()
    
    
    void BlockWorld::QueueObservedMarker(const Vision::ObservedMarker& marker)
    {
      obsMarkers_.emplace_back(marker);
      //obsMarkersByRobot_[seenByRobot].push_back(&obsMarkers_.back());
    }
    
    void BlockWorld::CommandRobotToDock(const RobotID_t whichRobot,
                                        const Block&    whichBlock)
    {
      Robot* robot = robotMgr_->GetRobotByID(whichRobot);
      if(robot != 0)
      {
        robot->dockWithBlock(whichBlock);
        
      } else {
        fprintf(stdout, "Invalid robot commanded to Dock.\n");
      }
    } // commandRobotToDock()
    
    // MatPiece has no rotation ambiguities but we still need to define this
    // static const here to instatiate an empty list.
    const std::vector<RotationMatrix3d> MatPiece::rotationAmbiguities_;
    
    std::vector<RotationMatrix3d> const& MatPiece::GetRotationAmbiguities() const
    {
      return MatPiece::rotationAmbiguities_;
    }

    
  } // namespace Cozmo
} // namespace Anki
