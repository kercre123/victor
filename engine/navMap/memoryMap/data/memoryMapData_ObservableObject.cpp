/**
 * File: memoryMapData_ObservableObject.cpp
 *
 * Author: Michael Willett
 * Date:   2017-07-31
 *
 * Description: Data for obstacle quads.
 *
 * Copyright: Anki, Inc. 2017
 **/
 
#include "memoryMapData_ObservableObject.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData_ObservableObject::MemoryMapData_ObservableObject(MemoryMapTypes::EContentType type, 
                                                               const ObjectID& i, 
                                                               const Poly2f& p,
                                                               TimeStamp_t t)
: MemoryMapData(type, t, true)
, id(i)
, boundingPoly(p)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMapData* MemoryMapData_ObservableObject::Clone() const
{
  return new MemoryMapData_ObservableObject(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData_ObservableObject::Equals(const MemoryMapData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const MemoryMapData_ObservableObject* castPtr = static_cast<const MemoryMapData_ObservableObject*>( other );
  const bool retv = (id == castPtr->id);
  return retv;
}

} // namespace Cozmo
} // namespace Anki
