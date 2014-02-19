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
#include "anki/cozmo/basestation/robot.h"


namespace Anki
{
  namespace Cozmo
  {
    // Forward declarations:
    class Robot;    
    class MessageHandler;
    
    class BlockWorld
    {
    public:
      static const unsigned int MaxRobots = 4;
      static bool ZAxisPointsUp; // normally true, false for Webots

      static BlockWorld* getInstance();
      
      const std::vector<Block>& get_blocks(const BlockID_t ofType) const;
      
      void Update(void);
      
      bool UpdateRobotPose(Robot* robot);
      
      uint32_t UpdateBlockPoses();
      
      void QueueObservedMarker(const Vision::ObservedMarker& marker);
                               //Robot* seenByRobot);
      
      void CommandRobotToDock(const RobotID_t whichRobot,
                              const Block&    whichBlock);
      
      ~BlockWorld();
      
    protected:
      
      static BlockWorld* singletonInstance_;
      
      BlockWorld(); // protected constructor for singleton
      
      RobotManager*    robotMgr_;
      MessageHandler*  msgHandler_;
      
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
      typedef std::map<ObjectType_t, std::map<ObjectID_t, Vision::ObservableObject*> > ObjectsMap_t;
      ObjectsMap_t existingBlocks_;
      ObjectsMap_t existingMatPieces_;
      
      // Store all the blocks in the world, with a reserved slot for
      // each type of block, and then a list of pointers to each block
      // of that type we've actually seen.
      std::vector< std::vector<Block> > blocks;
      
      
      void FindOverlappingObjects(const Vision::ObservableObject* objectSeen,
                                  const ObjectsMap_t& objectsExisting,
                                  std::vector<Vision::ObservableObject*>& overlappingExistingObjects) const;
      
      
      //template<class ObjectType>
      void AddAndUpdateObjects(const std::vector<Vision::ObservableObject*> objectsSeen,
                                   ObjectsMap_t& objectsExisting);
      
    }; // class BlockWorld

    inline BlockWorld* BlockWorld::getInstance()
    {
      // Instantiate singleton instance if not done already
      if(singletonInstance_ == 0) {
        singletonInstance_ = new BlockWorld();
      }
      return singletonInstance_;
    }
    
    inline const std::vector<Block>& BlockWorld::get_blocks(const BlockID_t ofType) const
    {
      CORETECH_ASSERT(ofType >= 0 && ofType < this->blocks.size());
      return this->blocks[ofType];
    }
    
  } // namespace Cozmo
} // namespace Anki



#endif // __Products_Cozmo__blockWorld__