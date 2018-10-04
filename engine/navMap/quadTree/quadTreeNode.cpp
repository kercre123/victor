/**
 * File: quadTreeNode.cpp
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
#include "quadTreeNode.h"
#include "quadTreeProcessor.h"

#include "coretech/common/engine/math/point_impl.h"

#include "util/math/math.h"
#include "util/cpuProfiler/cpuProfiler.h"

#include <unordered_map>
#include <limits>
#include <algorithm>

namespace Anki {
namespace Vector {

static_assert( !std::is_copy_assignable<QuadTreeNode>::value, "QuadTreeNode was designed non-copyable" );
static_assert( !std::is_copy_constructible<QuadTreeNode>::value, "QuadTreeNode was designed non-copyable" );
static_assert( !std::is_move_assignable<QuadTreeNode>::value, "QuadTreeNode was designed non-movable" );
static_assert( !std::is_move_constructible<QuadTreeNode>::value, "QuadTreeNode was designed non-movable" );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::QuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, ParentPtr parent)
: _center(center)
, _sideLen(sideLength)
, _boundingBox(center - Point3f(sideLength/2, sideLength/2, 0), center + Point3f(sideLength/2, sideLength/2, 0))
, _parent(parent)
, _level(level)
, _quadrant(quadrant)
, _content(MemoryMapDataPtr())
{
  DEV_ASSERT(_quadrant <= EQuadrant::Root, "QuadTreeNode.Constructor.InvalidQuadrant");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Subdivide(QuadTreeProcessor& processor)
{
  if ( !CanSubdivide() ) { return; }
  
  const float halfLen    = _sideLen * 0.50f;
  const float quarterLen = halfLen * 0.50f;
  const uint8_t cLevel = _level-1;

  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::PlusXPlusY , this) ); // up L
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::PlusXMinusY, this) ); // up R
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::MinusXPlusY , this) ); // lo L
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::MinusXMinusY, this) ); // lo E

  // our children may change later on, but until they do, assume they have our old content
  for ( auto& childPtr : _childrenPtr )
  {
    // use ForceContentType to make sure the processor is notified since the QTN constructor does not notify the processor
    childPtr->ForceSetDetectedContentType(_content.data, processor);
  }
  // clear the subdivided node content
  ForceSetDetectedContentType(MemoryMapDataPtr(), processor);

  // reset timestamps since they were cleared
  _content.data->SetFirstObservedTime(_childrenPtr[0]->GetData()->GetFirstObservedTime());
  _content.data->SetLastObservedTime( _childrenPtr[0]->GetData()->GetLastObservedTime());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Merge(const MemoryMapDataPtr newData, QuadTreeProcessor& processor)
{
  DEV_ASSERT(IsSubdivided(), "QuadTreeNode.Merge.InvalidState");

  // since we are going to destroy the children, notify the processor of all the descendants about to be destroyed
  DestroyNodes(_childrenPtr, processor);

  // make sure vector of children is empty to since IsSubdivided() checks this vectors length
  _childrenPtr.clear();
  
  // set our content to the one we will have after the merge
  ForceSetDetectedContentType(newData, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::TryAutoMerge(QuadTreeProcessor& processor)
{
  if (!IsSubdivided()) {
    return;
  }

  // can't merge if any children are subdivided
  for (const auto& child : _childrenPtr) {
    if ( child->IsSubdivided() ) {
      return;
    }
  }
  
  bool allChildrenEqual = true;
  
  // check if all children classified the same content (assumes node content equality is transitive)
  for(size_t i=0; i<_childrenPtr.size()-1; ++i)
  {
    allChildrenEqual &= (_childrenPtr[i]->GetContent() == _childrenPtr[i+1]->GetContent());
  }
  
  // we can merge and set that type on this parent
  if ( allChildrenEqual )
  {
    MemoryMapDataPtr nodeData = _childrenPtr[0]->GetData(); // do a copy since merging will destroy children
    nodeData->SetFirstObservedTime(GetData()->GetFirstObservedTime());
    nodeData->SetLastObservedTime(GetData()->GetLastObservedTime());
    
    Merge( nodeData, processor );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::ForceSetDetectedContentType(const MemoryMapDataPtr newData, QuadTreeProcessor& processor)
{
  const EContentType oldContentType = _content.data->type;
  const bool wasEmptyType = IsEmptyType();
  
  // this is where we can detect changes in content, for example new obstacles or things disappearing
  _content.data = newData;
  
  // notify processor only when content type changes, not if the underlaying info changes
  const bool typeChanged = oldContentType != _content.data->type;
  if ( typeChanged ) {
    // we no longer check if if the type changes from Invalid or Subdivided in the processor, 
    // so we should move the check here.
    processor.OnNodeContentTypeChanged( shared_from_this(), oldContentType, wasEmptyType);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::SwapChildrenAndContent(QuadTreeNode* otherNode, QuadTreeProcessor& processor )
{
  // swap children
  std::swap(_childrenPtr, otherNode->_childrenPtr);

  // notify the children of the parent change
  for ( auto& childPtr : _childrenPtr ) {
    childPtr->ChangeParent( this );
  }

  // notify the children of the parent change
  for ( auto& childPtr : otherNode->_childrenPtr ) {
    childPtr->ChangeParent( otherNode );
  }

  // swap contents by use of copy, since changes have to be notified to the processor
  MemoryMapDataPtr myPrevContent = _content.data;
  ForceSetDetectedContentType(otherNode->GetData(), processor);
  otherNode->ForceSetDetectedContentType(myPrevContent, processor);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::DestroyNodes(ChildrenVector& nodes, QuadTreeProcessor& processor)
{
  // iterate all nodes in vector
  for( auto& node : nodes )
  {
    // destroying its children
    DestroyNodes( node->_childrenPtr, processor );
    // and then itself
    processor.OnNodeDestroyed( std::const_pointer_cast<const QuadTreeNode>(node) );
    node.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::Maybe<QuadTreeNode::NodeCPtr> QuadTreeNode::GetChild(EQuadrant quadrant) const
{
  if ( IsSubdivided() ) {
    return std::const_pointer_cast<const QuadTreeNode>( _childrenPtr[(std::underlying_type<EQuadrant>::type)quadrant] );
  }
  return {};
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::AddSmallestDescendants(EDirection direction, NodeCPtrVector& descendants) const
{
  if ( !IsSubdivided() ) {
    descendants.emplace_back( this );
  } else {
    for (const auto& child : _childrenPtr) {
      if(!IsSibling(child->_quadrant, direction)) { child->AddSmallestDescendants(direction, descendants); };
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::Maybe<QuadTreeNode::NodeCPtr> QuadTreeNode::FindSingleNeighbor(EDirection direction) const
{
  EQuadrant destination = GetQuadrantInDirection(_quadrant, direction);
  // if stepping in the current direction keeps us under the same parent node
  if ( IsSibling(_quadrant, direction) ) {
    return _parent.bind([&](auto node) { return node->GetChild( destination ); } );
  } 

  Util::Maybe<NodeCPtr> parentNeighbor = _parent.bind([&](auto node) { return node->FindSingleNeighbor( direction ); } );
  Util::Maybe<NodeCPtr> directNeighbor = parentNeighbor.bind( [&](auto node) { return node->GetChild( destination ); });
  return directNeighbor.Or(parentNeighbor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::NodeCPtrVector QuadTreeNode::GetNeighbors() const
{
  NodeCPtrVector neighbors;
  
  FindSingleNeighbor(EDirection::North).fmap( [&](auto& node) { node->AddSmallestDescendants(EDirection::South, neighbors); });
  FindSingleNeighbor(EDirection::South).fmap( [&](auto& node) { node->AddSmallestDescendants(EDirection::North, neighbors); });
  FindSingleNeighbor(EDirection::East ).fmap( [&](auto& node) { node->AddSmallestDescendants(EDirection::West,  neighbors); });
  FindSingleNeighbor(EDirection::West ).fmap( [&](auto& node) { node->AddSmallestDescendants(EDirection::East,  neighbors); });
 
  return neighbors;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Fold Implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

namespace {
  // helper to make sure that we have a valid pointer if we are performing multithreaded operations
  template<class T>
  std::shared_ptr<T> PreservePointer(const std::shared_ptr<T>& ptr) {
    const std::weak_ptr<T> weak = ptr;
    return weak.lock();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Fold(FoldFunctor accumulator, FoldDirection dir)
{
  if (FoldDirection::BreadthFirst == dir) { accumulator(*this); }

  for ( auto& cPtr : _childrenPtr ) {
    if ( auto observe = PreservePointer<QuadTreeNode>(cPtr) ) { observe->Fold(accumulator, dir); }
  }

  if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Fold(FoldFunctorConst accumulator, FoldDirection dir) const
{ 
  if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 
  
  for ( const auto& cPtr : _childrenPtr )  {
    if ( auto observe = PreservePointer<const QuadTreeNode>(cPtr) ) { observe->Fold(accumulator, dir); }
  }
  
  if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
}

/*
  For calls that are constrained by some convex region, first we can potentially avoid excess collision checks
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

void QuadTreeNode::Fold(FoldFunctor accumulator, const FoldableRegion& region, FoldDirection dir)
{
  if ( region.IntersectsQuad(_boundingBox) )  {    
    // check if we can stop doing overlap checks
    if ( region.ContainsQuad(_boundingBox) ) {
      Fold(accumulator, dir);
    } else {
      if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 

      // filter out child nodes if we know the region won't intersect based off of AABB checks
      if ( IsSubdivided() ) {      
        u8 childFilter = 0b1111; // Bit field represents quadrants (-x, -y), (-x, +y), (+x, -y), (+x, +y)

        const AxisAlignedQuad& aabb = region.GetBoundingBox();
        if ( aabb.GetMinVertex().x() > _center.x() ) { childFilter &= 0b0011; } // only +x (top) nodes
        if ( aabb.GetMaxVertex().x() < _center.x() ) { childFilter &= 0b1100; } // only -x (bot) nodes
        if ( aabb.GetMinVertex().y() > _center.y() ) { childFilter &= 0b0101; } // only +y (left) nodes
        if ( aabb.GetMaxVertex().y() < _center.y() ) { childFilter &= 0b1010; } // only -y (right) nodes

        // iterate in reverse order relative to EQuadrant order to save some extra arithmetic
        u8 idx = 0;
        do {
          if (childFilter & 1) { 
            if ( auto observe = PreservePointer<QuadTreeNode>(_childrenPtr[idx]) ) { 
              observe->Fold(accumulator, region, dir);
            }
          }
          ++idx;
        } while ( childFilter >>= 1 ); 
      }

      if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
    }
  }
}

void QuadTreeNode::Fold(FoldFunctorConst accumulator, const FoldableRegion& region, FoldDirection dir) const
{
  if ( region.IntersectsQuad(_boundingBox) ) {    
    // check if we can stop doing overlap checks
    if ( region.ContainsQuad(_boundingBox) ) {
      Fold(accumulator, dir);
    } else {
      if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 

      // filter out child nodes if we know the region won't intersect based off of AABB checks
      if ( IsSubdivided() ) {      
        u8 childFilter = 0b1111; // Bit field represents quadrants (-x, -y), (-x, +y), (+x, -y), (+x, +y)
        
        const AxisAlignedQuad& aabb = region.GetBoundingBox();
        if ( aabb.GetMinVertex().x() > _center.x() ) { childFilter &= 0b0011; } // only +x (top) nodes
        if ( aabb.GetMaxVertex().x() < _center.x() ) { childFilter &= 0b1100; } // only -x (bot) nodes
        if ( aabb.GetMinVertex().y() > _center.y() ) { childFilter &= 0b0101; } // only +y (left) nodes
        if ( aabb.GetMaxVertex().y() < _center.y() ) { childFilter &= 0b1010; } // only -y (right) nodes

        // iterate in reverse order relative to EQuadrant order to save some extra arithmetic
        u8 idx = 0;
        do {
          if (childFilter & 1) { 
            if ( auto observe = PreservePointer<const QuadTreeNode>(_childrenPtr[idx]) ) { 
              observe->Fold(accumulator, region, dir); 
            }
          }
          ++idx;
        } while ( childFilter >>= 1 );
      }

      if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
    }
  }
}

} // namespace Vector
} // namespace Anki
