/**
 * File: navMeshQuadTree.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Mesh representation of known geometry and obstacles for/from navigation with quad trees.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MESH_QUAD_TREE_H
#define ANKI_COZMO_NAV_MESH_QUAD_TREE_H

#include "navMeshQuadTreeNode.h"
#include "navMeshQuadTreeProcessor.h"

#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapTypes.h"
#include "anki/common/basestation/math/point.h"
#include "anki/common/basestation/math/triangle.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMeshQuadTree
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor/destructor
  NavMeshQuadTree(VizManager* vizManager, Robot* robot);
  ~NavMeshQuadTree();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Render
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Render navmesh
  void DrawDebugProcessorInfo(size_t mapIdxHint) const;

  // Stop rendering navmesh
  void ClearDraw() const;
  
  // Forces redrawing even if Mesh thinks content hasn't changed. This is because QTProcessor now can modify
  // content, and navmesh has no other way of knowing that it happened
  void ForceRedraw() { _gfxDirty = true; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Broadcast
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Broadcast navmesh
  void Broadcast(uint32_t originID) const;
  void BroadcastMemoryMapDraw(uint32_t originID, size_t maxIdxHint) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // returns the precision of content data in the memory map. For example, if you add a point, and later query for it,
  // the region that the point generated to store the point could have an error of up to this length.
  float GetContentPrecisionMM() const;

  // return the Processor associated to this QuadTree for queries
  NavMeshQuadTreeProcessor& GetProcessor() { return _processor; }
  const NavMeshQuadTreeProcessor& GetProcessor() const { return _processor; }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // notify the navmesh that the given quad/point/line/triangle has the specified content
  // shiftAllowedCount: number of shifts we can do for a fully expanded root that still needs to move to
  // fit the data.
  void AddQuad(const Quad2f& quad, const NodeContent& nodeContent, int shiftAllowedCount);
  void AddLine(const Point2f& from, const Point2f& to, const NodeContent& nodeContent, int shiftAllowedCount);
  void AddTriangle(const Triangle2f& tri, const NodeContent& nodeContent, int shiftAllowedCount);
  void AddPoint(const Point2f& point, const NodeContent& nodeContent, int shiftAllowedCount);
  
  // merge the given quadtree into this quad tree, applying to the quads from other the given transform
  void Merge(const NavMeshQuadTree& other, const Pose3d& transform);
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Node operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Expand the root node so that the given quad/point/triangle is included in the navMesh, up to the max root size limit.
  // shiftAllowedCount: number of shifts we can do if the root reaches the max size upon expanding (or already is at max.)
  void Expand(const Quad2f& quadToCover, int shiftAllowedCount);
  void Expand(const Point2f& pointToInclude, int shiftAllowedCount);
  void Expand(const Triangle2f& triangleToCover, int shiftAllowedCount);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // set to true if data has changed since last time we send to gfx
  mutable bool _gfxDirty;

  // processor for this quadtree
  NavMeshQuadTreeProcessor _processor;

  // current root of the tree. It expands as needed
  NavMeshQuadTreeNode _root;
  
  VizManager* _vizManager;
  Robot*      _robot;
  
}; // class
  
} // namespace
} // namespace

#endif //
