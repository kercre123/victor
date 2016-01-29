/**
 * File: navMeshQuadTreeNode.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Nodes in the nav mesh, represented as quad tree nodes.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_NODE_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_NODE_H

#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/point.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class NavMeshQuadTreeProcessor;
using namespace NavMeshQuadTreeTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTreeNode
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  using NodeCPtrVector = std::vector<const NavMeshQuadTreeNode*>;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Crete node
  // it will allow subdivision as long as level is greater than 0
  NavMeshQuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, NavMeshQuadTreeNode* parent);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t GetLevel() const { return _level; }
  float GetSideLen() const { return _sideLen; }
  const Point3f& GetCenter() const { return _center; }
  ENodeContentType GetContentType() const { return _contentType; }
  
  // returns true if this node FULLY contains the given quad, false if any corner is not within this node's quad
  bool Contains(const Quad2f& quad) const;

  // Builds a quad from our coordinates
  Quad3f MakeQuad() const;
  Quad2f MakeQuadXY() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // helper for the specific add functions. It properly processes the quad down the tree for the given content
  bool AddQuad(const Quad2f& quad, ENodeContentType detectedContentType, NavMeshQuadTreeProcessor& processor);

  // Convert this node into a parent of its level, delegating its children to the new child that substitutes it
  // In order for a quadtree to be valid, the only way this could work without further operations is calling this
  // on a root node. Such responsibility lies in the caller, not in this node
  // Returns true if successfully expanded, false otherwise
  bool UpgradeRootLevel(const Point2f& direction, NavMeshQuadTreeProcessor& processor);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Exploration
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // find the neighbor of the same or higher level in the given direction
  const NavMeshQuadTreeNode* FindSingleNeighbor(EDirection direction) const;

  // find the group of smallest neighbors with whom this node shares a border.
  // they would be children of the same level neighbor. This is normally useful when our neighbor is subdivided but
  // we are not.
  // direction: direction in which we move to find the neighbors (4 cardinals)
  // iterationDirection: when there're more than one neighbor in that direction, which one comes first in the list
  // NOTE: this method is expected to NOT clear the vector before adding neighbors
  void AddSmallestNeighbors(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& neighbors) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds the necessary quads to the given vector to be rendered
  void AddQuadsToDraw(VizManager::SimpleQuadVector& quadVector) const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // invalid node id, used as default value
  enum { kInvalidNodeId = 0 };
  
  // info about moving towards a neighbor
  struct MoveInfo {
    EQuadrant neighborQuadrant;  // destination quadrant
    bool sharesParent;                                // whether destination quadrant is in the same parent
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // return true if this quad can subdivide
  bool CanSubdivide() const { return _level > 0; }
  
  // return true if this quad is already subdivided
  bool IsSubdivided() const { return !_children.empty(); }
  
  // returns true if this node can override children with the given content type (some changes in content
  // type are not allowed to preserve information). This is a necessity now to prevent Cliffs from being
  // removed by Clear. Note that eventually we have to support that since it's possible that the player
  // actually covers the cliff with something transitable
  bool CanOverrideChildrenWithContent(ENodeContentType contentType) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // subdivide/merge children
  void Subdivide(NavMeshQuadTreeProcessor& processor);
  void Merge(ENodeContentType newContentType, NavMeshQuadTreeProcessor& processor);

  // checks if all children are the same type, if so it removes the children and merges back to a single parent
  void TryAutoMerge(NavMeshQuadTreeProcessor& processor);
  
  // sets the content type to the detected one.
  // try checks por priority first, then calls force
  void TrySetDetectedContentType(ENodeContentType detectedContentType, NavMeshQuadTreeProcessor& processor);
  // force sets the type and updates shared container
  void ForceSetDetectedContentType(ENodeContentType detectedContentType, NavMeshQuadTreeProcessor& processor);
  
  // sets a new parent to this node. Used on expansions
  void ChangeParent(const NavMeshQuadTreeNode* newParent) { _parent = newParent; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Exploration
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   
  // calculate where we would land from a quadrant if we moved in the given direction
  static const MoveInfo* GetDestination(EQuadrant from, EDirection direction);
  
  // get the child in the given quadrant, or null if this node is not subdivided
  const NavMeshQuadTreeNode* GetChild(EQuadrant quadrant) const;

  // iterate until we reach the nodes that have a border in the given direction, and add them to the vector
  // NOTE: this method is expected to NOT clear the vector before adding descendants
  void AddSmallestDescendants(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& descendants) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // NOTE: try to minimize padding in these attributes

  // children when subdivided. Can be empty or have 4 nodes
  std::vector<NavMeshQuadTreeNode> _children;

  // coordinates of this quad
  Point3f _center;
  float   _sideLen;

  // parent node
  const NavMeshQuadTreeNode* _parent;

  // our level
  uint8_t _level;

  // quadrant within the parent
  EQuadrant _quadrant;
  
  // information about what's in this quad
  ENodeContentType _contentType;
    
}; // class
  
} // namespace
} // namespace

#endif //
