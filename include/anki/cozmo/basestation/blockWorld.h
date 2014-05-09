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
      
      typedef std::map<ObjectID_t, Vision::ObservableObject*> ObjectsMapByID_t;
      typedef std::map<ObjectType_t, ObjectsMapByID_t > ObjectsMap_t;
      
      //static const unsigned int MaxRobots = 4;
      //static bool ZAxisPointsUp; // normally true, false for Webots

      BlockWorld();
      //static BlockWorld* getInstance();
      
      void Init(RobotManager* robotMgr);
      
      void Update(void);
      
      bool UpdateRobotPose(Robot* robot);
      
      uint32_t UpdateBlockPoses();
      
      // Empties the queue of observed markers, returning the number that were
      // still in it before being cleared.
      uint32_t ClearObservedMarkers();
      
      void QueueObservedMarker(const Vision::ObservedMarker& marker);
                               //Robot* seenByRobot);
      
      void CommandRobotToDock(const RobotID_t whichRobot,
                              const Block&    whichBlock);
      
      // Clears all existing blocks in the world
      void ClearAllExistingBlocks();
      
      const Vision::ObservableObjectLibrary& GetBlockLibrary() const;
      const ObjectsMapByID_t& GetExistingBlocks(const ObjectType_t blockType) const;
      const ObjectsMap_t& GetAllExistingBlocks() const {return existingBlocks_;}
      const Vision::ObservableObjectLibrary& GetMatLibrary() const;
      
      const Vision::ObservableObject* GetObservableObjectByID(const ObjectID_t objectID) const;
      
      ~BlockWorld();
      
    protected:
      
      //static BlockWorld* singletonInstance_;
      
      //BlockWorld(); // protected constructor for singleton
      
      RobotManager*    robotMgr_;
      //MessageHandler*  msgHandler_;
      
      std::list<Vision::ObservedMarker> obsMarkers_;
      //std::map<Robot*, std::list<Vision::ObservedMarker*> > obsMarkersByRobot_;
      
      // Store all known observable objects (these are everything we know about,
      // separated by type of object, not necessarily what we've actually seen
      // yet)
      //Vision::ObservableObjectLibrary objectLibrary_; // not separated by type?
      Vision::ObservableObjectLibrary blockLibrary_;
      Vision::ObservableObjectLibrary matLibrary_;
      Vision::ObservableObjectLibrary otherObjectsLibrary_;
      
      // Store all observed objects, indexed first by Type, then by ID
      ObjectsMap_t existingBlocks_;
      ObjectsMap_t existingMatPieces_;
      
      static const ObjectsMapByID_t EmptyObjectMap;
      
      
      void FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                  const ObjectsMap_t& objectsExisting,
                                  std::vector<Vision::ObservableObject*>& overlappingExistingObjects) const;
      
      
      //template<class ObjectType>
      void AddAndUpdateObjects(const std::vector<Vision::ObservableObject*> objectsSeen,
                                   ObjectsMap_t& objectsExisting);
      
      // Global counter for assigning IDs to objects as they are created.
      // This means every object in the world has a unique ObjectID!
      ObjectID_t globalIDCounter;
      
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
    
    inline const Vision::ObservableObject* BlockWorld::GetObservableObjectByID(const ObjectID_t objectID) const
    {
      for (auto const & block : existingBlocks_) {
        auto const & objectByIdMap = block.second;
        auto objectIt = objectByIdMap.find(objectID);
        if (objectIt != objectByIdMap.end()) {
          return objectIt->second;
        }
      }
      return nullptr;
    }
    
  } // namespace Cozmo
} // namespace Anki



#endif // __Products_Cozmo__blockWorld__