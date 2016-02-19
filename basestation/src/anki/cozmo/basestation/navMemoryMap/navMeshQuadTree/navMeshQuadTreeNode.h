/**
 * File: navMeshQuadTreeNode.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Nodes in the nav mesh, represented as quad tree nodes.
 * Note nodes can work with a processor to speed up algorithms and searches, however this implementation supports
 * working with one processor only for any given node. Do not use more than one processor instance for nodes, or
 * otherwise leaks and bad pointer references will happen.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_NODE_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_NODE_H

#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/point.h"

#include "util/helpers/noncopyable.h"

#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

class NavMeshQuadTreeProcessor;
using namespace NavMeshQuadTreeTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTreeNode : private Util::noncopyable
{
public:

void TESTDONOTCOMIT();
void TESTDONOTCOMITRec();

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
  
  // Note: Destructor should call processor.OnNodeDestroyed for any processor the node has been registered to.
  // However, by design, we don't do this (no need to store processor pointers, etc). We can do it because of the
  // assumption that the processor(s) will be destroyed at the same time than nodes are, except in the case of
  // nodes that are merged into their parents, in which case we do notify the processor.
  // Alternatively processors would store weak_ptr, but no need for the moment given the above assumption
  // ~NavMeshQuadTreeNode();
  
  // with noncopyable this is not needed, but xcode insist on showing static_asserts in cpp as errors for a while,
  // which is annoying
  NavMeshQuadTreeNode(const NavMeshQuadTreeNode&&) = delete;
  NavMeshQuadTreeNode& operator=(const NavMeshQuadTreeNode&&) = delete;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  uint8_t GetLevel() const { return _level; }
  float GetSideLen() const { return _sideLen; }
  const Point3f& GetCenter() const { return _center; }
  const NodeContent& GetContent() const { return _content; }

  // consider using the concrete checks if you are going to do GetContentType() == X, in case the meaning of that
  // comparison changes
  ENodeContentType GetContentType() const { return _content.type; }
  bool IsContentTypeUnknown() const { return _content.type == ENodeContentType::Unknown; }
  
  // returns true if this node FULLY contains the given quad, false if any corner is not within this node's quad
  bool Contains(const Quad2f& quad) const;

  // Builds a quad from our coordinates
  Quad3f MakeQuad() const;
  Quad2f MakeQuadXY() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // helper for the specific add functions. It properly processes the quad down the tree for the given content
  bool AddQuad(const Quad2f& quad, NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);

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
 
  // adds to the given vector the smallest descendants of every branch. If a node is a leaf, it adds itself,
  // otherwise it recursively asks children to do the same
  void AddSmallestDescendantsDepthFirst(NodeCPtrVector& descendants) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds the necessary quads to the given vector to be rendered
  void AddQuadsToDraw(VizManager::SimpleQuadVector& quadVector) const;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // info about moving towards a neighbor
  struct MoveInfo {
    EQuadrant neighborQuadrant;  // destination quadrant
    bool sharesParent;           // whether destination quadrant is in the same parent
  };
  
  // type of overlap for quads
  enum class EContentOverlap { Partial, Total };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // return true if this quad can subdivide
  bool CanSubdivide() const { return _level > 0; }
  
  // return true if this quad is already subdivided
  bool IsSubdivided() const { return !_childrenPtr.empty(); }
  
  // returns true if this node can override children with the given content type (some changes in content
  // type are not allowed to preserve information). This is a necessity now to prevent Cliffs from being
  // removed by Clear. Note that eventually we have to support that since it's possible that the player
  // actually covers the cliff with something transitable
  bool CanOverrideSelfWithContent(ENodeContentType newContentType, EContentOverlap overlap ) const;
  bool CanOverrideSelfAndChildrenWithContent(ENodeContentType newContentType, EContentOverlap overlap) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // subdivide/merge children
  void Subdivide(NavMeshQuadTreeProcessor& processor);
  void Merge(NodeContent& newContent, NavMeshQuadTreeProcessor& processor);
  void ClearDescendants(NavMeshQuadTreeProcessor& processor);

  // checks if all children are the same type, if so it removes the children and merges back to a single parent
  void TryAutoMerge(NavMeshQuadTreeProcessor& processor);
  
  // sets the content type to the detected one.
  // try checks por priority first, then calls force
  void TrySetDetectedContentType(NodeContent& detectedContent, EContentOverlap overlap, NavMeshQuadTreeProcessor& processor);
  // force sets the type and updates shared container
  void ForceSetDetectedContentType(NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor);
  
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
  std::vector< std::unique_ptr<NavMeshQuadTreeNode> > _childrenPtr;

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
  NodeContent _content;
    
}; // class
  
} // namespace
} // namespace

#endif //
