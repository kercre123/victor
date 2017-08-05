/**
 * File: navMemoryMapQuadData_ProxObstacle.h
 *
 * Author: Michael Willett
 * Date:   2017-07-31
 *
 * Description: Data for obstacle quads.
 *
 * Copyright: Anki, Inc. 2017
 **/
 
#include "anki/cozmo/basestation/navMemoryMap/quadData/navMemoryMapQuadData_ProxObstacle.h"

#include "anki/common/basestation/math/point_impl.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMemoryMapQuadData_ProxObstacle::NavMemoryMapQuadData_ProxObstacle()
: INavMemoryMapQuadData(NavMemoryMapTypes::EContentType::ObstacleProx)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INavMemoryMapQuadData* NavMemoryMapQuadData_ProxObstacle::Clone() const
{
  return new NavMemoryMapQuadData_ProxObstacle(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadData_ProxObstacle::Equals(const INavMemoryMapQuadData* other) const
{
  if ( other == nullptr || other->type != type ) {
    return false;
  }

  const NavMemoryMapQuadData_ProxObstacle* castPtr = static_cast<const NavMemoryMapQuadData_ProxObstacle*>( other );
  const bool near = IsNearlyEqual( directionality, castPtr->directionality );
  return near;
}

} // namespace Cozmo
} // namespace Anki
