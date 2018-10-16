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
enum class EDirection { PlusX, PlusY, MinusX, MinusY };

// a sequence of quadrants that can be used to find a specific node in a full QuadTree without geometry checks
using NodeAddress = std::vector<EQuadrant>;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// EQuadrant to Vec2f
Vec2f Quadrant2Vec(EQuadrant dir) ;

// Vec2f to EQuadrant
QuadTreeTypes::EQuadrant Vec2Quadrant(const Vec2f& dir);

// step from a quadrant in direction
inline constexpr QuadTreeTypes::EQuadrant GetQuadrantInDirection(EQuadrant from, EDirection dir)
{
  // bit position 0 is the Y coordinate, bit position 1 is x, so toggle the appropriate bit with an XOR
  switch (dir) {
    case EDirection::PlusX:
    case EDirection::MinusX: { return (EQuadrant) ((std::underlying_type<EQuadrant>::type) from ^ 2); };
    case EDirection::PlusY:    
    case EDirection::MinusY: { return (EQuadrant) ((std::underlying_type<EQuadrant>::type) from ^ 1); };
  }
}

inline constexpr QuadTreeTypes::EDirection GetOppositeDirection(EDirection dir)
{
  // directions are defined in CW order, so just move two positions CW and grab the last two bits
  const EDirection ret = (EDirection)(((std::underlying_type<EDirection>::type)dir + 2) & 0b11);
  return ret;
}

inline constexpr bool IsSibling(EQuadrant from, EDirection dir)
{
  // bit position 0 is the Y coordinate, bit position 1 is x, so just compare state of these bits
  // for each direction
  switch (dir) {
    case EDirection::PlusX:  { return  ((std::underlying_type<EQuadrant>::type)from & 0b10); }
    case EDirection::MinusX: { return !((std::underlying_type<EQuadrant>::type)from & 0b10); }
    case EDirection::PlusY:  { return  ((std::underlying_type<EQuadrant>::type)from & 0b01); }
    case EDirection::MinusY: { return !((std::underlying_type<EQuadrant>::type)from & 0b01); }
  }
}

} // namespace
} // namespace
} // namespace

#endif //
