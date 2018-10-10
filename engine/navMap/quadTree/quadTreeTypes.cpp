/**
 * File: quadTreeTypes.cpp
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Type definitions for QuadTree.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "quadTreeTypes.h"

#include "engine/navMap/memoryMap/data/memoryMapData.h"

#include "coretech/common/engine/exceptions.h"


namespace Anki {
namespace Vector {
namespace QuadTreeTypes {
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NodeContent::NodeContent(const MemoryMapData& m)
: data(m.Clone()) {}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NodeContent::NodeContent(MemoryMapTypes::MemoryMapDataPtr m)
: data(m) {}
      
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator==(const NodeContent& other) const
{
  return (data == other.data);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator!=(const NodeContent& other) const
{
  const bool ret = !(this->operator==(other));
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* EDirectionToString(EDirection dir)
{
  switch (dir) {
    case QuadTreeTypes::EDirection::North:   { return "N"; };
    case QuadTreeTypes::EDirection::East:    { return "E"; };
    case QuadTreeTypes::EDirection::South:   { return "S"; };
    case QuadTreeTypes::EDirection::West:    { return "W"; };
  }
  return "Error";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vec3f EDirectionToNormalVec3f(EDirection dir)
{
  switch (dir) {
    case QuadTreeTypes::EDirection::North:   { return Vec3f{ 1.0f,  0.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::East:    { return Vec3f{ 0.0f, -1.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::South:   { return Vec3f{-1.0f,  0.0f, 0.0f}; };
    case QuadTreeTypes::EDirection::West:    { return Vec3f{ 0.0f,  1.0f, 0.0f}; };
  }
  
  DEV_ASSERT(!"Invalid direction", "EDirectionToNormalVec3f.InvalidDirection");
  return Vec3f{0.0f, 0.0f, 0.0f};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vec2f Quadrant2Vec(EQuadrant dir) 
{
  switch (dir) {
    case EQuadrant::PlusXPlusY:   { return { 1, 1}; };
    case EQuadrant::PlusXMinusY:  { return { 1,-1}; };
    case EQuadrant::MinusXPlusY:  { return {-1, 1}; };
    case EQuadrant::MinusXMinusY: { return {-1,-1}; };
    default:                      { return {0, 0}; };
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeTypes::EQuadrant Vec2Quadrant(const Vec2f& dir) 
{
  // NOTE: check sign bit explicitly here to discriminate the difference between -0.f and 0.f.
  //       This preserves the property that checking a vector reflected through the origin
  //       results in a quadrant reflected through the origin (this property is not true for
  //       vertical and horizontal lines when using float comparison operations, since 
  //       -0.f == 0.f by definition)  
  if ( signbit( dir.x() ) ) {
    return signbit( dir.y() ) ? EQuadrant::MinusXMinusY : EQuadrant::MinusXPlusY;
  } else {
    return signbit( dir.y() ) ? EQuadrant::PlusXMinusY : EQuadrant::PlusXPlusY;
  }
}

} // namespace
} // namespace
} // namespace
