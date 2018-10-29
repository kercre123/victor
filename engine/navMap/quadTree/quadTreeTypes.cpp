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
#include "coretech/common/engine/math/point_impl.h"

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static_assert(GetQuadrantInDirection(EQuadrant::PlusXPlusY,   EDirection::PlusX)  == EQuadrant::MinusXPlusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::PlusXPlusY,   EDirection::MinusX) == EQuadrant::MinusXPlusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::PlusXPlusY,   EDirection::PlusY)  == EQuadrant::PlusXMinusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::PlusXPlusY,   EDirection::MinusY) == EQuadrant::PlusXMinusY,  "bad quadrant calculation");

static_assert(GetQuadrantInDirection(EQuadrant::MinusXPlusY,  EDirection::PlusX)  == EQuadrant::PlusXPlusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::MinusXPlusY,  EDirection::MinusX) == EQuadrant::PlusXPlusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::MinusXPlusY,  EDirection::PlusY)  == EQuadrant::MinusXMinusY, "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::MinusXPlusY,  EDirection::MinusY) == EQuadrant::MinusXMinusY, "bad quadrant calculation");

static_assert(GetQuadrantInDirection(EQuadrant::PlusXMinusY,  EDirection::PlusX)  == EQuadrant::MinusXMinusY, "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::PlusXMinusY,  EDirection::MinusX) == EQuadrant::MinusXMinusY, "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::PlusXMinusY,  EDirection::PlusY)  == EQuadrant::PlusXPlusY,   "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::PlusXMinusY,  EDirection::MinusY) == EQuadrant::PlusXPlusY,   "bad quadrant calculation");

static_assert(GetQuadrantInDirection(EQuadrant::MinusXMinusY, EDirection::PlusX)  == EQuadrant::PlusXMinusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::MinusXMinusY, EDirection::MinusX) == EQuadrant::PlusXMinusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::MinusXMinusY, EDirection::PlusY)  == EQuadrant::MinusXPlusY,  "bad quadrant calculation");
static_assert(GetQuadrantInDirection(EQuadrant::MinusXMinusY, EDirection::MinusY) == EQuadrant::MinusXPlusY,  "bad quadrant calculation");

static_assert(GetOppositeDirection(EDirection::PlusX)  == EDirection::MinusX, "bad direction calculation");
static_assert(GetOppositeDirection(EDirection::MinusX) == EDirection::PlusX,  "bad direction calculation");
static_assert(GetOppositeDirection(EDirection::PlusY)  == EDirection::MinusY, "bad direction calculation");
static_assert(GetOppositeDirection(EDirection::MinusY) == EDirection::PlusY,  "bad direction calculation");

static_assert(!IsSibling(EQuadrant::PlusXPlusY, EDirection::PlusX),  "bad sibling calculation");
static_assert( IsSibling(EQuadrant::PlusXPlusY, EDirection::MinusX), "bad sibling calculation");
static_assert(!IsSibling(EQuadrant::PlusXPlusY, EDirection::PlusY),  "bad sibling calculation");
static_assert( IsSibling(EQuadrant::PlusXPlusY, EDirection::MinusY), "bad sibling calculation");

static_assert( IsSibling(EQuadrant::MinusXPlusY, EDirection::PlusX),  "bad sibling calculation");
static_assert(!IsSibling(EQuadrant::MinusXPlusY, EDirection::MinusX), "bad sibling calculation");
static_assert(!IsSibling(EQuadrant::MinusXPlusY, EDirection::PlusY),  "bad sibling calculation");
static_assert( IsSibling(EQuadrant::MinusXPlusY, EDirection::MinusY), "bad sibling calculation");

static_assert(!IsSibling(EQuadrant::PlusXMinusY, EDirection::PlusX),  "bad sibling calculation");
static_assert( IsSibling(EQuadrant::PlusXMinusY, EDirection::MinusX), "bad sibling calculation");
static_assert( IsSibling(EQuadrant::PlusXMinusY, EDirection::PlusY),  "bad sibling calculation");
static_assert(!IsSibling(EQuadrant::PlusXMinusY, EDirection::MinusY), "bad sibling calculation");

static_assert( IsSibling(EQuadrant::MinusXMinusY, EDirection::PlusX),  "bad sibling calculation");
static_assert(!IsSibling(EQuadrant::MinusXMinusY, EDirection::MinusX), "bad sibling calculation");
static_assert( IsSibling(EQuadrant::MinusXMinusY, EDirection::PlusY),  "bad sibling calculation");
static_assert(!IsSibling(EQuadrant::MinusXMinusY, EDirection::MinusY), "bad sibling calculation");

} // namespace
} // namespace
} // namespace
