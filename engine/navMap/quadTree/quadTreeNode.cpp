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
namespace Cozmo {

static_assert( !std::is_copy_assignable<QuadTreeNode>::value, "QuadTreeNode was designed non-copyable" );
static_assert( !std::is_copy_constructible<QuadTreeNode>::value, "QuadTreeNode was designed non-copyable" );
static_assert( !std::is_move_assignable<QuadTreeNode>::value, "QuadTreeNode was designed non-movable" );
static_assert( !std::is_move_constructible<QuadTreeNode>::value, "QuadTreeNode was designed non-movable" );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::QuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, QuadTreeNode* parent)
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
    processor.OnNodeContentTypeChanged(this, oldContentType, wasEmptyType);
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
    processor.OnNodeDestroyed( node.get() );
    node.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const QuadTreeNode::MoveInfo* QuadTreeNode::GetDestination(EQuadrant from, EDirection direction)
{
  static MoveInfo quadrantAndDirection[4][4] =
  {
    {
    /*quadrantAndDirection[EQuadrant::PlusXPlusY][EDirection::North]  =*/ {EQuadrant::MinusXPlusY , false},
    /*quadrantAndDirection[EQuadrant::PlusXPlusY][EDirection::East ]  =*/ {EQuadrant::PlusXMinusY, true },
    /*quadrantAndDirection[EQuadrant::PlusXPlusY][EDirection::South]  =*/ {EQuadrant::MinusXPlusY , true },
    /*quadrantAndDirection[EQuadrant::PlusXPlusY][EDirection::West ]  =*/ {EQuadrant::PlusXMinusY, false}
    },

    {
    /*quadrantAndDirection[EQuadrant::PlusXMinusY][EDirection::North] =*/ {EQuadrant::MinusXMinusY, false},
    /*quadrantAndDirection[EQuadrant::PlusXMinusY][EDirection::East ] =*/ {EQuadrant::PlusXPlusY , false},
    /*quadrantAndDirection[EQuadrant::PlusXMinusY][EDirection::South] =*/ {EQuadrant::MinusXMinusY, true},
    /*quadrantAndDirection[EQuadrant::PlusXMinusY][EDirection::West ] =*/ {EQuadrant::PlusXPlusY , true}
    },

    {
    /*quadrantAndDirection[EQuadrant::MinusXPlusY][EDirection::North]  =*/ {EQuadrant::PlusXPlusY , true},
    /*quadrantAndDirection[EQuadrant::MinusXPlusY][EDirection::East ]  =*/ {EQuadrant::MinusXMinusY, true},
    /*quadrantAndDirection[EQuadrant::MinusXPlusY][EDirection::South]  =*/ {EQuadrant::PlusXPlusY , false},
    /*quadrantAndDirection[EQuadrant::MinusXPlusY][EDirection::West ]  =*/ {EQuadrant::MinusXMinusY, false}
    },

    {
    /*quadrantAndDirection[EQuadrant::MinusXMinusY][EDirection::North] =*/ {EQuadrant::PlusXMinusY, true},
    /*quadrantAndDirection[EQuadrant::MinusXMinusY][EDirection::East ] =*/ {EQuadrant::MinusXPlusY , false},
    /*quadrantAndDirection[EQuadrant::MinusXMinusY][EDirection::South] =*/ {EQuadrant::PlusXMinusY, false},
    /*quadrantAndDirection[EQuadrant::MinusXMinusY][EDirection::West ] =*/ {EQuadrant::MinusXPlusY , true}
    }
  };
  
  DEV_ASSERT(from <= EQuadrant::Root, "QuadTreeNode.GetDestination.InvalidQuadrant");
  DEV_ASSERT(direction <= EDirection::West, "QuadTreeNode.GetDestination.InvalidDirection");
  
  // root can't move, for any other, apply the table
  const size_t fromIdx = std::underlying_type<EQuadrant>::type( from );
  const size_t dirIdx  = std::underlying_type<EDirection>::type( direction );
  const MoveInfo* ret = ( from == EQuadrant::Root ) ? nullptr : &quadrantAndDirection[fromIdx][dirIdx];
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const QuadTreeNode* QuadTreeNode::GetChild(EQuadrant quadrant) const
{
  const QuadTreeNode* ret =
    ( _childrenPtr.empty() ) ?
    ( nullptr ) :
    ( _childrenPtr[(std::underlying_type<EQuadrant>::type)quadrant].get() );
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::AddSmallestDescendants(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& descendants) const
{
  if ( !IsSubdivided() ) {
    descendants.push_back( this );
  } else {
  
    // depending on CW vs CCW, we iterate children in opposite orders
    const bool isCW = iterationDirection == EClockDirection::CW;
    EQuadrant firstChild = EQuadrant::Invalid;
    EQuadrant secondChild = EQuadrant::Invalid;
  
    switch (direction) {
      case EDirection::North:
      {
        firstChild  = isCW ? EQuadrant::PlusXPlusY  : EQuadrant::PlusXMinusY;
        secondChild = isCW ? EQuadrant::PlusXMinusY : EQuadrant::PlusXPlusY;
      }
      break;
      case EDirection::East:
      {
        firstChild  = isCW ? EQuadrant::PlusXMinusY : EQuadrant::MinusXMinusY;
        secondChild = isCW ? EQuadrant::MinusXMinusY : EQuadrant::PlusXMinusY;
      }
      break;
      case EDirection::South:
      {
        firstChild  = isCW ? EQuadrant::MinusXMinusY : EQuadrant::MinusXPlusY;
        secondChild = isCW ? EQuadrant::MinusXPlusY  : EQuadrant::MinusXMinusY;
      }
      break;
      case EDirection::West:
      {
        firstChild  = isCW ? EQuadrant::MinusXPlusY : EQuadrant::PlusXPlusY;
        secondChild = isCW ? EQuadrant::PlusXPlusY : EQuadrant::MinusXPlusY;
      }
      break;
      case EDirection::Invalid:
      {
        DEV_ASSERT(false, "QuadTreeNode.AddSmallDescendants.InvalidDirection");
      }
    }
    
    DEV_ASSERT(firstChild != EQuadrant::Invalid, "QuadTreeNode.AddSmallDescendants.InvalidFirstChild");
    DEV_ASSERT(secondChild != EQuadrant::Invalid, "QuadTreeNode.AddSmallDescendants.InvalidSecondChild");
    
    _childrenPtr[(std::underlying_type<EQuadrant>::type)firstChild ]->AddSmallestDescendants(direction, iterationDirection, descendants);
    _childrenPtr[(std::underlying_type<EQuadrant>::type)secondChild]->AddSmallestDescendants(direction, iterationDirection, descendants);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const QuadTreeNode* QuadTreeNode::FindSingleNeighbor(EDirection direction) const
{
  const QuadTreeNode* neighbor = nullptr;

  // find where we land by moving in that direction
  const MoveInfo* moveInfo = GetDestination(_quadrant, direction);
  if ( moveInfo != nullptr )
  {
    // check if it's the same parent
    if ( moveInfo->sharesParent )
    {
      // if so, it's a sibling
      DEV_ASSERT(_parent, "QuadTreeNode.FindSingleNeighbor.InvalidParent");
      neighbor = _parent->GetChild( moveInfo->neighborQuadrant );
      DEV_ASSERT(neighbor, "QuadTreeNode.FindSingleNeighbor.InvalidNeighbor");
    }
    else
    {
      // otherwise, find our parent's neighbor and get the proper child that would be next to us
      // note our parent can return null if we are on the border
      const QuadTreeNode* parentNeighbor = _parent->FindSingleNeighbor(direction);
      neighbor = parentNeighbor ? parentNeighbor->GetChild( moveInfo->neighborQuadrant ) : nullptr;
      // if the parentNeighbor was not subdivided, then he is our neighbor
      neighbor = neighbor ? neighbor : parentNeighbor;
    }
  }
  
  return neighbor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::AddSmallestNeighbors(EDirection direction,
  EClockDirection iterationDirection, NodeCPtrVector& neighbors) const
{
  const QuadTreeNode* firstNeighbor = FindSingleNeighbor(direction);
  if ( nullptr != firstNeighbor )
  {
    // direction and iterationDirection are with respect to the node, but the descendants with respect
    // to the neighbor are opposite.
    // For example, if we want my smallest neighbors to the North in CW direction, I ask my northern
    // same level neighbor to give me its Southern descendants in CCW direction.
    EDirection descendantDir = GetOppositeDirection(direction);
    EClockDirection descendantClockDir = GetOppositeClockDirection(iterationDirection);
    firstNeighbor->AddSmallestDescendants( descendantDir, descendantClockDir, neighbors);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Fold Implementations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Fold(FoldFunctor accumulator, FoldDirection dir)
{
  if (FoldDirection::BreadthFirst == dir) { accumulator(*this); }

  for ( auto& cPtr : _childrenPtr )
  {
    if (cPtr) cPtr->Fold(accumulator, dir);
  }

  if (FoldDirection::DepthFirst == dir) { accumulator(*this); }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Fold(FoldFunctorConst accumulator, FoldDirection dir) const
{ 
  if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 
  
  for ( const auto& cPtr : _childrenPtr )
  {
    // disambiguate const method call
    const QuadTreeNode* constPtr = cPtr.get();
    constPtr->Fold(accumulator, dir);
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

void QuadTreeNode::Fold(FoldFunctor accumulator, const BoundedConvexSet2f& region, FoldDirection dir)
{
  if ( region.Intersects(_boundingBox) )  {    
    // check if we can stop doing overlap checks
    if ( region.ContainsAll(_boundingBox.GetVertices()) ) {
      Fold(accumulator, dir);
    } else {
      if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 

      // filter out child nodes if we know the region won't intersect based off of AABB checks
      if ( IsSubdivided() ) {      
        u8 childFilter = 0b1111; // Bit field represents quadrants (-x, -y), (-x, +y), (+x, -y), (+x, +y)

        const AxisAlignedQuad& aabb = region.GetAxisAlignedBoundingBox();
        if ( aabb.GetMinVertex().x() > _center.x() ) { childFilter &= 0b0011; } // only +x (top) nodes
        if ( aabb.GetMaxVertex().x() < _center.x() ) { childFilter &= 0b1100; } // only -x (bot) nodes
        if ( aabb.GetMinVertex().y() > _center.y() ) { childFilter &= 0b0101; } // only +y (left) nodes
        if ( aabb.GetMaxVertex().y() < _center.y() ) { childFilter &= 0b1010; } // only -y (right) nodes

        // iterate in reverse order relative to EQuadrant order to save some extra arithmetic
        u8 idx = 0;
        do {
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

void QuadTreeNode::Fold(FoldFunctorConst accumulator, const BoundedConvexSet2f& region, FoldDirection dir) const
{
  if ( region.Intersects(_boundingBox) ) {    
    // check if we can stop doing overlap checks
    if ( region.ContainsAll(_boundingBox.GetVertices()) ) {
      Fold(accumulator, dir);
    } else {
      if (FoldDirection::BreadthFirst == dir) { accumulator(*this); } 

      // filter out child nodes if we know the region won't intersect based off of AABB checks
      if ( IsSubdivided() ) {      
        u8 childFilter = 0b1111; // Bit field represents quadrants (-x, -y), (-x, +y), (+x, -y), (+x, +y)
        
        const AxisAlignedQuad& aabb = region.GetAxisAlignedBoundingBox();
        if ( aabb.GetMinVertex().x() > _center.x() ) { childFilter &= 0b0011; } // only +x (top) nodes
        if ( aabb.GetMaxVertex().x() < _center.x() ) { childFilter &= 0b1100; } // only -x (bot) nodes
        if ( aabb.GetMinVertex().y() > _center.y() ) { childFilter &= 0b0101; } // only +y (left) nodes
        if ( aabb.GetMaxVertex().y() < _center.y() ) { childFilter &= 0b1010; } // only -y (right) nodes

        // iterate in reverse order relative to EQuadrant order to save some extra arithmetic
        u8 idx = 0;
        do {
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

} // namespace Cozmo
} // namespace Anki
