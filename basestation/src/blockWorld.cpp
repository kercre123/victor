
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

    int BlockWorld::ObjectFamily::UniqueFamilyCounter = 0;
    
    // Instantiate object families here:
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::MATS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::RAMPS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::BLOCKS;
    
    // Instantiating an object family increments the unique counter:
    BlockWorld::ObjectFamily::ObjectFamily() {
      SetValue(UniqueFamilyCounter++);
    }
    
    
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
      
      //////////////////////////////////////////////////////////////////////////
      // 1x1 Cubes
      //
      
      //blockLibrary_.AddObject(new Block_Cube1x1(Block::FUEL_BLOCK_TYPE));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::ANGRYFACE));

      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::BULLSEYE2));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::SQTARGET));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::FIRE));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::ANKILOGO));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::STAR5));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::DICE));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER1));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER2));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER3));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER4));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER5));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER6));

      //_objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::BANGBANGBANG));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::ARROW));
      
      //////////////////////////////////////////////////////////////////////////
      // 2x1 Blocks
      //
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_2x1(Block::Type::BANGBANGBANG));
      
      
      //////////////////////////////////////////////////////////////////////////
      // Mat Pieces
      //
      
      // Webots mat:
      _objectLibrary[ObjectFamily::MATS].AddObject(new MatPiece(MatPiece::Type::LETTERS_4x4));
      
      // Platform piece:
      _objectLibrary[ObjectFamily::MATS].AddObject(new MatPiece(MatPiece::Type::LARGE_PLATFORM));
      
      
      //////////////////////////////////////////////////////////////////////////
      // Ramps
      //
      _objectLibrary[ObjectFamily::RAMPS].AddObject(new Ramp());
      
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
        
        /*
        PRINT_NAMED_INFO("BlockWorld.AddToOcclusionMaps.AddingObjectOccluder",
                         "Adding object %d as an occluder for robot %d.\n",
                         object->GetID().GetValue(),
                         robot->GetID());
        */
        camera.AddOccluder(*object);
      }
      
    } // AddToOcclusionMaps()
    
    
    
    void BlockWorld::AddAndUpdateObjects(const std::vector<Vision::ObservableObject*>& objectsSeen,
                                         ObjectsMapByType_t& objectsExisting,
                                         const TimeStamp_t atTimestamp)
    {
      for(auto objSeen : objectsSeen) {

        //const float minDimSeen = objSeen->GetMinDim();
        
        // Store pointers to any existing objects that overlap with this one
        std::vector<Vision::ObservableObject*> overlappingObjects;
        FindOverlappingObjects(objSeen, objectsExisting, overlappingObjects);
        
        if(overlappingObjects.empty()) {
          // no existing objects overlapped with the objects we saw, so add it
          // as a new object
          objSeen->SetID();
          objectsExisting[objSeen->GetType()][objSeen->GetID()] = objSeen;
          
          Vision::ObservableObject* parentMat = nullptr;
          /*
          for(auto objectsByType : _existingObjects[ObjectFamily::MATS]) {
            for(auto objectsByID : objectsByType.second) {
              Vision::ObservableObject* mat = objectsByID.second;
              
              Pose3d newPoseWrtMat;
              if(objSeen->GetPose().GetWithRespectTo(mat->GetPose(), newPoseWrtMat) == true) {
                if(parentMat != nullptr) {
                  PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.ObjectOnMultipleMats",
                                      "New %s object, ID=%d, appears to be 'on' multiple mats simultaneously. Using first as parent.\n",
                                      objSeen->GetType().GetName().c_str(), objSeen->GetID().GetValue());
                } else {
                  objSeen->SetPose(newPoseWrtMat);
                  parentMat = mat;
                }
              }
            }
          }
           */
          
          PRINT_NAMED_INFO("BlockWorld.AddAndUpdateObjects.AddNewObject",
                           "Adding new %s object and ID=%d at (%.1f, %.1f, %.1f), relative to %s mat.\n",
                           objSeen->GetType().GetName().c_str(),
                           objSeen->GetID().GetValue(),
                           objSeen->GetPose().GetTranslation().x(),
                           objSeen->GetPose().GetTranslation().y(),
                           objSeen->GetPose().GetTranslation().z(),
                           parentMat==nullptr ? "NO" : parentMat->GetType().GetName().c_str());
          
          // Project this new object into each camera
          AddToOcclusionMaps(objSeen, robotMgr_);
          
        }
        else {
          if(overlappingObjects.size() > 1) {
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.MultipleOverlappingObjects",
                                "More than one overlapping object found -- will use first.\n");
            // TODO: do something smarter here?
          }
          
          /* This is pretty verbose...
          fprintf(stdout, "Merging observation of object type=%hu, ID=%hu at (%.1f, %.1f, %.1f)\n",
                  objSeen->GetType(), objSeen->GetID(),
                  objSeen->GetPose().GetTranslation().x(),
                  objSeen->GetPose().GetTranslation().y(),
                  objSeen->GetPose().GetTranslation().z());
          */
          
          /*
          Pose3d newPoseWrtOldPoseParent;
          if(objSeen->GetPose().GetWithRespectTo(*overlappingObjects[0]->GetPose().GetParent(), newPoseWrtOldPoseParent) == false) {
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.GetWrtFail",
                                "Could not find new pose w.r.t. old pose's parent. Will not update object's pose.\n");
          } else {
            // TODO: better way of merging existing/observed object pose
            overlappingObjects[0]->SetPose( newPoseWrtOldPoseParent );
          }
           */
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
      
    } // AddAndUpdateObjects()
    
    void BlockWorld::CheckForUnobservedObjects(TimeStamp_t atTimestamp)
    {
      // Create a list of unobserved objects for further consideration below.
      struct UnobservedObjectContainer {
        ObjectFamily family;
        Vision::ObservableObject*      object;
        
        UnobservedObjectContainer(ObjectFamily family_, Vision::ObservableObject* object_)
        : family(family_), object(object_) { }
      };
      std::vector<UnobservedObjectContainer> unobservedObjects;
      
      //for(auto & objectTypes : objectsExisting) {
      for(auto objectFamily : _existingObjects) {
        auto objectsExisting = objectFamily.second;
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
              unobservedObjects.emplace_back(objectFamily.first, objectIter->second);
            } // if object was not observed
            else {
              // Object was observed, update it's visualization
              object->Visualize();
            }
          } // for object IDs of this type
        } // for each object type
      } // for each object family
      
      // Now that the occlusion maps are complete, check each unobserved object's
      // visibility in each camera
      for(auto unobserved : unobservedObjects) {

        for(auto robotID : robotMgr_->GetRobotIDList()) {
          
          Robot* robot = robotMgr_->GetRobotByID(robotID);
          CORETECH_ASSERT(robot != NULL);
          
          if(unobserved.object->IsVisibleFrom(robot->GetCamera(), DEG_TO_RAD(45), 20.f, true) &&
             (robot->GetDockObject() != unobserved.object->GetID()))  // We expect a docking block to disappear from view!
          {
            // We "should" have seen the object! Delete it or mark it somehow
            CoreTechPrint("Removing object %d, which should have been seen, "
                          "but wasn't.\n", unobserved.object->GetID().GetValue());
            
            // Grab the object's type and ID before we delete it
            ObjectType objType = unobserved.object->GetType();
            ObjectID   objID   = unobserved.object->GetID();
            
            // Delete the object's instantiation (which will also erase its
            // visualization)
            delete unobserved.object;
            
            // Actually erase the object from blockWorld's container of
            // existing objects
            _existingObjects[unobserved.family][objType].erase(objID);

            didObjectsChange_ = true;
          }
          
        } // for each camera
      } // for each unobserved object
      
    } // CheckForUnobservedObjects()
    
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
                                              const std::set<ObjectFamily>& ignoreFamiles,
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
    
    
    bool BlockWorld::LocalizeRobotToMat(Robot* robot, const MatPiece* matSeen, MatPiece* existingMatPiece)
    {
     
      if(matSeen == nullptr) {
        PRINT_NAMED_ERROR("BlockWorld.LocalizeRobotToMat.MatSeenNullPointer", "\n");
        return false;
      } else if(existingMatPiece == nullptr) {
        PRINT_NAMED_ERROR("BlockWorld.LocalizeRobotToMat.ExistingMatPieceNullPointer", "\n");
        return false;
      }
      
      PRINT_NAMED_INFO("BlockWorld.LocalizeRobotToMat.MatSeenChain",
                       "%s\n", matSeen->GetPose().GetNamedPathToOrigin(true).c_str());
      
      PRINT_NAMED_INFO("BlockWorld.LocalizeRobotToMat.ExistingMatChain",
                       "%s\n", existingMatPiece->GetPose().GetNamedPathToOrigin(true).c_str());
      
      
      // Get computed RobotPoseStamp at the time the mat was observed.
      RobotPoseStamp* posePtr = nullptr;
      if (robot->GetComputedPoseAt(matSeen->GetLastObservedTime(), &posePtr) == RESULT_FAIL) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.CouldNotFindHistoricalPose", "Time %d\n", matSeen->GetLastObservedTime());
        return false;
      }
      
      
      // The computed historical pose is always stored w.r.t. the robot's world
      // origin and parent chains are lost. Re-connect here so that GetWithRespectTo
      // will work correctly
      // TODO: Feels like this should be done by the robot
      Pose3d robotPoseAtObsTime = posePtr->GetPose();
      robotPoseAtObsTime.SetParent(robot->GetWorldOrigin());

      /*
      // Get computed Robot pose at the time the mat was observed (note that this
      // also makes the pose have the robot's current world origin as its parent
      Pose3d robotPoseAtObsTime;
      if(robot->GetComputedPoseAt(matSeen->GetLastObservedTime(), robotPoseAtObsTime) != RESULT_OK) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.CouldNotComputeHistoricalPose", "Time %d\n", matSeen->GetLastObservedTime());
        return false;
      }
       */
      
      // Get the pose of the robot with respect to the observed mat piece
      Pose3d robotPoseWrtMat;
      if(robotPoseAtObsTime.GetWithRespectTo(matSeen->GetPose(), robotPoseWrtMat) == false) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.MatPoseOriginMisMatch",
                          "Could not get RobotPoseStamp w.r.t. matPose.\n");
        return false;
      }
      
      // Make the computed robot pose use the existing mat piece as its parent
      robotPoseWrtMat.SetParent(&existingMatPiece->GetPose());
      //robotPoseWrtMat.SetName(std::string("Robot_") + std::to_string(robot->GetID()));
      
      // Don't snap to horizontal or discrete Z levels when we see a mat marker
      // while on a ramp
      if(robot->IsOnRamp() == false)
      {
        // If there is any significant rotation, make sure that it is roughly
        // around the Z axis
        Radians rotAngle;
        Vec3f rotAxis;
        robotPoseWrtMat.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
        const float dotProduct = DotProduct(rotAxis, Z_AXIS_3D);
        const float dotProductThreshold = 0.0152f; // 1.f - std::cos(DEG_TO_RAD(10)); // within 10 degrees
        if(!NEAR(rotAngle.ToFloat(), 0, DEG_TO_RAD(10)) && !NEAR(std::abs(dotProduct), 1.f, dotProductThreshold)) {
          PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.RobotNotOnHorizontalPlane",
                              "Robot's Z axis is not well aligned with the world Z axis. "
                              "(angle=%.1fdeg, axis=(%.3f,%.3f,%.3f)\n",
                              rotAngle.getDegrees(), rotAxis.x(), rotAxis.y(), rotAxis.z());
        }
  /*
        // Snap to horizontal
        if(existingMatPiece->IsPoseOn(robotPoseWrtMat, 0, 10.f)) { 
          Vec3f robotPoseWrtMat_trans = robotPoseWrtMat.GetTranslation();
          robotPoseWrtMat_trans.z() = existingMatPiece->GetDrivingSurfaceHeight();
          robotPoseWrtMat.SetTranslation(robotPoseWrtMat_trans);
        }
        robotPoseWrtMat.SetRotation( robotPoseWrtMat.GetRotationAngle<'Z'>(), Z_AXIS_3D );
    */
      } // if robot is on ramp
      
      
      // TODO: Move this static to a member of BlockWorld
      static bool localizedToFixedMat = false;
      if(!localizedToFixedMat && !existingMatPiece->IsMoveable()) {
        // If we have not yet seen a fixed mat, and this is a fixed mat, rejigger
        // the origins so that we use it as the world origin
        PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.LocalizingToFixedMat",
                         "Localizing robot %d to fixed %s mat for the first time.\n",
                         robot->GetID(), existingMatPiece->GetType().GetName().c_str());
        
        if(robot->SetPoseOrigin(existingMatPiece->GetPose()) != RESULT_OK) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.SetPoseOriginFailure",
                            "Failed to update robot %d's pose origin when (re-)localizing it.\n", robot->GetID());
          return false;
        }
        
        localizedToFixedMat = true;
      }
      else if(!robot->IsLocalized()) {
        // If the robot is not yet localized, it is about to be, so we need to
        // update pose origins so that anything it has seen so far becomes rooted
        // to this mat's origin (whether mat is fixed or not)
        PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.LocalizingRobotFirstTime",
                         "Localizing robot %d for the first time (to %s mat).\n",
                         robot->GetID(), existingMatPiece->GetType().GetName().c_str());
        
        if(robot->SetPoseOrigin(existingMatPiece->GetPose()) != RESULT_OK) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.SetPoseOriginFailure",
                            "Failed to update robot %d's pose origin when (re-)localizing it.\n", robot->GetID());
          return false;
        }
        
        if(!existingMatPiece->IsMoveable()) {
          // If this also happens to be a fixed mat, then we have now localized
          // to a fixed mat
          localizedToFixedMat = true;
        }
      }
            
      // Add the new vision-based pose to the robot's history.
      //RobotPoseStamp p(robot->GetPoseFrameID(), robotPoseWrtMat.GetWithRespectToOrigin(), posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
      Pose3d robotPoseWrtOrigin = robotPoseWrtMat.GetWithRespectToOrigin();
      if(robot->AddVisionOnlyPoseToHistory(existingMatPiece->GetLastObservedTime(),
                                           robotPoseWrtOrigin.GetTranslation().x(),
                                           robotPoseWrtOrigin.GetTranslation().y(),
                                           robotPoseWrtOrigin.GetTranslation().z(),
                                           robotPoseWrtOrigin.GetRotationAngle<'Z'>().ToFloat(),
                                           posePtr->GetHeadAngle(),
                                           posePtr->GetLiftAngle()) != RESULT_OK)
      {
        PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.FailedAddingVisionOnlyPoseToHistory", "\n");
        return false;
      }
      
      // Mark the robot as now being localized to this mat
      // NOTE: this should be _after_ calling AddVisionOnlyPoseToHistory, since
      //    that function checks whether the robot is already localized
      robot->SetLocalizedTo(existingMatPiece->GetID());
      
      // Update the computed pose as well so that subsequent block pose updates
      // use obsMarkers whose camera's parent pose is correct. Note that we
      // store the pose w.r.t. the origin in history
      // TODO: Should SetPose() do the flattening w.r.t. origin?
      posePtr->SetPose(robot->GetPoseFrameID(), robotPoseWrtOrigin, posePtr->GetHeadAngle(), posePtr->GetLiftAngle());
      
      // Compute the new "current" pose from history which uses the
      // past vision-based "ground truth" pose we just computed.
      if(!robot->UpdateCurrPoseFromHistory(existingMatPiece->GetPose())) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.FailedUpdateCurrPoseFromHistory", "\n");
        return false;
      }
      
      
      
      PRINT_INFO("Using %s mat %d to localize robot %d at (%.3f,%.3f,%.3f), %.1fdeg@(%.2f,%.2f,%.2f)\n",
                 existingMatPiece->GetType().GetName().c_str(),
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
      
      return true;
    } // LocalizeRobotToMat()
    
    
    bool BlockWorld::UpdateRobotPose(Robot* robot, PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp, const TimeStamp_t atTimestamp)
    {
      bool wasPoseUpdated = false;
      
      // Extract only observed markers from obsMarkersAtTimestamp
      std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
      GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
      
      // Get all mat objects *seen by this robot's camera*
      std::vector<Vision::ObservableObject*> matsSeen;
      _objectLibrary[ObjectFamily::MATS].CreateObjectsFromMarkers(obsMarkersListAtTimestamp, matsSeen,
                                                                  (robot->GetCamera().GetID()));

      // Remove used markers from map container
      RemoveUsedMarkers(obsMarkersAtTimestamp);
      
      if(not matsSeen.empty()) {
        
        const bool wasLocalized = robot->IsLocalized();
        
        // Is the robot "on" any of the mats it sees?
        // TODO: What to do if robot is "on" more than one mat simultaneously?
        //ObjectID onObjectID; // initializes to unset
        MatPiece* onMat = nullptr;
        for(auto object : matsSeen) {
          
          // ObservedObjects are w.r.t. the arbitrary historical origin of the camera
          // that observed them.  Hook them up to the current robot origin now:
          CORETECH_ASSERT(object->GetPose().GetParent() != nullptr &&
                          object->GetPose().GetParent()->IsOrigin());
          object->SetPoseParent(robot->GetWorldOrigin());
          
          MatPiece* mat = dynamic_cast<MatPiece*>(object);
          CORETECH_ASSERT(mat != nullptr);
          
          if(mat->IsPoseOn(robot->GetPose(), 0, 10.f)) { // TODO: get heightTol from robot
            if(onMat != nullptr) {
              PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.OnMultiplMats",
                                  "Robot is 'on' multiple mats at the same time. Will just use the first for now.\n");
            } else {
              onMat = mat;
            }
          }
        }
        
        // This will point to the mat we decide to localize to below (or will
        // remain null if we choose not to localize to any mat we see)
        MatPiece* matToLocalizeTo = nullptr;
        
        if(onMat != nullptr)
        {
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.OnMatLocalization",
                           "Robot %d is on a %s mat and will localize to it.\n",
                           robot->GetID(), onMat->GetType().GetName().c_str());
          
          // If robot is "on" one of the mats it is currently seeing, localize
          // the robot to that mat
          matToLocalizeTo = onMat;
        }
        else {
          // If the robot is NOT "on" any of the mats it is seeing...
          
          if(robot->IsLocalized()) {
            // ... and the robot is already localized, then see if it is
            // localized to one of the mats it is seeing (but not "on")
            MatPiece* alreadyLocalizedToMat = nullptr;
            for(auto mat : matsSeen) {
              if(mat->GetID() == robot->GetLocalizedTo()) {
                alreadyLocalizedToMat = dynamic_cast<MatPiece*>(mat);
                CORETECH_ASSERT(alreadyLocalizedToMat != nullptr);
                break;
              }
            }
            
            if(alreadyLocalizedToMat != nullptr) {
              PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.NotOnMatLocalization",
                               "Robot %d will re-localize to the %s mat it is not on, but already localized to.\n",
                               robot->GetID(), alreadyLocalizedToMat->GetType().GetName().c_str());
              
              // The robot is localized to one of the mats it is seeing, even
              // though it is not _on_ that mat.  Remain localized to that mat
              // and update any others it is also seeing
              matToLocalizeTo = alreadyLocalizedToMat;
            }
            else {
              // The robot is localized to a mat it is not seeing (and is not "on"
              // any of the mats it _is_ seeing.  Just update the poses of the
              // mats it is seeing, but don't localize to any of them.
              PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.NotOnMatNoLocalize",
                               "Robot %d is localized to a mat it doesn't see, and will not localize to any of the %lu mats it sees but is not on.\n",
                               robot->GetID(), matsSeen.size());
            }
            
          } else {
            // ... and the robot is _not_ localized, choose the observed mat
            // with the closest observed marker (since that is likely to be the
            // most accurate) and localize to that one.  Update any others.
            f32 minDistSq = -1.f;
            MatPiece* closestMat = nullptr;
            for(auto mat : matsSeen) {
              std::vector<const Vision::KnownMarker*> observedMarkers;
              mat->GetObservedMarkers(observedMarkers, atTimestamp);
              if(observedMarkers.empty()) {
                PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.ObservedMatWithNoObservedMarkers",
                                  "We saw a mat piece but it is returning no observed markers for "
                                  "the current timestamp.\n");
                CORETECH_ASSERT(false); // TODO: handle this situation
              }
              
              Pose3d markerWrtRobot;
              for(auto obsMarker : observedMarkers) {
                if(obsMarker->GetPose().GetWithRespectTo(robot->GetPose(), markerWrtRobot) == false) {
                  PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.ObsMarkerPoseOriginMisMatch",
                                    "Could not get the pose of an observed marker w.r.t. the robot that "
                                    "supposedly observed it.\n");
                  CORETECH_ASSERT(false); // TODO: handle this situation
                }
                
                const f32 markerDistSq = markerWrtRobot.GetTranslation().LengthSq();
                if(closestMat == nullptr || markerDistSq < minDistSq) {
                  closestMat = dynamic_cast<MatPiece*>(mat);
                  CORETECH_ASSERT(closestMat != nullptr);
                  minDistSq = markerDistSq;
                }
              } // for each observed marker
            } // for each mat seen
            
            PRINT_NAMED_INFO("BLockWorld.UpdateRobotPose.NotOnMatLocalizationToClosest",
                             "Robot %d is not on a mat but will localize to %s mat ID=%d, which is the closest.\n",
                             robot->GetID(), closestMat->GetType().GetName().c_str(), closestMat->GetID().GetValue());
            
            matToLocalizeTo = closestMat;
            
          } // if/else robot is localized
        }
        
        ObjectsMapByType_t& existingMatPieces = _existingObjects[ObjectFamily::MATS];
        
        // Keep track of markers we saw on existing/instantiated mats, to use
        // for occlusion checking
        std::vector<const Vision::KnownMarker *> observedMarkers;

        if(matToLocalizeTo != nullptr) {
          
          MatPiece* existingMatPiece = nullptr;
          
          if(existingMatPieces.empty()) {
            // If this is the first mat piece, add it to the world using the world
            // origin as its pose
            PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.CreatingFirstMatPiece",
                             "Instantiating first mat piece in the world.\n");
            
            existingMatPiece = new MatPiece(matToLocalizeTo->GetType());
            //existingMatPiece->SetOrigin(Pose3d::GetWorldOrigin());
            existingMatPiece->SetID();
            existingMatPiece->SetPose( Pose3d() ); // Not really necessary, but ensures the ID makes it into the pose name
            assert(existingMatPiece->GetPose().GetParent() == nullptr);
            
            existingMatPieces[matToLocalizeTo->GetType()][matToLocalizeTo->GetID()] = existingMatPiece;

          }
          else {
            // We can't look up the existing piece by ID because the matToLocalizeTo
            // is just a mat we _saw_, not one we've instantiated.  So look for
            // one in approximately the same position, of those with the same
            // type:
            //Vision::ObservableObject* existingObject = GetObjectByID(matToLocalizeTo->GetID());
            Vision::ObservableObject* existingObject = nullptr;
            auto existingObjectsOfType = _existingObjects[ObjectFamily::MATS].find(matToLocalizeTo->GetType());
            if(existingObjectsOfType != _existingObjects[ObjectFamily::MATS].end())
            {
              for(auto existingObjectsByID : existingObjectsOfType->second)
              {
                Vision::ObservableObject* candidateObject = existingObjectsByID.second;
                if(matToLocalizeTo->IsSameAs(*candidateObject, 20.f, DEG_TO_RAD(20))) {
                  // Found a match!
                  existingObject = candidateObject;
                }
              }
            }
          
            if(existingObject == nullptr)
            {
              // If the mat we are about to localize to does not exist yet,
              // but it's not the first mat piece in the world, add it to the
              // world, and give it a new origin, relative to the current
              // world origin.
              Pose3d poseWrtWorldOrigin = matToLocalizeTo->GetPose().GetWithRespectToOrigin();
              /*
              if(matToLocalizeTo->GetPose().GetWithRespectTo(*Pose3d::GetWorldOrigin(), poseWrtWorldOrigin) == false) {
                PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.CouldNotGetPoseWrtWorldOrigin",
                                    "Could not get the pose of a newly-created (but not initial) mat "
                                    "w.r.t. the world origin.\n");
                // TODO: I probably need to handle this situation differently
                return false;
              }
               */
              
              existingMatPiece = new MatPiece(matToLocalizeTo->GetType());
              existingMatPiece->SetID();
              existingMatPiece->SetPose(poseWrtWorldOrigin);
              //existingMatPiece->SetOrigin(&Pose3d::AddOrigin(poseWrtWorldOrigin));

              existingMatPieces[existingMatPiece->GetType()][existingMatPiece->GetID()] = existingMatPiece;
              
              PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.LocalizingToNewMat",
                               "Robot %d localizing to new %s mat with ID=%d.\n",
                               robot->GetID(), existingMatPiece->GetType().GetName().c_str(),
                               existingMatPiece->GetID().GetValue());
              
            } else {
              // We are localizing to an existing mat piece: do not attempt to
              // update its pose (we can't both update the mat's pose and use it
              // to update the robot's pose at the same time!)
              existingMatPiece = dynamic_cast<MatPiece*>(existingObject);
              CORETECH_ASSERT(existingMatPiece != nullptr);
              
              PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.LocalizingToExistingMat",
                               "Robot %d localizing to existing %s mat with ID=%d.\n",
                               robot->GetID(), existingMatPiece->GetType().GetName().c_str(),
                               existingMatPiece->GetID().GetValue());
            }
          }
          
          existingMatPiece->SetLastObservedTime(matToLocalizeTo->GetLastObservedTime());
          existingMatPiece->UpdateMarkerObservationTimes(*matToLocalizeTo);
          
          existingMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);
          
          // Now localize to that mat
          wasPoseUpdated = LocalizeRobotToMat(robot, matToLocalizeTo, existingMatPiece);
          
        }
        
        // Update poses of any other mats we saw (but did not localize to),
        // just like they are any "regular" object,
        std::vector<Vision::ObservableObject*> otherMatsSeen;
        for(auto matSeen : matsSeen) {
          if(matSeen != matToLocalizeTo) {
            
            // TODO: need to probably pick the origin based on type (fixed: use world, movable: use fixed mat its on?)
            //Pose3d poseWrtOrigin;
            
            // Store pointers to any existing objects that overlap with this one
            std::vector<Vision::ObservableObject*> overlappingObjects;
            FindOverlappingObjects(matSeen, existingMatPieces, overlappingObjects);
            
            if(overlappingObjects.empty()) {
              // no existing mats overlapped with the mat we saw, so add it
              // as a new mat piece, relative to the world origin
              
              Pose3d poseWrtOrigin = matSeen->GetPose().GetWithRespectToOrigin();
              /*
              if(matSeen->GetPose().GetWithRespectTo(*Pose3d::GetWorldOrigin(), poseWrtOrigin) == false) {
                PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.CouldNotGetPoseWrtWorldOrigin",
                                    "Could not get the pose of a newly-created (but non-localizing) mat "
                                    "w.r.t. the world origin.\n");
                // TODO: I probably need to handle this situation differently
              } else {
               */
                MatPiece* newMatPiece = new MatPiece(matSeen->GetType());
                //newMatPiece->SetOrigin(&Pose3d::AddOrigin(poseWrtOrigin));
                newMatPiece->SetID();
                newMatPiece->SetPose(poseWrtOrigin);
                newMatPiece->SetLastObservedTime(matSeen->GetLastObservedTime());
                newMatPiece->UpdateMarkerObservationTimes(*matSeen);
                
                existingMatPieces[matSeen->GetType()][matSeen->GetID()] = newMatPiece;
                
                fprintf(stdout, "Adding new %s mat with ID=%d at (%.1f, %.1f, %.1f)\n",
                        newMatPiece->GetType().GetName().c_str(), newMatPiece->GetID().GetValue(),
                        newMatPiece->GetPose().GetTranslation().x(),
                        newMatPiece->GetPose().GetTranslation().y(),
                        newMatPiece->GetPose().GetTranslation().z());
                
                // Add observed mat markers to the occlusion map of the camera that saw
                // them, so we can use them to delete objects that should have been
                // seen between that marker and the robot
                newMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);
              //}
            }
            else {
              if(overlappingObjects.size() > 1) {
                fprintf(stdout, "More than one overlapping mat found -- will use first.\n");
                // TODO: do something smarter here?
              }
              
              Pose3d poseWrtOrigin = matSeen->GetPose().GetWithRespectToOrigin();
              /*if(matSeen->GetPose().GetWithRespectTo(*Pose3d::GetWorldOrigin(), poseWrtOrigin) == false) {
                PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.CouldNotGetPoseWrtWorldOrigin",
                                    "Could not get the pose of an existing mat "
                                    "w.r.t. the world origin.\n");
                // TODO: I probably need to handle this situation differently
              } else {
               */
                // TODO: better way of merging existing/observed object pose
                overlappingObjects[0]->SetPose( poseWrtOrigin );
                overlappingObjects[0]->SetLastObservedTime(matSeen->GetLastObservedTime());
                overlappingObjects[0]->UpdateMarkerObservationTimes(*matSeen);
                
                overlappingObjects[0]->GetObservedMarkers(observedMarkers, atTimestamp);
              //}
            } // if/else overlapping existing mats found
          } // if matSeen != matToLocalizeTo
          
          delete matSeen;
        }
        
        // Add observed mat markers to the occlusion map of the camera that saw
        // them, so we can use them to delete objects that should have been
        // seen between that marker and the robot
        for(auto obsMarker : observedMarkers) {
          /*
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.AddingMatMarkerOccluder",
                           "Adding mat marker '%s' as an occluder for robot %d.\n",
                           Vision::MarkerTypeStrings[obsMarker->GetCode()],
                           robot->GetID());
           */
          robot->GetCamera().AddOccluder(*obsMarker);
        }
        
        // If the robot just re-localized, trigger a draw of all objects, since
        // we may have seen things while de-localized whose locations can now be
        // snapped into place.
        if(!wasLocalized && robot->IsLocalized()) {
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.RobotRelocalized",
                           "Robot %d just localized after being de-localized.\n", robot->GetID());
          DrawAllObjects();
        }
        
      } // IF any mat piece was seen

      if(wasPoseUpdated) {
        PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.RobotPoseChain", "%s\n",
                         robot->GetPose().GetNamedPathToOrigin(true).c_str());
      }
      
      return wasPoseUpdated;
      
    } // UpdateRobotPose()
    
    size_t BlockWorld::UpdateObjectPoses(const Robot* robot,
                                         const Vision::ObservableObjectLibrary& objectLibrary,
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
        
        objectLibrary.CreateObjectsFromMarkers(obsMarkersListAtTimestamp, objectsSeen, robot->GetCamera().GetID());
        
        // Remove used markers from map
        RemoveUsedMarkers(obsMarkersAtTimestamp);
      
        for(auto object : objectsSeen) {
          // ObservedObjects are w.r.t. the arbitrary historical origin of the camera
          // that observed them.  Hook them up to the current robot origin now:
          CORETECH_ASSERT(object->GetPose().GetParent() != nullptr &&
                          object->GetPose().GetParent()->IsOrigin());
          object->SetPoseParent(robot->GetWorldOrigin());
        }
        
        // Use them to add or update existing blocks in our world
        AddAndUpdateObjects(objectsSeen, existingObjects, atTimestamp);
      }
      
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
        
          // Only update robot's poses using VisionMarkers while not on a ramp
          if(!robot->IsOnRamp()) {
            // Note that this removes markers from the list that it uses
            UpdateRobotPose(robot, currentObsMarkers, atTimestamp);
          }
        

          
          //
          // Find any observed blocks from the remaining markers
          //
          // Note that this removes markers from the list that it uses
          numObjectsObserved += UpdateObjectPoses(robot,
                                                  _objectLibrary[ObjectFamily::BLOCKS], currentObsMarkers,
                                                  _existingObjects[ObjectFamily::BLOCKS], atTimestamp);
          
          //
          // Find any observed ramps from the remaining markers
          //
          // Note that this removes markers from the list that it uses
          numObjectsObserved += UpdateObjectPoses(robot,
                                                  _objectLibrary[ObjectFamily::RAMPS], currentObsMarkers,
                                                  _existingObjects[ObjectFamily::RAMPS], atTimestamp);
          
          
        } // FOR each robotID
        

        
        // TODO: Deal with unknown markers?
        
        
        // Keep track of how many markers went unused by either robot or block
        // pose updating processes above
        numUnusedMarkers += currentObsMarkers.size();
        
        // Delete any objects that should have been observed but weren't,
        // visualize objects that were observed:
        CheckForUnobservedObjects(atTimestamp);
        
      } // for element in obsMarkers_
      
      //PRINT_NAMED_INFO("BlockWorld.Update.NumBlocksObserved", "Saw %d blocks\n", numBlocksObserved);
      
      // Check for unobserved, uncarried objects that overlap with any robot's position
      // TODO: better way of specifying which objects are obstacles and which are not
      for(auto & objectsByFamily : _existingObjects)
      {
        // For now, look for collision with anything other than Mat objects
        // NOTE: This assumes all other objects are DockableObjects below!!! (Becuase of IsBeingCarried() check)
        // TODO: How can we delete Mat objects (like platforms) whose positions we drive through
        if(objectsByFamily.first != ObjectFamily::MATS)
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
                                      object->GetID().GetValue(), robot->GetID());
                        
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
      
      camPose.SetName("PoseHistoryCamera_" + std::to_string(msg.timestamp));
      
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
        std::set<const Vision::ObservableObject*> const& mats = _objectLibrary[ObjectFamily::MATS].GetObjectsWithMarker(marker);
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
    
    void BlockWorld::ClearObjectsByFamily(const ObjectFamily family)
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
