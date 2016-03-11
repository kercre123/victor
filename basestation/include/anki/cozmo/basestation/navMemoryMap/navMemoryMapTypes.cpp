/**
 * File: navMemoryMapTypes.cpp
 *
 * Author: Raul
 * Date:   01/11/2016
 *
 * Description: Type definitions for the navMemoryMap.
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "navMemoryMapTypes.h"

namespace Anki {
namespace Cozmo {
namespace NavMemoryMapTypes {

bool ExpectsAdditionalData(EContentType type)
{
  // using switch to force at compilation type to decide on new types
  switch(type)
  {
    case EContentType::Unknown:
    case EContentType::ClearOfObstacle:
    case EContentType::ClearOfCliff:
    case EContentType::ObstacleCube:
    case EContentType::ObstacleUnrecognized:
    case EContentType::InterestingEdge:
    {
      return false;
    }
  
    case EContentType::Cliff:
    {
      return true;
    }
  }
}

} // namespace
} // namespace
} // namespace
