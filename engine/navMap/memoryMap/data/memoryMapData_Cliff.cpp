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

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData_Cliff::MemoryMapData_Cliff()
: MemoryMapData(MemoryMapTypes::EContentType::Cliff)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData* MemoryMapData_Cliff::Clone() const
{
  return new MemoryMapData_Cliff(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData_Cliff::Equals(const MemoryMapData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const MemoryMapData_Cliff* castPtr = static_cast<const MemoryMapData_Cliff*>( other );
  const bool near = IsNearlyEqual( directionality, castPtr->directionality );
  return near;
}

} // namespace Cozmo
} // namespace Anki
