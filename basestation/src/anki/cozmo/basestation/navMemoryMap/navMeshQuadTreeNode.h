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

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/point.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTreeNode
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  enum class EContentType {
    Subdivided,   // we are subdivided, so children hold more detailed info
    Unknown,      // no idea
    UnkownClear,  // what we know about the quad is clear
    Clear,        // we know the whole quad is clear
    Obstacle,          // we know at least part of the quad has an obstacle
  };

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Sets node at the given center, with the given side size, and that will allow its children to have the
  // given depth (0 will not allow children).
  NavMeshQuadTreeNode(const Point3f& center, float size, uint32_t maxDepth, EContentType contentType);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  float GetSize() const { return _size; }
  const Point3f& GetCenter() const { return _center; }
  EContentType GetContentType() const { return _contentType; }
  
  // returns true if this node FULLY contains the given quad, false if any corner is not within this node's quad
  bool Contains(const Quad2f& quad) const;

  // Builds a quad from our coordinates
  Quad3f MakeQuad() const;
  Quad2f MakeQuadXY() const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // process a clear quad that affected this node's parent. Returns true if the node changed
  bool AddClearQuad(const Quad2f& quad);
  
  // process an obstacle that affected this node's parent
  void AddObstacle();

  // Convert this node into a parent of its level, delegating its children to the new child that substitutes it
  void UpgradeToParent(const Point2f& direction);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // adds the necessary quads to the given vector to be rendered
  void AddQuadsToDraw(VizManager::SimpleQuadVector& quadVector) const;
  
private:
 
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // subdivide/merge children
  void Subdivide();
  void Merge();
  
  // checks if all children are the same type, so that we can merge automatically
  void TryAutoMerge();
  
  // return true if this quad can subdivide
  bool CanSubdivide() const { return _maxDepth > 0; }
  
  // return true if this quad is already subdivided
  bool IsSubdivided() const { return !_children.empty(); }
    
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // depth
  uint32_t _maxDepth;
  
  // information about what's in this quad
  EContentType _contentType;
  
  // coordinates of this quad
  Point3f _center;
  float   _size;

  // children when subdivided. Can be empty or have 4 nodes
  std::vector<NavMeshQuadTreeNode> _children;
  
}; // class
  
} // namespace
} // namespace

#endif //
