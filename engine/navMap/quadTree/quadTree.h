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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class QuadTree
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // constructor/destructor
  QuadTree(MemoryMapDataPtr rootData);
  ~QuadTree();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // returns the precision of content data in the memory map. For example, if you add a point, and later query for it,
  // the region that the point generated to store the point could have an error of up to this length.
  float GetContentPrecisionMM() const;

  // return the Processor associated to this QuadTree for queries
        QuadTreeProcessor& GetProcessor()       { return _processor; }
  const QuadTreeProcessor& GetProcessor() const { return _processor; }
  uint8_t GetLevel() const                      { return _root.GetLevel(); }
  
  const MemoryMapDataPtr GetRootNodeData() const { return _root.GetData(); }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // notify the QT that the given poly has the specified content
  // shiftAllowedCount: number of shifts we can do for a fully expanded root that still needs to move to
  // fit the data.
  bool Insert(const FastPolygon& poly, MemoryMapDataPtr data);
  
  // modify content bounded by poly. Note that if the poly extends outside the current size of the root node,
  // it will not expand the root node
  bool Transform(NodeTransformFunction transform);
  bool Transform(const Poly2f& poly, NodeTransformFunction transform);
  
  // merge the given quadtree into this quad tree, applying to the quads from other the given transform
  bool Merge(const QuadTree& other, const Pose3d& transform);

  // runs the provided accumulator function through all nodes of the tree recursively
  void Fold(FoldFunctor accumulator)                                  { _root.Fold(accumulator);         }
  void Fold(FoldFunctor accumulator, const Poly2f& region)            { _root.Fold(accumulator, region); }
  void Fold(FoldFunctorConst accumulator)                       const { _root.Fold(accumulator);         }
  void Fold(FoldFunctorConst accumulator, const Poly2f& region) const { _root.Fold(accumulator, region); }

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Node operations
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Expand the root node so that the given quad/point/triangle is included in the navMesh, up to the max root size limit.
  // shiftAllowedCount: number of shifts we can do if the root reaches the max size upon expanding (or already is at max.)
  bool ExpandToFit(const Poly2f& polyToCover);  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // processor for this quadtree
  QuadTreeProcessor _processor;

  // current root of the tree. It expands as needed
  QuadTreeNode _root; 
}; // class
  
} // namespace
} // namespace

#endif //
