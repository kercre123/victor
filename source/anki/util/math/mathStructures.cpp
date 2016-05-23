/**
 * File: mathStructures.cpp
 *
 * Author: raul
 * Created: 04/15/15
 *
 * Description: Abstracting math structures from their implementation in case we need to change libraries.
 *
 * Copyright: Anki, Inc.
 *
**/

#include "mathStructures.h"

namespace Anki {
namespace Util {
namespace Math {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Additional functionality
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

void AABB2ExpandToContain(AABB2& aabb2, const Vector2& point)
{

  if ( point.x > aabb2.max.x ) {
    aabb2.max.x = point.x;
  } else if ( point.x < aabb2.min.x ) {
    aabb2.min.x = point.x;
  }
  
  if ( point.y > aabb2.max.y ) {
    aabb2.max.y = point.y;
  } else if ( point.y < aabb2.min.y ) {
    aabb2.min.y = point.y;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

bool AABB2CollideFull(const AABB2& one, const AABB2& other, const float epsilon)
{
  if ( one.min.x >= (other.max.x - epsilon) ||
       one.max.x <= (other.min.x + epsilon) ||
       one.min.y >= (other.max.y - epsilon) ||
       one.max.y <= (other.min.y + epsilon) )
  {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

bool AABB2CollideOrTouch(const AABB2& one, const AABB2& other, const float epsilon)
{
  if ( one.min.x > (other.max.x - epsilon) ||
       one.max.x < (other.min.x + epsilon) ||
       one.min.y > (other.max.y - epsilon) ||
       one.max.y < (other.min.y + epsilon) )
  {
    return false;
  }
  return true;
}

} // namespace
} // namespace
} // namespace

