/**
 * File: quadTreeNode.h
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

#ifndef ANKI_COZMO_QUAD_TREE_NODE_H
#define ANKI_COZMO_QUAD_TREE_NODE_H

#include "quadTreeTypes.h"

#include "engine/viz/vizManager.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/navMap/memoryMap/data/memoryMapData.h"

#include "coretech/common/engine/math/fastPolygon2d.h"
#include "coretech/common/engine/math/axisAlignedHyperCube.h"
#include "coretech/common/engine/math/ball.h"

#include "util/helpers/noncopyable.h"

#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

class QuadTreeProcessor;
using namespace QuadTreeTypes;
using namespace MemoryMapTypes;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class QuadTreeNode : private Util::noncopyable
{
  friend class QuadTree;
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using NodeCPtrVector = std::vector<const QuadTreeNode*>;  
  using NodeContent    = QuadTreeTypes::NodeContent;
  using FoldFunctor    = QuadTreeTypes::FoldFunctor;
  using FoldDirection  = QuadTreeTypes::FoldDirection;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // Crete node
  // it will allow subdivision as long as level is greater than 0
  QuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, QuadTreeNode* parent);
  
  // Note: Destructor should call processor.OnNodeDestroyed for any processor the node has been registered to.
  // However, by design, we don't do this (no need to store processor pointers, etc). We can do it because of the
  // assumption that the processor(s) will be destroyed at the same time than nodes are, except in the case of
  // nodes that are merged into their parents, or when we shift the root, in which cases we do notify the processor.
  // Alternatively processors would store weak_ptr, but no need for the moment given the above assumption.
  // ~QuadTreeNode();
  
  // with noncopyable this is not needed, but xcode insist on showing static_asserts in cpp as errors for a while,
  // which is annoying
  QuadTreeNode(const QuadTreeNode&&) = delete;
  QuadTreeNode& operator=(const QuadTreeNode&&) = delete;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool    IsRootNode() const              { return _parent == nullptr; }
  bool    IsSubdivided() const            { return !_childrenPtr.empty(); }
  uint8_t GetLevel() const                { return _level; }
  float   GetSideLen() const              { return _sideLen; }
  const   Point3f& GetCenter() const      { return _center; }
  MemoryMapDataPtr GetData() const        { return _content.data; }
  const NodeContent& GetContent() const   { return _content; }

  // Builds a quad from our coordinates
  Quad2f MakeQuadXY(const float padding_mm=0.0f) const;
  const AxisAlignedQuad& GetBoundingBox() const { return _boundingBox; }
  
  // return if the node contains any useful data
  bool IsEmptyType() const { return (IsSubdivided() || (_content.data->type == EContentType::Unknown));  }


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // run the provided accumulator function recursively over the tree for all nodes intersecting with region (if provided).
  // NOTE: any recursive call through the QTN should be implemented by fold so all collision checks happen in a consistant manner
  void Fold(FoldFunctorConst accumulator, FoldDirection dir = FoldDirection::BreadthFirst) const;

  template <class ConvexType>
  typename std::enable_if<std::is_base_of<ConvexPointSet2f, ConvexType>::value>::type
  Fold(FoldFunctorConst accumulator, const ConvexType& region, FoldDirection dir = FoldDirection::BreadthFirst) const;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Exploration
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // find the neighbor of the same or higher level in the given direction
  const QuadTreeNode* FindSingleNeighbor(EDirection direction) const;

  // find the group of smallest neighbors with whom this node shares a border.
  // they would be children of the same level neighbor. This is normally useful when our neighbor is subdivided but
  // we are not.
  // direction: direction in which we move to find the neighbors (4 cardinals)
  // iterationDirection: when there're more than one neighbor in that direction, which one comes first in the list
  // NOTE: this method is expected to NOT clear the vector before adding neighbors
  void AddSmallestNeighbors(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& neighbors) const;
 
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // info about moving towards a neighbor
  struct MoveInfo {
    EQuadrant neighborQuadrant;  // destination quadrant
    bool sharesParent;           // whether destination quadrant is in the same parent
  };
    
  // container for each node's children
  using ChildrenVector = std::vector< std::unique_ptr<QuadTreeNode> >;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Query
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -                 
  
  // return true if this quad can subdivide
  bool CanSubdivide() const { return _level > 0; }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Modification
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // subdivide/merge children
  void Subdivide(QuadTreeProcessor& processor);
  void Merge(const MemoryMapDataPtr newContent, QuadTreeProcessor& processor);

  // checks if all children are the same type, if so it removes the children and merges back to a single parent
  void TryAutoMerge(QuadTreeProcessor& processor);
  
  // force sets the type and updates shared container
  void ForceSetDetectedContentType(const MemoryMapDataPtr detectedContent, QuadTreeProcessor& processor);
  
  // sets a new parent to this node. Used on expansions
  void ChangeParent(const QuadTreeNode* newParent) { _parent = newParent; }
  
  // swaps children and content with 'otherNode', updating the children's parent pointer
  void SwapChildrenAndContent(QuadTreeNode* otherNode, QuadTreeProcessor& processor);
  
  // read the note in destructor on why we manually destroy nodes when they are removed
  static void DestroyNodes(ChildrenVector& nodes, QuadTreeProcessor& processor);

  // run the provided accumulator function recursively over the tree for all nodes intersecting with region (if provided).
  // NOTE: mutable recursive calls should remain private to ensure tree invariants are held
  void Fold(FoldFunctor accumulator, FoldDirection dir = FoldDirection::BreadthFirst);
  
  template <class ConvexType>
  typename std::enable_if<std::is_base_of<ConvexPointSet2f, ConvexType>::value>::type
  Fold(FoldFunctor accumulator, const ConvexType& region, FoldDirection dir = FoldDirection::BreadthFirst);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Exploration
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
   
  // calculate where we would land from a quadrant if we moved in the given direction
  static const MoveInfo* GetDestination(EQuadrant from, EDirection direction);
  
  // get the child in the given quadrant, or null if this node is not subdivided
  const QuadTreeNode* GetChild(EQuadrant quadrant) const;

  // iterate until we reach the nodes that have a border in the given direction, and add them to the vector
  // NOTE: this method is expected to NOT clear the vector before adding descendants
  void AddSmallestDescendants(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& descendants) const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // NOTE: try to minimize padding in these attributes

  // children when subdivided. Can be empty or have 4 nodes
  std::vector< std::unique_ptr<QuadTreeNode> > _childrenPtr;

  // coordinates of this quad
  Point3f _center;
  float   _sideLen;

  AxisAlignedQuad _boundingBox;

  // parent node
  const QuadTreeNode* _parent;

  // our level
  uint8_t _level;

  // quadrant within the parent
  EQuadrant _quadrant;
  
  // information about what's in this quad
  NodeContent _content;
    
}; // class
  
/*
  For calls that are constrained by some convex region, first we can potnetially avoid excess collision checks
  if the current node is fully contained by the Fold Region. In the example below, nodes 1 through 6 need intersection 
  checks, but nodes A through D do not since their parent is fully contained by Fold Region
 
                    +-----------------+------------------+
                    |                 |                  |
                    |                 |                  |
                    |                 |                  |
                    |         1       |        2         |
                    |                 |                  |
                    |    . . . . . . . . .<- Fold        |
                    |    .            |  .   Region      |
                    +----+----#########--+---------------+
                    |    .    # A | B #  .               |
                    |    4    #---+---#  .               |
                    |    .    # D | C #  .               |
                    +----+----#########  .     3         |
                    |    .    |       |  .               |
                    |    6 . .|. .5. .|. .               |
                    |         |       |                  |
                    +---------+-------+------------------+

*/

template <class ConvexType>
inline typename std::enable_if<std::is_base_of<ConvexPointSet2f, ConvexType>::value>::type
QuadTreeNode::Fold(FoldFunctor accumulator, const ConvexType& region, FoldDirection dir)
{
  if ( _boundingBox.Intersects(region) )
  {    
    // check if we can stop doing overlap checks
    if ( _boundingBox.IsContainedBy(region) )
    {
      Fold(accumulator, dir);
    } else {
      if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 

      // filter out child nodes if we know the region won't intersect based off of AABB checks
      if (IsSubdivided())
      {      
        u8 childFilter = 0b1111; // Bit field represents quadrants (-x, -y), (-x, +y), (+x, -y), (+x, +y)
        
        if ( region.GetMinX() > _center.x() ) { childFilter &= 0b0011; } // only +x (top) nodes
        if ( region.GetMaxX() < _center.x() ) { childFilter &= 0b1100; } // only -x (bot) nodes
        if ( region.GetMinY() > _center.y() ) { childFilter &= 0b0101; } // only +y (left) nodes
        if ( region.GetMaxY() < _center.y() ) { childFilter &= 0b1010; } // only -y (right) nodes

        // iterate in reverse order relative to EQuadrant order to save some extra arithmetic
        u8 idx = 0;
        do
        {
          if (childFilter & 1) { 
            _childrenPtr[idx]->Fold(accumulator, region, dir); 
          }
          ++idx;
        } while ( childFilter >>= 1 ); 
      }

      if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
    }
  }
}

template <class ConvexType>
inline typename std::enable_if<std::is_base_of<ConvexPointSet2f, ConvexType>::value>::type
QuadTreeNode::Fold(FoldFunctorConst accumulator, const ConvexType& region, FoldDirection dir) const
{
  if ( _boundingBox.Intersects(region) )
  {    
    // check if we can stop doing overlap checks
    if ( _boundingBox.IsContainedBy(region) )
    {
      Fold(accumulator, dir);
    } else {
      if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 

      // filter out child nodes if we know the region won't intersect based off of AABB checks
      if (IsSubdivided())
      {      
        u8 childFilter = 0b1111; // Bit field represents quadrants (-x, -y), (-x, +y), (+x, -y), (+x, +y)
        
        if ( region.GetMinX() > _center.x() ) { childFilter &= 0b0011; } // only +x (top) nodes
        if ( region.GetMaxX() < _center.x() ) { childFilter &= 0b1100; } // only -x (bot) nodes
        if ( region.GetMinY() > _center.y() ) { childFilter &= 0b0101; } // only +y (left) nodes
        if ( region.GetMaxY() < _center.y() ) { childFilter &= 0b1010; } // only -y (right) nodes

        // iterate in reverse order relative to EQuadrant order to save some extra arithmetic
        u8 idx = 0;
        do 
        {
          if (childFilter & 1) { 
            ((const QuadTreeNode*) _childrenPtr[idx].get())->Fold(accumulator, region, dir);
           }
          ++idx;
        } while ( childFilter >>= 1 );
      }

      if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
    }
  }
}

} // namespace
} // namespace

#endif //
