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

#include "coretech/common/engine/math/point.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"

#include <cstdint>
#include <type_traits>

namespace Anki {
namespace Cozmo {

class MemoryMapData;
class QuadTreeNode;

namespace QuadTreeTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// content for each node. INavMemoryMapQuadData is polymorphic depending on the content type
struct NodeContent {
  explicit NodeContent(const MemoryMapData& m);
  explicit NodeContent(MemoryMapTypes::MemoryMapDataPtr m);
  
  // comparison operators
  bool operator==(const NodeContent& other) const;
  bool operator!=(const NodeContent& other) const;
  
  MemoryMapTypes::MemoryMapDataPtr data;
};

using FoldFunctor      = std::function<void (QuadTreeNode& node)>;
using FoldFunctorConst = std::function<void (const QuadTreeNode& node)>;

enum class FoldDirection { DepthFirst, BreadthFirst };

// position with respect to the parent
enum class EQuadrant : uint8_t {
  TopLeft  = 0,
  TopRight = 1,
  BotLeft  = 2,
  BotRight = 3,
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
