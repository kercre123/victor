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

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeNode::NavMeshQuadTreeNode(const Point3f &center, float size, uint32_t maxDepth, EContentType contentType)
: _maxDepth(maxDepth)
, _contentType(contentType)
, _center(center)
, _size(size)
{
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
  const float halfSize = _size * 0.5f;
  Quad3f ret
  {
    {_center.x()+halfSize, _center.y()+halfSize, _center.z()}, // up L
    {_center.x()-halfSize, _center.y()+halfSize, _center.z()}, // lo L
    {_center.x()+halfSize, _center.y()-halfSize, _center.z()}, // up R
    {_center.x()-halfSize, _center.y()-halfSize, _center.z()}  // lo R
  };
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Quad2f NavMeshQuadTreeNode::MakeQuadXY() const
{
  const float halfSize = _size * 0.5f;
  Quad2f ret
  {
    {_center.x()+halfSize, _center.y()+halfSize}, // up L
    {_center.x()-halfSize, _center.y()+halfSize}, // lo L
    {_center.x()+halfSize, _center.y()-halfSize}, // up R
    {_center.x()-halfSize, _center.y()-halfSize}  // lo R
  };
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddClearQuad(const Quad2f& quad)
{
  // we need to subdivide or fit within the clear quad, since this implementation doesn't keep partial
  // info within a quad, that's what subquads would be for
  ASSERT_NAMED( CanSubdivide() ||
                ( FLT_LE(_size,(quad.GetMaxX()-quad.GetMinX())) && (FLT_LE(_size,quad.GetMaxY()-quad.GetMinY())) ),
                "NavMeshQuadTreeNode.AddClearQuad.ClearQuadIsTooSmall");

  // if we are already clear, we won't gain new info, so we can skip
  if ( _contentType == EContentType::Clear ) {
    return;
  }

  const Quad2f& myQuad = MakeQuadXY();
  if ( myQuad.Intersects(quad) )
  {
    // am I fully contained within the clear quad?
    if ( quad.Contains(myQuad) )
    {
      // merge if subdivided
      if ( IsSubdivided() ) {
        Merge();
      }
      
      // we are fully clear
      _contentType = EContentType::Clear;
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
        _contentType = EContentType::Subdivided;
      }
      
      // if we have children, delegate on them
      const bool isSubdivided = IsSubdivided();
      if ( isSubdivided )
      {
        // ask children to add quad
        for( auto& child : _children ) {
          child.AddClearQuad(quad);
        }
        
        // try to automerge
        TryAutoMerge();
      }
      else
      {
        // we can't subdivide more, flag as some degree of clear
        EContentType newType = EContentType::Unknown;
        switch(_contentType)
        {
          case EContentType::Subdivided:
          {
            // we can't be subdivided, given the code before
            ASSERT_NAMED(false, "NavMeshQuadTreeNode.AddClearQuad.InvalidContentType");
            break;
          }
          case EContentType::Unknown:
          case EContentType::UnkownClear:
          {
            // part of us is not known, part is clear
            newType = EContentType::UnkownClear;
            break;
          }
          case EContentType::Clear:
          {
            // we are fully clear
            newType = EContentType::Clear;
            break;
          }
          case EContentType::Obstacle:
          {
            // we are not fully cleared, but part of us was obstacle in the past
            newType = EContentType::Obstacle;
            break;
          }
        }
        
        // set new type
        ASSERT_NAMED(newType!=EContentType::Unknown, "NavMeshQuadTreeNode.AddClearQuad.UnexpectedNewType");
        _contentType = newType;
      }
    }
  }
  else
  {
    // no new info, keep old
  }
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
      case EContentType::Subdivided : { color = Anki::NamedColors::BLACK;     break; }
      case EContentType::Unknown    : { color = Anki::NamedColors::DARKGRAY;  break; }
      case EContentType::UnkownClear: { color = Anki::NamedColors::DARKGREEN; break; }
      case EContentType::Clear      : { color = Anki::NamedColors::GREEN;     break; }
      case EContentType::Obstacle   : { color = Anki::NamedColors::RED;       break; }
    }
    color.SetAlpha(0.25f);
    quadVector.emplace_back(VizManager::MakeSimpleQuad(color, _center, _size));
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
  ASSERT_NAMED(CanSubdivide() && !IsSubdivided(), "NavMeshQuadTreeNode.Subdivide.InvalidState");
  assert( _maxDepth > 0 );
  
  const float halfSize    = _size * 0.50f;
  const float quarterSize = halfSize * 0.50f;
  _children.emplace_back( Point3f{_center.x()+quarterSize, _center.y()+quarterSize, _center.z()}, halfSize, _maxDepth-1, _contentType ); // up L
  _children.emplace_back( Point3f{_center.x()-quarterSize, _center.y()+quarterSize, _center.z()}, halfSize, _maxDepth-1, _contentType ); // lo L
  _children.emplace_back( Point3f{_center.x()+quarterSize, _center.y()-quarterSize, _center.z()}, halfSize, _maxDepth-1, _contentType ); // up R
  _children.emplace_back( Point3f{_center.x()-quarterSize, _center.y()-quarterSize, _center.z()}, halfSize, _maxDepth-1, _contentType ); // lo E
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::Merge()
{
  ASSERT_NAMED(IsSubdivided(), "NavMeshQuadTreeNode.Merge.InvalidState");
  _children.clear();
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
    
    // we are fully clear
    _contentType = childType;
  }
}


} // namespace Cozmo
} // namespace Anki
