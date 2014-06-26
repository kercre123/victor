
// TODO: this include is shared b/w BS and Robot.  Move up a level.
#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/common/shared/utilities_shared.h"

#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
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
      
      blockLibrary_.AddObject(new Block_Cube1x1(Block::ARROW_BLOCK_TYPE));
      
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
              block->Visualize();
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
            
            // Erase the vizualized block
            VizManager::getInstance()->EraseCuboid(object->GetID());
            
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
                                             const std::set<ObjectID_t>& ignoreIDs,
                                             const bool ignoreCarriedBlocks) const
    {
      for(auto & blocksWithType : existingBlocks_)
      {
        const bool useType = ignoreTypes.find(blocksWithType.first) == ignoreTypes.end();
        if(useType) {
          for(auto & blockAndId : blocksWithType.second)
          {
            Block* block = dynamic_cast<Block*>(blockAndId.second);
            CORETECH_THROW_IF(block == nullptr);
            if (ignoreCarriedBlocks && !block->IsBeingCarried()) {
              const f32 blockHeight = block->GetPose().get_translation().z();
              
              // If this block's ID is not in the ignore list, then we will use it
              const bool useID = ignoreIDs.find(blockAndId.first) == ignoreIDs.end();
              
              if( (blockHeight >= minHeight) && (blockHeight <= maxHeight) && useID )
              {
                rectangles.emplace_back(block->GetBoundingQuadXY(padding));
              }
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
        
        const bool wasLocalized = robot->IsLocalized();
        
        if(matsSeen.size() > 1) {
          PRINT_NAMED_WARNING("MultipleMatPiecesObserved",
                              "Robot %d observed more than one mat pieces at "
                              "the same time; will only use first for now.",
                              robot->GetID());
        }
        
        MatPiece* firstSeenMatPiece = dynamic_cast<MatPiece*>(matsSeen[0]);
        CORETECH_ASSERT(firstSeenMatPiece != nullptr);
        
        // Get computed RobotPoseStamp at the time the mat was observed.
        RobotPoseStamp* posePtr = nullptr;
        if (robot->GetComputedPoseAt(firstSeenMatPiece->GetLastObservedTime(), &posePtr) == RESULT_FAIL) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.CouldNotFindHistoricalPose", "Time %d\n", matsSeen[0]->GetLastObservedTime());
          return false;
        }
        
        // Get the pose of the robot with respect to the observed mat piece
        Pose3d robotPoseWrtMat;
        if(posePtr->GetPose().getWithRespectTo(firstSeenMatPiece->GetPose(), robotPoseWrtMat) == false) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.MatPoseOriginMisMatch",
                            "Could not get RobotPoseStamp w.r.t. matPose.\n");
          return false;
        }
        
        // If there is any significant rotation, make sure that it is roughly
        // around the Z axis
        Radians rotAngle;
        Vec3f rotAxis;
        robotPoseWrtMat.get_rotationVector().get_angleAndAxis(rotAngle, rotAxis);
        const float dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
        const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
        if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.RobotNotOnHorizontalPlane", "");
          return false;
        }
        
        if(existingMatPieces_.empty()) {
          // We haven't seen the mat yet.  Create the first mat piece in the world.
          // Not supporting multiple mat pieces, just use the already-seen one from here on.
          // TODO: Deal with multiple mat pieces and updating their poses w.r.t. one another.
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.CreatingMatPiece",
                           "Instantiating one and only mat piece in the world.\n");
          
          firstSeenMatPiece->SetID(++globalIDCounter);
          existingMatPieces_[firstSeenMatPiece->GetType()][firstSeenMatPiece->GetID()] = new MatPiece(firstSeenMatPiece->GetType());
        }

        // Grab the existing mat piece that matches the one we saw.  For now,
        // their should only every be one.
        auto matPiecesByType = existingMatPieces_.find(firstSeenMatPiece->GetType());
        if(matPiecesByType == existingMatPieces_.end() || matPiecesByType->second.empty()) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.UnexpectedMatPieceType",
                            "Saw new mat piece type that didn't match the any already in existence.\n");
          return false;
        }
        else if(matPiecesByType->second.size() > 1) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.MultipleMatPiecesWithType",
                              "There are more than one mat pieces in existance with type %d.\n",
                              firstSeenMatPiece->GetType());
        }
        
        MatPiece* existingMatPiece = dynamic_cast<MatPiece*>(matPiecesByType->second.begin()->second);
        CORETECH_ASSERT(existingMatPiece != nullptr);
        
        // Update lastObserved times of the existing mat piece
        existingMatPiece->SetLastObservedTime(firstSeenMatPiece->GetLastObservedTime());
        existingMatPiece->UpdateMarkerObservationTimes(*firstSeenMatPiece);
        
        // Make the computed robot pose use the existing mat piece as its parent
        robotPoseWrtMat.set_parent(&existingMatPiece->GetPose());
       
        // We have a new ("ground truth") key frame. Increment the pose frame!
        robot->IncrementPoseFrameID();
        
        // Add the new vision-based pose to the robot's history.
        RobotPoseStamp p(robot->GetPoseFrameID(), robotPoseWrtMat, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
        if(robot->AddVisionOnlyPoseToHistory(firstSeenMatPiece->GetLastObservedTime(), p) != RESULT_OK) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.FailedAddingVisionOnlyPoseToHistory", "");
        }
        
        // Update the computed pose as well so that subsequent block pose updates
        // use obsMarkers whose camera's parent pose is correct
        posePtr->SetPose(robot->GetPoseFrameID(), robotPoseWrtMat, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());

        // Compute the new "current" pose from history which uses the
        // past vision-based "ground truth" pose we just computed.
        if(!robot->UpdateCurrPoseFromHistory()) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.FailedUpdateCurrPoseFromHistory", "");
          return false;
        }
        wasPoseUpdated = true;
        
        PRINT_INFO("Using mat %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f)\n",
                   existingMatPiece->GetID(), robot->GetID(),
                   robot->GetPose().get_translation().x(),
                   robot->GetPose().get_translation().y(),
                   robot->GetPose().get_translation().z(),
                   robot->GetPose().get_rotationAngle().getDegrees(),
                   robot->GetPose().get_rotationAxis().x(),
                   robot->GetPose().get_rotationAxis().y(),
                   robot->GetPose().get_rotationAxis().z());
        
        // Send the ground truth pose that was computed instead of the new current
        // pose and let the robot deal with updating its current pose based on the
        // history that it keeps.
        robot->SendAbsLocalizationUpdate();
        
        // Add observed mat markers to the occlusion map of the camera that saw
        // them, so we can use them to delete objects that should have been
        // seen between that marker and the robot
        std::vector<const Vision::KnownMarker *> observedMarkers;
        existingMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);
        for(auto obsMarker : observedMarkers) {
          robot->GetCamera().AddOccluder(*obsMarker);
        }
        
        // Done using mat pieces to localize, which were cloned from library
        // mat objects.  Delete them now so we don't leak.
        for(auto matSeen : matsSeen) {
          delete matSeen;
        }
        
        // If the robot just re-localized, trigger a draw of all blocks, since
        // we may have seen things while de-localized whose locations can now be
        // snapped into place.
        if(!wasLocalized && robot->IsLocalized()) {
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.RobotRelocalized",
                           "Robot %d just localized after being de-localized.\n", robot->GetID());
          DrawAllBlocks();
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
          if(block->GetLastObservedTime() < lastObsMarkerTime && !block->IsBeingCarried())
          {
            for(auto robotID : robotMgr_->GetRobotIDList())
            {
              Robot* robot = robotMgr_->GetRobotByID(robotID);
              CORETECH_ASSERT(robot != NULL);
              
              // Check block's bounding box in same coordinates as this robot to
              // see if it intersects with the robot's bounding box. Also check to see
              // block and the robot are at overlapping heights.  Skip this check
              // entirely if the block isn't in the same coordinate tree as the
              // robot.
              Pose3d blockPoseWrtRobotOrigin;
              if(block->GetPose().getWithRespectTo(robot->GetPose().FindOrigin(), blockPoseWrtRobotOrigin) == true)
              {
                const Quad2f blockBBox = block->GetBoundingQuadXY(blockPoseWrtRobotOrigin);
                const f32    blockHeight = blockPoseWrtRobotOrigin.get_translation().z();
                const f32    blockSize   = 0.5f*block->GetSize().Length();
                const f32    blockTop    = blockHeight + blockSize;
                const f32    blockBottom = blockHeight - blockSize;
                
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
                    
                    // Erase the block (with a postfix increment of the iterator)
                    blocksOfType.second.erase(blockIter++);
                    didErase = true;
                    didBlocksChange_ = true;
                    
                    break; // no need to check other robots, block already gone
                  } // if quads intersect
                } // if robot is not picking or placing
              } // if we got block pose wrt robot origin
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
    
    Result BlockWorld::QueueObservedMarker(const MessageVisionMarker& msg, Robot& robot)
    {
      Result lastResult = RESULT_OK;
      
      Vision::Camera camera(robot.GetCamera());
      
      if(!camera.IsCalibrated()) {
        PRINT_NAMED_WARNING("MessageHandler::CalibrationNotSet",
                            "Received VisionMarker message from robot before "
                            "camera calibration was set on Basestation.");
        return RESULT_FAIL;
      }
      
      // Get corners
      Quad2f corners;
      
      corners[Quad::TopLeft].x()     = msg.x_imgUpperLeft;
      corners[Quad::TopLeft].y()     = msg.y_imgUpperLeft;
      
      corners[Quad::BottomLeft].x()  = msg.x_imgLowerLeft;
      corners[Quad::BottomLeft].y()  = msg.y_imgLowerLeft;
      
      corners[Quad::TopRight].x()    = msg.x_imgUpperRight;
      corners[Quad::TopRight].y()    = msg.y_imgUpperRight;
      
      corners[Quad::BottomRight].x() = msg.x_imgLowerRight;
      corners[Quad::BottomRight].y() = msg.y_imgLowerRight;
      
      
      // Get historical robot pose at specified timestamp to get
      // head angle and to attach as parent of the camera pose.
      TimeStamp_t t;
      RobotPoseStamp* p = nullptr;
      HistPoseKey poseKey;
      lastResult = robot.ComputeAndInsertPoseIntoHistory(msg.timestamp, t, &p, &poseKey);
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_WARNING("MessageHandler.ProcessMessageVisionMarker.HistoricalPoseNotFound",
                            "Time: %d, hist: %d to %d\n",
                            msg.timestamp, robot.GetPoseHistory().GetOldestTimeStamp(),
                            robot.GetPoseHistory().GetNewestTimeStamp());
        return lastResult;
      }
      
      // Compute pose from robot body to camera
      // Start with canonical (untilted) headPose
      Pose3d camPose(robot.GetHeadCamPose());
      
      // Rotate that by the given angle
      RotationVector3d Rvec(-p->GetHeadAngle(), Y_AXIS_3D);
      camPose.rotateBy(Rvec);
      
      // Precompute with robot body to neck pose
      camPose.preComposeWith(robot.GetNeckPose());
      
      // Set parent pose to be the historical robot pose
      camPose.set_parent(&(p->GetPose()));
      
      // Update the head camera's pose
      camera.SetPose(camPose);
      
      // Create observed marker
      Vision::ObservedMarker marker(t, msg.markerType, corners, camera);
      
      // Finally actuall queue the marker
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
            if(markerPose.getWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
              VizManager::getInstance()->DrawMatMarker(quadID++, matMarker->Get3dCorners(markerPose), VIZ_COLOR_OBSERVED_QUAD);
            } else {
              PRINT_NAMED_WARNING("BlockWorld.QueueObservedMarker.MarkerOriginNotCameraOrigin",
                                  "Cannot visualize a Mat marker whose pose origin is not the camera's origin that saw it.\n");
            }
          }
        }
        
      } // 3D marker visualization
      
      return lastResult;
      
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
        robot->ExecuteDockingSequence(whichBlock.GetID());
        
      } else {
        CoreTechPrint("Invalid robot commanded to Dock.\n");
      }
    } // commandRobotToDock()

    void BlockWorld::ClearAllExistingBlocks() {
      existingBlocks_.clear();
      globalIDCounter = 0;
      didBlocksChange_ = true;
      VizManager::getInstance()->EraseAllCuboids();
    }
    
    void BlockWorld::ClearBlocksByType(const ObjectType_t type)
    {
      auto blocksWithType = existingBlocks_.find(type);
      if(blocksWithType != existingBlocks_.end()) {
        
        // Erase all the visualized blocks of this type
        for(auto & block : blocksWithType->second) {
          VizManager::getInstance()->EraseCuboid(block.first);
        }
        
        // Erase this entry in the map of block types
        existingBlocks_.erase(type);
        
        didBlocksChange_ = true;
      }
      
    } // ClearBlocksByType()
    

    bool BlockWorld::ClearBlock(const ObjectID_t withID)
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
          
          // Erase the vizualized block
          VizManager::getInstance()->EraseCuboid(withID);

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
    
    
    void BlockWorld::DrawAllBlocks() const
    {
      for(auto & blocksByType : existingBlocks_) {
        for(auto & blocksByID : blocksByType.second) {
          const Block* block = dynamic_cast<Block*>(blocksByID.second);
          CORETECH_ASSERT(block != nullptr);
          block->Visualize();
        }
      }
    } // DrawAllBlocks()
    
  } // namespace Cozmo
} // namespace Anki
