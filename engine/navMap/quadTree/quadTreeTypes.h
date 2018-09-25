/**
 * File: quadTreeTypes.h
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Type definitions for QuadTree.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_TYPES_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_TYPES_H

#include "engine/navMap/memoryMap/data/memoryMapDataWrapper.h"
#include "coretech/common/engine/math/pointSetUnion.h"

#include <cstdint>
#include <type_traits>

namespace Anki {
namespace Vector {

class MemoryMapData;
class QuadTreeNode;

namespace QuadTreeTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using MemoryMapDataPtr       = MemoryMapDataWrapper<MemoryMapData>;

// content for each node. INavMemoryMapQuadData is polymorphic depending on the content type
struct NodeContent {
  explicit NodeContent(const MemoryMapData& m);
  explicit NodeContent(MemoryMapDataPtr m);
  
  // comparison operators
  bool operator==(const NodeContent& other) const;
  bool operator!=(const NodeContent& other) const;
  
  MemoryMapDataPtr data;
};


using FoldFunctor      = std::function<void (QuadTreeNode& node)>;
using FoldFunctorConst = std::function<void (const QuadTreeNode& node)>;

// wrapper class for specifying the interface between QT actions and geometry methods
class FoldableRegion {
public:
  FoldableRegion(const BoundedConvexSet2f& set) 
  : _aabb(set.GetAxisAlignedBoundingBox())
  {
    using std::placeholders::_1;

    // NOTE: lambda incurs some ~5% perforamnce overhead from the std::function interface, 
    //       so use std::bind where possible to reduce function deref overhead. lambda implementations
    //       are included in comments for clarity on what the bind method should evaluate to

    Contains       = std::bind( &BoundedConvexSet2f::Contains, &set, _1 );
                // ~ [&set](const Point2f& p) { return set.Contains(p); };
    ContainsQuad   = std::bind( &BoundedConvexSet2f::ContainsAll, &set, std::bind(&AxisAlignedQuad::GetVertices, _1) );
                // ~ [&set](const AxisAlignedQuad& q) { return set.ContainsAll(q.GetVertices()); };
    IntersectsQuad = std::bind( &BoundedConvexSet2f::Intersects, &set, _1 );
                // ~ [&set](const AxisAlignedQuad& q) { return set.Intersects(q); };
  }

  // allow Union types
  template <typename T, typename U>
  FoldableRegion(const PointSetUnion2f<T,U>& set) 
  : _aabb(set.GetAxisAlignedBoundingBox())
  {
    using std::placeholders::_1;

    Contains       = std::bind( &PointSetUnion2f<T,U>::Contains, &set, _1 );
                // ~ [&set](const Point2f& p) { return set.Contains(p); };
    ContainsQuad   = std::bind( &PointSetUnion2f<T,U>::ContainsHyperCube, &set, _1 );
                // ~ [&set](const AxisAlignedQuad& q) { return set.ContainsHyperCube(q); };
    IntersectsQuad = std::bind( &PointSetUnion2f<T,U>::Intersects, &set, _1 );
                // ~ [&set](const AxisAlignedQuad& q) { return set.Intersects(q); };
  }

  std::function<bool(const Point2f&)>         Contains;
  std::function<bool(const AxisAlignedQuad&)> ContainsQuad;
  std::function<bool(const AxisAlignedQuad&)> IntersectsQuad;

  // cache AABB since the set is const at this point
  const AxisAlignedQuad& GetBoundingBox() const { return _aabb; };

private:
  const AxisAlignedQuad _aabb;
};

enum class FoldDirection { DepthFirst, BreadthFirst };

// position with respect to the parent
enum class EQuadrant : uint8_t {
  PlusXPlusY   = 0,
  PlusXMinusY  = 1,
  MinusXPlusY  = 2,
  MinusXMinusY = 3,
  Root     = 4, // needed for the root node, who has no parent
  Invalid  = 255
};

// movement direction
enum class EDirection { North, East, South, West, Invalid };

// rotation direction
enum EClockDirection { CW, CCW };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// return the opposite direction to the one given (eg: North vs South, West vs East)
inline QuadTreeTypes::EDirection GetOppositeDirection(EDirection dir);

// return the opposite clock direction to the one given (eg: CW vs CCW)
inline QuadTreeTypes::EClockDirection GetOppositeClockDirection(EClockDirection dir);

// iterate directions in the specified rotation/clock direction
inline QuadTreeTypes::EDirection GetNextDirection(EDirection dir, EClockDirection iterationDir );

// EDirection to String
const char* EDirectionToString(EDirection dir);

// EDirection to Vec3f
Vec3f EDirectionToNormalVec3f(EDirection dir);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline QuadTreeTypes::EDirection GetOppositeDirection(EDirection dir)
{
  const EDirection ret = (EDirection)(((std::underlying_type<EDirection>::type)dir + 2) % 4);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline QuadTreeTypes::EClockDirection GetOppositeClockDirection(EClockDirection dir)
{
  const EClockDirection ret = (dir == EClockDirection::CW) ? EClockDirection::CCW : EClockDirection::CW;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline QuadTreeTypes::EDirection GetNextDirection(EDirection dir, EClockDirection iterationDir )
{
  EDirection next;
  if ( iterationDir == EClockDirection::CW ) {
    next = (EDirection)(((std::underlying_type<EClockDirection>::type)dir + 1) % 4);
  } else {
    next = dir == EDirection::North ? EDirection::South : (EDirection)((std::underlying_type<EClockDirection>::type)dir-1);
  }
  return next;
}

} // namespace
} // namespace
} // namespace

#endif //
