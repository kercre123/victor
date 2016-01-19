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
#include "navMeshQuadTreeNode.h"
#include "navMeshQuadTreeProcessor.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "util/math/constantsAndMacros.h"

#include <unordered_map>
#include <limits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeNode::NavMeshQuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, NavMeshQuadTreeNode* parent)
: _center(center)
, _sideLen(sideLength)
, _parent(parent)
, _level(level)
, _quadrant(quadrant)
, _contentType(EContentType::Unknown)
{
  CORETECH_ASSERT(_quadrant <= EQuadrant::Root);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::Contains(const Quad2f& quad) const
{
  const bool ret = MakeQuadXY().Contains(quad);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Quad3f NavMeshQuadTreeNode::MakeQuad() const
{
  const float halfLen = _sideLen * 0.5f;
  Quad3f ret
  {
    {_center.x()+halfLen, _center.y()+halfLen, _center.z()}, // up L
    {_center.x()-halfLen, _center.y()+halfLen, _center.z()}, // lo L
    {_center.x()+halfLen, _center.y()-halfLen, _center.z()}, // up R
    {_center.x()-halfLen, _center.y()-halfLen, _center.z()}  // lo R
  };
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Quad2f NavMeshQuadTreeNode::MakeQuadXY() const
{
  const float halfLen = _sideLen * 0.5f;
  Quad2f ret
  {
    {_center.x()+halfLen, _center.y()+halfLen}, // up L
    {_center.x()-halfLen, _center.y()+halfLen}, // lo L
    {_center.x()+halfLen, _center.y()-halfLen}, // up R
    {_center.x()-halfLen, _center.y()-halfLen}  // lo R
  };
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddClearQuad(const Quad2f& quad, NavMeshQuadTreeProcessor& processor)
{
  const bool changed = AddQuad(quad, EContentType::Clear, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddObstacleCube(const Quad2f& quad, NavMeshQuadTreeProcessor& processor)
{
  const bool changed = AddQuad(quad, EContentType::ObstacleCube, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddObstacleUnrecognized(const Quad2f& quad, NavMeshQuadTreeProcessor& processor)
{
  const bool changed = AddQuad(quad, EContentType::ObstacleUnrecognized, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddCliff(const Quad2f &quad, NavMeshQuadTreeProcessor& processor)
{
  const bool changed = AddQuad(quad, EContentType::Cliff, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::UpgradeRootLevel(const Point2f& direction, NavMeshQuadTreeProcessor& processor)
{
  CORETECH_ASSERT( !NEAR_ZERO(direction.x()) && !NEAR_ZERO(direction.y()) );
  
  // reached expansion limit
  if ( _level == std::numeric_limits<uint8_t>::max() ) {
    return false;
  }

  // save my old children to store in the child that is taking my spot
  std::vector<NavMeshQuadTreeNode> oldChildren;
  std::swap(oldChildren, _children);

  const bool xPlus = FLT_GTR_ZERO(direction.x());
  const bool yPlus = FLT_GTR_ZERO(direction.y());
  
  // move to its new center
  const float oldHalfLen = _sideLen * 0.50f;
  _center.x() = _center.x() + (xPlus ? oldHalfLen : -oldHalfLen);
  _center.y() = _center.y() + (yPlus ? oldHalfLen : -oldHalfLen);

  // create new children
  _children.emplace_back( Point3f{_center.x()+oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::TopLeft , this ); // up L
  _children.emplace_back( Point3f{_center.x()+oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::TopRight, this ); // up R
  _children.emplace_back( Point3f{_center.x()-oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::BotLeft , this ); // lo L
  _children.emplace_back( Point3f{_center.x()-oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::BotRight, this ); // lo R

  // calculate the child that takes my place by using the opposite direction to expansion
  size_t childIdx = 0;
  if      ( !xPlus &&  yPlus ) { childIdx = 1; }
  else if (  xPlus && !yPlus ) { childIdx = 2; }
  else if (  xPlus &&  yPlus ) { childIdx = 3; }
  NavMeshQuadTreeNode& childTakingMyPlace = _children[childIdx];
  
  // set the new parent in my old children
  for ( auto& child : oldChildren ) {
    child.ChangeParent( &childTakingMyPlace );
  }
  
  // swap children with the temp
  std::swap(childTakingMyPlace._children, oldChildren);

  // set the content type I had in the child that takes my place
  childTakingMyPlace.ForceSetDetectedContentType( _contentType, processor );
  ForceSetDetectedContentType( EContentType::Subdivided, processor );
  
  // upgrade my remaining stats
  _sideLen = _sideLen * 2.0f;
  ++_level;
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddQuadsToDraw(VizManager::SimpleQuadVector& quadVector) const
{
  // if we have children, delegate on them, otherwise render ourselves
  if ( _children.empty() )
  {
    ColorRGBA color =  Anki::NamedColors::WHITE;
    switch(_contentType)
    {
      case EContentType::Subdivided          : { color = Anki::NamedColors::BLUE;     color.SetAlpha(1.0f); break; }
      case EContentType::Unknown             : { color = Anki::NamedColors::DARKGRAY; color.SetAlpha(0.2f); break; }
      case EContentType::Cliff               : { color = Anki::NamedColors::BLACK;    color.SetAlpha(0.8f); break; }
      case EContentType::Clear               : { color = Anki::NamedColors::GREEN;    color.SetAlpha(0.2f); break; }
      case EContentType::ObstacleCube        : { color = Anki::NamedColors::RED;      color.SetAlpha(0.5f); break; }
      case EContentType::ObstacleUnrecognized: { color = Anki::NamedColors::MAGENTA;  color.SetAlpha(0.5f); break; }
    }
    //quadVector.emplace_back(VizManager::MakeSimpleQuad(color, Point3f{_center.x(), _center.y(), _center.z()+_level*100}, _sideLen));
    quadVector.emplace_back(VizManager::MakeSimpleQuad(color, _center, _sideLen));
  }
  else
  {
    // delegate on each child
    for( const auto& child : _children ) {
      child.AddQuadsToDraw(quadVector);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::Subdivide(NavMeshQuadTreeProcessor& processor)
{
  CORETECH_ASSERT(CanSubdivide() && !IsSubdivided());
  CORETECH_ASSERT(_level > 0);
  
  const float halfLen    = _sideLen * 0.50f;
  const float quarterLen = halfLen * 0.50f;
  const uint8_t cLevel = _level-1;
  _children.emplace_back( Point3f{_center.x()+quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::TopLeft , this ); // up L
  _children.emplace_back( Point3f{_center.x()+quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::TopRight, this ); // up R
  _children.emplace_back( Point3f{_center.x()-quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::BotLeft , this ); // lo L
  _children.emplace_back( Point3f{_center.x()-quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::BotRight, this ); // lo E

  // our children may change later on, but until they do, assume they have our old content
  for ( auto& child : _children )
  {
    child.ForceSetDetectedContentType(_contentType, processor);
  }
  
  // set our content type to subdivided
  ForceSetDetectedContentType(EContentType::Subdivided, processor);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::Merge(EContentType newContentType, NavMeshQuadTreeProcessor& processor)
{
  ASSERT_NAMED(IsSubdivided(), "NavMeshQuadTreeNode.Merge.InvalidState");
  
  // since we are going to destroy the children, notify the processor
  for ( auto& child : _children ) {
    processor.OnNodeDestroyed(&child);
  }
  
  // now remove children
  _children.clear();
  
  // set our content type to the one we will have after the merge
  ForceSetDetectedContentType(newContentType, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::CanOverrideChildrenWithContent(EContentType contentType) const
{
  // cliff can override any other
  if ( contentType == EContentType::Cliff ) {
    return true;
  }

  // if the node is a cliff, it can't be overridden
  if ( _contentType == EContentType::Cliff ) {
    return false;
  }

  // ask children if they can
  for ( const auto& child : _children )
  {
    const bool canOverrideChild = child.CanOverrideChildrenWithContent( contentType );
    if ( !canOverrideChild ) {
      return false;
    }
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::TryAutoMerge(NavMeshQuadTreeProcessor& processor)
{
  CORETECH_ASSERT(IsSubdivided());

  // check if all children classified the same content type
  EContentType childType = _children[0].GetContentType();
  if ( childType == EContentType::Subdivided ) {
    // any subdivided quad prevents the parent from merging
    return;
  }
  
  bool allChildrenEqual = true;
  for(size_t idx1=0; idx1<_children.size()-1; ++idx1)
  {
    for(size_t idx2=idx1+1; idx2<_children.size(); ++idx2)
    {
      if ( _children[idx1].GetContentType() != _children[idx2].GetContentType() )
      {
        allChildrenEqual = false;
        break;
      }
    }
  }
  
  // if they did, we can merge and set that type on this parent
  if ( allChildrenEqual )
  {
    Merge( childType, processor );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddQuad(const Quad2f &quad, EContentType detectedContentType, NavMeshQuadTreeProcessor& processor)
{
  // we need to subdivide or fit within the given quad, since this implementation doesn't keep partial
  // info within nodes, that's what subquads would be for.
  ASSERT_NAMED( CanSubdivide() ||
                ( FLT_LE(_sideLen,(quad.GetMaxX()-quad.GetMinX())) && (FLT_LE(_sideLen,quad.GetMaxY()-quad.GetMinY())) ),
                "NavMeshQuadTreeNode.AddQuad.InputQuadTooSmall");

  // if we are already of the expected type, we won't gain any info, no need to process
  if ( _contentType == detectedContentType ) {
    return false;
  }
  
  // to check for changes
  EContentType previousType = _contentType;
  bool childChanged = false;

  // check if the quad affects us
  const Quad2f& myQuad = MakeQuadXY();
  if ( myQuad.Intersects(quad) )
  {
    // am I fully contained within the quad and can override my children? that would allow merging
    if ( quad.Contains(myQuad) && CanOverrideChildrenWithContent(detectedContentType) )
    {
      // merge if subdivided
      if ( IsSubdivided() )
      {
        // merge to the new content type, we already made sure we can override the type
        Merge( detectedContentType, processor );
      }
      else
      {
        // we are the new type (if we can override, which we should since we checked for override)
        TrySetDetectedContentType( detectedContentType, processor );
      }
    }
    else
    {
      // see if we can subdivide
      const bool wasSubdivided = IsSubdivided();
      if ( !wasSubdivided && CanSubdivide() )
      {
        // do now
        Subdivide( processor );
      }
      
      // if we have children, delegate on them
      const bool isSubdivided = IsSubdivided();
      if ( isSubdivided )
      {
        // ask children to add quad
        for( auto& child : _children ) {
          childChanged = child.AddQuad(quad, detectedContentType, processor) || childChanged;
        }
        
        // try to automerge (if it does, our content type will change from subdivided to the merged type)
        TryAutoMerge(processor);
      }
      else
      {
        TrySetDetectedContentType(detectedContentType, processor);
      }
    }
  }
  
  const bool ret = (_contentType != previousType) || childChanged;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::TrySetDetectedContentType(EContentType detectedContentType, NavMeshQuadTreeProcessor& processor)
{
  // This is here temporarily to prevent Clear from overriding Cliffs. I need to think how we want to support
  // cliffs directly under Cozmo
  if ( _contentType == EContentType::Cliff )
  {
    return;
  }

  // do the change
  ForceSetDetectedContentType(detectedContentType, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::ForceSetDetectedContentType(EContentType detectedContentType, NavMeshQuadTreeProcessor& processor)
{
  if ( _contentType != detectedContentType )
  {
    const EContentType oldcontent = _contentType;
    
    // this is where we can detect changes in content, for example new obstacles or things disappearing
    _contentType = detectedContentType;
    
    // notify the processor
    processor.OnNodeContentTypeChanged(this, oldcontent, _contentType);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const NavMeshQuadTreeNode::MoveInfo* NavMeshQuadTreeNode::GetDestination(EQuadrant from, EDirection direction)
{
  static MoveInfo quadrantAndDirection[4][4] =
  {
    {
    /*quadrantAndDirection[EQuadrant::TopLeft][EDirection::North]  =*/ {EQuadrant::BotLeft , false},
    /*quadrantAndDirection[EQuadrant::TopLeft][EDirection::East ]  =*/ {EQuadrant::TopRight, true },
    /*quadrantAndDirection[EQuadrant::TopLeft][EDirection::South]  =*/ {EQuadrant::BotLeft , true },
    /*quadrantAndDirection[EQuadrant::TopLeft][EDirection::West ]  =*/ {EQuadrant::TopRight, false}
    },

    {
    /*quadrantAndDirection[EQuadrant::TopRight][EDirection::North] =*/ {EQuadrant::BotRight, false},
    /*quadrantAndDirection[EQuadrant::TopRight][EDirection::East ] =*/ {EQuadrant::TopLeft , false},
    /*quadrantAndDirection[EQuadrant::TopRight][EDirection::South] =*/ {EQuadrant::BotRight, true},
    /*quadrantAndDirection[EQuadrant::TopRight][EDirection::West ] =*/ {EQuadrant::TopLeft , true}
    },

    {
    /*quadrantAndDirection[EQuadrant::BotLeft][EDirection::North]  =*/ {EQuadrant::TopLeft , true},
    /*quadrantAndDirection[EQuadrant::BotLeft][EDirection::East ]  =*/ {EQuadrant::BotRight, true},
    /*quadrantAndDirection[EQuadrant::BotLeft][EDirection::South]  =*/ {EQuadrant::TopLeft , false},
    /*quadrantAndDirection[EQuadrant::BotLeft][EDirection::West ]  =*/ {EQuadrant::BotRight, false}
    },

    {
    /*quadrantAndDirection[EQuadrant::BotRight][EDirection::North] =*/ {EQuadrant::TopRight, true},
    /*quadrantAndDirection[EQuadrant::BotRight][EDirection::East ] =*/ {EQuadrant::BotLeft , false},
    /*quadrantAndDirection[EQuadrant::BotRight][EDirection::South] =*/ {EQuadrant::TopRight, false},
    /*quadrantAndDirection[EQuadrant::BotRight][EDirection::West ] =*/ {EQuadrant::BotLeft , true}
    }
  };
  
  CORETECH_ASSERT( from <= EQuadrant::Root && direction <= EDirection::West );
  
  // root can't move, for any other, apply the table
  const size_t fromIdx = std::underlying_type<EQuadrant>::type( from );
  const size_t dirIdx  = std::underlying_type<EDirection>::type( direction );
  const MoveInfo* ret = ( from == EQuadrant::Root ) ? nullptr : &quadrantAndDirection[fromIdx][dirIdx];
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const NavMeshQuadTreeNode* NavMeshQuadTreeNode::GetChild(EQuadrant quadrant) const
{
  const NavMeshQuadTreeNode* ret =
    ( _children.empty() ) ?
    ( nullptr ) :
    ( &_children[(std::underlying_type<EQuadrant>::type)quadrant] );
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddSmallestDescendants(EDirection direction, EClockDirection iterationDirection, NodeCPtrVector& descendants) const
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
        firstChild  = isCW ? EQuadrant::TopLeft  : EQuadrant::TopRight;
        secondChild = isCW ? EQuadrant::TopRight : EQuadrant::TopLeft;
      }
      break;
      case EDirection::East:
      {
        firstChild  = isCW ? EQuadrant::TopRight : EQuadrant::BotRight;
        secondChild = isCW ? EQuadrant::BotRight : EQuadrant::TopRight;
      }
      break;
      case EDirection::South:
      {
        firstChild  = isCW ? EQuadrant::BotRight : EQuadrant::BotLeft;
        secondChild = isCW ? EQuadrant::BotLeft  : EQuadrant::BotRight;
      }
      break;
      case EDirection::West:
      {
        firstChild  = isCW ? EQuadrant::BotLeft : EQuadrant::TopLeft;
        secondChild = isCW ? EQuadrant::TopLeft : EQuadrant::BotLeft;
      }
      break;
      case EDirection::Invalid:
      {
        CORETECH_ASSERT(false);
      }
    }
    
    CORETECH_ASSERT(firstChild != EQuadrant::Invalid);
    CORETECH_ASSERT(secondChild != EQuadrant::Invalid);
    _children[(std::underlying_type<EQuadrant>::type)firstChild ].AddSmallestDescendants(direction, iterationDirection, descendants);
    _children[(std::underlying_type<EQuadrant>::type)secondChild].AddSmallestDescendants(direction, iterationDirection, descendants);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const NavMeshQuadTreeNode* NavMeshQuadTreeNode::FindSingleNeighbor(EDirection direction) const
{
  const NavMeshQuadTreeNode* neighbor = nullptr;

  // find where we land by moving in that direction
  const MoveInfo* moveInfo = GetDestination(_quadrant, direction);
  if ( moveInfo != nullptr )
  {
    // check if it's the same parent
    if ( moveInfo->sharesParent )
    {
      // if so, it's a sibling
      CORETECH_ASSERT(_parent);
      neighbor = _parent->GetChild( moveInfo->neighborQuadrant );
      CORETECH_ASSERT(neighbor);
    }
    else
    {
      // otherwise, find our parent's neighbor and get the proper child that would be next to us
      // note our parent can return null if we are on the border
      const NavMeshQuadTreeNode* parentNeighbor = _parent->FindSingleNeighbor(direction);
      neighbor = parentNeighbor ? parentNeighbor->GetChild( moveInfo->neighborQuadrant ) : nullptr;
      // if the parentNeighbor was not subdivided, then he is our neighbor
      neighbor = neighbor ? neighbor : parentNeighbor;
    }
  }
  
  return neighbor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddSmallestNeighbors(EDirection direction,
  EClockDirection iterationDirection, NodeCPtrVector& neighbors) const
{
  const NavMeshQuadTreeNode* firstNeighbor = FindSingleNeighbor(direction);
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


} // namespace Cozmo
} // namespace Anki
