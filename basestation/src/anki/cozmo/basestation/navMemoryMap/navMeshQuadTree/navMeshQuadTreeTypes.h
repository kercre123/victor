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

namespace Anki {
namespace Cozmo {
namespace NavMeshQuadTreeTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// content detected in nodes
enum class EContentType {
  Subdivided,           // we are subdivided, children hold more detailed info
  Unknown,              // no idea
  Clear,                // what we know about the node is clear (could be partial info)
  ObstacleCube,         // we have seen an obstacle in part of the node and we know the obstacle was a cube
  ObstacleUnrecognized, // we have seen an obstacle in part of the node but we don't know what it is
  Cliff,                // we have seen a cliff in part of the node
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// position with respect to the parent
enum EQuadrant : uint8_t {
  TopLeft  = 0,
  TopRight = 1,
  BotLeft  = 2,
  BotRight = 3,
  Root     = 4 // needed for the root node, who has no parent
};

} // namespace
} // namespace
} // namespace

#endif //
