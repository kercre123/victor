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

//#include "anki/messaging/basestation/messagingInterface.h"

namespace Anki
{
  namespace Cozmo
  {
    // Forward declarations:
    class Robot;
    
    class BlockWorld
    {
    public:
      static const unsigned int MaxBlockTypes = 255;
      static const unsigned int MaxRobots = 4;
      static bool ZAxisPointsUp; // normally true, false for Webots
      
      // Constructors:
      BlockWorld(); //MessagingInterface* msgInterface);
      ~BlockWorld();
      
      void   addRobot(const u32 withID);
      Robot& getRobot(const u32 ID);
      size_t getNumRobots() const;
      
      const std::vector<Block>& get_blocks(const BlockType ofType) const;
      
      void queueMessage(const u8 *);
      
      void update(void);
      
      void commandRobotToDock(const size_t whichRobot);
      
    protected:
      
      std::queue<const u8 *> messages;
      
      // This can point to either a real or a simulated messaging interface
      //MessagingInterface* msgInterface;
      
      // Store all the blocks in the world, with a reserved slot for
      // each type of block, and then a list of pointers to each block
      // of that type we've actually seen.
      // TODO: inner vector could actual blocks instead of pointers?
      //typedef std::map<Block::Type, std::vector<Block*> > BlockList;
      typedef std::vector< std::vector<Block> > BlockList_type;
      BlockList_type blocks;
      
      // Store all the robots in the world:
      // TODO: should RobotList be a fixed-length array with MaxRobots entries?
      //typedef std::map<u8, Robot*> RobotList_type;
      typedef std::vector<Robot> RobotList_type;
      RobotList_type robots;
            
    }; // class BlockWorld

    inline size_t BlockWorld::getNumRobots() const
    { return this->robots.size(); }
    
    inline Robot& BlockWorld::getRobot(const u32 ID)
    {
      CORETECH_ASSERT(ID < this->getNumRobots());
      return this->robots[ID];
    }
    
    inline const std::vector<Block>& BlockWorld::get_blocks(const BlockType ofType) const
    {
      CORETECH_ASSERT(ofType >= 0 && ofType < this->blocks.size());
      return this->blocks[ofType];
    }
    
  } // namespace Cozmo
} // namespace Anki



#endif // __Products_Cozmo__blockWorld__