/**
 * File: navMemoryMapQuadData_Cliff.h
 *
 * Author: Raul
 * Date:   08/02/2016
 *
 * Description: Data for Cliff quads.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "anki/cozmo/basestation/navMemoryMap/quadData/navMemoryMapQuadData_Cliff.h"

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMemoryMapQuadData_Cliff::NavMemoryMapQuadData_Cliff()
: INavMemoryMapQuadData(NavMemoryMapTypes::EContentType::Cliff)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INavMemoryMapQuadData* NavMemoryMapQuadData_Cliff::Clone() const
{
  return new NavMemoryMapQuadData_Cliff(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadData_Cliff::Equals(const INavMemoryMapQuadData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const NavMemoryMapQuadData_Cliff* castPtr = static_cast<const NavMemoryMapQuadData_Cliff*>( other );
  const bool near = IsNearlyEqual( directionality, castPtr->directionality );
  return near;
}

} // namespace Cozmo
} // namespace Anki
