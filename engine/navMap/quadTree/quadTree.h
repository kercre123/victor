/**
 * File: quadTree.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Mesh representation of known geometry and obstacles for/from navigation with quad trees.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_QUAD_TREE_H
#define ANKI_COZMO_QUAD_TREE_H

#include "quadTreeNode.h"
#include "quadTreeProcessor.h"

#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "coretech/common/engine/math/point.h"
#include "coretech/common/engine/math/triangle.h"

namespace Anki {
namespace Cozmo {

class Robot;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class QuadTree
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor/destructor
  QuadTree(VizManager* vizManager, Robot* robot, MemoryMapData rootData);
  ~QuadTree();
  
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
  // NOTE: (mrw)
  //   The only difference between Broadcast and BoadcastMemoryMapDraw is whether or not we
  //   are converting EContentType to ENodeContentTypeEnum or ENodeContentTypeDebugVizEnum. Why can't
  //   the consumers share this data? 
  void Broadcast(uint32_t originID) const;
  void BroadcastMemoryMapDraw(uint32_t originID, size_t maxIdxHint) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // returns the precision of content data in the memory map. For example, if you add a point, and later query for it,
  // the region that the point generated to store the point could have an error of up to this length.
  float GetContentPrecisionMM() const;

  // return the Processor associated to this QuadTree for queries
        QuadTreeProcessor& GetProcessor()       { return _processor; }
  const QuadTreeProcessor& GetProcessor() const { return _processor; }
  
  const NodeContent& GetRootNodeContent() const { return _root.GetContent(); }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // notify the QT that the given poly has the specified content
  // shiftAllowedCount: number of shifts we can do for a fully expanded root that still needs to move to
  // fit the data.
  void Insert(const FastPolygon& poly, const MemoryMapData& data, int shiftAllowedCount);
  
  // modify content bounded by poly. Note that if the poly extends outside the current size of the root node,
  // it will not expand the root node
  void Transform(const Poly2f& poly, NodeTransformFunction transform) {_root.Transform(poly, transform, _processor);}
  
  void FindIf(const Poly2f& poly, NodePredicate pred, MemoryMapDataConstList& output) {_root.FindIf(poly, pred, output);}
  
  // merge the given quadtree into this quad tree, applying to the quads from other the given transform
  void Merge(const QuadTree& other, const Pose3d& transform);
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Node operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Expand the root node so that the given quad/point/triangle is included in the navMesh, up to the max root size limit.
  // shiftAllowedCount: number of shifts we can do if the root reaches the max size upon expanding (or already is at max.)
  void Expand(const Poly2f& polyToCover, int shiftAllowedCount);  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // set to true if data has changed since last time we send to gfx
  mutable bool _gfxDirty;

  // processor for this quadtree
  QuadTreeProcessor _processor;

  // current root of the tree. It expands as needed
  QuadTreeNode _root;
  
  VizManager* _vizManager;
  Robot*      _robot;
  
}; // class
  
} // namespace
} // namespace

#endif //
