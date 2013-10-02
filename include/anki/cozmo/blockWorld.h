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

#include <vector>

namespace Anki
{
  namespace Cozmo
  {
    // Forward declarations:
    class Block;
    class Robot;
    
    class BlockWorld
    {
    public:
      const unsigned int MaxBlockTypes = 255;
      
      // Constructors:
      BlockWorld();
      ~BlockWorld();
      
      void addRobot(void);
      
    protected:
      
      // Store all the blocks in the world, with a reserved slot for
      // each type of block, and then a list of pointers to each block
      // of that type we've actually seen.
      //typedef std::map<Block::Type, std::vector<Block*> > BlockList;
      typedef std::vector< std::vector<Block*> > BlockList_type;
      BlockList_type blocks;
      
      // Store all the robots in the world:
      typedef std::vector<Robot*> RobotList_type;
      RobotList_type robots;
      
    }; // class BlockWorld
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // __Products_Cozmo__blockWorld__