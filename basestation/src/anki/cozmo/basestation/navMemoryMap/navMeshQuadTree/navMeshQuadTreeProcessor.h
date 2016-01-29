/**
 * File: navMeshQuadTreeProcessor.h
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Class for processing a navMeshQuadTree. It relies on the navMeshQuadTree and navMeshQuadTreeNodes to
 * share the proper information for the Processor.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_PROCESSOR_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_PROCESSOR_H

#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapTypes.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "util/helpers/templateHelpers.h"

#include <unordered_map>
#include <unordered_set>

namespace Anki {
namespace Cozmo {

class NavMeshQuadTreeNode;
using namespace NavMeshQuadTreeTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTreeProcessor
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor
  NavMeshQuadTreeProcessor();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Notifications from nodes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // set root
  void SetRoot(const NavMeshQuadTreeNode* node) { _root = node; }

  // notification when the content type changes for the given node
  void OnNodeContentTypeChanged(const NavMeshQuadTreeNode* node, ENodeContentType oldContent, ENodeContentType newContent);

  // notification when a node is going to be removed entirely
  void OnNodeDestroyed(const NavMeshQuadTreeNode* node);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // returns true if we have borders detected of the given type, or we think we do without having to actually calcualte
  // them at this moment (which is slightly more costly and requires non-const access)
  bool HasBorders(ENodeContentType innerType, ENodeContentType outerType) const;

  // retrieve the borders for the given combination of content types
  void GetBorders(ENodeContentType innerType, ENodeContentType outerType, NavMemoryMapTypes::BorderVector& outBorders);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Debug
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // render debug information for the processor (will only redraw if required)
  void Draw() const;
 
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  struct BorderWaypoint {
    BorderWaypoint()
      : from(nullptr), to(nullptr), direction(EDirection::Invalid), isEnd(false), isSeed(false) {}
    BorderWaypoint(const NavMeshQuadTreeNode* f, const NavMeshQuadTreeNode* t, EDirection dir, bool end)
      : from(f), to(t), direction(dir), isEnd(end), isSeed(false) {}
    const NavMeshQuadTreeNode* from;  // inner quad
    const NavMeshQuadTreeNode* to;    // outer quad
    EDirection direction; // neighbor 4-direction between from and to
    bool isEnd; // true if this is the last waypoint of a border, for example when the obstacle finishes
    bool isSeed; // just a flag fore debugging. True if this waypoint was the first in a border
  };
  
  struct BorderCombination {
    using BorderWaypointVector = std::vector<BorderWaypoint>;
    BorderCombination() : dirty(true) {}
    bool dirty; // flag when border is dirty (needs recalculation)
    BorderWaypointVector waypoints;
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // true if we have a need to cache the given content type, false otherwise
  static bool IsCached(ENodeContentType contentType);
  
  // returns a color used to represent the given contentType for debugging purposes
  static ColorRGBA GetDebugColor(ENodeContentType contentType);
  
  // returns a number that represents the given combination inner-outer
  static uint32_t GetBorderTypeKey(ENodeContentType innerType, ENodeContentType outerType);

  // given a border waypoint, calculate its center in 3D
  static Vec3f CalculateBorderWaypointCenter(const BorderWaypoint& waypoint);

  // given 3d points and their neighbor directions, calculate a 3D border definition (line + normal)
  static NavMemoryMapTypes::Border MakeBorder(const Point3f& origin, const Point3f& dest, EDirection firstEDirection, EDirection lastEDirection);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // sets all current borders are dirty/invalid, in need of being recalculated
  void InvalidateBorders();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing borders
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // adds border information
  void AddBorderWaypoint(const NavMeshQuadTreeNode* from, const NavMeshQuadTreeNode* to, EDirection dir);
  
  // flags the last border waypoint as finishing a border, so that it doesn't connect with the next one
  void FinishBorder();
  
  // iterate over current nodes and finding borders between the specified types
  // note this deletes previous borders for other types (in the future we may prefer to find them all at the same time)
  void FindBorders(ENodeContentType innerType, ENodeContentType outerType);
  
  // checks if currently we have a node of innerType that would become a seed if we computed borders
  bool HasBorderSeed(ENodeContentType innerType, ENodeContentType outerType) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // add quads from the given contentType to the vector for rendering
  void AddQuadsToDraw(ENodeContentType contentType,
    VizManager::SimpleQuadVector& quadVector, const ColorRGBA& color, float zOffset) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using NodeSet = std::unordered_set<const NavMeshQuadTreeNode*>;
  using NodeSetPerType = std::unordered_map<ENodeContentType, NodeSet, Anki::Util::EnumHasher>;
  using BorderMap = std::map<uint32_t, BorderCombination>;

  // cache of nodes/quads classified per type for faster processing
  NodeSetPerType _nodeSets;
  
  // borders detected in the last search for each combination of content types (inner + outer)
  BorderMap _bordersPerContentCombination;
  
  // used during process for easier/faster access to the proper combination
  BorderCombination* _currentBorderCombination;
  
  // pointer to the root of the tree
  const NavMeshQuadTreeNode* _root;
  
  // true if there have been changes since last drawn
  mutable bool _contentGfxDirty;
  mutable bool _borderGfxDirty;
  
}; // class
  
} // namespace
} // namespace

#endif //
