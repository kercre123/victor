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
  void OnNodeContentTypeChanged(const NavMeshQuadTreeNode* node, EContentType oldContent, EContentType newContent);

  // notification when a node is going to be removed entirely
  void OnNodeDestroyed(const NavMeshQuadTreeNode* node);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Debug
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // render debug information for the processor (will only redraw if required)
  void Draw() const;
 
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // true if we have a need to cache the given content type, false otherwise
  static bool IsCached(EContentType contentType);
  
  // returns a color used to represent the given contentType for debugging purposes
  static ColorRGBA GetDebugColor(EContentType contentType);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Processing borders
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // adds border information
  void AddBorderWaypoint(const NavMeshQuadTreeNode* from, const NavMeshQuadTreeNode* to);
  
  // flags the last border waypoint as finishing a border, so that it doesn't connect with the next one
  void FinishBorder();
  
  // iterate over current nodes and finding borders between the specified types
  // note this deletes previous borders for other types (in the future we may prefer to find them all at the same time)
  void FindBorders(EContentType innerType, EContentType outerType);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // add quads from the given contentType to the vector for rendering
  void AddQuadsToDraw(EContentType contentType,
    VizManager::SimpleQuadVector& quadVector, const ColorRGBA& color, float zOffset) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct BorderWaypoint {
    BorderWaypoint() : from(nullptr), to(nullptr), isEnd(false), isSeed(false) {}
    BorderWaypoint(const NavMeshQuadTreeNode* f, const NavMeshQuadTreeNode* t, bool end) : from(f), to(t), isEnd(end), isSeed(false) {}
    const NavMeshQuadTreeNode* from;  // inner quad
    const NavMeshQuadTreeNode* to;    // outer quad
    bool isEnd; // true if this is the last waypoint of a border, for example when the obstacle finishes
    bool isSeed; // just a flag fore debugging. True if this waypoint was the first in a border
  };
  
  using NodeSet = std::unordered_set<const NavMeshQuadTreeNode*>;
  using NodeSetPerType = std::unordered_map<EContentType, NodeSet, Anki::Util::EnumHasher>;
  using BorderWaypointVector = std::vector<BorderWaypoint>;

  // cache of nodes/quads classified per type for faster processing
  NodeSetPerType _nodeSets;
  
  // borders detected in the last search
  BorderWaypointVector _borders;
  
  // pointer to the root of the tree
  const NavMeshQuadTreeNode* _root;
  
  // true if there have been changes since last drawn
  mutable bool _contentGfxDirty;
  mutable bool _borderGfxDirty;
  
}; // class
  
} // namespace
} // namespace

#endif //
