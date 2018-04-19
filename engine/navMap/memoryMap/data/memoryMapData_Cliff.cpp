/**
 * File: memoryMapData_Cliff.h
 *
 * Author: Raul
 * Date:   08/02/2016
 *
 * Description: Data for Cliff quads.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "memoryMapData_Cliff.h"
#include "clad/types/memoryMap.h"
#include "coretech/common/engine/math/point_impl.h"

namespace {
  const float kRotationTolerance = 1e-6f;
}

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData_Cliff::MemoryMapData_Cliff(const Pose3d& cliffPose, TimeStamp_t t)
: MemoryMapData(MemoryMapTypes::EContentType::Cliff, t, true)
, pose(cliffPose)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapTypes::MemoryMapDataPtr MemoryMapData_Cliff::Clone() const
{
  return MemoryMapDataWrapper<MemoryMapData_Cliff>(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData_Cliff::Equals(const MemoryMapData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const MemoryMapData_Cliff* castPtr = static_cast<const MemoryMapData_Cliff*>( other );
  const bool isNearLocation = IsNearlyEqual( pose.GetTranslation(), castPtr->pose.GetTranslation() );
  const bool isNearRotation = IsNearlyEqual( pose.GetRotation(), castPtr->pose.GetRotation(), kRotationTolerance );
  
  return ( isNearLocation && isNearRotation );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ExternalInterface::ENodeContentTypeEnum MemoryMapData_Cliff::GetExternalContentType() const
{
  return ExternalInterface::ENodeContentTypeEnum::Cliff;
}

} // namespace Cozmo
} // namespace Anki
