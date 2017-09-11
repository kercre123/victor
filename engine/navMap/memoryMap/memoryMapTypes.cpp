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
#include "memoryMapTypes.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace MemoryMapTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ExpectsAdditionalData(EContentType type)
{
  DEV_ASSERT(type != EContentType::_Count, "MemoryMapTypes.ExpectsAdditionalData.UsingControlTypeIsNotAllowed");
  
  // using switch to force at compilation type to decide on new types
  switch(type)
  {
    case EContentType::Unknown:
    case EContentType::ClearOfObstacle:
    case EContentType::ClearOfCliff:
    case EContentType::ObstacleCube:
    case EContentType::ObstacleCubeRemoved:
    case EContentType::ObstacleCharger:
    case EContentType::ObstacleChargerRemoved:
    case EContentType::ObstacleUnrecognized:
    case EContentType::InterestingEdge:
    case EContentType::NotInterestingEdge:
    {
      return false;
    }
    case EContentType::Cliff:
    case EContentType::ObstacleProx:
    {
      return true;
    }
    
    case EContentType::_Count:
    {
      return false;
    }
  }
}

} // namespace
} // namespace
} // namespace
