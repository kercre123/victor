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
bool NavMeshQuadTreeNode::AddClearQuad(const Quad2f& quad)
{
  const bool ret = AddQuad(quad, EContentType::Clear);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddObstacle(const Quad2f& quad)
{
  const bool ret = AddQuad(quad, EContentType::Obstacle);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddCliff(const Quad2f &quad)
{
  const bool ret = AddQuad(quad, EContentType::Cliff);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::UpgradeToParent(const Point2f& direction)
{
  CORETECH_ASSERT( !NEAR_ZERO(direction.x()) && !NEAR_ZERO(direction.y()) );

  // save my old children to store in the child that is taking my spot
  std::vector<NavMeshQuadTreeNode> oldChildren;
  std::swap(oldChildren, _children);

  const bool xPlus = FLT_GTR_ZERO(direction.x());
  const bool yPlus = FLT_GTR_ZERO(direction.y());
  
  // move to its new center
  const float oldHalfSize = _size * 0.50f;
  _center.x() = _center.x() + (xPlus ? oldHalfSize : -oldHalfSize);
  _center.y() = _center.y() + (yPlus ? oldHalfSize : -oldHalfSize);

  // create new children
  _children.emplace_back( Point3f{_center.x()+oldHalfSize, _center.y()+oldHalfSize, _center.z()}, _size, _maxDepth, EContentType::Unknown ); // up L
  _children.emplace_back( Point3f{_center.x()-oldHalfSize, _center.y()+oldHalfSize, _center.z()}, _size, _maxDepth, EContentType::Unknown ); // lo L
  _children.emplace_back( Point3f{_center.x()+oldHalfSize, _center.y()-oldHalfSize, _center.z()}, _size, _maxDepth, EContentType::Unknown ); // up R
  _children.emplace_back( Point3f{_center.x()-oldHalfSize, _center.y()-oldHalfSize, _center.z()}, _size, _maxDepth, EContentType::Unknown ); // lo E

  // calculate the child that takes my place by using the opposite direction to expansion
  size_t childIdx = 0;
  if      (  xPlus && !yPlus ) { childIdx = 1; }
  else if ( !xPlus &&  yPlus ) { childIdx = 2; }
  else if (  xPlus &&  yPlus ) { childIdx = 3; }
  
  // swap children with the temp
  std::swap(_children[childIdx]._children, oldChildren);
  _children[childIdx]._contentType = _contentType; // inherit content type
  
  // upgrade my remaining stats
  _size = _size * 2.0f;
  ++_maxDepth;
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
      case EContentType::Subdivided : { color = Anki::NamedColors::MAGENTA;   break; }
      case EContentType::Unknown    : { color = Anki::NamedColors::DARKGRAY;  break; }
      case EContentType::Cliff      : { color = Anki::NamedColors::BLACK;     break; }
      case EContentType::Clear      : { color = Anki::NamedColors::GREEN;     break; }
      case EContentType::Obstacle   : { color = Anki::NamedColors::RED;       break; }
    }
    color.SetAlpha(0.75f);
    //quadVector.emplace_back(VizManager::MakeSimpleQuad(color, Point3f{_center.x(), _center.y(), _center.z()+_maxDepth*100}, _size));
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
bool NavMeshQuadTreeNode::CanOverrideChildrenWithContent(EContentType contentType) const
{
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
                ( FLT_LE(_size,(quad.GetMaxX()-quad.GetMinX())) && (FLT_LE(_size,quad.GetMaxY()-quad.GetMinY())) ),
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



} // namespace Cozmo
} // namespace Anki
