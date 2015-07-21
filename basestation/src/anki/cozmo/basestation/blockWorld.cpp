
// TODO: this include is shared b/w BS and Robot.  Move up a level.
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "anki/common/shared/utilities_shared.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/rect_impl.h"


#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/basestation/markerlessObject.h"
#include "anki/cozmo/basestation/comms/robot/robotMessages.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

#include "bridge.h"
#include "flatMat.h"
#include "platform.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/cozmo/basestation/charger.h"
#include "anki/cozmo/basestation/humanHead.h"

#include "robotMessageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

// The amount of time a proximity obstacle exists beyond the latest detection
#define PROX_OBSTACLE_LIFETIME_MS  4000

// The sensor value that must be met/exceeded in order to have detected an obstacle
#define PROX_OBSTACLE_DETECT_THRESH   5

// Make the (very restrictive) assumption that there is only ever one of each
// type of object in the world at a time (e.g. a single "AngryFace" block or a
// single "Fire" block). So if we see one, always match it to the one we've already
// seen, if it exists.
//  0 = turn this off
//  1 = turn this on just for physical robots
//  2 = turn this on for physical and simulatd robots
#define ONLY_ALLOW_ONE_OBJECT_PER_TYPE 2

#define DEBUG_ROBOT_POSE_UPDATES 0
#if DEBUG_ROBOT_POSE_UPDATES
#  define PRINT_LOCALIZATION_INFO(...) PRINT_NAMED_INFO("Localization", __VA_ARGS__)
#else
#  define PRINT_LOCALIZATION_INFO(...)
#endif

namespace Anki
{
  namespace NamedColors {
    // Add some BlockWorld-specific named colors:
    const ColorRGBA EXECUTED_PATH              (1.f, 0.0f, 0.0f, 1.0f);
    const ColorRGBA PREDOCKPOSE                (1.f, 0.0f, 0.0f, 0.75f);
    const ColorRGBA PRERAMPPOSE                (0.f, 0.0f, 1.0f, 0.75f);
    const ColorRGBA SELECTED_OBJECT            (0.f, 1.0f, 0.0f, 0.0f);
    const ColorRGBA BLOCK_BOUNDING_QUAD        (0.f, 0.0f, 1.0f, 0.75f);
    const ColorRGBA OBSERVED_QUAD              (1.f, 0.0f, 0.0f, 0.75f);
    const ColorRGBA ROBOT_BOUNDING_QUAD        (0.f, 0.8f, 0.0f, 0.75f);
    const ColorRGBA REPLAN_BLOCK_BOUNDING_QUAD (1.f, 0.1f, 1.0f, 0.75f);
  }
  
  namespace Cozmo
  {
    
    // Minimum number of times we need to observe an object to keep it and signal
    // that we saw it.
    // TODO: Move this to some config/parameters file somewhere
    const int MIN_TIMES_TO_OBSERVE_OBJECT = 2;
    
    int BlockWorld::ObjectFamily::UniqueFamilyCounter = 0;
    
    // Instantiate object families here:
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::MATS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::RAMPS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::CHARGERS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::BLOCKS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::ACTIVE_BLOCKS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::MARKERLESS_OBJECTS;
    const BlockWorld::ObjectFamily BlockWorld::ObjectFamily::HUMAN_HEADS;
    
    // Instantiating an object family increments the unique counter:
    BlockWorld::ObjectFamily::ObjectFamily() {
      SetValue(UniqueFamilyCounter++);
    }
    
    BlockWorld::BlockWorld(Robot* robot)
    : _robot(robot)
    , _didObjectsChange(false)
    , _canDeleteObjects(true)
    , _canAddObjects(true)
    , _enableDraw(false)
    {
      CORETECH_ASSERT(_robot != nullptr);
      
      // TODO: Create each known block / matpiece from a configuration/definitions file
      
      //////////////////////////////////////////////////////////////////////////
      // 1x1 Cubes
      //
      
      //blockLibrary_.AddObject(new Block_Cube1x1(Block::FUEL_BLOCK_TYPE));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::ANGRYFACE));

      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::BULLSEYE2));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::BULLSEYE2_INVERTED));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::SQTARGET));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::FIRE));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::ANKILOGO));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::STAR5));
      
      //_objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::DICE));
      
      /*
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER1));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER2));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER3));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER4));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER5));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::NUMBER6));
       */
      //_objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::BANGBANGBANG));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::ARROW));
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::FLAG));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::FLAG2));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::FLAG_INVERTED));
      
      // For CREEP Test
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::SPIDER));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::KITTY));
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_Cube1x1(Block::Type::BEE));
      
      //////////////////////////////////////////////////////////////////////////
      // 1x1 Light Cubes
      //

      _objectLibrary[ObjectFamily::ACTIVE_BLOCKS].AddObject(new ActiveCube(Block::Type::LIGHTCUBE1));
      _objectLibrary[ObjectFamily::ACTIVE_BLOCKS].AddObject(new ActiveCube(Block::Type::LIGHTCUBE2));
      _objectLibrary[ObjectFamily::ACTIVE_BLOCKS].AddObject(new ActiveCube(Block::Type::LIGHTCUBE3));
      _objectLibrary[ObjectFamily::ACTIVE_BLOCKS].AddObject(new ActiveCube(Block::Type::LIGHTCUBE4));

      
      //////////////////////////////////////////////////////////////////////////
      // 2x1 Blocks
      //
      
      _objectLibrary[ObjectFamily::BLOCKS].AddObject(new Block_2x1(Block::Type::BANGBANGBANG));
      
      
      //////////////////////////////////////////////////////////////////////////
      // Mat Pieces
      //
      
      // Flag mats:
      //_objectLibrary[ObjectFamily::MATS].AddObject(new FlatMat(FlatMat::Type::LETTERS_4x4));
      _objectLibrary[ObjectFamily::MATS].AddObject(new FlatMat(FlatMat::Type::GEARS_4x4));
      
      // Platform piece:
      //_objectLibrary[ObjectFamily::MATS].AddObject(new Platform(Platform::Type::LARGE_PLATFORM));
      
      // Long Bridge
      //_objectLibrary[ObjectFamily::MATS].AddObject(new Bridge(Bridge::Type::LONG_BRIDGE));
      
      // Short Bridge
      // TODO: Need to update short bridge markers so they don't look so similar to long bridge at oblique viewing angle
      // _objectLibrary[ObjectFamily::MATS].AddObject(new MatPiece(MatPiece::Type::SHORT_BRIDGE));
      
      
      //////////////////////////////////////////////////////////////////////////
      // Ramps
      //
      //_objectLibrary[ObjectFamily::RAMPS].AddObject(new Ramp());
      
      
      //////////////////////////////////////////////////////////////////////////
      // Charger
      //
      _objectLibrary[ObjectFamily::CHARGERS].AddObject(new Charger());
      
      
      //////////////////////////////////////////////////////////////////////////
      // Faces
      //
      _objectLibrary[ObjectFamily::HUMAN_HEADS].AddObject(new HumanHead());

      
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
    
    
    void CheckForOverlapHelper(const Vision::ObservableObject* objectToMatch,
                               Vision::ObservableObject* objectToCheck,
                               std::vector<Vision::ObservableObject*>& overlappingObjects)
    {
      
      // TODO: smarter block pose comparison
      //const float minDist = 5.f; // TODO: make parameter ... 0.5f*std::min(minDimSeen, objExist->GetMinDim());
      
      //const float distToExist_mm = (objExist.second->GetPose().GetTranslation() -
      //                              <robotThatSawMe???>->GetPose().GetTranslation()).length();
      
      //const float distThresh_mm = distThresholdFraction * distToExist_mm;
      
      //Pose3d P_diff;
      if( objectToCheck->IsSameAs(*objectToMatch) ) {
        overlappingObjects.push_back(objectToCheck);
      } /*else {
         fprintf(stdout, "Not merging: Tdiff = %.1fmm, Angle_diff=%.1fdeg\n",
         P_diff.GetTranslation().length(), P_diff.GetRotationAngle().getDegrees());
         objExist.second->IsSameAs(*objectSeen, distThresh_mm, angleThresh, P_diff);
         }*/
      
    } // CheckForOverlapHelper()
  
    
    void BlockWorld::FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                            const ObjectsMapByType_t& objectsExisting,
                                            std::vector<Vision::ObservableObject*>& overlappingExistingObjects) const
    {
      auto objectsExistingIter = objectsExisting.find(objectSeen->GetType());
      if(objectsExistingIter != objectsExisting.end()) {
        for(auto objectToCheck : objectsExistingIter->second) {
          CheckForOverlapHelper(objectSeen, objectToCheck.second, overlappingExistingObjects);
        }
      }
      
    } // FindOverlappingObjects()
    

    void BlockWorld::FindOverlappingObjects(const Vision::ObservableObject* objectExisting,
                                            const std::vector<Vision::ObservableObject*>& objectsSeen,
                                            std::vector<Vision::ObservableObject*>& overlappingSeenObjects) const
    {
      for(auto objectToCheck : objectsSeen) {
        CheckForOverlapHelper(objectExisting, objectToCheck, overlappingSeenObjects);
      }
    }
    
    
    void BlockWorld::FindIntersectingObjects(const Vision::ObservableObject* objectSeen,
                                             const std::set<ObjectFamily>& ignoreFamiles,
                                             const std::set<ObjectType>& ignoreTypes,
                                             const std::set<ObjectID>& ignoreIDs,
                                             std::vector<Vision::ObservableObject*>& intersectingExistingObjects,
                                             f32 padding_mm) const
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
                const bool useID = ignoreIDs.find(objectAndId.first) == ignoreIDs.end();
                if(useID) {
                  Vision::ObservableObject* objExist = objectAndId.second;
                  
                  // Get quads of both objects and check for intersection
                  Quad2f quadExist = objExist->GetBoundingQuadXY(objExist->GetPose(), padding_mm);
                  Quad2f quadSeen = objectSeen->GetBoundingQuadXY(objectSeen->GetPose(), padding_mm);
                  
                  if( quadExist.Intersects(quadSeen) ) {
                    intersectingExistingObjects.push_back(objExist);
                  }
                } // if useID
              }  // for each object
            }  // if not ignoreType
          }  // for each type
        }  // if not ignoreFamily
      } // for each family
      
    } // FindIntersectingObjects()


    void BlockWorld::AddAndUpdateObjects(const std::vector<Vision::ObservableObject*>& objectsSeen,
                                         const ObjectFamily& inFamily,
                                         const TimeStamp_t atTimestamp)
    {
      ObjectsMapByType_t& objectsExisting = _existingObjects[inFamily];
      
      for(auto objSeen : objectsSeen) {

        //const float minDimSeen = objSeen->GetMinDim();
        
        // Store pointers to any existing objects that overlap with this one
        //std::vector<Vision::ObservableObject*> overlappingObjects;
        //FindOverlappingObjects(objSeen, objectsExisting, overlappingObjects);
        Vision::ObservableObject* matchingObject = FindClosestMatchingObject(*objSeen,
                                                                             objSeen->GetSameDistanceTolerance(),
                                                                             objSeen->GetSameAngleTolerance());
        
        // If this is the object we're carrying, do nothing and continue to the next observed object
        if ((matchingObject != nullptr) && (matchingObject->GetID() == _robot->GetCarryingObject())) {
          delete objSeen;
          continue;
        }
        
        
        // As of now the object will be w.r.t. the robot's origin.  If we
        // observed it to be on a mat, however, make it relative to that mat.
        const f32 objectDiagonal = objSeen->GetSameDistanceTolerance().Length();
        Vision::ObservableObject* parentMat = nullptr;

        for(auto objectsByType : _existingObjects[ObjectFamily::MATS]) {
          for(auto objectsByID : objectsByType.second) {
            MatPiece* mat = dynamic_cast<MatPiece*>(objectsByID.second);
            assert(mat != nullptr);
            
            // Don't make this mat the parent of any objects until it has been
            // seen enough time
            if(mat->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
              Pose3d newPoseWrtMat;
              // TODO: Better height tolerance approach
              if(mat->IsPoseOn(objSeen->GetPose(), objectDiagonal*.5f, objectDiagonal*.5f, newPoseWrtMat)) {
                objSeen->SetPose(newPoseWrtMat);
                parentMat = mat;
              }
            }
          }
        }
        
        std::vector<Point2f> projectedCorners;
        f32 observationDistance = 0;
        Vision::ObservableObject* observedObject = nullptr;

        if(matchingObject == nullptr) {
          
#         if ONLY_ALLOW_ONE_OBJECT_PER_TYPE > 0
          
          // See if there are any existing objects in the world with the same type
          // as the one we're seeing. Also make sure that they have not already
          // been seen in this same frame (to prevent signaling multiple objects
          // of the same type being seen simultaneously). Finally, only do this
          // if it's a physical robot, dependning on the ONLY_ALLOW_ONE_OBJECT_PER_TYPE
          // setting.
          ObjectsMapByID_t objectsWithType = GetExistingObjectsByType(objSeen->GetType());
          if(!objectsWithType.empty()
#            if ONLY_ALLOW_ONE_OBJECT_PER_TYPE == 1
             && _robot->IsPhysical()
#            endif
             )
          {
            // We already know about an object of this type. Assume the one we
            // are seeing is that one. Just update it to be in the pose of the
            // observed object.
            
            // By definition, we can't have more than one object of this type
            assert(objectsWithType.size() == 1);
            
            observedObject = objectsWithType.begin()->second;
            
            if(observedObject->GetLastObservedTime() < objSeen->GetLastObservedTime()) {
              
              assert(observedObject->GetType() == objSeen->GetType());
              
              PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.UpdatingByType",
                                  "Did not match observed object to existing %s object "
                                  "by pose, but assuming there's only one that must match "
                                  "existing ID = %d. (since ONLY_ALLOW_ONE_OBJECT_PER_TYPE = %d)\n",
                                  objSeen->GetType().GetName().c_str(),
                                  observedObject->GetID().GetValue(),
                                  ONLY_ALLOW_ONE_OBJECT_PER_TYPE);
              
              observedObject->SetPose( objSeen->GetPose() );
              
              // If we are matching based solely on type to an existing object that
              // has only been seen once, don't update the observed time (so that we
              // also don't update the number of times observed), since the poses
              // don't actually match and we really want to increment the number of
              // times of observed only if we re-see an object in the same place.
              // (If we are here, we didn't see it in the same place; we are only
              // updating it because we are assuming it's the same object based on
              // type.)
              if(observedObject->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
                // Update lastObserved times of this object
                observedObject->SetLastObservedTime(objSeen->GetLastObservedTime());
                observedObject->UpdateMarkerObservationTimes(*objSeen);
              }
              
              // Project this existing object into the robot's camera, using its new pose
              _robot->GetCamera().ProjectObject(*observedObject, projectedCorners, observationDistance);
              
              // If the object is being carried, uncarry it
              if (_robot->GetCarryingObject() == observedObject->GetID()) {
                PRINT_NAMED_INFO("BlockWorld.AddAndUpdateObjects.SawCarryObject",
                                 "Uncarrying object ID=%d because it was observed\n", (int)observedObject->GetID());
                _robot->UnSetCarryingObject();
              }
            } else {
              PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObjects.UpdatingByType",
                                  "Ignoring the second simultaneously-seen %s object "
                                  "(since ONLY_ALLOW_ONE_OBJECT_PER_TYPE = %d)\n",
                                  objSeen->GetType().GetName().c_str(),
                                  ONLY_ALLOW_ONE_OBJECT_PER_TYPE);
            }
            
            // Now that we've merged in objSeen, we can delete it because we
            // will no longer be using it.  Otherwise, we'd leak.
            delete objSeen;
            
          } else {
            // Otherwise, add a new object
#         endif // ONLY_ALLOW_ONE_OBJECT_PER_TYPE
            
          if(!_canAddObjects) {
            PRINT_NAMED_WARNING("BlockWorld.AddAndUpdateObject.AddingDisabled",
                                "Saw a new %s%s object, but adding objects is disabled.\n",
                                objSeen->IsActive() ? "active " : "",
                                objSeen->GetType().GetName().c_str());
            
            // Delete this object since we're not going to add it or merge it
            delete objSeen;
            
            // Keep looking through objects we saw
            continue;
          }
            
          // no existing objects overlapped with the objects we saw, so add it
          // as a new object
          AddNewObject(objectsExisting, objSeen);
          
          PRINT_NAMED_INFO("BlockWorld.AddAndUpdateObjects.AddNewObject",
                           "Adding new %s%s object and ID=%d at (%.1f, %.1f, %.1f), relative to %s mat.\n",
                           objSeen->IsActive() ? "active " : "",
                           objSeen->GetType().GetName().c_str(),
                           objSeen->GetID().GetValue(),
                           objSeen->GetPose().GetTranslation().x(),
                           objSeen->GetPose().GetTranslation().y(),
                           objSeen->GetPose().GetTranslation().z(),
                           parentMat==nullptr ? "NO" : parentMat->GetType().GetName().c_str());
          
          // Don't add as an occluder the very first time we see the object; wait
          // until we've seen it MIN_TIMES_TO_OBSERVE_OBJECT times.
          // // Project this new object into the robot's camera:
          // //_robot->GetCamera().ProjectObject(*objSeen, projectedCorners, observationDistance);
          
          observedObject = objSeen;
            
#         if ONLY_ALLOW_ONE_OBJECT_PER_TYPE
          } // if/else if(!objectsWithType.empty())
#         endif
          
          /*
           PRINT_NAMED_INFO("BlockWorld.AddToOcclusionMaps.AddingObjectOccluder",
           "Adding object %d as an occluder for robot %d.\n",
           object->GetID().GetValue(),
           robot->GetID());
           */
          
        } else {
          
          matchingObject->SetPose( objSeen->GetPose() );
          
          // Update lastObserved times of this object
          matchingObject->SetLastObservedTime(objSeen->GetLastObservedTime());
          matchingObject->UpdateMarkerObservationTimes(*objSeen);
          
          observedObject = matchingObject;
          
          /* This is pretty verbose... 
          fprintf(stdout, "Merging observation of object type=%s, with ID=%d at (%.1f, %.1f, %.1f), timestamp=%d\n",
                  objSeen->GetType().GetName().c_str(),
                  overlappingObjects[0]->GetID().GetValue(),
                  objSeen->GetPose().GetTranslation().x(),
                  objSeen->GetPose().GetTranslation().y(),
                  objSeen->GetPose().GetTranslation().z(),
                  overlappingObjects[0]->GetLastObservedTime());
          */
          
          // Project this existing object into the robot's camera, using its new pose
          _robot->GetCamera().ProjectObject(*matchingObject, projectedCorners, observationDistance);
          
          // Add all observed markers of this object as occluders:
          std::vector<const Vision::KnownMarker *> observedMarkers;
          observedObject->GetObservedMarkers(observedMarkers);
          for(auto marker : observedMarkers) {
            _robot->GetCamera().AddOccluder(*marker);
          }
          
          // Now that we've merged in objSeen, we can delete it because we
          // will no longer be using it.  Otherwise, we'd leak.
          delete objSeen;
          
        } // if/else overlapping existing objects found
     
        CORETECH_ASSERT(observedObject != nullptr);
        
        // If this is an active object and has not been identified yet, identify
        // it now.
        if(observedObject->IsActive() && !observedObject->IsIdentified()) {
          // TODO: Need to do more here probably...
          observedObject->Identify();
        }
        
        const ObjectID obsID = observedObject->GetID();
        const ObjectType obsType = observedObject->GetType();
        
        // Sanity check: this should not happen, but we're seeing situations where
        // objects think they are being carried when the robot doesn't think it
        // is carrying that object
        // TODO: Eventually, we should be able to remove this check
        ActionableObject* actionObject = dynamic_cast<ActionableObject*>(observedObject);
        if(actionObject != nullptr) {
          if(actionObject->IsBeingCarried() && _robot->GetCarryingObject() != obsID) {
            PRINT_NAMED_WARNING("BlockWorld.CycleSelectedObject",
                                "Object %d thinks it is being carried, but does not match "
                                "robot %d's carried object ID (%d). Setting as uncarried.\n",
                                obsID.GetValue(), _robot->GetID(),
                                _robot->GetCarryingObject().GetValue());
            actionObject->SetBeingCarried(false);
          }
        }
        
        if(obsID.IsUnknown()) {
          PRINT_NAMED_ERROR("BlockWorld.AddAndUpdateObjects.IDnotSet",
                            "ID of new/re-observed object not set.\n");
        }
        
        if(observedObject->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT)
        {
          // Use the projected corners to add an occluder and to keep track of the
          // bounding quads of all the observed objects in this Update
          //_robot->GetCamera().AddOccluder(projectedCorners, observationDistance);
          
          Rectangle<f32> boundingBox(projectedCorners);
          //_obsProjectedObjects.emplace_back(obsID, boundingBox);
          _currentObservedObjectIDs.push_back(obsID);
          
          // Signal the observation of this object, with its bounding box:
          const Vec3f& obsObjTrans = observedObject->GetPose().GetTranslation();
          const UnitQuaternion<float>& q = observedObject->GetPose().GetRotation().GetQuaternion();
          Radians topMarkerOrientation(0);
          if(observedObject->IsActive()) {
            ActiveCube* activeCube = dynamic_cast<ActiveCube*>(observedObject);
            if(activeCube == nullptr) {
              PRINT_NAMED_ERROR("BlockWorld.AddAndUpdateObjects",
                                "ObservedObject %d with IsActive()==true could not be cast to ActiveCube.\n",
                                obsID.GetValue());
            } else {
              topMarkerOrientation = activeCube->GetTopMarkerOrientation();
              
              //PRINT_INFO("Object %d's rotation around Z = %.1fdeg\n", obsID.GetValue(),
              //           topMarkerOrientation.getDegrees());
            }
          }
          CozmoEngineSignals::RobotObservedObjectSignal().emit(_robot->GetID(),
                                                               inFamily,
                                                               obsType,
                                                               obsID,
                                                               true, // markers are visible
                                                               boundingBox.GetX(),
                                                               boundingBox.GetY(),
                                                               boundingBox.GetWidth(),
                                                               boundingBox.GetHeight(),
                                                               obsObjTrans.x(),
                                                               obsObjTrans.y(),
                                                               obsObjTrans.z(),
                                                               q.w(), q.x(), q.y(), q.z(),
                                                               topMarkerOrientation.ToFloat(),
                                                               observedObject->IsActive());
        } // if(observedObject->GetNumTimesObserved() > MIN_TIMES_TO_OBSERVE_OBJECT)
        
        if(_robot->GetTrackToObject().IsSet() &&
           obsID == _robot->GetTrackToObject() &&
           !_robot->IsHeadLocked())
        {
          UpdateTrackToObject(observedObject);
        }

        _didObjectsChange = true;
      } // for each object seen
      
      
    } // AddAndUpdateObjects()
    
    void BlockWorld::UpdateTrackToObject(const Vision::ObservableObject* observedObject)
    {
      assert(observedObject != nullptr);
      
      // Find the observed marker closest to the robot and use that as the one we
      // track to
      std::vector<const Vision::KnownMarker*> observedMarkers;
      observedObject->GetObservedMarkers(observedMarkers, observedObject->GetLastObservedTime());
      
      if(observedMarkers.empty()) {
        PRINT_NAMED_ERROR("BlockWorld.UpdateTrackToObject",
                          "No markers on observed object %d marked as observed since time %d, "
                          "expecting at least one.\n",
                          observedObject->GetID().GetValue(), observedObject->GetLastObservedTime());
      } else {
        const Vec3f& robotTrans = _robot->GetPose().GetTranslation();
        
        const Vision::KnownMarker* closestMarker = nullptr;
        f32 minDistSq = std::numeric_limits<f32>::max();
        f32 xDist = 0.f, yDist = 0.f, zDist = 0.f;
        
        for(auto marker : observedMarkers) {
          Pose3d markerPose;
          if(false == marker->GetPose().GetWithRespectTo(*_robot->GetWorldOrigin(), markerPose)) {
            PRINT_NAMED_ERROR("BlockWorld.UpdateTrackToObject",
                              "Could not get pose of observed marker w.r.t. world for head tracking.\n");
          } else {
            
            const f32 xDist_crnt = markerPose.GetTranslation().x() - robotTrans.x();
            const f32 yDist_crnt = markerPose.GetTranslation().y() - robotTrans.y();
            
            const f32 currentDistSq = xDist_crnt*xDist_crnt + yDist_crnt*yDist_crnt;
            if(currentDistSq < minDistSq) {
              closestMarker = marker;
              minDistSq = currentDistSq;
              xDist = xDist_crnt;
              yDist = yDist_crnt;
              
              // Keep track of best zDist too, so we don't have to redo the GetWithRespectTo call outside this loop
              // NOTE: This isn't perfectly accurate since it doesn't take into account the
              // the head angle and is simply using the neck joint (which should also
              // probably be queried from the robot instead of using the constant here)
              zDist = markerPose.GetTranslation().z() - (robotTrans.z() + NECK_JOINT_POSITION[2]);
            }
          }
        } // For all markers
        
        if(closestMarker == nullptr) {
          PRINT_NAMED_ERROR("BlockWorld.UpdateTrackToObject", "No closest marker found!\n");
        } else {
          const f32 minDist = std::sqrt(minDistSq);
          const f32 headAngle = std::atan(zDist/(minDist + 1e-6f));
          //_robot->MoveHeadToAngle(headAngle, 5.f, 2.f);
          MessagePanAndTiltHead msg;
          msg.headTiltAngle_rad = headAngle;
          msg.bodyPanAngle_rad = 0.f;
          
          if(false == _robot->IsTrackingObjectWithHeadOnly()) {
            // Also rotate ("pan") body:
            const Radians panAngle = std::atan2(yDist, xDist);// - _robot->GetPose().GetRotationAngle<'Z'>();
            msg.bodyPanAngle_rad = panAngle.ToFloat();
          }
          /*
          PRINT_NAMED_INFO("BlockWorld.UpdateTrackToObject",
                           "Tilt = %.1fdeg, pan = %.1fdeg\n",
                           RAD_TO_DEG(msg.headTiltAngle_rad),
                           RAD_TO_DEG(msg.bodyPanAngle_rad));
          */
          _robot->SendMessage(msg);
        }
      } // if/else observedMarkers.empty()
      
    } // UpdateTrackToObject()
    
    u32 BlockWorld::CheckForUnobservedObjects(TimeStamp_t atTimestamp)
    {
      u32 numVisibleObjects = 0;
      
      // Create a list of unobserved objects for further consideration below.
      struct UnobservedObjectContainer {
        ObjectFamily family;
        ObjectType   type;
        Vision::ObservableObject*      object;
        
        UnobservedObjectContainer(ObjectFamily family_, ObjectType type_, Vision::ObservableObject* object_)
        : family(family_), type(type_), object(object_) { }
      };
      std::vector<UnobservedObjectContainer> unobservedObjects;
      
      //for(auto & objectTypes : objectsExisting) {
      for(auto & objectFamily : _existingObjects)
      {
        for(auto & objectsByType : objectFamily.second)
        {
          ObjectsMapByID_t& objectIdMap = objectsByType.second;
          for(auto objectIter = objectIdMap.begin();
              objectIter != objectIdMap.end(); )
          {
            Vision::ObservableObject* object = objectIter->second;
            
            if(object->GetLastObservedTime() < atTimestamp) {
              if(object->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT) {
                // If this object has only been seen once and that was too long ago,
                // just delete it
                PRINT_NAMED_INFO("BlockWorld.CheckForUnobservedObjects",
                                 "Removing %s object %d that was only observed %d time(s).\n",
                                 object->GetType().GetName().c_str(),
                                 object->GetID().GetValue(),
                                 object->GetNumTimesObserved());
                objectIter = ClearObject(objectIter, objectsByType.first, objectFamily.first);
              } else {
                // Otherwise, add it to the list for further checks below to see if
                // we "should" have seen the object
                
                //AddToOcclusionMaps(object, robotMgr_); // TODO: Used to do this too, put it back?
                unobservedObjects.emplace_back(objectFamily.first, objectsByType.first, objectIter->second);
                ++objectIter;
                
              }
            } else {
              // Object _was_ observed
              ++objectIter;
            } // if/else object was not observed
            
          } // for object IDs of this type
        } // for each object type
      } // for each object family
      
      // TODO: Don't bother with this if the robot is docking? (picking/placing)??
      // Now that the occlusion maps are complete, check each unobserved object's
      // visibility in each camera
      for(auto unobserved : unobservedObjects) {
        
        // Remove objects that should have been visible based on their last known
        // location, but which must not be there because we saw something behind
        // that location:
        const Vision::Camera& camera = _robot->GetCamera();
        const u16 xBorderPad = static_cast<u16>(0.05*static_cast<f32>(camera.GetCalibration().GetNcols()));
        const u16 yBorderPad = static_cast<u16>(0.05*static_cast<f32>(camera.GetCalibration().GetNrows()));
        if(unobserved.object->IsVisibleFrom(camera, DEG_TO_RAD(45), 20.f, true,
                                            xBorderPad, yBorderPad) &&
           (_robot->GetDockObject() != unobserved.object->GetID()))  // We expect a docking block to disappear from view!
        {
          // We "should" have seen the object! Delete it or mark it somehow
          CoreTechPrint("Removing object %d, which should have been seen, "
                        "but wasn't.\n", unobserved.object->GetID().GetValue());
          
          ClearObject(unobserved.object, unobserved.type, unobserved.family);
        } else if(unobserved.family != ObjectFamily::MATS && unobserved.object->GetID() != _robot->GetCarryingObject()) {
          // If the object should _not_ be visible (i.e. none of its markers project
          // into the camera), but some part of the object is within frame, it is
          // close enough, and was seen fairly recently, then
          // let listeners know it's "visible" but not identifiable, so we can
          // still interact with it in the UI, for example.
          
          // Did we see this currently-unobserved object in the last N seconds?
          // This is to avoid using this feature (reporting unobserved objects
          // that project into the image as observed) too liberally, and instead
          // only for objects seen pretty recently, e.g. for the case that we
          // have driven in too close and can't see an object we were just approaching.
          // TODO: Expose / remove / fine-tune this setting
          const s32 seenWithin_sec = -1; // Set to <0 to disable
          const bool seenRecently = (seenWithin_sec < 0 ||
                                     _robot->GetLastMsgTimestamp() - unobserved.object->GetLastObservedTime() < seenWithin_sec*1000);
          
          // How far away is the object from our current position? Again, to be
          // conservative, we are only going to use this feature if the object is
          // pretty close to the robot.
          // TODO: Expose / remove / fine-tune this setting
          const f32 distThreshold_mm = -1.f; // 150.f; // Set to <0 to disable
          const bool closeEnough = (distThreshold_mm < 0.f ||
                                    (_robot->GetPose().GetTranslation() -
                                    unobserved.object->GetPose().GetTranslation()).LengthSq() < distThreshold_mm*distThreshold_mm);
          
          // Check any of the markers should be visible and that the reason for
          // them not being visible is not occlusion.
          // For now just ignore the left and right 22.5% of the image blocked by the lift,
          // *iff* we are using VGA images, which have a wide enough FOV to be occluded
          // by the lift. (I.e., assume QVGA is a cropped, narrower FOV)
          // TODO: Actually project a lift into the image and figure out what it will occlude
          u16 xBorderPad = 0;
          switch(camera.GetCalibration().GetNcols())
          {
            case 640:
              xBorderPad = static_cast<u16>(0.225f * static_cast<f32>(camera.GetCalibration().GetNcols()));
              break;
            case 400:
              // TODO: How much should be occluded?
              xBorderPad = static_cast<u16>(0.20f * static_cast<f32>(camera.GetCalibration().GetNcols()));
              break;
            case 320:
              // Nothing to do, leave at zero
              break;
            default:
              // Not expecting other resolutions
              PRINT_NAMED_WARNING("BlockWorld.CheckForUnobservedObjects",
                                  "Unexpeted camera calibration ncols=%d.\n",
                                  camera.GetCalibration().GetNcols());
          }
          
          Vision::KnownMarker::NotVisibleReason reason;
          bool markersShouldBeVisible = false;
          bool markerIsOccluded = false;
          for(auto & marker : unobserved.object->GetMarkers()) {
            if(marker.IsVisibleFrom(_robot->GetCamera(), DEG_TO_RAD(45), 20.f, false, xBorderPad, 0, reason)) {
              // As soon as one marker is visible, we can stop
              markersShouldBeVisible = true;
              break;
            } else if(reason == Vision::KnownMarker::NotVisibleReason::OCCLUDED) {
              // Flag that any of the markers was not visible because it was occluded
              // If this is the case, then we don't want to signal this object as
              // partially visible.
              markerIsOccluded = true;
            }
            // This should never be true because we set requireSomethingBehind to false in IsVisibleFrom() above.
            assert(reason != Vision::KnownMarker::NotVisibleReason::NOTHING_BEHIND);
          }

          if(seenRecently && closeEnough && !markersShouldBeVisible && !markerIsOccluded)
          {
            // First three checks for object passed, now see if any of the object's
            // corners are in our FOV
            f32 distance;
            std::vector<Point2f> projectedCorners;
            _robot->GetCamera().ProjectObject(*unobserved.object, projectedCorners, distance);
            if(distance > 0.f) { // in front of camera?
              for(auto & corner : projectedCorners) {
                
                if(camera.IsWithinFieldOfView(corner)) {
                  
                  Rectangle<f32> boundingBox(projectedCorners);
                  //_obsProjectedObjects.emplace_back(obsID, boundingBox);
                  _currentObservedObjectIDs.push_back(unobserved.object->GetID());
                  
                  // Signal the observation of this object, with its bounding box:
                  const Vec3f& obsObjTrans = unobserved.object->GetPose().GetTranslation();
                  const UnitQuaternion<float>& q = unobserved.object->GetPose().GetRotation().GetQuaternion();
                  Radians topMarkerOrientation(0);
                  if(unobserved.object->IsActive()) {
                    ActiveCube* activeCube = dynamic_cast<ActiveCube*>(unobserved.object);
                    if(activeCube == nullptr) {
                      PRINT_NAMED_ERROR("BlockWorld.CheckForUnobservedObjects",
                                        "UnobservedObject %d with IsActive()==true could not be cast to ActiveCube.\n",
                                        unobserved.object->GetID().GetValue());
                    } else {
                      topMarkerOrientation = activeCube->GetTopMarkerOrientation();
                      
//                      PRINT_INFO("Unobserved object %d's rotation around Z = %.1fdeg\n",
//                                 unobserved.object->GetID().GetValue(),
//                                 topMarkerOrientation.getDegrees());
                    }
                  }
                  CozmoEngineSignals::RobotObservedObjectSignal().emit(_robot->GetID(),
                                                                       unobserved.family,
                                                                       unobserved.type,
                                                                       unobserved.object->GetID(),
                                                                       false, // marker not visible
                                                                       boundingBox.GetX(),
                                                                       boundingBox.GetY(),
                                                                       boundingBox.GetWidth(),
                                                                       boundingBox.GetHeight(),
                                                                       obsObjTrans.x(),
                                                                       obsObjTrans.y(),
                                                                       obsObjTrans.z(),
                                                                       q.w(), q.x(), q.y(), q.z(),
                                                                       topMarkerOrientation.ToFloat(),
                                                                       unobserved.object->IsActive());
                  ++numVisibleObjects;
                } // if(IsWithinFieldOfView)
              } // for(each projectedCorner)
            } // if(distance > 0)
          }
        }
        
      } // for each unobserved object
      
      return numVisibleObjects;
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

    void BlockWorld::GetObstacles(std::vector<std::pair<Quad2f,ObjectID> >& boundingBoxes, const f32 padding) const
    {
      std::set<ObjectID> ignoreIDs = {
        _robot->GetCarryingObject() // TODO: what if robot is carrying multiple objects?
      };
      
      // If the robot is localized, check to see if it is "on" the mat it is
      // localized to. If so, ignore the mat as an obstacle.
      // Note that the reason for checking IsPoseOn is that it's possible the
      // robot is localized to a mat it sees but is not on because it has not
      // yet seen the mat it is on. (For example, robot see side of platform
      // and localizes to it because it hasn't seen a marker on the flat mat
      // it is driving on.)
      if(_robot->IsLocalized()) {
        MatPiece* mat = dynamic_cast<MatPiece*>(GetObjectByIDandFamily(_robot->GetLocalizedTo(), ObjectFamily::MATS));
        if(mat != nullptr) {
          if(mat->IsPoseOn(_robot->GetPose(), 0.f, .25*ROBOT_BOUNDING_Z)) {
            // Ignore the ID of the mat we're on
            ignoreIDs.insert(_robot->GetLocalizedTo());
            
            // Add any "unsafe" regions this mat has
            mat->GetUnsafeRegions(boundingBoxes, padding);
          }
        } else {
          PRINT_NAMED_WARNING("BlockWorld.GetObstacles.LocalizedToNullMat",
                              "Robot %d is localized to object ID=%d, but "
                              "that object returned a NULL MatPiece pointer.\n",
                              _robot->GetID(), _robot->GetLocalizedTo().GetValue());
        }
      }
      
      // Figure out height filters in world coordinates (because GetObjectBoundingBoxesXY()
      // uses heights of objects in world coordinates)
      const Pose3d robotPoseWrtOrigin = _robot->GetPose().GetWithRespectToOrigin();
      const f32 minHeight = robotPoseWrtOrigin.GetTranslation().z();
      const f32 maxHeight = minHeight + _robot->GetHeight();
      
      GetObjectBoundingBoxesXY(minHeight, maxHeight, padding, boundingBoxes,
                               std::set<ObjectFamily>(),
                               std::set<ObjectType>(),
                               ignoreIDs);
    } // GetObstacles()
    
    
    void BlockWorld::GetObjectBoundingBoxesXY(const f32 minHeight,
                                              const f32 maxHeight,
                                              const f32 padding,
                                              std::vector<std::pair<Quad2f,ObjectID> >& rectangles,
                                              const std::set<ObjectFamily>& ignoreFamiles,
                                              const std::set<ObjectType>& ignoreTypes,
                                              const std::set<ObjectID>& ignoreIDs) const
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
                const bool useID = ignoreIDs.find(objectAndId.first) == ignoreIDs.end();
                if(useID)
                {
                  Vision::ObservableObject* object = objectAndId.second;
                  if(object == nullptr) {
                    PRINT_NAMED_WARNING("BlockWorld.GetObjectBoundingBoxesXY.NullObjectPointer",
                                        "ObjectID %d corresponds to NULL ObservableObject pointer.\n",
                                        objectAndId.first.GetValue());
                  } else if(object->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
                    const f32 objectHeight = objectAndId.second->GetPose().GetWithRespectToOrigin().GetTranslation().z();
                    if( (objectHeight >= minHeight) && (objectHeight <= maxHeight) )
                    {
                      rectangles.emplace_back(objectAndId.second->GetBoundingQuadXY(padding), objectAndId.first);
                    }
                  }
                } // if useID
              } // for each ID
            } // if(useType)
          } // for each type
        } // if useFamily
      } // for each family
      
    } // GetObjectBoundingBoxesXY()
    
    
    bool BlockWorld::DidObjectsChange() const {
      return _didObjectsChange;
    }

    
    bool BlockWorld::UpdateRobotPose(PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp, const TimeStamp_t atTimestamp)
    {
      bool wasPoseUpdated = false;
      
      // Extract only observed markers from obsMarkersAtTimestamp
      std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
      GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
      
      // Get all mat objects *seen by this robot's camera*
      std::vector<Vision::ObservableObject*> matsSeen;
      _objectLibrary[ObjectFamily::MATS].CreateObjectsFromMarkers(obsMarkersListAtTimestamp, matsSeen,
                                                                  (_robot->GetCamera().GetID()));

      // Remove used markers from map container
      RemoveUsedMarkers(obsMarkersAtTimestamp);
      
      if(not matsSeen.empty()) {
        /*
        // TODO: False mat marker localization causes the mat to be created in weird places which is messing up game dev.
        //       Seems to happen particular when looking at the number 1. Disabled for now.
        PRINT_NAMED_WARNING("UpdateRobotPose.TempIgnore", "Ignoring mat marker. Robot localization disabled.");
        return false;
        */
        
        // Is the robot "on" any of the mats it sees?
        // TODO: What to do if robot is "on" more than one mat simultaneously?
        MatPiece* onMat = nullptr;
        for(auto object : matsSeen) {
          
          // ObservedObjects are w.r.t. the arbitrary historical origin of the camera
          // that observed them.  Hook them up to the current robot origin now:
          CORETECH_ASSERT(object->GetPose().GetParent() != nullptr &&
                          object->GetPose().GetParent()->IsOrigin());
          object->SetPoseParent(_robot->GetWorldOrigin());
          
          MatPiece* mat = dynamic_cast<MatPiece*>(object);
          CORETECH_ASSERT(mat != nullptr);
          
          // Does this mat pose make sense? I.e., is the top surface flat enough
          // that we could drive on it?
          Vec3f rotAxis;
          Radians rotAngle;
          mat->GetPose().GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
          if(std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) &&                // There's any rotation to speak of
             !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(45)))  // That rotation's axis more than 45 degrees from vertical
          {
            PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose",
                             "Refusing to localize to %s mat with rotation %.1f degrees around (%.1f,%.1f,%.1f) axis.\n",
                             mat->GetType().GetName().c_str(),
                             rotAngle.getDegrees(),
                             rotAxis.x(), rotAxis.y(), rotAxis.z());
          }else if(mat->IsPoseOn(_robot->GetPose(), 0, 15.f)) { // TODO: get heightTol from robot
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
          
          PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.OnMatLocalization",
                                  "Robot %d is on a %s mat and will localize to it.\n",
                                  _robot->GetID(), onMat->GetType().GetName().c_str());
          
          // If robot is "on" one of the mats it is currently seeing, localize
          // the robot to that mat
          matToLocalizeTo = onMat;
        }
        else {
          // If the robot is NOT "on" any of the mats it is seeing...
          
          if(_robot->IsLocalized()) {
            // ... and the robot is already localized, then see if it is
            // localized to one of the mats it is seeing (but not "on")
            // Note that we must match seen and existing objects by their pose
            // here, and not by ID, because "seen" objects have not ID assigned
            // yet.

            Vision::ObservableObject* existingMatLocalizedTo = GetObjectByID(_robot->GetLocalizedTo());
            if(existingMatLocalizedTo == nullptr) {
              PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.ExistingMatLocalizedToNull",
                                "Robot %d is localized to mat with ID=%d, but that mat does not exist in the world.\n",
                                _robot->GetID(), _robot->GetLocalizedTo().GetValue());
              return false;
            }
            
            std::vector<Vision::ObservableObject*> overlappingMatsSeen;
            FindOverlappingObjects(existingMatLocalizedTo, matsSeen, overlappingMatsSeen);
            
            if(overlappingMatsSeen.empty()) {
              // The robot is localized to a mat it is not seeing (and is not "on"
              // any of the mats it _is_ seeing.  Just update the poses of the
              // mats it is seeing, but don't localize to any of them.
              PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.NotOnMatNoLocalize",
                                      "Robot %d is localized to a mat it doesn't see, and will not localize to any of the %lu mats it sees but is not on.\n",
                                      _robot->GetID(), matsSeen.size());
            }
            else {
              if(overlappingMatsSeen.size() > 1) {
                PRINT_STREAM_WARNING("BlockWorld.UpdateRobotPose.MultipleOverlappingMats",
                                    "Robot " << _robot->GetID() << " is seeing " << overlappingMatsSeen.size() << " (i.e. more than one) mats "
                                    "overlapping with the existing mat it is localized to. "
                                    "Will use first.");
              }
              
              PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.NotOnMatLocalization",
                                      "Robot %d will re-localize to the %s mat it is not on, but already localized to.\n",
                                      _robot->GetID(), overlappingMatsSeen[0]->GetType().GetName().c_str());
              
              // The robot is localized to one of the mats it is seeing, even
              // though it is not _on_ that mat.  Remain localized to that mat
              // and update any others it is also seeing
              matToLocalizeTo = dynamic_cast<MatPiece*>(overlappingMatsSeen[0]);
              CORETECH_ASSERT(matToLocalizeTo != nullptr);
            }
            
            
          } else {
            // ... and the robot is _not_ localized, choose the observed mat
            // with the closest observed marker (since that is likely to be the
            // most accurate) and localize to that one.
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
                if(obsMarker->GetPose().GetWithRespectTo(_robot->GetPose(), markerWrtRobot) == false) {
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
            
            PRINT_LOCALIZATION_INFO("BLockWorld.UpdateRobotPose.NotOnMatLocalizationToClosest",
                                    "Robot %d is not on a mat but will localize to %s mat ID=%d, which is the closest.\n",
                                    _robot->GetID(), closestMat->GetType().GetName().c_str(), closestMat->GetID().GetValue());
            
            matToLocalizeTo = closestMat;
            
          } // if/else robot is localized
        } // if/else (onMat != nullptr)
        
        ObjectsMapByType_t& existingMatPieces = _existingObjects[ObjectFamily::MATS];
        
        // Keep track of markers we saw on existing/instantiated mats, to use
        // for occlusion checking
        std::vector<const Vision::KnownMarker *> observedMarkers;

        MatPiece* existingMatPiece = nullptr;
        
        // If we found a suitable mat to localize to, and we've seen it enough
        // times, then use it for localizing
        if(matToLocalizeTo != nullptr) {
          
          if(existingMatPieces.empty()) {
            // If this is the first mat piece, add it to the world using the world
            // origin as its pose
            PRINT_STREAM_INFO("BlockWorld.UpdateRobotPose.CreatingFirstMatPiece",
                       "Instantiating first mat piece in the world.");
            
            existingMatPiece = dynamic_cast<MatPiece*>(matToLocalizeTo->CloneType());
            assert(existingMatPiece != nullptr);
            AddNewObject(existingMatPieces, existingMatPiece);
            
            existingMatPiece->SetPose( Pose3d() ); // Not really necessary, but ensures the ID makes it into the pose name, which is helpful for debugging
            assert(existingMatPiece->GetPose().GetParent() == nullptr);
            
          }
          else {
            // We can't look up the existing piece by ID because the matToLocalizeTo
            // is just a mat we _saw_, not one we've instantiated.  So look for
            // one in approximately the same position, of those with the same
            // type:
            //Vision::ObservableObject* existingObject = GetObjectByID(matToLocalizeTo->GetID());
            std::vector<Vision::ObservableObject*> existingObjects;
            FindOverlappingObjects(matToLocalizeTo, _existingObjects[ObjectFamily::MATS], existingObjects);
          
            if(existingObjects.empty())
            {
              // If the mat we are about to localize to does not exist yet,
              // but it's not the first mat piece in the world, add it to the
              // world, and give it a new origin, relative to the current
              // world origin.
              Pose3d poseWrtWorldOrigin = matToLocalizeTo->GetPose().GetWithRespectToOrigin();
              
              existingMatPiece = dynamic_cast<MatPiece*>(matToLocalizeTo->CloneType());
              assert(existingMatPiece != nullptr);
              AddNewObject(existingMatPieces, existingMatPiece);
              existingMatPiece->SetPose(poseWrtWorldOrigin); // Do after AddNewObject, once ID is set
              
              PRINT_STREAM_INFO("BlockWorld.UpdateRobotPose.LocalizingToNewMat",
                         "Robot " << _robot->GetID() << " localizing to new "
                                << existingMatPiece->GetType().GetName() << " mat with ID=" << existingMatPiece->GetID().GetValue() << ".");
              
            } else {
              if(existingObjects.size() > 1) {
                PRINT_NAMED_WARNING("BlockWorld.UpdateRobotPose.MultipleExistingObjectMatches",
                              "Robot %d found multiple existing mats matching the one it "
                              "will localize to - using first.\n", _robot->GetID());
              }
              
              // We are localizing to an existing mat piece: do not attempt to
              // update its pose (we can't both update the mat's pose and use it
              // to update the robot's pose at the same time!)
              existingMatPiece = dynamic_cast<MatPiece*>(existingObjects.front());
              CORETECH_ASSERT(existingMatPiece != nullptr);
              
              PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.LocalizingToExistingMat",
                                      "Robot %d localizing to existing %s mat with ID=%d.\n",
                                      _robot->GetID(), existingMatPiece->GetType().GetName().c_str(),
                                      existingMatPiece->GetID().GetValue());
            }
          } // if/else (existingMatPieces.empty())
          
          existingMatPiece->SetLastObservedTime(matToLocalizeTo->GetLastObservedTime());
          existingMatPiece->UpdateMarkerObservationTimes(*matToLocalizeTo);
          existingMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);
          
          if(existingMatPiece->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
            // Now localize to that mat
            //wasPoseUpdated = LocalizeRobotToMat(robot, matToLocalizeTo, existingMatPiece);
            if(_robot->LocalizeToMat(matToLocalizeTo, existingMatPiece) == RESULT_OK) {
              wasPoseUpdated = true;
            }
          }
          
        } // if(matToLocalizeTo != nullptr)
        
        // Update poses of any other mats we saw (but did not localize to),
        // just like they are any "regular" object, unless that mat is the
        // robot's current "world" origin, [TODO:] in which case we will update the pose
        // of the mat we are on w.r.t. that world.
        for(auto matSeen : matsSeen) {
          if(matSeen != matToLocalizeTo) {
            
            // TODO: Make this w.r.t. whatever the robot is currently localized to?
            Pose3d poseWrtOrigin = matSeen->GetPose().GetWithRespectToOrigin();
            
            // Does this mat pose make sense? I.e., is the top surface flat enough
            // that we could drive on it?
            Vec3f rotAxis;
            Radians rotAngle;
            poseWrtOrigin.GetRotationVector().GetAngleAndAxis(rotAngle, rotAxis);
            if(std::abs(rotAngle.ToFloat()) > DEG_TO_RAD(5) &&                // There's any rotation to speak of
               !AreUnitVectorsAligned(rotAxis, Z_AXIS_3D(), DEG_TO_RAD(45)))  // That rotation's axis more than 45 degrees from vertical
            {
              PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose",
                               "Ignoring observation of %s mat with rotation %.1f degrees around (%.1f,%.1f,%.1f) axis.\n",
                               matSeen->GetType().GetName().c_str(),
                               rotAngle.getDegrees(),
                               rotAxis.x(), rotAxis.y(), rotAxis.z());
              continue;
            }
            
            // Store pointers to any existing objects that overlap with this one
            std::vector<Vision::ObservableObject*> overlappingObjects;
            FindOverlappingObjects(matSeen, existingMatPieces, overlappingObjects);
            
            if(overlappingObjects.empty()) {
              // no existing mats overlapped with the mat we saw, so add it
              // as a new mat piece, relative to the world origin
              Vision::ObservableObject* newMatPiece = matSeen->CloneType();
              AddNewObject(existingMatPieces, newMatPiece);
              newMatPiece->SetPose(poseWrtOrigin); // do after AddNewObject, once ID is set
              
              // TODO: Make clone copy the observation times
              newMatPiece->SetLastObservedTime(matSeen->GetLastObservedTime());
              newMatPiece->UpdateMarkerObservationTimes(*matSeen);
              
              PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose",
                               "Adding new %s mat with ID=%d at (%.1f, %.1f, %.1f)\n",
                               newMatPiece->GetType().GetName().c_str(), newMatPiece->GetID().GetValue(),
                               newMatPiece->GetPose().GetTranslation().x(),
                               newMatPiece->GetPose().GetTranslation().y(),
                               newMatPiece->GetPose().GetTranslation().z());
              
              // Add observed mat markers to the occlusion map of the camera that saw
              // them, so we can use them to delete objects that should have been
              // seen between that marker and the robot
              newMatPiece->GetObservedMarkers(observedMarkers, atTimestamp);

            }
            else {
              if(overlappingObjects.size() > 1) {
                PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose",
                                        "More than one overlapping mat found -- will use first.\n");
                // TODO: do something smarter here?
              }
              
              if(&(overlappingObjects[0]->GetPose()) != _robot->GetWorldOrigin()) {
                // The overlapping mat object is NOT the world origin mat, whose
                // pose we don't want to update.
                // Update existing observed mat we saw but are not on w.r.t.
                // the robot's current world origin
                
                // TODO: better way of merging existing/observed object pose
                overlappingObjects[0]->SetPose( poseWrtOrigin );
                
              } else {
                /* PUNT - not sure this is workign, nor we want to bother with this for now...
                CORETECH_ASSERT(robot->IsLocalized());
                
                // Find the mat the robot is currently localized to
                MatPiece* localizedToMat = nullptr;
                for(auto & objectsByType : existingMatPieces) {
                  auto objectsByIdIter = objectsByType.second.find(robot->GetLocalizedTo());
                  if(objectsByIdIter != objectsByType.second.end()) {
                    localizedToMat = dynamic_cast<MatPiece*>(objectsByIdIter->second);
                  }
                }

                CORETECH_ASSERT(localizedToMat != nullptr);
                
                // Update the mat we are localized to (but may not have seen) w.r.t. the existing
                // observed world origin mat we did see from it.  This should in turn
                // update the pose of everything on that mat.
                Pose3d newPose;
                if(localizedToMat->GetPose().GetWithRespectTo(matSeen->GetPose(), newPose) == false) {
                  PRINT_NAMED_ERROR("BlockWorld.UpdateRobotPose.FailedToUpdateWrtObservedOrigin",
                                    "Robot %d failed to get pose of existing %s mat it is on w.r.t. observed world origin mat.\n",
                                    robot->GetID(), existingMatPiece->GetType().GetName().c_str());
                }
                newPose.SetParent(robot->GetWorldOrigin());
                // TODO: Switch new pose to be w.r.t. whatever robot is localized to??
                localizedToMat->SetPose(newPose);
                 */
              }
              
              overlappingObjects[0]->SetLastObservedTime(matSeen->GetLastObservedTime());
              overlappingObjects[0]->UpdateMarkerObservationTimes(*matSeen);
              overlappingObjects[0]->GetObservedMarkers(observedMarkers, atTimestamp);
              
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
          _robot->GetCamera().AddOccluder(*obsMarker);
        }
        
        /* Always re-drawing everything now
        // If the robot just re-localized, trigger a draw of all objects, since
        // we may have seen things while de-localized whose locations can now be
        // snapped into place.
        if(!wasLocalized && robot->IsLocalized()) {
          PRINT_NAMED_INFO("BlockWorld.UpdateRobotPose.RobotRelocalized",
                           "Robot %d just localized after being de-localized.\n", robot->GetID());
          DrawAllObjects();
        }
        */
      } // IF any mat piece was seen

      if(wasPoseUpdated) {
        PRINT_LOCALIZATION_INFO("BlockWorld.UpdateRobotPose.RobotPoseChain", "%s\n",
                                _robot->GetPose().GetNamedPathToOrigin(true).c_str());
      }
      
      return wasPoseUpdated;
      
    } // UpdateRobotPose()
    
    size_t BlockWorld::UpdateObjectPoses(PoseKeyObsMarkerMap_t& obsMarkersAtTimestamp,
                                         const ObjectFamily& inFamily,
                                         const TimeStamp_t atTimestamp)
    {
      const Vision::ObservableObjectLibrary& objectLibrary = _objectLibrary[inFamily];
      
      std::vector<Vision::ObservableObject*> objectsSeen;
      
      // Don't bother with this update at all if we didn't see at least one
      // marker (which is our indication we got an update from the robot's
      // vision system
      if(not _obsMarkers.empty()) {
        
        // Extract only observed markers from obsMarkersAtTimestamp
        std::list<Vision::ObservedMarker*> obsMarkersListAtTimestamp;
        GetObsMarkerList(obsMarkersAtTimestamp, obsMarkersListAtTimestamp);
        
        objectLibrary.CreateObjectsFromMarkers(obsMarkersListAtTimestamp, objectsSeen);
        
        // Remove used markers from map
        RemoveUsedMarkers(obsMarkersAtTimestamp);
      
        for(auto object : objectsSeen) {
          // ObservedObjects are w.r.t. the arbitrary historical origin of the camera
          // that observed them.  Hook them up to the current robot origin now:
          CORETECH_ASSERT(object->GetPose().GetParent() != nullptr &&
                          object->GetPose().GetParent()->IsOrigin());
          object->SetPoseParent(_robot->GetWorldOrigin());
        }
        
        // Use them to add or update existing blocks in our world
        AddAndUpdateObjects(objectsSeen, inFamily, atTimestamp);
      }
      
      return objectsSeen.size();
      
    } // UpdateObjectPoses()

    Result BlockWorld::UpdateProxObstaclePoses()
    {
      TimeStamp_t lastTimestamp = _robot->GetLastMsgTimestamp();
      
      // Add prox obstacle if detected and one doesn't already exist
      for (ProxSensor_t sensor = (ProxSensor_t)(0); sensor < NUM_PROX; sensor = (ProxSensor_t)(sensor + 1)) {
        if (!_robot->IsProxSensorBlocked(sensor) && _robot->GetProxSensorVal(sensor) >= PROX_OBSTACLE_DETECT_THRESH) {
          
          // Create an instance of the detected object
          MarkerlessObject *m = new MarkerlessObject(MarkerlessObject::Type::PROX_OBSTACLE);
          
          // Get pose of detected object relative to robot according to which sensor it was detected by.
          Pose3d proxTransform = Robot::ProxDetectTransform[sensor];
          
          // Raise origin of object above ground.
          // NOTE: Assuming detected obstacle is at ground level no matter what angle the head is at.
          Pose3d raiseObject(0, Z_AXIS_3D(), Vec3f(0,0,0.5f*m->GetSize().z()));
          proxTransform = proxTransform * raiseObject;
          
          proxTransform.SetParent(_robot->GetPose().GetParent());
          
          // Compute pose of detected object
          Pose3d obsPose(_robot->GetPose());
          obsPose = obsPose * proxTransform;
          m->SetPose(obsPose);
          m->SetPoseParent(_robot->GetPose().GetParent());
          
          // Check if this prox obstacle already exists
          std::vector<Vision::ObservableObject*> existingObjects;
          FindOverlappingObjects(m, _existingObjects[ObjectFamily::MARKERLESS_OBJECTS], existingObjects);
          
          // Update the last observed time of existing overlapping obstacles
          for(auto obj : existingObjects) {
            obj->SetLastObservedTime(lastTimestamp);
          }
          
          // No need to add the obstacle again if it already exists
          if (!existingObjects.empty()) {
            delete m;
            return RESULT_OK;
          }
          
          
          // Check if the obstacle intersects with any other existing objects in the scene.
          std::set<ObjectFamily> ignoreFamilies;
          std::set<ObjectType> ignoreTypes;
          std::set<ObjectID> ignoreIDs;
          if(_robot->IsLocalized()) {
            // Ignore the mat object that the robot is localized to (?)
            ignoreIDs.insert(_robot->GetLocalizedTo());
          }
          FindIntersectingObjects(m, ignoreFamilies, ignoreTypes, ignoreIDs, existingObjects, 0);
          if (!existingObjects.empty()) {
            delete m;
            return RESULT_OK;
          }

          
          m->SetLastObservedTime(lastTimestamp);
          AddNewObject(ObjectFamily::MARKERLESS_OBJECTS, m);
          _didObjectsChange = true;
        }
      } // end for all prox sensors
      
      // Delete any existing prox objects that are too old.
      // Note that we use find() here because there may not be any markerless objects
      // yet, and using [] indexing will create things.
      auto markerlessFamily = _existingObjects.find(ObjectFamily::MARKERLESS_OBJECTS);
      if(markerlessFamily != _existingObjects.end())
      {
        auto proxTypeMap = markerlessFamily->second.find(MarkerlessObject::Type::PROX_OBSTACLE);
        if(proxTypeMap != markerlessFamily->second.end())
        {
          for (auto proxObsIter = proxTypeMap->second.begin();
               proxObsIter != proxTypeMap->second.end();
               /* increment iter in loop, depending on erase*/)
          {
            if (lastTimestamp - proxObsIter->second->GetLastObservedTime() > PROX_OBSTACLE_LIFETIME_MS)
            {
              proxObsIter = ClearObject(proxObsIter, MarkerlessObject::Type::PROX_OBSTACLE,
                                        ObjectFamily::MARKERLESS_OBJECTS);
              
            } else {
              // Didn't erase anything, increment iterator
              ++proxObsIter;
            }
          }
        }
      }
      
      return RESULT_OK;
    }
    
    
    Result BlockWorld::Update(uint32_t& numObjectsObserved)
    {
      numObjectsObserved = 0;
      
      // New timestep, new set of occluders.  Get rid of anything registered as
      // an occluder with the robot's camera
      _robot->GetCamera().ClearOccluders();
      
      // New timestep, clear list of observed object bounding boxes
      //_obsProjectedObjects.clear();
      _currentObservedObjectIDs.clear();
      
      static TimeStamp_t lastObsMarkerTime = 0;
      
      // Now we're going to process all the observed messages, grouped by
      // timestamp
      size_t numUnusedMarkers = 0;
      for(auto obsMarkerListMapIter = _obsMarkers.begin();
          obsMarkerListMapIter != _obsMarkers.end();
          ++obsMarkerListMapIter)
      {
        PoseKeyObsMarkerMap_t& currentObsMarkers = obsMarkerListMapIter->second;
        const TimeStamp_t atTimestamp = obsMarkerListMapIter->first;
        
        lastObsMarkerTime = std::max(lastObsMarkerTime, atTimestamp);
        
        //
        // Localize robots using mat observations
        //
        
        // Remove observed markers whose historical poses have become invalid.
        // This shouldn't happen! If it does, robotStateMsgs may be buffering up somewhere.
        // Increasing history time window would fix this, but it's not really a solution.
        for(auto poseKeyMarkerPair = currentObsMarkers.begin(); poseKeyMarkerPair != currentObsMarkers.end();) {
          if ((poseKeyMarkerPair->second.GetSeenBy().GetID() == _robot->GetCamera().GetID()) &&
              !_robot->IsValidPoseKey(poseKeyMarkerPair->first)) {
            PRINT_NAMED_WARNING("BlockWorld.Update.InvalidHistPoseKey", "key=%d\n", poseKeyMarkerPair->first);
            poseKeyMarkerPair = currentObsMarkers.erase(poseKeyMarkerPair);
          } else {
            ++poseKeyMarkerPair;
          }
        }
        
        // Remove markers enclosed within other markers:
        for(auto markerIter1 = currentObsMarkers.begin(); markerIter1 != currentObsMarkers.end(); ++markerIter1)
        {
          const Vision::ObservedMarker& marker1    = markerIter1->second;
          const TimeStamp_t             timestamp1 = markerIter1->first;
          
          for(auto markerIter2 = currentObsMarkers.begin(); markerIter2 != currentObsMarkers.end(); /* incrementing decided in loop */ )
          {
            const Vision::ObservedMarker& marker2    = markerIter2->second;
            const TimeStamp_t             timestamp2 = markerIter2->first;
            
            // These two markers must be different and observed at the same time
            if(markerIter1 != markerIter2 && timestamp1 == timestamp2) {
              
              // See if #2 is inside #1
              bool marker2isInsideMarker1 = true;
              for(auto & corner : marker2.GetImageCorners()) {
                if(marker1.GetImageCorners().Contains(corner) == false) {
                  marker2isInsideMarker1 = false;
                  break;
                }
              }
              
              if(marker2isInsideMarker1) {
                PRINT_NAMED_INFO("BlockWorld.Update",
                                 "Removing %s marker completely contained within %s marker.\n",
                                 marker1.GetCodeName(), marker2.GetCodeName());
                // Note: erase does increment of iterator for us
                markerIter2 = currentObsMarkers.erase(markerIter2);
              } else {
                // Need to iterate marker2
                ++markerIter2;
              } // if/else marker2isInsideMarker1
            } else {
              // Need to iterate marker2
              ++markerIter2;
            } // if/else marker1 != marker2 && time1 != time2
          } // for markerIter2
        } // for markerIter1
        
        // Only update robot's poses using VisionMarkers while not on a ramp
        if(!_robot->IsOnRamp()) {
          if (!_robot->IsPhysical() || !SKIP_PHYS_ROBOT_LOCALIZATION) {
            // Note that this removes markers from the list that it uses
            UpdateRobotPose(currentObsMarkers, atTimestamp);
          }
        }
        
        // Reset the flag telling us objects changed here, before we update any objects:
        _didObjectsChange = false;
        
        //
        // Find any observed blocks from the remaining markers
        //
        // Note that this removes markers from the list that it uses
        numObjectsObserved += UpdateObjectPoses(currentObsMarkers, ObjectFamily::BLOCKS, atTimestamp);
        
        //
        // Find any observed active blocks from the remaining markers
        //
        // Note that this removes markers from the list that it uses
        numObjectsObserved += UpdateObjectPoses(currentObsMarkers, ObjectFamily::ACTIVE_BLOCKS, atTimestamp);
        
        //
        // Find any observed ramps from the remaining markers
        //
        // Note that this removes markers from the list that it uses
        numObjectsObserved += UpdateObjectPoses(currentObsMarkers, ObjectFamily::RAMPS, atTimestamp);

        //
        // Find any observed chargers from the remaining markers
        //
        // Note that this removes markers from the list that it uses
        numObjectsObserved += UpdateObjectPoses(currentObsMarkers, ObjectFamily::CHARGERS, atTimestamp);
        
        //
        // Find any observed human heads from the remaining "markers"
        //
        // Note that this removes markers from the list that it uses
        numObjectsObserved += UpdateObjectPoses(currentObsMarkers, ObjectFamily::HUMAN_HEADS, atTimestamp);
        
        // TODO: Deal with unknown markers?
        
        // Keep track of how many markers went unused by either robot or block
        // pose updating processes above
        numUnusedMarkers += currentObsMarkers.size();
        
        for(auto & unusedMarker : currentObsMarkers) {
          PRINT_NAMED_INFO("BlockWorld.Update.UnusedMarker",
                           "An observed %s marker went unused.\n",
                           unusedMarker.second.GetCodeName());
        }
        
        // Delete any objects that should have been observed but weren't,
        // visualize objects that were observed:
        numObjectsObserved += CheckForUnobservedObjects(atTimestamp);
        
      } // for element in _obsMarkers
      
      if(_obsMarkers.empty()) {
        // Even if there were no markers observed, check to see if there are
        // any previously-observed objects that are partially visible (some part
        // of them projects into the image even if none of their markers fully do)
        numObjectsObserved += CheckForUnobservedObjects(_robot->GetLastImageTimeStamp());
      }
      
      if(numObjectsObserved == 0) {
        // If we didn't see/update anything, send a signal saying so
        CozmoEngineSignals::RobotObservedNothingSignal().emit(_robot->GetID());
      }
      
      //PRINT_NAMED_INFO("BlockWorld.Update.NumBlocksObserved", "Saw %d blocks\n", numBlocksObserved);
      
      // Check for unobserved, uncarried objects that overlap with any robot's position
      // TODO: better way of specifying which objects are obstacles and which are not
      for(auto & objectsByFamily : _existingObjects)
      {
        // For now, look for collision with anything other than Mat objects
        // NOTE: This assumes all other objects are DockableObjects below!!! (Becuase of IsBeingCarried() check)
        // TODO: How can we delete Mat objects (like platforms) whose positions we drive through
        if(objectsByFamily.first != ObjectFamily::MATS &&
           objectsByFamily.first != ObjectFamily::MARKERLESS_OBJECTS &&
           objectsByFamily.first != ObjectFamily::HUMAN_HEADS)
        {
          for(auto & objectsByType : objectsByFamily.second)
          {
            
            //for(auto & objectsByID : objectsByType.second)
            for(auto objectIter = objectsByType.second.begin();
                objectIter != objectsByType.second.end(); /* increment based on whether we erase */)
            {
              ActionableObject* object = dynamic_cast<ActionableObject*>(objectIter->second);
              if(object == nullptr) {
                PRINT_NAMED_ERROR("BlockWorld.Update.ExpectingDockableObject",
                                  "In robot/object collision check, can currently only handle ActionableObjects.\n");
                continue;
              }
              
              bool didErase = false;
              if(object->GetLastObservedTime() < _robot->GetLastImageTimeStamp() &&
                 !object->IsBeingCarried())
              {
                // Don't worry about collision while picking or placing since we
                // are trying to get close to blocks in these modes.
                // TODO: specify whether we are picking/placing _this_ block
                if(!_robot->IsPickingOrPlacing())
                {
                  // Check block's bounding box in same coordinates as this robot to
                  // see if it intersects with the robot's bounding box. Also check to see
                  // block and the robot are at overlapping heights.  Skip this check
                  // entirely if the block isn't in the same coordinate tree as the
                  // robot.
                  Pose3d objectPoseWrtRobotOrigin;
                  if(object->GetPose().GetWithRespectTo(_robot->GetPose().FindOrigin(), objectPoseWrtRobotOrigin) == true)
                  {
                    const Quad2f objectBBox = object->GetBoundingQuadXY(objectPoseWrtRobotOrigin);
                    const f32    objectHeight = objectPoseWrtRobotOrigin.GetTranslation().z();
                    /*
                     const f32    blockSize   = 0.5f*object->GetSize().Length();
                     const f32    blockTop    = objectHeight + blockSize;
                     const f32    blockBottom = objectHeight - blockSize;
                     */
                    const f32 robotBottom = _robot->GetPose().GetTranslation().z();
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
                    
                    const bool bboxIntersects   = objectBBox.Intersects(_robot->GetBoundingQuadXY());
                    
                    if( inSamePlane && bboxIntersects )
                    {
                      PRINT_NAMED_INFO("BlockWorld.Update",
                                       "Removing object %d, which intersects robot %d's bounding quad.\n",
                                       object->GetID().GetValue(), _robot->GetID());
                      
                      // Erase the vizualized block and its projected quad
                      //VizManager::getInstance()->EraseCuboid(object->GetID());

                      // Erase the block (with a postfix increment of the iterator)
                      objectIter = ClearObject(objectIter, objectsByType.first, objectsByFamily.first);
                      didErase = true;
                      
                      break; // no need to check other robots, block already gone
                    } // if quads intersect
                  } // if we got block pose wrt robot origin
                } // if robot is not picking or placing

              } // if block was not observed
              
              if(!didErase) {
                ++objectIter;
              }
              
            } // for each object of this type
          } // for each object type
        } // if not in the Mat family
      } // for each object family
      
      if(numUnusedMarkers > 0) {
        if (!_robot->IsPhysical() || !SKIP_PHYS_ROBOT_LOCALIZATION) {
          PRINT_NAMED_WARNING("BlockWorld.Update.UnusedMarkers",
                              "%zu observed markers did not match any known objects and went unused.\n",
                              numUnusedMarkers);
        }
      }
     
      // Toss any remaining markers?
      ClearAllObservedMarkers();
      
      Result lastResult = UpdateProxObstaclePoses();
      if(lastResult != RESULT_OK) {
        return lastResult;
      }

      return RESULT_OK;
      
    } // Update()
    
    
    Result BlockWorld::QueueObservedMarker(HistPoseKey& poseKey, Vision::ObservedMarker& marker)
    {
      Result lastResult = RESULT_OK;
      
      // Finally actually queue the marker
      _obsMarkers[marker.GetTimeStamp()].emplace(poseKey, marker);
            
      
      return lastResult;
      
    } // QueueObservedMarker()
    
    void BlockWorld::ClearAllObservedMarkers()
    {
      _obsMarkers.clear();
    }
    
    void BlockWorld::ClearAllExistingObjects()
    {
      for(auto & objectsByFamily : _existingObjects) {
        for(auto objectsByType : objectsByFamily.second) {
          for(auto objectsByID : objectsByType.second) {
            ClearObjectHelper(objectsByID.second);
          }
        }
      }
      
      _existingObjects.clear();

      ObjectID::Reset();
    }
    
    void BlockWorld::ClearObjectHelper(Vision::ObservableObject* object)
    {
      if(object == nullptr) {
        PRINT_NAMED_WARNING("BlockWorld.ClearObjectHelper.NullObjectPointer", "BlockWorld asked to clear a null object pointer.\n");
      } else {
        // Check to see if this object is the one the robot is localized to.
        // If so, the robot needs to be delocalized:
        if(_robot->GetLocalizedTo() == object->GetID()) {
          PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.DelocalizingRobot",
                           "Delocalizing robot %d, which is currently localized to %s "
                           "object with ID=%d, which is about to be deleted.\n",
                           _robot->GetID(), object->GetType().GetName().c_str(), object->GetID().GetValue());
          _robot->Delocalize();
        }
        
        // TODO: If this is a mat piece, check to see if there are any objects "on" it (COZMO-138)
        // If so, delete them too or update their poses somehow? (Deleting seems easier)
        
        // Check to see if this object is the one the robot is carrying.
        if(_robot->GetCarryingObject() == object->GetID()) {
          PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.ClearingCarriedObject",
                           "Clearing %s object %d which robot %d thinks it is carrying.\n",
                           object->GetType().GetName().c_str(),
                           object->GetID().GetValue(),
                           _robot->GetID());
          _robot->UnSetCarryingObject();
        }
        
        if(_selectedObject == object->GetID()) {
          PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.ClearingSelectedObject",
                           "Clearing %s object %d which is currently selected.\n",
                           object->GetType().GetName().c_str(),
                           object->GetID().GetValue());
          _selectedObject.UnSet();
        }
        
        if(_robot->GetTrackToObject() == object->GetID()) {
          PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.ClearingTrackHeadToObject",
                           "Clearing %s object %d which robot %d is currently tracking its head to.\n",
                           object->GetType().GetName().c_str(),
                           object->GetID().GetValue(),
                           _robot->GetID());
          _robot->DisableTrackToObject();
        }
        
        if(object->IsActive()) {
          PRINT_NAMED_INFO("BlockWorld.ClearObjectHelper.TurningOffLights",
                           "Sending message to turn off active object %d's lights because "
                           "it is being deleted.\n", object->GetID().GetValue());
          _robot->TurnOffObjectLights(object->GetID());
        }
        
        // Notify any listeners that this object is being deleted
        if(object->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT) {
          CozmoEngineSignals::RobotDeletedObjectSignal().emit(_robot->GetID(), object->GetID().GetValue());
        }
        
        // NOTE: The object should erase its own visualization upon destruction
        delete object;
        
        // Flag that we removed an object
        _didObjectsChange = true;
      }
    }
    
    Vision::ObservableObject* BlockWorld::FindObjectOnTopOf(const Vision::ObservableObject& objectOnBottom,
                                                            f32 zTolerance) const
    {
      Point3f sameDistTol(objectOnBottom.GetSize());
      sameDistTol.z() = zTolerance;
      sameDistTol = objectOnBottom.GetPose().GetRotation() * sameDistTol;
      sameDistTol.Abs();
      
      // Find the point at the top middle of the object on bottom
      Point3f rotatedBtmSize(objectOnBottom.GetPose().GetRotation() * objectOnBottom.GetSize());
      Point3f topOfObjectOnBottom(objectOnBottom.GetPose().GetTranslation());
      topOfObjectOnBottom.z() += 0.5f*std::abs(rotatedBtmSize.z());
      
      for(auto & objectsByFamily : _existingObjects) {
        for(auto & objectsByType : objectsByFamily.second) {
          for(auto & objectsByID : objectsByType.second) {
            Vision::ObservableObject* candidateObject = objectsByID.second;
            
            if(candidateObject->GetID() != objectOnBottom.GetID()) {
              // Find the point at bottom middle of the object we're checking to be on top
              Point3f rotatedTopSize(candidateObject->GetPose().GetRotation() * candidateObject->GetSize());
              Point3f bottomOfCandidateObject(candidateObject->GetPose().GetTranslation());
              bottomOfCandidateObject.z() -= 0.5f*std::abs(rotatedTopSize.z());
              
              // If the top of the bottom object and the bottom the candidate top object are
              // close enough together, return this as the object on top
              Point3f dist(topOfObjectOnBottom);
              dist -= bottomOfCandidateObject;
              dist.Abs();
              
              if(dist < sameDistTol) {
                return candidateObject;
              }
            } // IF not object on bottom (by ID)
          }
        }
      }
      
      return nullptr;
    }
    
    Vision::ObservableObject* BlockWorld::FindObjectClosestTo(const Pose3d& pose,
                                                              const Vec3f&  distThreshold,
                                                              const std::set<ObjectID>& ignoreIDs,
                                                              const std::set<ObjectType>& ignoreTypes,
                                                              const std::set<ObjectFamily>& ignoreFamilies) const
    {
      // TODO: Keep some kind of OctTree data structure to make these queries faster?
      
      Vec3f closestDist(std::numeric_limits<f32>::max());
      Vision::ObservableObject* matchingObject = nullptr;
      
      for(auto & objectsByFamily : _existingObjects) {
        if(ignoreFamilies.find(objectsByFamily.first) == ignoreFamilies.end() ) {
          for(auto & objectsByType : objectsByFamily.second) {
            if(ignoreTypes.find(objectsByType.first) == ignoreTypes.end()) {
              for(auto & objectsByID : objectsByType.second) {
                if(ignoreIDs.find(objectsByID.first) == ignoreIDs.end()) {
                  Vec3f dist = ComputeVectorBetween(pose, objectsByID.second->GetPose());
                  dist.Abs();
                  if(dist < closestDist) {
                    closestDist = dist;
                    matchingObject = objectsByID.second;
                  }
                } // ignoreIDs
              }
            } // ignoreTypes
          }
        } // ignoreFamiles
      }
      
      if(matchingObject != nullptr && !(closestDist < distThreshold)) {
        matchingObject = nullptr;
      }
      return matchingObject;
    }
    
    Vision::ObservableObject* BlockWorld::FindClosestMatchingObject(const Vision::ObservableObject& object,
                                                                    const Vec3f& distThreshold,
                                                                    const Radians& angleThreshold)
    {
      Vision::ObservableObject* closestObject = nullptr;
      Vec3f closestDist(distThreshold);
      Radians closestAngle(angleThreshold);
      
      for(auto & objectsByFamily : _existingObjects) {
        for(auto & objectsByType : objectsByFamily.second) {
          for(auto & objectsByID : objectsByType.second) {
            Vision::ObservableObject* candidateObject = objectsByID.second;
            
            if(candidateObject->GetID() != object.GetID())
            {
              // Check to see if this candidate matches (has same type and is in roughly
              // the same pose as) the given object. If so, update the thresholds
              // so that we only find matching objects even closer than this one
              // from now on.
              Vec3f Tdiff;
              Radians angleDiff;
              if(candidateObject->IsSameAs(object, closestDist, closestAngle, Tdiff, angleDiff)) {
                closestObject = candidateObject;
                closestDist = Tdiff.GetAbs();
                closestAngle = angleDiff.getAbsoluteVal();
              }
            } // IF not object on bottom (by ID)
          }
        }
      }
      
      return closestObject;
    }
    
    void BlockWorld::ClearObjectsByFamily(const ObjectFamily family)
    {
      if(_canDeleteObjects) {
        ObjectsMapByFamily_t::iterator objectsWithFamily = _existingObjects.find(family);
        if(objectsWithFamily != _existingObjects.end()) {
          for(auto & objectsByType : objectsWithFamily->second) {
            for(auto & objectsByID : objectsByType.second) {
              ClearObjectHelper(objectsByID.second);
            }
          }
          
          _existingObjects.erase(family);
        }
      } else {
        PRINT_NAMED_WARNING("BlockWorld.ClearObjectsByFamily",
                            "Will not delete family %d objects because object deletion is disabled.\n",
                            family.GetValue());
      }
    }
    
    void BlockWorld::ClearObjectsByType(const ObjectType type)
    {
      if(_canDeleteObjects) {
        for(auto & objectsByFamily : _existingObjects) {
          ObjectsMapByType_t::iterator objectsWithType = objectsByFamily.second.find(type);
          if(objectsWithType != objectsByFamily.second.end()) {
            for(auto & objectsByID : objectsWithType->second) {
              ClearObjectHelper(objectsByID.second);
            }
            
            objectsByFamily.second.erase(objectsWithType);
            
            // Types are unique.  No need to keep looking
            return;
          }
        }
      } else {
        PRINT_NAMED_WARNING("BlockWorld.ClearObjectsByType",
                            "Will not delete %s objects because object deletion is disabled.\n",
                            type.GetName().c_str());

      }
    } // ClearBlocksByType()
    

    bool BlockWorld::ClearObject(const ObjectID withID)
    {
      for(auto & objectsByFamily : _existingObjects) {
        for(auto & objectsByType : objectsByFamily.second) {
          auto objectWithIdIter = objectsByType.second.find(withID);
          if(objectWithIdIter != objectsByType.second.end()) {
            
            // Allow deletion of specific object ID iff deletion is enable OR if
            // this object is being deleted because it wasn't observed enought times
            if(_canDeleteObjects ||
               objectWithIdIter->second->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT)
            {
              // Remove the object from the world
              ClearObjectHelper(objectWithIdIter->second);
              objectsByType.second.erase(objectWithIdIter);
              
              // IDs are unique, so we can return as soon as the ID is found and cleared
              return true;
            } else {
              PRINT_NAMED_WARNING("BlockWorld.ClearObject",
                                  "Will not delete object %d because object deletion is disabled.\n",
                                  withID.GetValue());
              return false;
            }
          }
        }
      }
      
      // Never found the specified ID
      return false;
    } // ClearObject()
    
    
    
    BlockWorld::ObjectsMapByID_t::iterator BlockWorld::ClearObject(const ObjectsMapByID_t::iterator objIter,
                                                                   const ObjectType&   withType,
                                                                   const ObjectFamily& fromFamily)
    {
      Vision::ObservableObject* object = objIter->second;
      
      if(_canDeleteObjects || object->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT) {
        ClearObjectHelper(object);
        
        return _existingObjects[fromFamily][withType].erase(objIter);
      } else {
        PRINT_NAMED_WARNING("BlockWorld.ClearObject",
                            "Will not delete object %d because object deletion is disabled.\n",
                            object->GetID().GetValue());
        auto retIter(objIter);
        return ++retIter;
      }
      
    }
    
    void BlockWorld::ClearObject(Vision::ObservableObject* object,
                                 const ObjectType&   withType,
                                 const ObjectFamily& fromFamily)
    {
      if(_canDeleteObjects || object->GetNumTimesObserved() < MIN_TIMES_TO_OBSERVE_OBJECT) {
        ObjectID objID = object->GetID();
        ClearObjectHelper(object);
        
        // Actually erase the object from blockWorld's container of
        // existing objects
        _existingObjects[fromFamily][withType].erase(objID);
      } else {
        PRINT_NAMED_WARNING("BlockWorld.ClearObject",
                            "Will not delete object %d because object deletion is disabled.\n",
                            object->GetID().GetValue());
      }
    }
    
    
    bool BlockWorld::SelectObject(const ObjectID objectID)
    {
      ActionableObject* newSelection = dynamic_cast<ActionableObject*>(GetObjectByID(objectID));
      
      if(newSelection != nullptr) {
        if(_selectedObject.IsSet()) {
          // Unselect current object of interest, if it still exists (Note that it may just get
          // reselected here, but I don't think we care.)
          // Mark new object of interest as selected so it will draw differently
          ActionableObject* oldSelection = dynamic_cast<ActionableObject*>(GetObjectByID(_selectedObject));
          if(oldSelection != nullptr) {
            oldSelection->SetSelected(false);
          }
        }
        
        newSelection->SetSelected(true);
        _selectedObject = objectID;
        PRINT_STREAM_INFO("BlockWorld.SelectObject", "Selected Object with ID=" << objectID.GetValue());
        
        return true;
      } else {
        PRINT_STREAM_WARNING("BlockWorld.SelectObject.InvalidID",
                            "Object with ID=" << objectID.GetValue() << " not found. Not updating selected object.");
        return false;
      }
    } // SelectObject()
    
    void BlockWorld::CycleSelectedObject()
    {
      if(_selectedObject.IsSet()) {
        // Unselect current object of interest, if it still exists (Note that it may just get
        // reselected here, but I don't think we care.)
        // Mark new object of interest as selected so it will draw differently
        ActionableObject* object = dynamic_cast<ActionableObject*>(GetObjectByID(_selectedObject));
        if(object != nullptr) {
          object->SetSelected(false);
        }
      }
      
      bool currSelectedObjectFound = false;
      bool newSelectedObjectSet = false;
      
      // Iterate through all the objects
      auto const & allObjects = GetAllExistingObjects();
      for(auto const & objectsByFamily : allObjects)
      {
        // Markerless objects are not Actionable, so ignore them for selection
        if(objectsByFamily.first != ObjectFamily::MARKERLESS_OBJECTS)
        {
          for (auto const & objectsByType : objectsByFamily.second){
            
            //PRINT_INFO("currType: %d\n", blockType.first);
            for (auto const & objectsByID : objectsByType.second) {
              
              ActionableObject* object = dynamic_cast<ActionableObject*>(objectsByID.second);
              if(object != nullptr && object->HasPreActionPoses() && !object->IsBeingCarried() &&
                 object->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT)
              {
                //PRINT_INFO("currID: %d\n", block.first);
                if (currSelectedObjectFound) {
                  // Current block of interest has been found.
                  // Set the new block of interest to the next block in the list.
                  _selectedObject = object->GetID();
                  newSelectedObjectSet = true;
                  //PRINT_INFO("new block found: id %d  type %d\n", block.first, blockType.first);
                  break;
                } else if (object->GetID() == _selectedObject) {
                  currSelectedObjectFound = true;
                  //PRINT_INFO("curr block found: id %d  type %d\n", block.first, blockType.first);
                }
              }
            } // for each ID
            
            if (newSelectedObjectSet) {
              break;
            }
            
          } // for each type
          
          if(newSelectedObjectSet) {
            break;
          }
        } // if family != MARKERLESS_OBJECTS
      } // for each family
      
      // If the current object of interest was found, but a new one was not set
      // it must have been the last block in the map. Set the new object of interest
      // to the first object in the map as long as it's not the same object.
      if (!currSelectedObjectFound || !newSelectedObjectSet) {
        
        // Find first object
        ObjectID firstObject; // initialized to un-set
        for(auto const & objectsByFamily : allObjects) {
          for (auto const & objectsByType : objectsByFamily.second) {
            for (auto const & objectsByID : objectsByType.second) {
              const ActionableObject* object = dynamic_cast<ActionableObject*>(objectsByID.second);
              if(object != nullptr && object->HasPreActionPoses() && !object->IsBeingCarried() &&
                object->GetNumTimesObserved() >= MIN_TIMES_TO_OBSERVE_OBJECT)
              {
                firstObject = objectsByID.first;
                break;
              }
            }
            if (firstObject.IsSet()) {
              break;
            }
          }
          
          if (firstObject.IsSet()) {
            break;
          }
        } // for each family
        
        
        if (firstObject == _selectedObject || !firstObject.IsSet()){
          //PRINT_INFO("Only one object in existence.");
        } else {
          //PRINT_INFO("Setting object of interest to first block\n");
          _selectedObject = firstObject;
        }
      }
      
      // Mark new object of interest as selected so it will draw differently
      ActionableObject* object = dynamic_cast<ActionableObject*>(GetObjectByID(_selectedObject));
      if (object != nullptr) {
        object->SetSelected(true);
        PRINT_STREAM_INFO("BlockWorld.CycleSelectedObject", "Object of interest: ID = " << _selectedObject.GetValue());
      } else {
        PRINT_STREAM_INFO("BlockWorld.CycleSelectedObject", "No object of interest found");
      }
  
    } // CycleSelectedObject()
    
    
    void BlockWorld::EnableDraw(bool on)
    {
      _enableDraw = on;
    }
    
    void BlockWorld::DrawObsMarkers() const
    {
      if (_enableDraw) {
        for (auto poseKeyMarkerMapAtTimestamp : _obsMarkers) {
          for (auto poseKeyMarkerMap : poseKeyMarkerMapAtTimestamp.second) {
            const Quad2f& q = poseKeyMarkerMap.second.GetImageCorners();
            f32 scaleF = 1.0f;
            switch(IMG_STREAM_RES) {
              case Vision::CAMERA_RES_CVGA:
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
      
      // (Re)Draw the selected object separately so we can get its pre-action poses
      if(GetSelectedObject().IsSet())
      {
        ActionableObject* selectedObject = dynamic_cast<ActionableObject*>(GetObjectByID(GetSelectedObject()));
        if(selectedObject == nullptr) {
          PRINT_NAMED_ERROR("BlockWorld.DrawAllObjects.NullSelectedObject",
                            "Selected object ID = %d, but it came back null.\n",
                            GetSelectedObject().GetValue());
        } else {
          if(selectedObject->IsSelected() == false) {
            PRINT_NAMED_WARNING("BlockWorld.DrawAllObjects.SelectionMisMatch",
                                "Object %d is selected in BlockWorld but does not have its "
                                "selection flag set.\n", GetSelectedObject().GetValue());
          }
          
          std::vector<std::pair<Quad2f,ObjectID> > obstacles;
          _robot->GetBlockWorld().GetObstacles(obstacles);
          selectedObject->VisualizePreActionPoses(obstacles, &_robot->GetPose());
        }
      } // if selected object is set
      
    } // DrawAllObjects()
    
  } // namespace Cozmo
} // namespace Anki
