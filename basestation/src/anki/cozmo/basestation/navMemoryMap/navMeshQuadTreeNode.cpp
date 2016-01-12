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

#include "anki/common/basestation/math/quad_impl.h"
#include "util/math/constantsAndMacros.h"

#include <unordered_map>
#include <limits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeNode::NavMeshQuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, EContentType contentType, NavMeshQuadTreeNode* parent)
: _parent(parent)
, _level(level)
, _quadrant(quadrant)
, _contentType(contentType)
, _center(center)
, _sideLen(sideLength)
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
bool NavMeshQuadTreeNode::AddClearQuad(const Quad2f& quad)
{
  const bool changed = AddQuad(quad, EContentType::Clear);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddObstacleCube(const Quad2f& quad)
{
  const bool changed = AddQuad(quad, EContentType::ObstacleCube);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddObstacleUnrecognized(const Quad2f& quad)
{
  const bool changed = AddQuad(quad, EContentType::ObstacleUnrecognized);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddCliff(const Quad2f &quad)
{
  const bool changed = AddQuad(quad, EContentType::Cliff);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::UpgradeRootLevel(const Point2f& direction)
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
  _children.emplace_back( Point3f{_center.x()+oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, TopLeft , EContentType::Unknown, this ); // up L
  _children.emplace_back( Point3f{_center.x()+oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, TopRight , EContentType::Unknown, this ); // up R
  _children.emplace_back( Point3f{_center.x()-oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, BotLeft, EContentType::Unknown, this ); // lo L
  _children.emplace_back( Point3f{_center.x()-oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, BotRight, EContentType::Unknown, this ); // lo R

  // calculate the child that takes my place by using the opposite direction to expansion
  size_t childIdx = 0;
  if      (  xPlus && !yPlus ) { childIdx = 1; }
  else if ( !xPlus &&  yPlus ) { childIdx = 2; }
  else if (  xPlus &&  yPlus ) { childIdx = 3; }
  NavMeshQuadTreeNode& childTakingMyPlace = _children[childIdx];
  
  // set the new parent in my old children
  for ( auto& child : oldChildren ) {
    child.ChangeParent( &childTakingMyPlace );
  }
  
  // swap children with the temp
  std::swap(childTakingMyPlace._children, oldChildren);

  // set the content type I had in the child that takes my place
  childTakingMyPlace._contentType = _contentType;
  _contentType = EContentType::Subdivided;
  
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
      case EContentType::Subdivided          : { color = Anki::NamedColors::BLUE;   break; }
      case EContentType::Unknown             : { color = Anki::NamedColors::DARKGRAY;  break; }
      case EContentType::Cliff               : { color = Anki::NamedColors::BLACK;     break; }
      case EContentType::Clear               : { color = Anki::NamedColors::GREEN;     break; }
      case EContentType::ObstacleCube        : { color = Anki::NamedColors::RED;       break; }
      case EContentType::ObstacleUnrecognized: { color = Anki::NamedColors::MAGENTA;   break; }
    }
    color.SetAlpha(0.75f);
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
void NavMeshQuadTreeNode::Subdivide()
{
  CORETECH_ASSERT(CanSubdivide() && !IsSubdivided());
  CORETECH_ASSERT(_level > 0);
  
  const float halfLen    = _sideLen * 0.50f;
  const float quarterLen = halfLen * 0.50f;
  const uint8_t cLevel = _level-1;
  _children.emplace_back( Point3f{_center.x()+quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, TopLeft , _contentType, this ); // up L
  _children.emplace_back( Point3f{_center.x()+quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, TopRight, _contentType, this ); // up R
  _children.emplace_back( Point3f{_center.x()-quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, BotLeft , _contentType, this ); // lo L
  _children.emplace_back( Point3f{_center.x()-quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, BotRight, _contentType, this ); // lo E
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::Merge()
{
  ASSERT_NAMED(IsSubdivided(), "NavMeshQuadTreeNode.Merge.InvalidState");
  _children.clear();
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
void NavMeshQuadTreeNode::TryAutoMerge()
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
    Merge();
    
    // we have the same type as the children
    SetDetectedContentType( childType );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddQuad(const Quad2f &quad, EContentType detectedContentType)
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
    // am I fully contained within the clear quad?
    if ( quad.Contains(myQuad) && CanOverrideChildrenWithContent(detectedContentType) )
    {
      // merge if subdivided
      if ( IsSubdivided() ) {
        Merge();
      }
      
      // we are fully the new type
      SetDetectedContentType( detectedContentType );
    }
    else
    {
      // see if we can subdivide
      const bool wasSubdivided = IsSubdivided();
      if ( !wasSubdivided && CanSubdivide() )
      {
        // do now
        Subdivide();
        
        // we are subdivided
        SetDetectedContentType( EContentType::Subdivided );
      }
      
      // if we have children, delegate on them
      const bool isSubdivided = IsSubdivided();
      if ( isSubdivided )
      {
        // ask children to add quad
        for( auto& child : _children ) {
          childChanged = child.AddQuad(quad, detectedContentType) || childChanged;
        }
        
        // try to automerge (if it does, our content type will change from subdivided to the merged type)
        TryAutoMerge();
      }
      else
      {
        SetDetectedContentType(detectedContentType);
      }
    }
  }
  
  const bool ret = (_contentType != previousType) || childChanged;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::SetDetectedContentType(EContentType detectedContentType)
{
  // This is here temporarily to prevent Clear from overriding Cliffs. I need to think how we want to support
  // cliffs directly under Cozmo
  if ( _contentType == EContentType::Cliff )
  {
    return;
  }

  // this is where we can detect changes in content, for example new obstacles or things disappearing
  _contentType = detectedContentType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeNode::EDirection NavMeshQuadTreeNode::GetOppositeDirection(EDirection dir)
{
  const EDirection ret = (EDirection)((dir + 2) % 4);
  return ret;
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
  const MoveInfo* ret = ( from == EQuadrant::Root ) ? nullptr : &quadrantAndDirection[from][direction];
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
void NavMeshQuadTreeNode::AddSmallestDescendants(EDirection direction, std::vector<const NavMeshQuadTreeNode*>& descendants) const
{
  if ( !IsSubdivided() ) {
    descendants.push_back( this );
  } else {
    switch (direction) {
      case North:
      {
        _children[EQuadrant::TopLeft].AddSmallestDescendants(direction, descendants);
        _children[EQuadrant::TopRight].AddSmallestDescendants(direction, descendants);
      }
      break;
      case East:
      {
        _children[EQuadrant::TopRight].AddSmallestDescendants(direction, descendants);
        _children[EQuadrant::BotRight].AddSmallestDescendants(direction, descendants);
      }
      break;
      case South:
      {
        _children[EQuadrant::BotLeft].AddSmallestDescendants(direction, descendants);
        _children[EQuadrant::BotRight].AddSmallestDescendants(direction, descendants);
      }
      break;
      case West:
      {
        _children[EQuadrant::TopLeft].AddSmallestDescendants(direction, descendants);
        _children[EQuadrant::BotLeft].AddSmallestDescendants(direction, descendants);
      }
      break;
    }
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
void NavMeshQuadTreeNode::AddSmallestNeighbors(EDirection direction, std::vector<const NavMeshQuadTreeNode*>& neighbors) const
{
  const NavMeshQuadTreeNode* firstNeighbor = FindSingleNeighbor(direction);
  if ( nullptr != firstNeighbor )
  {
    firstNeighbor->AddSmallestDescendants( GetOppositeDirection(direction), neighbors);
  }
}


} // namespace Cozmo
} // namespace Anki
