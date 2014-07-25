
// TODO: this include is shared b/w BS and Robot.  Move up a level.
#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/common/shared/utilities_shared.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"


#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/messages.h"
#include "anki/cozmo/basestation/robot.h"

#include "ramp.h"

#include "messageHandler.h"
#include "vizManager.h"

namespace Anki
{
  namespace Cozmo
  {
    //BlockWorld* BlockWorld::singletonInstance_ = 0;
    
    const BlockWorld::ObjectsMapByID_t   BlockWorld::EmptyObjectMapByID;
    const BlockWorld::ObjectsMapByType_t BlockWorld::EmptyObjectMapByType;
    
    const Vision::ObservableObjectLibrary BlockWorld::EmptyObjectLibrary;

    
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
    , didObjectsChange_(false)
//    , globalIDCounter(0)
    , enableDraw_(false)
//    : robotMgr_(RobotManager::getInstance()),
//      msgHandler_(MessageHandler::getInstance())
    {
      // TODO: Create each known block / matpiece from a configuration/definitions file
      
      //
      // 1x1 Cubes
      //
      //blockLibrary_.AddObject(new Block_Cube1x1(Block::FUEL_BLOCK_TYPE));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::ANGRYFACE));

      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::BULLSEYE2));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::SQTARGET));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::FIRE));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::ANKILOGO));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::STAR5));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::DICE));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::NUMBER1));
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::NUMBER2));
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::NUMBER3));
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::NUMBER4));
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::NUMBER5));
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::NUMBER6));

      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::BANGBANGBANG));
      
      _objectLibrary[BLOCK_FAMILY].AddObject(new Block_Cube1x1(Block::Type::ARROW));
      
      //
      // 2x1 Blocks
      //
      
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
      // Mat Pieces
      //
      
      // Webots mat:
      MatPiece* mat = new MatPiece(MatPiece::Type::LETTERS_4x4);
        
      _objectLibrary[MAT_FAMILY].AddObject(mat);
      
      //
      // Ramps
      //
      _objectLibrary[RAMP_FAMILY].AddObject(new Ramp());
      
    } // BlockWorld() Constructor
    
    BlockWorld::~BlockWorld()
    {
      
      for(auto objectFamily : _existingObjects) {
        for(auto objectTypes : objectFamily.second) {
          for(auto objectIDs : objectTypes.second) {
            delete objectIDs.second;
          }
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
                                            const ObjectsMapByType_t& objectsExisting,
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
          
          //const float distToExist_mm = (objExist.second->GetPose().GetTranslation() -
          //                              <robotThatSawMe???>->GetPose().GetTranslation()).length();
          
          //const float distThresh_mm = distThresholdFraction * distToExist_mm;
          
          Pose3d P_diff;
          if( objExist.second->IsSameAs(*objectSeen, distThresh_mm, angleThresh, P_diff) ) {
            overlappingExistingObjects.push_back(objExist.second);
          } /*else {
            fprintf(stdout, "Not merging: Tdiff = %.1fmm, Angle_diff=%.1fdeg\n",
                    P_diff.GetTranslation().length(), P_diff.GetRotationAngle().getDegrees());
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
                                         ObjectsMapByType_t& objectsExisting,
                                         const TimeStamp_t atTimestamp)
    {
      for(auto objSeen : objectsSeen) {
        
        //const float minDimSeen = objSeen->GetMinDim();
        
        // Store pointers to any existing blocks that overlap with this one
        std::vector<Vision::ObservableObject*> overlappingObjects;
        FindOverlappingObjects(objSeen, objectsExisting, overlappingObjects);
        
        if(overlappingObjects.empty()) {
          // no existing objects overlapped with the objects we saw, so add it
          // as a new object
          objSeen->SetID(); //++globalIDCounter);
          objectsExisting[objSeen->GetType()][objSeen->GetID()] = objSeen;
          
          fprintf(stdout, "Adding new object with type=%d and ID=%d at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetType().GetValue(), objSeen->GetID().GetValue(),
                  objSeen->GetPose().GetTranslation().x(),
                  objSeen->GetPose().GetTranslation().y(),
                  objSeen->GetPose().GetTranslation().z());
          
          // Project this new block into each camera
          AddToOcclusionMaps(objSeen, robotMgr_);
          
        }
        else {
          if(overlappingObjects.size() > 1) {
            fprintf(stdout, "More than one overlapping object found -- will use first.\n");
            // TODO: do something smarter here?
          }
          
          /* This is pretty verbose...
          fprintf(stdout, "Merging observation of object type=%hu, ID=%hu at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetType(), objSeen->GetID(),
                  objSeen->GetPose().GetTranslation().x(),
                  objSeen->GetPose().GetTranslation().y(),
                  objSeen->GetPose().GetTranslation().z());
          */
          
          // TODO: better way of merging existing/observed object pose
          overlappingObjects[0]->SetPose( objSeen->GetPose() );
          
          // Update lastObserved times of this object
          overlappingObjects[0]->SetLastObservedTime(objSeen->GetLastObservedTime());
          overlappingObjects[0]->UpdateMarkerObservationTimes(*objSeen);
          
          // Project this existing object into each camera, using its new pose
          AddToOcclusionMaps(overlappingObjects[0], robotMgr_);

          // Now that we've merged in objSeen, we can delete it because we
          // will no longer be using it.  Otherwise, we'd leak.
          delete objSeen;
          
        } // if/else overlapping existing objects found
     
        didObjectsChange_ = true;
      } // for each object seen
      
      // Create a list of unobserved objects for further consideration below.
      // Use pairs of iterators to make deleting blocks below easier.
      std::vector<std::pair<ObjectsMapByType_t::iterator, ObjectsMapByID_t::iterator> > unobservedObjects;
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
            object->Visualize();
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
          
          if(object->IsVisibleFrom(robot->GetCamera(), DEG_TO_RAD(45), 20.f, true) &&
             (robot->GetDockObject() != unobserved.second->first))  // We expect a docking block to disappear from view!
          {
            // We "should" have seen the object! Delete it or mark it somehow
            CoreTechPrint("Removing object %d, which should have been seen, "
                          "but wasn't.\n", object->GetID());
            
            // Delete the object's instantiation (which will also erase its
            // visualization)
            delete unobserved.second->second;
            
            // Actually erase the object from blockWorld's container of
            // existing objects, using the iterator pointing to it
            unobserved.first->second.erase(unobserved.second);
            
            didObjectsChange_ = true;
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

    
    
    void BlockWorld::GetObjectBoundingBoxesXY(const f32 minHeight, const f32 maxHeight,
                                              const f32 padding,
                                              std::vector<Quad2f>& rectangles,
                                              const std::set<ObjectFamily_t>& ignoreFamiles,
                                              const std::set<ObjectType>& ignoreTypes,
                                              const std::set<ObjectID>& ignoreIDs,
                                              const bool ignoreCarriedObjects) const
    {
      for(auto & objectsByFamily : _existingObjects)
      {
        const bool useFamily = ignoreFamiles.find(objectsByFamily.first) == ignoreFamiles.end();
        if(useFamily) {
          for(auto & objectsByType : objectsByFamily.second)
          {
            const bool useType = ignoreTypes.find(objectsByType.first) == ignoreTypes.end();
            if(useType) {
              for(auto & objectAndId : objectsByType.second)
              {
                DockableObject* object = dynamic_cast<DockableObject*>(objectAndId.second);
                CORETECH_THROW_IF(object == nullptr);
                if (ignoreCarriedObjects && !object->IsBeingCarried()) {
                  const f32 blockHeight = object->GetPose().GetTranslation().z();
                  
                  // If this block's ID is not in the ignore list, then we will use it
                  const bool useID = ignoreIDs.find(objectAndId.first) == ignoreIDs.end();
                  
                  if( (blockHeight >= minHeight) && (blockHeight <= maxHeight) && useID )
                  {
                    rectangles.emplace_back(object->GetBoundingQuadXY(padding));
                  }
                }
              }
            } // if(useType)
          } // for each type
        } // if useFamily
      } // for each family
      
    } // GetObjectBoundingBoxesXY()
    
    
    bool BlockWorld::DidBlocksChange() const {
      return didObjectsChange_;
    }
    
    
    bool BlockWorld::UpdateRobotPose(Robot* robot, PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp, const TimeStamp_t atTimestamp)
    {
      bool wasPoseUpdated = false;
      
      // Extract only observed markers from obsMarkersAtTimestamp
      std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
      GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
      
      // Get all mat objects *seen by this robot's camera*
      std::vector<Vision::ObservableObject*> matsSeen;
      _objectLibrary[MAT_FAMILY].CreateObjectsFromMarkers(obsMarkersListAtTimestamp, matsSeen,
                                                          (robot->GetCamera().GetID()));

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
        if(posePtr->GetPose().GetWithRespectTo(firstSeenMatPiece->GetPose(), robotPoseWrtMat) == false) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.MatPoseOriginMisMatch",
                            "Could not get RobotPoseStamp w.r.t. matPose.\n");
          return false;
        }
        
        // If there is any significant rotation, make sure that it is roughly
        // around the Z axis
        Radians rotAngle;
        Vec3f rotAxis;
        robotPoseWrtMat.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
        const float dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
        const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
        if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.RobotNotOnHorizontalPlane",
                              "Robot's Z axis is not well aligned with the world Z axis.\n");
          return false;
        }
        
        ObjectsMapByType_t& existingMatPieces = _existingObjects[MAT_FAMILY];
        
        if(_existingObjects[MAT_FAMILY].empty()) {
          // We haven't seen the mat yet.  Create the first mat piece in the world.
          // Not supporting multiple mat pieces, just use the already-seen one from here on.
          // TODO: Deal with multiple mat pieces and updating their poses w.r.t. one another.
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.CreatingMatPiece",
                           "Instantiating one and only mat piece in the world.\n");
          
          MatPiece* newMatPiece = new MatPiece(firstSeenMatPiece->GetType(), true);
          newMatPiece->SetID();
          existingMatPieces[firstSeenMatPiece->GetType()][firstSeenMatPiece->GetID()] = newMatPiece;
        }

        // Grab the existing mat piece that matches the one we saw.  For now,
        // their should only every be one.
        auto matPiecesByType = existingMatPieces.find(firstSeenMatPiece->GetType());
        if(matPiecesByType == existingMatPieces.end() || matPiecesByType->second.empty()) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.UnexpectedMatPieceType",
                            "Saw new mat piece type that didn't match the any already in existence.\n");
          return false;
        }
        else if(matPiecesByType->second.size() > 1) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.MultipleMatPiecesWithType",
                              "There are more than one mat pieces in existance with type %d.\n",
                              firstSeenMatPiece->GetType().GetValue());
        }
        
        MatPiece* existingMatPiece = dynamic_cast<MatPiece*>(matPiecesByType->second.begin()->second);
        CORETECH_ASSERT(existingMatPiece != nullptr);
        
        // Update lastObserved times of the existing mat piece
        existingMatPiece->SetLastObservedTime(firstSeenMatPiece->GetLastObservedTime());
        existingMatPiece->UpdateMarkerObservationTimes(*firstSeenMatPiece);
        
        // Make the computed robot pose use the existing mat piece as its parent
        robotPoseWrtMat.SetParent(&existingMatPiece->GetPose());
        
        // Snap to horizontal
        Vec3f robotPoseWrtMat_trans = robotPoseWrtMat.GetTranslation();
        robotPoseWrtMat_trans.z() = 0; // TODO: can't do this if we are on top of something!
        robotPoseWrtMat.SetTranslation(robotPoseWrtMat_trans);
        robotPoseWrtMat.SetRotation( robotPoseWrtMat.GetRotationAngle<'Z'>(), Z_AXIS_3D );
        
        
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
                   existingMatPiece->GetID().GetValue(), robot->GetID(),
                   robot->GetPose().GetTranslation().x(),
                   robot->GetPose().GetTranslation().y(),
                   robot->GetPose().GetTranslation().z(),
                   robot->GetPose().GetRotationAngle<'Z'>().getDegrees(),
                   robot->GetPose().GetRotationAxis().x(),
                   robot->GetPose().GetRotationAxis().y(),
                   robot->GetPose().GetRotationAxis().z());
        
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
          DrawAllObjects();
        }
        
      } // IF any mat piece was seen

      return wasPoseUpdated;
      
    } // UpdateRobotPose()
    
    size_t BlockWorld::UpdateObjectPoses(const Vision::ObservableObjectLibrary& objectLibrary,
                                         PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp,
                                         ObjectsMapByType_t& existingObjects,
                                         const TimeStamp_t atTimestamp)
    {
      didObjectsChange_ = false;
      
      std::vector<Vision::ObservableObject*> objectsSeen;
      
      // Don't bother with this update at all if we didn't see at least one
      // marker (which is our indication we got an update from the robot's
      // vision system
      if(not obsMarkers_.empty()) {
        
        // Extract only observed markers from obsMarkersAtTimestamp
        std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
        GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
        
        objectLibrary.CreateObjectsFromMarkers(obsMarkersListAtTimestamp, objectsSeen);
        
        // Remove used markers from map
        RemoveUsedMarkers(obsMarkersAtTimestamp);
        
      }
      
      // Use them to add or update existing blocks in our world
      // NOTE: we still want to run this even if we didn't see markers because
      // we want to possibly delete any _unobserved_ objects (e.g. ones behind
      // whom we saw mat markers)
      AddAndUpdateObjects(objectsSeen, existingObjects, atTimestamp);
      
      return objectsSeen.size();
      
    } // UpdateObjectPoses()
    
    void BlockWorld::Update(uint32_t& numObjectsObserved)
    {
      CORETECH_ASSERT(isInitialized_);
      CORETECH_ASSERT(robotMgr_ != NULL);
      
      numObjectsObserved = 0;
      
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
            if ((poseKeyMarkerPair->second.GetSeenBy().GetID() == robot->GetCamera().GetID()) &&
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
        numObjectsObserved += UpdateObjectPoses(_objectLibrary[BLOCK_FAMILY], currentObsMarkers,
                                               _existingObjects[BLOCK_FAMILY], atTimestamp);

        //
        // Find any observed ramps from the remaining markers
        //
        // Note that this removes markers from the list that it uses
        numObjectsObserved += UpdateObjectPoses(_objectLibrary[RAMP_FAMILY], currentObsMarkers,
                                              _existingObjects[RAMP_FAMILY], atTimestamp);

        
        // TODO: Deal with unknown markers?
        
        
        // Keep track of how many markers went unused by either robot or block
        // pose updating processes above
        numUnusedMarkers += currentObsMarkers.size();
        
      } // for element in obsMarkers_
      
      //PRINT_NAMED_INFO("BlockWorld.Update.NumBlocksObserved", "Saw %d blocks\n", numBlocksObserved);
      
      // Check for unobserved, uncarried objects that overlap with any robot's position
      // TODO: better way of specifying which objects are obstacles and which are not
      for(auto & objectsByFamily : _existingObjects)
      {
        // For now, look for collision with anything other than Mat objects
        // NOTE: This assumes all other objects are DockableObjects below!!! (Becuase of IsBeingCarried() check)
        if(objectsByFamily.first != MAT_FAMILY)
        {
          for(auto & objectsByType : objectsByFamily.second)
          {
            
            //for(auto & objectsByID : objectsByType.second)
            for(auto objectIter = objectsByType.second.begin();
                objectIter != objectsByType.second.end(); /* increment based on whether we erase */)
            {
              DockableObject* object = dynamic_cast<DockableObject*>(objectIter->second);
              if(object == nullptr) {
                PRINT_NAMED_ERROR("BlockWorld.Update.ExpectingDockableObject",
                                  "In robot/object collision check, can currently only handle DockableObjects.\n");
                continue;
              }
              
              bool didErase = false;
              if(object->GetLastObservedTime() < lastObsMarkerTime && !object->IsBeingCarried())
              {
                for(auto robotID : robotMgr_->GetRobotIDList())
                {
                  Robot* robot = robotMgr_->GetRobotByID(robotID);
                  CORETECH_ASSERT(robot != NULL);
                  
                  // Don't worry about collision while picking or placing since we
                  // are trying to get close to blocks in these modes.
                  // TODO: specify whether we are picking/placing _this_ block
                  if(!robot->IsPickingOrPlacing())
                  {
                    // Check block's bounding box in same coordinates as this robot to
                    // see if it intersects with the robot's bounding box. Also check to see
                    // block and the robot are at overlapping heights.  Skip this check
                    // entirely if the block isn't in the same coordinate tree as the
                    // robot.
                    Pose3d objectPoseWrtRobotOrigin;
                    if(object->GetPose().GetWithRespectTo(robot->GetPose().FindOrigin(), objectPoseWrtRobotOrigin) == true)
                    {
                      const Quad2f objectBBox = object->GetBoundingQuadXY(objectPoseWrtRobotOrigin);
                      const f32    objectHeight = objectPoseWrtRobotOrigin.GetTranslation().z();
                      /*
                       const f32    blockSize   = 0.5f*object->GetSize().Length();
                       const f32    blockTop    = objectHeight + blockSize;
                       const f32    blockBottom = objectHeight - blockSize;
                       */
                      const f32 robotBottom = robot->GetPose().GetTranslation().z();
                      const f32 robotTop    = robotBottom + ROBOT_BOUNDING_Z;
                      
                      // TODO: Better check for being in the same plane that takes the
                      //       vertical extent of the object (in its current pose) into account
                      const bool inSamePlane = (objectHeight >= robotBottom && objectHeight <= robotTop);
                      /*
                       const bool topIntersects    = (((blockTop >= robotBottom) && (blockTop <= robotTop)) ||
                       ((robotTop >= blockBottom) && (robotTop <= blockTop)));
                       
                       const bool bottomIntersects = (((blockBottom >= robotBottom) && (blockBottom <= robotTop)) ||
                       ((robotBottom >= blockBottom) && (robotBottom <= blockTop)));
                       */
                      
                      const bool bboxIntersects   = objectBBox.Intersects(robot->GetBoundingQuadXY());
                      
                      if( inSamePlane && bboxIntersects )
                      {
                        CoreTechPrint("Removing object %d, which intersects robot %d's bounding quad.\n",
                                      object->GetID(), robot->GetID());
                        
                         // Erase the vizualized block and its projected quad
                         //VizManager::getInstance()->EraseCuboid(object->GetID());
                         
                         // Erase the block (with a postfix increment of the iterator)
                        delete object;
                        objectIter = objectsByType.second.erase(objectIter);
                        didErase = true;
                        
                        break; // no need to check other robots, block already gone
                      } // if quads intersect
                    } // if we got block pose wrt robot origin
                  } // if robot is not picking or placing
                } // for each robot
              } // if block was not observed
              
              if(!didErase) {
                ++objectIter;
              }
              
            } // for each object of this type
          } // for each object type
        } // if not in the Mat family
      } // for each object family
      
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
      camPose.RotateBy(Rvec);
      
      // Precompute with robot body to neck pose
      camPose.PreComposeWith(robot.GetNeckPose());
      
      // Set parent pose to be the historical robot pose
      camPose.SetParent(&(p->GetPose()));
      
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
         markerPose = markerPose.GetWithRespectTo(Pose3d::World);
         VizManager::getInstance()->DrawQuad(quadID++, blockMarker->Get3dCorners(markerPose), VIZ_COLOR_OBSERVED_QUAD);
         }
         }
         */
        
        // Mat Markers
        std::set<const Vision::ObservableObject*> const& mats = _objectLibrary[MAT_FAMILY].GetObjectsWithMarker(marker);
        for(auto mat : mats) {
          std::vector<Vision::KnownMarker*> const& matMarkers = mat->GetMarkersWithCode(marker.GetCode());
          
          for(auto matMarker : matMarkers) {
            Pose3d markerPose = marker.GetSeenBy().ComputeObjectPose(marker.GetImageCorners(),
                                                                     matMarker->Get3dCorners(canonicalPose));
            if(markerPose.GetWithRespectTo(marker.GetSeenBy().GetPose().FindOrigin(), markerPose) == true) {
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
    
    void BlockWorld::ClearAllExistingObjects()
    {
      //globalIDCounter = 0;
      ObjectID::Reset();
      
    }
    
    void BlockWorld::ClearObjectsByFamily(const ObjectFamily_t family)
    {
      ObjectsMapByFamily_t::iterator objectsWithFamily = _existingObjects.find(family);
      if(objectsWithFamily != _existingObjects.end()) {
        for(auto objectsByType : objectsWithFamily->second) {
          for(auto objectsByID : objectsByType.second) {
            delete objectsByID.second;
          }
        }
        
        _existingObjects.erase(family);
        didObjectsChange_ = true;
      }
    }
    
    void BlockWorld::ClearObjectsByType(const ObjectType type)
    {
      for(auto & objectsByFamily : _existingObjects) {
        ObjectsMapByType_t::iterator objectsWithType = objectsByFamily.second.find(type);
        if(objectsWithType != objectsByFamily.second.end()) {
          for(auto & objectsByID : objectsWithType->second) {
            delete objectsByID.second;
          }
        
          objectsByFamily.second.erase(objectsWithType);
          didObjectsChange_ = true;
          
          // Types are unique.  No need to keep looking
          return;
        }
      }
      
    } // ClearBlocksByType()
    

    bool BlockWorld::ClearObject(const ObjectID withID)
    {
      for(auto & objectsByFamily : _existingObjects) {
        for(auto & objectsByType : objectsByFamily.second) {
          auto objectWithIdIter = objectsByType.second.find(withID);
          if(objectWithIdIter != objectsByType.second.end()) {
            
            // Remove the object from the world
            // NOTE: The object should erase its own visualization upon destruction
            delete objectWithIdIter->second;
            objectsByType.second.erase(objectWithIdIter);
            
            // Flag that we removed an object
            didObjectsChange_ = true;
            
            // IDs are unique, so we can return as soon as the ID is found and cleared
            return true;
          }
        }
      }
     
      // Never found the specified ID
      return false;
    } // ClearObject()
    
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
    
    void BlockWorld::DrawAllObjects() const
    {
      for(auto & objectsByFamily : _existingObjects) {
        for(auto & objectsByType : objectsByFamily.second) {
          for(auto & objectsByID : objectsByType.second) {
            Vision::ObservableObject* object = objectsByID.second;
            object->Visualize();
          }
        }
        
      }
    } // DrawAllBlocks()
    
  } // namespace Cozmo
} // namespace Anki
