/**
 * File: navMeshQuadTreeTypes.h
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Type definitions for navMeshQuadTree.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_TYPES_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_TYPES_H

#include <cstdint>
#include <type_traits>

namespace Anki {
namespace Cozmo {
namespace NavMeshQuadTreeTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// content detected in nodes
enum class EContentType : uint8_t {
  Subdivided,           // we are subdivided, children hold more detailed info
  Unknown,              // no idea
  Clear,                // what we know about the node is clear (could be partial info)
  ObstacleCube,         // we have seen an obstacle in part of the node and we know the obstacle was a cube
  ObstacleUnrecognized, // we have seen an obstacle in part of the node but we don't know what it is
  Cliff,                // we have seen a cliff in part of the node
};

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
inline NavMeshQuadTreeTypes::EDirection GetOppositeDirection(EDirection dir);

// return the opposite clock direction to the one given (eg: CW vs CCW)
inline NavMeshQuadTreeTypes::EClockDirection GetOppositeClockDirection(EClockDirection dir);

// iterate directions in the specified rotation/clock direction
inline NavMeshQuadTreeTypes::EDirection GetNextDirection(EDirection dir, EClockDirection iterationDir );

// EDirection to String
inline const char* EDirectionToString(EDirection dir);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline NavMeshQuadTreeTypes::EDirection GetOppositeDirection(EDirection dir)
{
  const EDirection ret = (EDirection)(((std::underlying_type<EDirection>::type)dir + 2) % 4);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline NavMeshQuadTreeTypes::EClockDirection GetOppositeClockDirection(EClockDirection dir)
{
  const EClockDirection ret = (dir == EClockDirection::CW) ? EClockDirection::CCW : EClockDirection::CW;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline NavMeshQuadTreeTypes::EDirection GetNextDirection(EDirection dir, EClockDirection iterationDir )
{
  EDirection next;
  if ( iterationDir == EClockDirection::CW ) {
    next = (EDirection)(((std::underlying_type<EClockDirection>::type)dir + 1) % 4);
  } else {
    next = dir == EDirection::North ? EDirection::South : (EDirection)((std::underlying_type<EClockDirection>::type)dir-1);
  }
  return next;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline const char* EDirectionToString(EDirection dir)
{
  switch (dir) {
    case NavMeshQuadTreeTypes::EDirection::North:   { return "N"; };
    case NavMeshQuadTreeTypes::EDirection::East:    { return "E" ;};
    case NavMeshQuadTreeTypes::EDirection::West:    { return "W"; };
    case NavMeshQuadTreeTypes::EDirection::South:   { return "S"; };
    case NavMeshQuadTreeTypes::EDirection::Invalid: { return "Invalid"; };
  }
  return "Error";
}

} // namespace
} // namespace
} // namespace

#endif //
