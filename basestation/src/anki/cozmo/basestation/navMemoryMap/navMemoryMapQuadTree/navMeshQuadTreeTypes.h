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

#include "anki/common/basestation/math/point.h"

#include <cstdint>
#include <type_traits>

namespace Anki {
namespace Cozmo {

struct INavMemoryMapQuadData;

namespace NavMeshQuadTreeTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Types
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// content detected in nodes
enum class ENodeContentType : uint8_t {
  Invalid,               // invalid type (not set)
  Subdivided,            // we are subdivided, children hold more detailed info
  Unknown,               // no idea
  ClearOfObstacle,       // what we know about the node is clear (could be partial info)
  ClearOfCliff,          // what we know about the node is clear (could be partial info)
  ObstacleCube,          // we have seen an obstacle in part of the node and we know the obstacle was a cube
  ObstacleCubeRemoved,   // there used to be a cube in this area, but it has moved somewhere else
  ObstacleCharger,       // we have seen a charger in part of the node
  ObstacleChargerRemoved,// there used to be a charger in this area, but it has moved somewhere else
  ObstacleUnrecognized,  // we have seen an obstacle in part of the node but we don't know what it is
  Cliff,                 // we have seen a cliff in part of the node
  InterestingEdge,       // we have seen a vision edge and it's interesting
  NotInterestingEdge,    // we have visited an interesting edge, so it's not interesting anymore
};

// variable type in which we can pack ENodeContentType as flags. Check ENodeContentTypeToFlag
using ENodeContentTypePackedType = uint32_t;

// content for each node. INavMemoryMapQuadData is polymorphic depending on the content type
struct NodeContent {
  explicit NodeContent(ENodeContentType t) : type(t), data(nullptr) {}
  
  // comparison operators
  bool operator==(const NodeContent& other) const;
  bool operator!=(const NodeContent& other) const;
  
  ENodeContentType type;
  std::shared_ptr<INavMemoryMapQuadData> data;
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

// converts ENodeContentType values into flag bits. This is handy because I want to store ENodeContentType in
// the smallest type possible since we have a lot of quad nodes, but I want to pass groups as bit flags in one
// packed variable
ENodeContentTypePackedType ENodeContentTypeToFlag(ENodeContentType nodeContentType);

// String representing ENodeContentType for debugging purposes
const char* ENodeContentTypeToString(ENodeContentType nodeContentType);

// return the opposite direction to the one given (eg: North vs South, West vs East)
inline NavMeshQuadTreeTypes::EDirection GetOppositeDirection(EDirection dir);

// return the opposite clock direction to the one given (eg: CW vs CCW)
inline NavMeshQuadTreeTypes::EClockDirection GetOppositeClockDirection(EClockDirection dir);

// iterate directions in the specified rotation/clock direction
inline NavMeshQuadTreeTypes::EDirection GetNextDirection(EDirection dir, EClockDirection iterationDir );

// EDirection to String
const char* EDirectionToString(EDirection dir);

// EDirection to Vec3f
Vec3f EDirectionToNormalVec3f(EDirection dir);

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

} // namespace
} // namespace
} // namespace

#endif //
