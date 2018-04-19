/**
 * File: memoryMapData_ProxObstacle.cpp
 *
 * Author: Michael Willett
 * Date:   2017-07-31
 *
 * Description: Data for obstacle quads (explored and unexplored).
 *
 * Copyright: Anki, Inc. 2017
 **/
 
#include "memoryMapData_ProxObstacle.h"
#include "clad/types/memoryMap.h"
#include "coretech/common/engine/math/point_impl.h"

namespace Anki {
namespace Cozmo {
  
namespace {
  const float kRotationTolerance = 1e-6f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData_ProxObstacle::MemoryMapData_ProxObstacle(ExploredType explored, const Pose2d& pose, TimeStamp_t t)
: MemoryMapData( MemoryMapTypes::EContentType::ObstacleProx,  t, true)
, _pose(pose)
, _explored(explored)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapTypes::MemoryMapDataPtr MemoryMapData_ProxObstacle::Clone() const
{
  return MemoryMapDataWrapper<MemoryMapData_ProxObstacle>(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData_ProxObstacle::Equals(const MemoryMapData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const MemoryMapData_ProxObstacle* castPtr = static_cast<const MemoryMapData_ProxObstacle*>( other );
  const bool isNearLocation = IsNearlyEqual( _pose.GetTranslation(), castPtr->_pose.GetTranslation() );
  const bool isNearRotation = _pose.GetAngle().IsNear( castPtr->_pose.GetAngle(), kRotationTolerance );
  const bool exploredSame = _explored == castPtr->_explored;
  return isNearLocation && isNearRotation && exploredSame;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ExternalInterface::ENodeContentTypeEnum MemoryMapData_ProxObstacle::GetExternalContentType() const
{
  return _explored ? ExternalInterface::ENodeContentTypeEnum::ObstacleProxExplored : ExternalInterface::ENodeContentTypeEnum::ObstacleProx;
}

} // namespace Cozmo
} // namespace Anki
