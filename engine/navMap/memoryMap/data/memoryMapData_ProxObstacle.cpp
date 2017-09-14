/**
 * File: memoryMapData_ProxObstacle.h
 *
 * Author: Michael Willett
 * Date:   2017-07-31
 *
 * Description: Data for obstacle quads.
 *
 * Copyright: Anki, Inc. 2017
 **/
 
#include "memoryMapData_ProxObstacle.h"

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData_ProxObstacle::MemoryMapData_ProxObstacle()
: MemoryMapData(MemoryMapTypes::EContentType::ObstacleProx)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData* MemoryMapData_ProxObstacle::Clone() const
{
  return new MemoryMapData_ProxObstacle(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData_ProxObstacle::Equals(const MemoryMapData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const MemoryMapData_ProxObstacle* castPtr = static_cast<const MemoryMapData_ProxObstacle*>( other );
  const bool near = IsNearlyEqual( directionality, castPtr->directionality );
  return near;
}

} // namespace Cozmo
} // namespace Anki
