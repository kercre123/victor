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

#ifndef __Products_Cozmo__blockWorld__
#define __Products_Cozmo__blockWorld__

#include <queue>
#include <map>
#include <vector>

#include "anki/common/types.h"
#include "anki/common/basestation/exceptions.h"

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
    
    class BlockWorld
    {
    public:
      
      using ObjectsMapByID_t = std::map<ObjectID_t, Vision::ObservableObject*>;
      using ObjectsMap_t     = std::map<ObjectType_t, ObjectsMapByID_t >;
      
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
      
      void CommandRobotToDock(const RobotID_t whichRobot,
                              Block&    whichBlock);
      
      // Clears all existing blocks in the world
      void ClearAllExistingBlocks();
      
      // Clear all blocks with the specified type
      void ClearBlocksByType(const ObjectType_t type);
      
      // Clear a block with a specific ID. Returns true if block with that ID
      // is found and cleared, false otherwise.
      bool ClearBlock(const ObjectID_t withID);
      
      const Vision::ObservableObjectLibrary& GetBlockLibrary() const;
      const ObjectsMapByID_t& GetExistingBlocks(const ObjectType_t blockType) const;
      const ObjectsMap_t& GetAllExistingBlocks() const {return existingBlocks_;}
      const Vision::ObservableObjectLibrary& GetMatLibrary() const;
      
      // Return a pointer to a block with the specified ID. If that block
      // does not exist, nullptr is returned.  Be sure to ALWAYS check
      // for the return being null!
      Block* GetBlockByID(const ObjectID_t objectID) const;
      
      // Finds all blocks in the world whose centers are within the specified
      // heights off the ground (z dimension) and returns a vector of quads
      // of their outlines on the ground plane (z=0).  Can also pad the
      // bounding boxes by a specified amount. If ignoreIDs is not empty, then
      // bounding boxes of blocks with an ID present in the set will not be
      // returned. Analogous behavior for ignoreTypes.  The last flag indicates
      // whether blocks being carried by any robot are included in the results.
      void GetBlockBoundingBoxesXY(const f32 minHeight, const f32 maxHeight,
                                   const f32 padding,
                                   std::vector<Quad2f>& boundingBoxes,
                                   const std::set<ObjectType_t>& ignoreTypes = std::set<ObjectType_t>(),
                                   const std::set<ObjectID_t>& ignoreIDs = std::set<ObjectID_t>(),
                                   const bool ignoreCarriedBlocks = true) const;
      
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
      
      
      // Methods
      
      //BlockWorld(); // protected constructor for singleton

      bool UpdateRobotPose(Robot* robot, PoseKeyObsMarkerMap_t& obsMarkers, const TimeStamp_t atTimestamp);
      
      size_t UpdateObjectPoses(const Vision::ObservableObjectLibrary& objectsLibrary,
                               PoseKeyObsMarkerMap_t& obsMarkers,
                               ObjectsMap_t& existingObjects,
                               const TimeStamp_t atTimestamp);
      
      void FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                  const ObjectsMap_t& objectsExisting,
                                  std::vector<Vision::ObservableObject*>& overlappingExistingObjects) const;
      
      
      //template<class ObjectType>
      void AddAndUpdateObjects(const std::vector<Vision::ObservableObject*>& objectsSeen,
                               ObjectsMap_t& objectsExisting,
                               const TimeStamp_t atTimestamp);
      
      // Remove all posekey-marker pairs from the map if marker is marked used
      void RemoveUsedMarkers(PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap);
      
      // Generates a list of ObservedMarker pointers that reference the actual ObservedMarkers
      // stored in poseKeyObsMarkerMap
      void GetObsMarkerList(const PoseKeyObsMarkerMap_t& poseKeyObsMarkerMap,
                            std::list<Vision::ObservedMarker*>& lst);
      

      // Member Variables
      
      //static BlockWorld* singletonInstance_;
      
      bool             isInitialized_;
      RobotManager*    robotMgr_;
      //MessageHandler*  msgHandler_;
      
      ObsMarkerListMap_t obsMarkers_;
      //std::map<Robot*, std::list<Vision::ObservedMarker*> > obsMarkersByRobot_;
      
      // Store all known observable objects (these are everything we know about,
      // separated by type of object, not necessarily what we've actually seen
      // yet)
      //Vision::ObservableObjectLibrary objectLibrary_; // not separated by type?
      Vision::ObservableObjectLibrary blockLibrary_;
      Vision::ObservableObjectLibrary matLibrary_;
      Vision::ObservableObjectLibrary rampLibrary_;
      
      // Store all observed objects, indexed first by Type, then by ID
      // NOTE: If a new ObjectsMap_t is added here, a pointer to it needs to
      //   be stored in allExistingObjects_ (below), initialized in the
      //   BlockWorld cosntructor.
      ObjectsMap_t existingBlocks_;
      ObjectsMap_t existingMatPieces_;
      ObjectsMap_t existingRamps_;
      
      // An array storing pointers to all the ObjectsMap_t's above so that we
      // we can easily loop over all types of objects.
      std::array<ObjectsMap_t*, 3> allExistingObjects_;
      
      bool didObjectsChange_;
      
      static const ObjectsMapByID_t EmptyObjectMap;
      
      // Global counter for assigning IDs to objects as they are created.
      // This means every object in the world has a unique ObjectID!
      ObjectID_t globalIDCounter;
      
      // For allowing the calling of VizManager draw functions
      bool enableDraw_;
      
    }; // class BlockWorld

    /*
    inline BlockWorld* BlockWorld::getInstance()
    {
      // Instantiate singleton instance if not done already
      if(singletonInstance_ == 0) {
        singletonInstance_ = new BlockWorld();
      }
      return singletonInstance_;
    }
     */
    
    inline const BlockWorld::ObjectsMapByID_t& BlockWorld::GetExistingBlocks(const ObjectType_t blockType) const
    {
      auto blocksWithID = existingBlocks_.find(blockType);
      if(blocksWithID != existingBlocks_.end()) {
        return blocksWithID->second;
      } else {
        return BlockWorld::EmptyObjectMap;
      }
    }
    
    inline const Vision::ObservableObjectLibrary& BlockWorld::GetBlockLibrary() const {
      return blockLibrary_;
    }
    
    inline const Vision::ObservableObjectLibrary& BlockWorld::GetMatLibrary() const {
      return matLibrary_;
    }
    
    inline Block* BlockWorld::GetBlockByID(const ObjectID_t objectID) const
    {
      for (auto const & block : existingBlocks_) {
        auto const & objectByIdMap = block.second;
        auto objectIt = objectByIdMap.find(objectID);
        if (objectIt != objectByIdMap.end()) {
          Block* block = dynamic_cast<Block*>(objectIt->second);
          CORETECH_ASSERT(block != nullptr);
          return block;
        }
      }
      return nullptr;
    }
    
  } // namespace Cozmo
} // namespace Anki



#endif // __Products_Cozmo__blockWorld__