/**
 * File: blockWorld.h
 *
 * Author: Andrew Stein (andrew)
 * Created: 10/1/2013
 *
 * Information on last revision to this file:
 *    $LastChangedDate$
 *    $LastChangedBy$
 *    $LastChangedRevision$
 *
 * Description: Defines a world of Cozmo robots and blocks.
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef ANKI_COZMO_BLOCKWORLD_H
#define ANKI_COZMO_BLOCKWORLD_H

#include <queue>
#include <map>
#include <vector>

#include "anki/common/types.h"
#include "anki/common/basestation/exceptions.h"

#include "anki/vision/basestation/observableObjectLibrary.h"

#include "anki/cozmo/basestation/block.h"
#include "anki/cozmo/basestation/mat.h"
#include "anki/cozmo/shared/cozmoTypes.h"


namespace Anki
{
  namespace Cozmo
  {
    // Forward declarations:
    class Robot;
    class RobotManager;
    class MessageHandler;
    
    namespace NamedColors {
      // Add some BlockWorld-specific named colors:
      extern const ColorRGBA EXECUTED_PATH              ;
      extern const ColorRGBA PREDOCKPOSE                ;
      extern const ColorRGBA PRERAMPPOSE                ;
      extern const ColorRGBA SELECTED_OBJECT            ;
      extern const ColorRGBA BLOCK_BOUNDING_QUAD        ;
      extern const ColorRGBA OBSERVED_QUAD              ;
      extern const ColorRGBA ROBOT_BOUNDING_QUAD        ;
      extern const ColorRGBA REPLAN_BLOCK_BOUNDING_QUAD ;
    }
    
    class BlockWorld
    {
    public:

      
      class ObjectFamily : public UniqueEnumeratedValue<int>
      {
      public:
        ObjectFamily();
        
        // Define new ObjectFamilies here:
        // (and be sure to instantiate them in the .cpp file)
        static const ObjectFamily MATS;
        static const ObjectFamily RAMPS;
        static const ObjectFamily BLOCKS;
        
      protected:
        static int UniqueFamilyCounter;
      }; // class ObjectFamily
      
      using ObjectsMapByID_t     = std::map<ObjectID, Vision::ObservableObject*>;
      using ObjectsMapByType_t   = std::map<ObjectType, ObjectsMapByID_t >;
      using ObjectsMapByFamily_t = std::map<ObjectFamily, ObjectsMapByType_t>;
      
      //static const unsigned int MaxRobots = 4;
      //static bool ZAxisPointsUp; // normally true, false for Webots

      BlockWorld();
      //static BlockWorld* getInstance();
      
      void Init(RobotManager* robotMgr);
      
      // Update the BlockWorld's state by processing all queued ObservedMarkers
      // and updating robots' poses and blocks' poses from them.
      void Update(uint32_t& numBlocksObserved);
      
      // Empties the queue of all observed markers
      void ClearAllObservedMarkers();
      
      Result QueueObservedMarker(const MessageVisionMarker& msg, Robot& robot);
      
      // Clear all existing objects in the world
      void ClearAllExistingObjects();
      
      // Clear all objects with the specified family
      void ClearObjectsByFamily(const BlockWorld::ObjectFamily family);
      
      // Clear all objects with the specified type
      void ClearObjectsByType(const ObjectType type);
      
      // Clear an object with a specific ID. Returns true if object with that ID
      // is found and cleared, false otherwise.
      bool ClearObject(const ObjectID withID);
      
      const Vision::ObservableObjectLibrary& GetObjectLibrary(ObjectFamily whichFamily) const;

      const ObjectsMapByFamily_t& GetAllExistingObjects() const;
      
      const ObjectsMapByType_t& GetExistingObjectsByFamily(const ObjectFamily whichFamily) const;
      
      // NOTE: Like IDs, object types are unique across objects so they can be
      //       used without specifying which family.
      const ObjectsMapByID_t& GetExistingObjectsByType(const ObjectType whichType) const;
      
      // Return a pointer to an object with the specified ID. If that object
      // does not exist, nullptr is returned.  Be sure to ALWAYS check
      // for the return being null!
      Vision::ObservableObject* GetObjectByID(const ObjectID objectID) const;
      
      // Same as above, but only searches a given family of objects
      Vision::ObservableObject* GetObjectByIDandFamily(const ObjectID objectID, const ObjectFamily inFamily) const;
            
      // Finds all blocks in the world whose centers are within the specified
      // heights off the ground (z dimension) and returns a vector of quads
      // of their outlines on the ground plane (z=0).  Can also pad the
      // bounding boxes by a specified amount. If ignoreIDs is not empty, then
      // bounding boxes of blocks with an ID present in the set will not be
      // returned. Analogous behavior for ignoreTypes/ignoreFamilies.
      // The last flag indicates whether objects being carried by any robot are
      // included in the results.
      void GetObjectBoundingBoxesXY(const f32 minHeight, const f32 maxHeight,
                                    const f32 padding,
                                    std::vector<Quad2f>& boundingBoxes,
                                    const std::set<ObjectFamily>& ignoreFamilies = {ObjectFamily::MATS},
                                    const std::set<ObjectType>& ignoreTypes = {{}},
                                    const std::set<ObjectID>& ignoreIDs = {{}},
                                    const bool ignoreCarriedObjects = true) const;
      
      // Returns true if any blocks were moved, added, or deleted on the
      // last update. Useful, for example, to know whether to update the
      // visualization.
      bool DidBlocksChange() const;
      
      ~BlockWorld();
      
      
      // === Draw functions ===
      void EnableDraw(bool on);

      // Visualize markers in image display
      void DrawObsMarkers() const;
      
      // Call every existing object's Visualize() method
      void DrawAllObjects() const;
      
    protected:
      
      // Typedefs / Aliases
      //using ObsMarkerContainer_t = std::multiset<Vision::ObservedMarker, Vision::ObservedMarker::Sorter()>;
      //using ObsMarkerList_t = std::list<Vision::ObservedMarker>;
      using PoseKeyObsMarkerMap_t = std::multimap<HistPoseKey, Vision::ObservedMarker>;
      using ObsMarkerListMap_t = std::map<TimeStamp_t, PoseKeyObsMarkerMap_t>;
      
      //
      // Member Methods
      //
      
      bool UpdateRobotPose(Robot* robot, PoseKeyObsMarkerMap_t& obsMarkers, const TimeStamp_t atTimestamp);
      
      size_t UpdateObjectPoses(const Robot* seenByRobot,
                               const Vision::ObservableObjectLibrary& objectsLibrary,
                               PoseKeyObsMarkerMap_t& obsMarkers,
                               ObjectsMapByType_t& existingObjects,
                               const TimeStamp_t atTimestamp);
      
      void FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                  const ObjectsMapByType_t& objectsExisting,
                                  std::vector<Vision::ObservableObject*>& overlappingExistingObjects) const;
      
      void CheckForUnobservedObjects(TimeStamp_t atTimestamp);
      
      // Helpers for actually inserting a new object into a new family using
      // its type and ID. Object's ID will be set if it isn't already.
      void AddNewObject(const ObjectFamily toFamily, Vision::ObservableObject* object);
      void AddNewObject(ObjectsMapByType_t& existingFamily, Vision::ObservableObject* object);
      
      //template<class ObjectType>
      void AddAndUpdateObjects(const std::vector<Vision::ObservableObject*>& objectsSeen,
                               ObjectsMapByType_t& objectsExisting,
                               const TimeStamp_t atTimestamp);
      
      // Remove all posekey-marker pairs from the map if marker is marked used
      void RemoveUsedMarkers(PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap);
      
      // Generates a list of ObservedMarker pointers that reference the actual ObservedMarkers
      // stored in poseKeyObsMarkerMap
      void GetObsMarkerList(const PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap,
                            std::list<Vision::ObservedMarker*>& lst);
      

      ObjectsMapByID_t::iterator ClearObject(ObjectsMapByID_t::iterator objectIter, ObjectsMapByID_t& inContainer);
      
      //
      // Member Variables
      //
      
      bool             isInitialized_;
      RobotManager*    robotMgr_;
      //MessageHandler*  msgHandler_;
      
      ObsMarkerListMap_t obsMarkers_;
      //std::map<Robot*, std::list<Vision::ObservedMarker*> > obsMarkersByRobot_;
      
      // Store all known observable objects (these are everything we know about,
      // separated by class of object, not necessarily what we've actually seen
      // yet, but what everything we are aware of)
      std::map<ObjectFamily, Vision::ObservableObjectLibrary> _objectLibrary;
      //Vision::ObservableObjectLibrary blockLibrary_;
      //Vision::ObservableObjectLibrary matLibrary_;
      //Vision::ObservableObjectLibrary rampLibrary_;
      
      
      // Store all observed objects, indexed first by Type, then by ID
      // NOTE: If a new ObjectsMap_t is added here, a pointer to it needs to
      //   be stored in allExistingObjects_ (below), initialized in the
      //   BlockWorld cosntructor.
      ObjectsMapByFamily_t _existingObjects;
      // ObjectsMapByID_t _existingObjectsByID; TODO: flat list for faster finds?
      
      //ObjectsMap_t existingBlocks_;
      //ObjectsMap_t existingMatPieces_;
      //ObjectsMap_t existingRamps_;
      
      // An array storing pointers to all the ObjectsMap_t's above so that we
      // we can easily loop over all types of objects.
      //std::array<ObjectsMap_t*, 3> allExistingObjects_;
      
      bool didObjectsChange_;
      
      static const Vision::ObservableObjectLibrary EmptyObjectLibrary;
      
      static const ObjectsMapByID_t    EmptyObjectMapByID;
      static const ObjectsMapByType_t  EmptyObjectMapByType;
      
      // Global counter for assigning IDs to objects as they are created.
      // This means every object in the world has a unique ObjectID!
      //ObjectID globalIDCounter;
      
      // For allowing the calling of VizManager draw functions
      bool enableDraw_;
      
    }; // class BlockWorld

    
    inline const Vision::ObservableObjectLibrary& BlockWorld::GetObjectLibrary(ObjectFamily whichFamily) const
    {
      auto objectsWithFamilyIter = _objectLibrary.find(whichFamily);
      if(objectsWithFamilyIter != _objectLibrary.end()) {
        return objectsWithFamilyIter->second;
      } else {
        return BlockWorld::EmptyObjectLibrary;
      }
    }
    
    inline const BlockWorld::ObjectsMapByFamily_t& BlockWorld::GetAllExistingObjects() const
    {
      return _existingObjects;
    }
    
    inline const BlockWorld::ObjectsMapByType_t& BlockWorld::GetExistingObjectsByFamily(const ObjectFamily whichFamily) const
    {
      auto objectsWithFamilyIter = _existingObjects.find(whichFamily);
      if(objectsWithFamilyIter != _existingObjects.end()) {
        return objectsWithFamilyIter->second;
      } else {
        return BlockWorld::EmptyObjectMapByType;
      }
    }
    
    inline const BlockWorld::ObjectsMapByID_t& BlockWorld::GetExistingObjectsByType(const ObjectType whichType) const
    {
      for(auto & objectsByFamily : _existingObjects) {
        auto objectsWithType = objectsByFamily.second.find(whichType);
        if(objectsWithType != objectsByFamily.second.end()) {
          return objectsWithType->second;
        }
      }
      
      // Type not found!
      return BlockWorld::EmptyObjectMapByID;
    }
    
    inline Vision::ObservableObject* BlockWorld::GetObjectByID(const ObjectID objectID) const
    {
      // TODO: Maintain a separate map indexed directly by ID so we don't have to loop over the outer maps?
      
      for(auto & objectsByFamily : _existingObjects) {
        for(auto & objectsByType : objectsByFamily.second) {
          auto objectsByIdIter = objectsByType.second.find(objectID);
          if(objectsByIdIter != objectsByType.second.end()) {
            return objectsByIdIter->second;
          }
        }
      }
      
      // ID not found!
      return nullptr;
    }
    
    inline Vision::ObservableObject* BlockWorld::GetObjectByIDandFamily(const ObjectID objectID, const ObjectFamily inFamily) const
    {
      // TODO: Maintain a separate map indexed directly by ID so we don't have to loop over the outer maps?
      
      for(auto & objectsByType : GetExistingObjectsByFamily(inFamily)) {
        auto objectsByIdIter = objectsByType.second.find(objectID);
        if(objectsByIdIter != objectsByType.second.end()) {
          return objectsByIdIter->second;
        }
      }
      
      // ID not found!
      return nullptr;
    }
    
    inline void BlockWorld::AddNewObject(ObjectsMapByType_t& existingFamily, Vision::ObservableObject* object)
    {
      if(!object->GetID().IsSet()) {
        object->SetID();
      }
      
      existingFamily[object->GetType()][object->GetID()] = object;
    }
    
    inline void BlockWorld::AddNewObject(const ObjectFamily toFamily, Vision::ObservableObject* object)
    {
      AddNewObject(_existingObjects[toFamily], object);
    }

    
  } // namespace Cozmo
} // namespace Anki



#endif // ANKI_COZMO_BLOCKWORLD_H