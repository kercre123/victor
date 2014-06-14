
// TODO: this include is shared b/w BS and Robot.  Move up a level.
#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/common/shared/utilities_shared.h"

#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"


#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robot.h"

#include "messageHandler.h"
#include "vizManager.h"

namespace Anki
{
  namespace Cozmo
  {
    //BlockWorld* BlockWorld::singletonInstance_ = 0;
    const std::map<ObjectID_t, Vision::ObservableObject*> BlockWorld::EmptyObjectMap;
    
    /*
    BlockWorld::BlockWorld( MessagingInterface* msgInterfaceIn )
    : msgInterface(msgInterfaceIn), blocks(MaxBlockTypes), zAxisPointsUp(true)
    {
      
    }
     */
    
    //bool BlockWorld::ZAxisPointsUp = true;
    
    
    BlockWorld::BlockWorld( )
    : isInitialized_(false)
    , robotMgr_(NULL)
    , didBlocksChange_(false)
    , globalIDCounter(0)
    , enableDraw_(false)
//    : robotMgr_(RobotManager::getInstance()),
//      msgHandler_(MessageHandler::getInstance())
    {
      // TODO: Create each known block / matpiece from a configuration/definitions file
      
      //
      // 1x1 Cubes
      //
      //blockLibrary_.AddObject(new Block_Cube1x1(Block::FUEL_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::ANGRYFACE_BLOCK_TYPE));

      blockLibrary_.AddObject(new Block_Cube1x1(Block::BULLSEYE2_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::SQTARGET_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::FIRE_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::ANKILOGO_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::STAR5_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::DICE_BLOCK_TYPE));
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::NUMBER1_BLOCK_TYPE));
      blockLibrary_.AddObject(new Block_Cube1x1(Block::NUMBER2_BLOCK_TYPE));
      blockLibrary_.AddObject(new Block_Cube1x1(Block::NUMBER3_BLOCK_TYPE));
      blockLibrary_.AddObject(new Block_Cube1x1(Block::NUMBER4_BLOCK_TYPE));
      blockLibrary_.AddObject(new Block_Cube1x1(Block::NUMBER5_BLOCK_TYPE));
      blockLibrary_.AddObject(new Block_Cube1x1(Block::NUMBER6_BLOCK_TYPE));

      blockLibrary_.AddObject(new Block_Cube1x1(Block::BANGBANGBANG_BLOCK_TYPE));
      
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
        block->SetColor(0, 0, 0xff);
        block->SetSize(120.f, 60.f, 60.f);
        block->SetName("TEMP2x1");
        blockLibrary_.AddObject(block);
        */
      }
      
      //
      //
      //
      ObjectType_t matType=0;
      
      // Webots mat:
      MatPiece* mat = new MatPiece(++matType);
        
      matLibrary_.AddObject(mat);
      
    } // BlockWorld() Constructor
    
    BlockWorld::~BlockWorld()
    {
      for(auto blockTypes : existingBlocks_) {
        for(auto blockIDs : blockTypes.second) {
          delete blockIDs.second;
        }
      }
        
    } // ~BlockWorld() Destructor
    
    
    void BlockWorld::Init(RobotManager* robotMgr)
    {
      robotMgr_ = robotMgr;
      
      isInitialized_ = true;
    }
    
    
    //template<class ObjectType>
    void BlockWorld::FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                            const ObjectsMap_t& objectsExisting,
                                            std::vector<Vision::ObservableObject*>& overlappingExistingObjects) const
    {
      // TODO: We should really be taking uncertainty/distance into account
      //const float distThresholdFraction = 0.05f;
      const float   distThresh_mm = 20.f; // large to handle higher error at a distance
      const Radians angleThresh( DEG_TO_RAD(45.f) );
      
      // TODO: make angle threshold also vary with distance?
      // TODO: make these parameters/arguments
      
      auto objectsExistingIter = objectsExisting.find(objectSeen->GetType());
      if(objectsExistingIter != objectsExisting.end())
      {
        for(auto objExist : objectsExistingIter->second)
        {
          // TODO: smarter block pose comparison
          //const float minDist = 5.f; // TODO: make parameter ... 0.5f*std::min(minDimSeen, objExist->GetMinDim());
          
          //const float distToExist_mm = (objExist.second->GetPose().get_translation() -
          //                              <robotThatSawMe???>->GetPose().get_translation()).length();
          
          //const float distThresh_mm = distThresholdFraction * distToExist_mm;
          
          Pose3d P_diff;
          if( objExist.second->IsSameAs(*objectSeen, distThresh_mm, angleThresh, P_diff) ) {
            overlappingExistingObjects.push_back(objExist.second);
          } /*else {
            fprintf(stdout, "Not merging: Tdiff = %.1fmm, Angle_diff=%.1fdeg\n",
                    P_diff.get_translation().length(), P_diff.get_rotationAngle().getDegrees());
            objExist.second->IsSameAs(*objectSeen, distThresh_mm, angleThresh, P_diff);
          }*/
          
        } // for each existing object of this type
      }
      
    } // FindOverlappingObjects()
    
    
    
    void ClearAllOcclusionMaps(RobotManager* robotMgr)
    {
      for(auto robotID : robotMgr->GetRobotIDList()) {
        Robot* robot = robotMgr->GetRobotByID(robotID);
        CORETECH_ASSERT(robot != NULL);
        Vision::Camera& camera = robot->GetCamera();
        camera.ClearOccluders();
      }
    } // ClearAllOcclusionMaps()

    void AddToOcclusionMaps(const Vision::ObservableObject* object,
                            RobotManager* robotMgr)
    {

      for(auto robotID : robotMgr->GetRobotIDList()) {
        Robot* robot = robotMgr->GetRobotByID(robotID);
        CORETECH_ASSERT(robot != NULL);
        
        Vision::Camera& camera = robot->GetCamera();
        
        camera.AddOccluder(*object);
      }
      
    } // AddToOcclusionMaps()
    
    
    
    void BlockWorld::AddAndUpdateObjects(const std::vector<Vision::ObservableObject*>& objectsSeen,
                                         ObjectsMap_t& objectsExisting,
                                         const TimeStamp_t atTimestamp)
    {
      for(auto objSeen : objectsSeen) {
        
        //const float minDimSeen = objSeen->GetMinDim();
        
        // Store pointers to any existing blocks that overlap with this one
        std::vector<Vision::ObservableObject*> overlappingObjects;
        FindOverlappingObjects(objSeen, objectsExisting, overlappingObjects);
        
        if(overlappingObjects.empty()) {
          // no existing blocks overlapped with the block we saw, so add it
          // as a new block
          objSeen->SetID(++globalIDCounter);
          objectsExisting[objSeen->GetType()][objSeen->GetID()] = objSeen;
          
          fprintf(stdout, "Adding new block with type=%hu and ID=%hu at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetType(), objSeen->GetID(),
                  objSeen->GetPose().get_translation().x(),
                  objSeen->GetPose().get_translation().y(),
                  objSeen->GetPose().get_translation().z());
          
          // Project this new block into each camera
          AddToOcclusionMaps(objSeen, robotMgr_);
          
        }
        else {
          if(overlappingObjects.size() > 1) {
            fprintf(stdout, "More than one overlapping block found -- will use first.\n");
            // TODO: do something smarter here?
          }
          
          /* This is pretty verbose...
          fprintf(stdout, "Merging observation of block type=%hu, ID=%hu at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetType(), objSeen->GetID(),
                  objSeen->GetPose().get_translation().x(),
                  objSeen->GetPose().get_translation().y(),
                  objSeen->GetPose().get_translation().z());
          */
          
          // TODO: better way of merging existing/observed block pose
          overlappingObjects[0]->SetPose( objSeen->GetPose() );
          
          // Update lastObserved times of this block
          overlappingObjects[0]->SetLastObservedTime(objSeen->GetLastObservedTime());
          overlappingObjects[0]->UpdateMarkerObservationTimes(*objSeen);
          
          // Project this existing block into each camera, using its new pose
          AddToOcclusionMaps(overlappingObjects[0], robotMgr_);

          // Now that we've merged in objSeen, we can delete it because we
          // will no longer be using it.  Otherwise, we'd leak.
          delete objSeen;
          
        } // if/else overlapping existing blocks found
     
        didBlocksChange_ = true;
      } // for each block seen
      
      // Create a list of unobserved objects for further consideration below.
      // Use pairs of iterators to make deleting blocks below easier.
      std::vector<std::pair<ObjectsMap_t::iterator, ObjectsMapByID_t::iterator> > unobservedObjects;
      //for(auto & objectTypes : objectsExisting) {
      for(auto objectTypeIter = objectsExisting.begin();
          objectTypeIter != objectsExisting.end(); ++objectTypeIter)
      {
        ObjectsMapByID_t& objectIdMap = objectTypeIter->second;
        for(auto objectIter = objectIdMap.begin();
            objectIter != objectIdMap.end(); ++objectIter)
        {
          Vision::ObservableObject* object = objectIter->second;;
          if(object->GetLastObservedTime() < atTimestamp) {
            //AddToOcclusionMaps(object, robotMgr_); // TODO: Used to do this too, put it back?
            unobservedObjects.emplace_back(objectTypeIter, objectIter);
          } // if object was not observed
          else {
            // Object was observed, update it's visualization

            // TODO: make this more generic. For now, we are assuming all objects are blocks
            const Block* block = dynamic_cast<Block*>(object);
            if(block != nullptr) {
              // TODO: use the block's color?
              block->Visualize(VIZ_COLOR_DEFAULT);
            }
          }
        } // for object IDs of this type
      } // for each object type
      
      // Now that the occlusion maps are complete, check each unobserved object's
      // visibility in each camera
      for(auto unobserved : unobservedObjects) {
        // NOTE: this is assuming all objects are blocks
        Vision::ObservableObject* object = unobserved.second->second;
        
        for(auto robotID : robotMgr_->GetRobotIDList()) {
          
          Robot* robot = robotMgr_->GetRobotByID(robotID);
          CORETECH_ASSERT(robot != NULL);
          
          if(object->IsVisibleFrom(robot->GetCamera(), DEG_TO_RAD(45), 20.f, true))
          {
            // We "should" have seen the object! Delete it or mark it somehow
            CoreTechPrint("Removing object %d, which should have been seen, "
                          "but wasn't.\n", object->GetID());
            
            // Erase the vizualized block and its projected quad
            VizManager::getInstance()->EraseCuboid(object->GetID());
            VizManager::getInstance()->EraseQuad(object->GetID());
            
            // Actually erase the object from blockWorld's container of
            // existing objects, using the iterator pointing to it
            unobserved.first->second.erase(unobserved.second);
            
            didBlocksChange_ = true;
          }
          
        } // for each camera
      } // for each unobserved object

      
    } // AddAndUpdateObjects()
    
    void BlockWorld::GetObsMarkerList(const PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap,
                                      std::list<Vision::ObservedMarker*>& lst)
    {
      lst.clear();
      for(auto & poseKeyMarkerPair : poseKeyObsMarkerMap)
      {
        lst.push_back((Vision::ObservedMarker*)(&(poseKeyMarkerPair.second)));
      }
    }

    void BlockWorld::RemoveUsedMarkers(PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap)
    {
      for(auto poseKeyMarkerPair = poseKeyObsMarkerMap.begin(); poseKeyMarkerPair != poseKeyObsMarkerMap.end();)
      {
        if (poseKeyMarkerPair->second.IsUsed()) {
          poseKeyMarkerPair = poseKeyObsMarkerMap.erase(poseKeyMarkerPair);
        } else {
          ++poseKeyMarkerPair;
        }
      }
    }

    
    
    void BlockWorld::GetBlockBoundingBoxesXY(const f32 minHeight, const f32 maxHeight,
                                             const f32 padding,
                                             std::vector<Quad2f>& rectangles,
                                             const std::set<ObjectType_t>& ignoreTypes,
                                             const std::set<ObjectID_t>& ignoreIDs) const
    {
      for(auto & blocksWithType : existingBlocks_)
      {
        const bool useType = ignoreTypes.find(blocksWithType.first) == ignoreTypes.end();
        if(useType) {
          for(auto & blockAndId : blocksWithType.second)
          {
            Block* block = dynamic_cast<Block*>(blockAndId.second);
            CORETECH_THROW_IF(block == nullptr);
            const f32 blockHeight = block->GetPose().get_translation().z();
            
            // If this block's ID is not in the ignore list, then we will use it
            const bool useID = ignoreIDs.find(blockAndId.first) == ignoreIDs.end();
            
            if( (blockHeight >= minHeight) && (blockHeight <= maxHeight) && useID )
            {
              rectangles.emplace_back(block->GetBoundingQuadXY(padding));
            }
          }
        } // if(useType)
      }
    } // GetBlockBoundingBoxesXY()
    
    
    bool BlockWorld::DidBlocksChange() const {
      return didBlocksChange_;
    }
    
    
    bool BlockWorld::UpdateRobotPose(Robot* robot, PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp, const TimeStamp_t atTimestamp)
    {
      bool wasPoseUpdated = false;
      
      // Extract only observed markers from obsMarkersAtTimestamp
      std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
      GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
      
      // Get all mat objects *seen by this robot's camera*
      std::vector<Vision::ObservableObject*> matsSeen;
      matLibrary_.CreateObjectsFromMarkers(obsMarkersListAtTimestamp, matsSeen,
                                           (robot->GetCamera().GetId()));

      // Remove used markers from map
      RemoveUsedMarkers(obsMarkersAtTimestamp);
      
      // TODO: what to do when a robot sees multiple mat pieces at the same time
      if(not matsSeen.empty()) {
        if(matsSeen.size() > 1) {
          PRINT_NAMED_WARNING("MultipleMatPiecesObserved",
                              "Robot %d observed more than one mat pieces at "
                              "the same time; will only use first for now.",
                              robot->GetID());
        }
        
        // Add observed mat markers to the occlusion map of the camera that saw
        // them, so we can use them to delete objects that should have been
        // seen between that marker and the robot
        std::vector<const Vision::KnownMarker *> observedMarkers;
        matsSeen[0]->GetObservedMarkers(observedMarkers, atTimestamp);
        for(auto obsMarker : observedMarkers) {
          robot->GetCamera().AddOccluder(*obsMarker);
        }
        
       /*
        // At this point the mat's pose should be relative to the robot's
        // camera's pose
        const Pose3d* matWrtCamera = &(matsSeen[0]->GetPose());
        CORETECH_ASSERT(matWrtCamera->get_parent() ==
                        &(robot->GetCamera().get_pose())); // MatPose's parent is camera
        */
        
        /*
         PRINT_INFO("Observed mat w.r.t. camera is (%f,%f,%f)\n",
         matWrtCamera->get_translation().x(),
         matWrtCamera->get_translation().y(),
         matWrtCamera->get_translation().z());
         */
        
        /*
        // Now get the pose of the robot relative to the mat, using the pose
        // tree
        CORETECH_ASSERT(matWrtCamera->get_parent()->get_parent()->get_parent() ==
                        &(robot->get_pose())); // Robot pose is just a couple more up the pose chain
        
        Pose3d newPose( robot->get_pose().getWithRespectTo(matWrtCamera) );
        */
        
        // Get computed RobotPoseStamp at the time the object was observed.
        RobotPoseStamp* posePtr = nullptr;
        if (robot->GetComputedPoseAt(matsSeen[0]->GetLastObservedTime(), &posePtr) == RESULT_FAIL) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.CouldNotFindHistoricalPose", "Time %d\n", matsSeen[0]->GetLastObservedTime());
          return false;
        }
        
        const Pose3d* matPose = &(matsSeen[0]->GetPose());
        Pose3d newPose( posePtr->GetPose().getWithRespectTo(matPose) );
        
        /*
        Pose3d P_diff;
        CORETECH_ASSERT( newPose.IsSameAs((*(robot->GetCamera().get_pose().get_parent()) *
                                           robot->GetCamera().get_pose() *
                                           (*matPose)).getInverse(),
                                          5.f, 5*M_PI/180.f, P_diff) );
        */
        
        /*
         Pose3d newPose
         ( (*(robot->GetCamera().get_pose().get_parent()) *
         robot->GetCamera().get_pose() *
         (*matWrtCamera)).getInverse() );
         */
        newPose.set_parent(Pose3d::World); // robot->get_pose().get_parent() );
        
        // If there is any significant rotation, make sure that it is roughly
        // around the Z axis
        // TODO: Should grab the actual z-axis rotation here instead of assuming the rotationAngle is good enough.
        Radians rotAngle;
        Vec3f rotAxis;
        newPose.get_rotationVector().get_angleAndAxis(rotAngle, rotAxis);
        const float dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
        const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
        if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.RobotNotOnHorizontalPlane", "");
          return false;
        }

        // We have a new ("ground truth") key frame. Increment the pose frame!
        robot->IncrementPoseFrameID();
        
        // Add the new vision-based pose to the robot's history.
        RobotPoseStamp p(robot->GetPoseFrameID(), newPose, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
        if(robot->AddVisionOnlyPoseToHistory(matsSeen[0]->GetLastObservedTime(), p) != RESULT_OK) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.FailedAddingVisionOnlyPoseToHistory", "");
        }
        
        // Update the computed pose as well so that subsequent block pose updates
        // use obsMarkers whose camera's parent pose is correct
        posePtr->SetPose(robot->GetPoseFrameID(), newPose, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());

        // Compute the new "current" pose from history which uses the
        // past vision-based "ground truth" pose we just computed.
        if(!robot->UpdateCurrPoseFromHistory()) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.FailedUpdateCurrPoseFromHistory", "");
          return false;
        }
        wasPoseUpdated = true;
        
        PRINT_INFO("Using mat %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f)\n",
                   matsSeen[0]->GetID(), robot->GetID(),
                   robot->GetPose().get_translation().x(),
                   robot->GetPose().get_translation().y(),
                   robot->GetPose().get_translation().z(),
                   robot->GetPose().get_rotationAngle().getDegrees(),
                   robot->GetPose().get_rotationAxis().x(),
                   robot->GetPose().get_rotationAxis().y(),
                   robot->GetPose().get_rotationAxis().z());
        
        // Send the ground truth pose that was computed instead of the new current pose and let the robot deal with
        // updating its current pose based on the history that it keeps.
        robot->SendAbsLocalizationUpdate();
        
        // Done using mat pieces to localize, which were cloned from library
        // mat objects.  Delete them now so we don't leak.
        for(auto matSeen : matsSeen) {
          delete matSeen;
        }
        
      } // IF any mat piece was seen

      return wasPoseUpdated;
      
    } // UpdateRobotPose()
    
    
    size_t BlockWorld::UpdateBlockPoses(PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp, const TimeStamp_t atTimestamp)
    {
      didBlocksChange_ = false;
      
      std::vector<Vision::ObservableObject*> blocksSeen;
      
      // Don't bother with this update at all if we didn't see at least one
      // marker (which is our indication we got an update from the robot's
      // vision system
      if(not obsMarkers_.empty()) {
        
        // Extract only observed markers from obsMarkersAtTimestamp
        std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
        GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
        
        blockLibrary_.CreateObjectsFromMarkers(obsMarkersListAtTimestamp, blocksSeen);
        
        // Remove used markers from map
        RemoveUsedMarkers(obsMarkersAtTimestamp);
        
      }
      
      // Use them to add or update existing blocks in our world
      // NOTE: we still want to run this even if we didn't see markers because
      // we want to possibly delete any _unobserved_ objects (e.g. ones behind
      // whom we saw mat markers)
      AddAndUpdateObjects(blocksSeen, existingBlocks_, atTimestamp);
      
      return blocksSeen.size();
      
    } // UpdateBlockPoses()
    
    void BlockWorld::Update(uint32_t& numBlocksObserved)
    {
      CORETECH_ASSERT(isInitialized_);
      CORETECH_ASSERT(robotMgr_ != NULL);
      
      numBlocksObserved = 0;
      
      // New timestep, new set of occluders.  Get rid of anything registered as
      // an occluder with any robot's camera
      ClearAllOcclusionMaps(robotMgr_);

      static TimeStamp_t lastObsMarkerTime = 0;
      
      // Now we're going to process all the observed messages, grouped by
      // timestamp
      size_t numUnusedMarkers = 0;
      for(auto obsMarkerListMapIter = obsMarkers_.begin();
          obsMarkerListMapIter != obsMarkers_.end();
          ++obsMarkerListMapIter)
      {
        PoseKeyObsMarkerMap_t& currentObsMarkers = obsMarkerListMapIter->second;
        const TimeStamp_t atTimestamp = obsMarkerListMapIter->first;
        
        lastObsMarkerTime = std::max(lastObsMarkerTime, atTimestamp);
        
        //
        // Localize robots using mat observations
        //
        for(auto robotID : robotMgr_->GetRobotIDList())
        {
          Robot* robot = robotMgr_->GetRobotByID(robotID);
          
          CORETECH_ASSERT(robot != NULL);
      
          // Remove observed markers whose historical poses have become invalid.
          // This shouldn't happen! If it does, robotStateMsgs may be buffering up somewhere.
          // Increasing history time window would fix this, but it's not really a solution.
          for(auto poseKeyMarkerPair = currentObsMarkers.begin(); poseKeyMarkerPair != currentObsMarkers.end();) {
            if ((poseKeyMarkerPair->second.GetSeenBy().GetId() == robot->GetCamera().GetId()) &&
                !robot->IsValidPoseKey(poseKeyMarkerPair->first)) {
              PRINT_NAMED_WARNING("BlockWorld.Update.InvalidHistPoseKey", "key=%d\n", poseKeyMarkerPair->first);
              poseKeyMarkerPair = currentObsMarkers.erase(poseKeyMarkerPair++);
            } else {
              ++poseKeyMarkerPair;
            }
          }
        
          
          // Note that this removes markers from the list that it uses
          UpdateRobotPose(robot, currentObsMarkers, atTimestamp);
        
        } // FOR each robotID
      
      
        //
        // Find any observed blocks from the remaining markers
        //
        // Note that this removes markers from the list that it uses
        numBlocksObserved += UpdateBlockPoses(currentObsMarkers, atTimestamp);
     
        // TODO: Deal with unknown markers?
        
        
        // Keep track of how many markers went unused by either robot or block
        // pose updating processes above
        numUnusedMarkers += currentObsMarkers.size();
        
      } // for element in obsMarkers_
      
      //PRINT_NAMED_INFO("BlockWorld.Update.NumBlocksObserved", "Saw %d blocks\n", numBlocksObserved);
      
      // Check for unobserved, uncarried blocks that overlap with any robot's position
      for(auto & blocksOfType : existingBlocks_) {
        
        for(auto blockIter = blocksOfType.second.begin();
            blockIter != blocksOfType.second.end(); )
        {
          Block* block = dynamic_cast<Block*>(blockIter->second);
          CORETECH_THROW_IF(block == nullptr);
          
          bool didErase = false;
          if(block->GetLastObservedTime() < lastObsMarkerTime && !block->GetIsBeingCarried())
          {
            // Check block's bounding box in world coorindates (since a block
            // could be relative to robot if being carried) to see if it
            // intersects with any robot bounding box.  Also check to see
            // block and the robot are at overlapping heights.
            const Pose3d blockPoseWrtWorld = block->GetPose().getWithRespectTo(Pose3d::World);
            const Quad2f blockBBox = block->GetBoundingQuadXY(blockPoseWrtWorld);
            const f32    blockHeight = blockPoseWrtWorld.get_translation().z();
            const f32    blockSize   = 0.5f*block->GetSize().Length();
            const f32    blockTop    = blockHeight + blockSize;
            const f32    blockBottom = blockHeight - blockSize;
            
            for(auto robotID : robotMgr_->GetRobotIDList())
            {
              Robot* robot = robotMgr_->GetRobotByID(robotID);
              CORETECH_ASSERT(robot != NULL);

              // Don't worry about collision while picking or placing since we
              // are trying to get close to blocks in these modes.
              // TODO: specify whether we are picking/placing _this_ block
              if(!robot->IsPickingOrPlacing())
              {
                const f32 robotBottom = robot->GetPose().get_translation().z();
                const f32 robotTop    = robotBottom + ROBOT_BOUNDING_Z;
                
                const bool topIntersects    = (((blockTop >= robotBottom) && (blockTop <= robotTop)) ||
                                               ((robotTop >= blockBottom) && (robotTop <= blockTop)));
                
                const bool bottomIntersects = (((blockBottom >= robotBottom) && (blockBottom <= robotTop)) ||
                                               ((robotBottom >= blockBottom) && (robotBottom <= blockTop)));
                
                const bool bboxIntersects   = blockBBox.Intersects(robot->GetBoundingQuadXY());
                
                if( (topIntersects || bottomIntersects) && bboxIntersects )
                {
                  CoreTechPrint("Removing block %d, which intersects robot %d's bounding quad.\n",
                                block->GetID(), robot->GetID());
                  
                  // Erase the vizualized block and its projected quad
                  VizManager::getInstance()->EraseCuboid(block->GetID());
                  VizManager::getInstance()->EraseQuad(block->GetID());
                  
                  // Erase the block (with a postfix increment of the iterator)
                  blocksOfType.second.erase(blockIter++);
                  didErase = true;
                  didBlocksChange_ = true;
                  
                  break; // no need to check other robots, block already gone
                } // if quads intersect
              } // if robot is not picking or placing
            } // for each robot
          } // if block was not observed
          
          if(!didErase) {
            ++blockIter;
          }
          
        } // for each block of this type
      } // for each block type
      
      
      if(numUnusedMarkers > 0) {
        CoreTechPrint("%u observed markers did not match any known objects and went unused.\n",
                      numUnusedMarkers);
      }
     
      // Toss any remaining markers?
      ClearAllObservedMarkers();
      
      
      // Let the robot manager do whatever it's gotta do to update the
      // robots in the world.
      robotMgr_->UpdateAllRobots();

      
    } // Update()
    
    
    void BlockWorld::QueueObservedMarker(const Vision::ObservedMarker& marker, const HistPoseKey poseKey)
    {
      obsMarkers_[marker.GetTimeStamp()].emplace(poseKey, marker);

      // Visualize the marker in 3D
      // TODO: disable this block when not debugging / visualizing
      if(true){
        
        // Note that this incurs extra computation to compute the 3D pose of
        // each observed marker so that we can draw in the 3D world, but this is
        // purely for debug / visualization
        u32 quadID = 0;
        
        // When requesting the markers' 3D corners below, we want them
        // not to be relative to the object the marker is part of, so we
        // will request them at a "canonical" pose (no rotation/translation)
        const Pose3d canonicalPose;
        
        /*
        // Block Markers
        std::set<const Vision::ObservableObject*> const& blocks = blockLibrary_.GetObjectsWithMarker(marker);
        for(auto block : blocks) {
          std::vector<Vision::KnownMarker*> const& blockMarkers = block->GetMarkersWithCode(marker.GetCode());

          for(auto blockMarker : blockMarkers) {
            
            Pose3d markerPose = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                     blockMarker->Get3dCorners(canonicalPose));
            markerPose = markerPose.getWithRespectTo(Pose3d::World);
            VizManager::getInstance()->DrawQuad(quadID++, blockMarker->Get3dCorners(markerPose), VIZ_COLOR_OBSERVED_QUAD);
          }
        }
         */
        
        // Mat Markers
        std::set<const Vision::ObservableObject*> const& mats = matLibrary_.GetObjectsWithMarker(marker);
        for(auto mat : mats) {
          std::vector<Vision::KnownMarker*> const& matMarkers = mat->GetMarkersWithCode(marker.GetCode());
          
          for(auto matMarker : matMarkers) {
            Pose3d markerPose = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                     matMarker->Get3dCorners(canonicalPose));
            markerPose = markerPose.getWithRespectTo(Pose3d::World);
            VizManager::getInstance()->DrawQuad(quadID++, matMarker->Get3dCorners(markerPose), VIZ_COLOR_OBSERVED_QUAD);
          }
        }
        
      } // 3D marker visualization
      
    } // QueueObservedMarker()
    
    void BlockWorld::ClearAllObservedMarkers()
    {
      obsMarkers_.clear();
    }
    
    void BlockWorld::CommandRobotToDock(const RobotID_t whichRobot,
                                        Block&    whichBlock)
    {
      Robot* robot = robotMgr_->GetRobotByID(whichRobot);
      if(robot != 0)
      {
        robot->ExecuteDockingSequence(&whichBlock);
        
      } else {
        CoreTechPrint("Invalid robot commanded to Dock.\n");
      }
    } // commandRobotToDock()

    void BlockWorld::ClearAllExistingBlocks() {
      existingBlocks_.clear();
      globalIDCounter = 0;
      didBlocksChange_ = true;
    }

    bool BlockWorld::ClearBlock(const BlockID_t withID)
    {
      bool wasCleared = false;
      
      for(auto & blocksByType : existingBlocks_) {
        auto blockWithID = blocksByType.second.find(withID);
        if(blockWithID != blocksByType.second.end()) {
          if(wasCleared) {
            // If wasCleared is already true, there must have been >= 2
            // with the specified ID, which should not happend
            CoreTechPrint("Found multiple blocks with ID=%d in BlockWorld::ClearBlock().\n", withID);
          }
          
          // Erase the vizualized block and its projected quad
          VizManager::getInstance()->EraseCuboid(withID);
          VizManager::getInstance()->EraseQuad(withID);

          // Remove the block from the world
          blocksByType.second.erase(blockWithID);
          
          // Flag that we removed a block
          wasCleared = true;
          didBlocksChange_ = true;
        }
      }
      
      return wasCleared;
    } // ClearBlock()
    
    void BlockWorld::EnableDraw(bool on)
    {
      enableDraw_ = on;
    }
    
    void BlockWorld::DrawObsMarkers() const
    {
      if (enableDraw_) {
        for (auto poseKeyMarkerMapAtTimestamp : obsMarkers_) {
          for (auto poseKeyMarkerMap : poseKeyMarkerMapAtTimestamp.second) {
            const Quad2f& q = poseKeyMarkerMap.second.GetImageCorners();
            f32 scaleF = 1.0f;
            switch(IMG_STREAM_RES) {
              case Vision::CAMERA_RES_QVGA:
                break;
              case Vision::CAMERA_RES_QQVGA:
                scaleF *= 0.5;
                break;
              case Vision::CAMERA_RES_QQQVGA:
                scaleF *= 0.25;
                break;
              case Vision::CAMERA_RES_QQQQVGA:
                scaleF *= 0.125;
                break;
              default:
                printf("WARNING (DrawObsMarkers): Unsupported streaming res %d\n", IMG_STREAM_RES);
                break;
            }
            VizManager::getInstance()->SendTrackerQuad(q[Quad::TopLeft].x()*scaleF,     q[Quad::TopLeft].y()*scaleF,
                                                       q[Quad::TopRight].x()*scaleF,    q[Quad::TopRight].y()*scaleF,
                                                       q[Quad::BottomRight].x()*scaleF, q[Quad::BottomRight].y()*scaleF,
                                                       q[Quad::BottomLeft].x()*scaleF,  q[Quad::BottomLeft].y()*scaleF);
          }
        }
      }
    }
    
    
  } // namespace Cozmo
} // namespace Anki
