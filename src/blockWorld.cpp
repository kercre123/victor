
#include "anki/cozmo/blockWorld.h"
#include "anki/cozmo/block.h"
#include "anki/cozmo/robot.h"

namespace Anki
{
  namespace Cozmo
  {
    
    BlockWorld::BlockWorld( void )
    : blocks(MaxBlockTypes)
    {
      
    }
    
    BlockWorld::~BlockWorld(void)
    {
      // Delete all instantiated blocks:
      
      // For each vector of block types
      for( const std::vector<Block*>& typeList : this->blocks )
      {
        // For each block in the vector of this type of blocks"
        for( Block* block : typeList )
        {
          if(block != NULL)
          {
            delete block;
          }
        } // for each block
      } // for each block type
      
      // Delete all instantiated robots:
      for( Robot* robot : this->robots )
      {
        if(robot != NULL)
        {
          delete robot;
        }
      }
      
    } // BlockWorld Destructor
    
    void BlockWorld::addRobot(void)
    {
      robots.push_back(new Robot());
    } // addRobot()
    
  } // namespace Cozmo
} // namespace Anki
