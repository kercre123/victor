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
QuadTreeNode::QuadTreeNode(ParentPtr parent, EQuadrant quadrant)
: _boundingBox({0,0}, {0,0})
, _parent(parent)
, _quadrant(quadrant)
, _content(MemoryMapDataPtr())
{
  auto initFromParent = [&](auto parentCPtr) { 
    float halfLen = parentCPtr->GetSideLen() * .25f;
    _sideLen     = parentCPtr->GetSideLen() * .5f;
    _center      = parentCPtr->GetCenter() + Quadrant2Vec(_quadrant) * halfLen;
    _level       = parentCPtr->GetLevel() - 1;
    _boundingBox = AxisAlignedQuad(_center - Point2f(halfLen), _center + Point2f(halfLen));
  };
  
  _parent.FMap(initFromParent);
  ResetAddress();

  DEV_ASSERT(_quadrant <= EQuadrant::Root, "QuadTreeNode.Constructor.InvalidQuadrant");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::ResetAddress()
{
  _parent.FMap( [&](const QuadTreeNode* p) { 
    _address = NodeAddress(p->GetAddress());
    _address.push_back(_quadrant);
   });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Subdivide()
{
  if ( !CanSubdivide() ) { return; }

  ParentPtr backPtr = ParentPtr::Just(this);
  _childrenPtr.emplace_back( new QuadTreeNode(backPtr, EQuadrant::PlusXPlusY) );   // up L
  _childrenPtr.emplace_back( new QuadTreeNode(backPtr, EQuadrant::PlusXMinusY) );  // up R
  _childrenPtr.emplace_back( new QuadTreeNode(backPtr, EQuadrant::MinusXPlusY) );  // lo L
  _childrenPtr.emplace_back( new QuadTreeNode(backPtr, EQuadrant::MinusXMinusY) ); // lo E
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::MoveDataToChildren(QuadTreeProcessor& processor)
{
  if ( !IsSubdivided() || _content.data->type == EContentType::Unknown ) { return; }

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
void QuadTreeNode::Join(const MemoryMapDataPtr newData, QuadTreeProcessor& processor)
{
  DEV_ASSERT(IsSubdivided(), "QuadTreeNode.Join.InvalidState");

  // since we are going to destroy the children, notify the processor of all the descendants about to be destroyed
  DestroyNodes(_childrenPtr, processor);

  // make sure vector of children is empty to since IsSubdivided() checks this vectors length
  _childrenPtr.clear();
  
  // set our content to the one we will have after the join
  ForceSetDetectedContentType(newData, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::TryAutoMerge(QuadTreeProcessor& processor)
{
  if (!IsSubdivided()) {
    return;
  }

  // can't join if any children are subdivided
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
    
    Join( nodeData, processor );
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
    processor.OnNodeContentTypeChanged( this, oldContentType, wasEmptyType);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::SwapChildrenAndContent(QuadTreeNode* otherNode, QuadTreeProcessor& processor )
{
  // swap children
  std::swap(_childrenPtr, otherNode->_childrenPtr);

  // notify the children of the parent change
  for ( auto& childPtr : _childrenPtr ) {
    childPtr->ChangeParent( ParentPtr::Just(this) );
  }

  // notify the children of the parent change
  for ( auto& childPtr : otherNode->_childrenPtr ) {
    childPtr->ChangeParent( ParentPtr::Just(otherNode) );
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
    processor.OnNodeDestroyed( node.get() );
    node.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::MaybeCNode QuadTreeNode::GetChild(EQuadrant quadrant) const
{
  if ( IsSubdivided() && (quadrant != EQuadrant::Invalid) && (quadrant != EQuadrant::Root)) {
    return MaybeCNode::Just(_childrenPtr[(std::underlying_type<EQuadrant>::type)quadrant]);
  }
  return MaybeCNode::Nothing();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::MaybeNode QuadTreeNode::GetChild(EQuadrant quadrant)
{
  if ( IsSubdivided() && (quadrant != EQuadrant::Invalid) && (quadrant != EQuadrant::Root)) {
    return MaybeNode::Just(_childrenPtr[(std::underlying_type<EQuadrant>::type)quadrant]);
  }
  return MaybeNode::Nothing();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::MaybeNode QuadTreeNode::GetNodeAtAddress(const NodeAddress& addr)
{
  if (addr.size() > _address.size()) {
    auto nextNode = GetChild(addr[_address.size()]);
    return nextNode.Bind( [&addr] (auto& node) { return node->GetNodeAtAddress(addr); } );
  } else if (addr == _address) {
    return MaybeNode::Just(shared_from_this());
  } 
  return MaybeNode::Nothing();
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
QuadTreeNode::MaybeCNode QuadTreeNode::FindSingleNeighbor(EDirection direction) const
{
  EQuadrant destination = GetQuadrantInDirection(_quadrant, direction);
  // if stepping in the current direction keeps us under the same parent node
  if ( IsSibling(_quadrant, direction) ) {
    return _parent.Bind([&](auto node) { return node->GetChild( destination ); } );
  } 

  MaybeCNode parentNeighbor = _parent.Bind([&](auto node) { return node->FindSingleNeighbor( direction ); } );
  MaybeCNode directNeighbor = parentNeighbor.Bind( [&](auto node) { return node->GetChild( destination ); });
  return directNeighbor.ThisOr(parentNeighbor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::NodeCPtrVector QuadTreeNode::GetNeighbors() const
{
  NodeCPtrVector neighbors;
  
  FindSingleNeighbor(EDirection::North).FMap( [&](auto& node) { node->AddSmallestDescendants(EDirection::South, neighbors); });
  FindSingleNeighbor(EDirection::South).FMap( [&](auto& node) { node->AddSmallestDescendants(EDirection::North, neighbors); });
  FindSingleNeighbor(EDirection::East ).FMap( [&](auto& node) { node->AddSmallestDescendants(EDirection::West,  neighbors); });
  FindSingleNeighbor(EDirection::West ).FMap( [&](auto& node) { node->AddSmallestDescendants(EDirection::East,  neighbors); });
 
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
