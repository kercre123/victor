/**
 * File: navMeshQuadTreeNode.h
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
#include "navMeshQuadTreeNode.h"
#include "navMeshQuadTreeProcessor.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "util/math/constantsAndMacros.h"

#include <unordered_map>
#include <limits>

namespace Anki {
namespace Cozmo {

static_assert( !std::is_copy_assignable<NavMeshQuadTreeNode>::value, "NavMeshQuadTreeNode was designed non-copyable" );
static_assert( !std::is_copy_constructible<NavMeshQuadTreeNode>::value, "NavMeshQuadTreeNode was designed non-copyable" );
static_assert( !std::is_move_assignable<NavMeshQuadTreeNode>::value, "NavMeshQuadTreeNode was designed non-movable" );
static_assert( !std::is_move_constructible<NavMeshQuadTreeNode>::value, "NavMeshQuadTreeNode was designed non-movable" );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeNode::NavMeshQuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, NavMeshQuadTreeNode* parent)
: _center(center)
, _sideLen(sideLength)
, _parent(parent)
, _level(level)
, _quadrant(quadrant)
, _content(ENodeContentType::Unknown)
{
  CORETECH_ASSERT(_quadrant <= EQuadrant::Root);

  // processor would need to know otherwise, like we do in ForceSetDetectedContentType
  CORETECH_ASSERT(_content.type == ENodeContentType::Unknown);
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
Quad2f NavMeshQuadTreeNode::MakeQuadXY(const float padding_mm) const
{
  const float halfLen = (_sideLen * 0.5f) + padding_mm;
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
const std::unique_ptr<NavMeshQuadTreeNode>& NavMeshQuadTreeNode::GetChildAt(size_t index) const
{
  if ( index < _childrenPtr.size() ) {
    return _childrenPtr[index];
  }
  else
  {
    PRINT_NAMED_ERROR("NavMeshQuadTreeNode.GetChildAt.InvalidIndex",
      "Index %zu is greater than number of children %zu. Returning null",
      index, _childrenPtr.size());
    static std::unique_ptr<NavMeshQuadTreeNode> nullPtr;
    return nullPtr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddQuad(const Quad2f &quad, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  // if we won't gain any new info, no need to process
  const bool isSameInfo = _content == detectedContent;
  if ( isSameInfo ) {
    return false;
  }
  
  // to check for changes
  NodeContent previousContent = _content;
  bool childChanged = false;

  // check if the quad affects us
  const Quad2f& myQuad = MakeQuadXY();
  if ( myQuad.Intersects(quad) )
  {
    EContentOverlap overlap = EContentOverlap::Partial; // default value may change later
  
    // am I fully contained within the quad?
    if ( quad.Contains(myQuad) )
    {
      overlap = EContentOverlap::Total;
      
      // if subdivided
      if ( IsSubdivided() )
      {
        // we are subdivided, see if we can merge children or we should tell them to add the new quad
        if ( CanOverrideSelfAndChildrenWithContent(detectedContent.type, overlap) )
        {
          // merge to the new content, we already made sure we can override the type
          Merge(detectedContent, processor);
        }
        else
        {
          // delegate on children
          for( auto& childPtr : _childrenPtr ) {
            childChanged = childPtr->AddQuad(quad, detectedContent, processor) || childChanged;
          }
        }
      }
      else
      {
        // we can try to set our content, since we fit fully and we don't have children
        TrySetDetectedContentType( detectedContent, overlap, processor );
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
        for( auto& childPtr : _childrenPtr ) {
          childChanged = childPtr->AddQuad(quad, detectedContent, processor) || childChanged;
        }
        
        // try to automerge (if it does, our content type will change from subdivided to the merged type)
        TryAutoMerge(processor);
      }
      else
      {
        TrySetDetectedContentType(detectedContent, overlap, processor);
      }
    }
  }
  
  const bool ret = (_content != previousContent) || childChanged;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::UpgradeRootLevel(const Point2f& direction, NavMeshQuadTreeProcessor& processor)
{
  CORETECH_ASSERT( !NEAR_ZERO(direction.x()) || !NEAR_ZERO(direction.y()) );
  
  // reached expansion limit
  if ( _level == std::numeric_limits<uint8_t>::max() ) {
    return false;
  }

  // save my old children to store in the child that is taking my spot
  std::vector< std::unique_ptr<NavMeshQuadTreeNode> > oldChildren;
  std::swap(oldChildren, _childrenPtr);

  const bool xPlus = FLT_GTR_ZERO(direction.x());
  const bool yPlus = FLT_GTR_ZERO(direction.y());
  
  // move to its new center
  const float oldHalfLen = _sideLen * 0.50f;
  _center.x() = _center.x() + (xPlus ? oldHalfLen : -oldHalfLen);
  _center.y() = _center.y() + (yPlus ? oldHalfLen : -oldHalfLen);

  // create new children
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()+oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::TopLeft , this) ); // up L
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()+oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::TopRight, this) ); // up R
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()-oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::BotLeft , this) ); // lo L
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()-oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::BotRight, this) ); // lo R

  // calculate the child that takes my place by using the opposite direction to expansion
  size_t childIdx = 0;
  if      ( !xPlus &&  yPlus ) { childIdx = 1; }
  else if (  xPlus && !yPlus ) { childIdx = 2; }
  else if (  xPlus &&  yPlus ) { childIdx = 3; }
  NavMeshQuadTreeNode& childTakingMyPlace = *_childrenPtr[childIdx];
  
  // set the new parent in my old children
  for ( auto& childPtr : oldChildren ) {
    childPtr->ChangeParent( &childTakingMyPlace );
  }
  
  // swap children with the temp
  std::swap(childTakingMyPlace._childrenPtr, oldChildren);

  // set the content type I had in the child that takes my place
  childTakingMyPlace.ForceSetDetectedContentType( _content, processor );
  
  NodeContent emptySubdividedContent(ENodeContentType::Subdivided);
  ForceSetDetectedContentType(emptySubdividedContent, processor);
  
  // upgrade my remaining stats
  _sideLen = _sideLen * 2.0f;
  ++_level;
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddQuadsToDraw(VizManager::SimpleQuadVector& quadVector) const
{
  // if we have children, delegate on them, otherwise render ourselves
  if ( _childrenPtr.empty() )
  {
    ColorRGBA color =  Anki::NamedColors::WHITE;
    switch(_content.type)
    {
      case ENodeContentType::Invalid                : { color = Anki::NamedColors::WHITE;    color.SetAlpha(1.0f); break; }
      case ENodeContentType::Subdivided             : { color = Anki::NamedColors::CYAN;     color.SetAlpha(1.0f); break; }
      case ENodeContentType::Unknown                : { color = Anki::NamedColors::DARKGRAY; color.SetAlpha(0.2f); break; }
      case ENodeContentType::ClearOfObstacle        : { color = Anki::NamedColors::GREEN;    color.SetAlpha(0.5f); break; }
      case ENodeContentType::ClearOfCliff           : { color = Anki::NamedColors::DARKGREEN;color.SetAlpha(0.8f); break; }
      case ENodeContentType::ObstacleCube           : { color = Anki::NamedColors::RED;      color.SetAlpha(0.5f); break; }
      case ENodeContentType::ObstacleCubeRemoved    : { color = Anki::NamedColors::WHITE;    color.SetAlpha(1.0f); break; } // we actually don't store this type, it clears ObstacleCube
      case ENodeContentType::ObstacleCharger        : { color = Anki::NamedColors::ORANGE;   color.SetAlpha(0.5f); break; }
      case ENodeContentType::ObstacleChargerRemoved : { color = Anki::NamedColors::WHITE;    color.SetAlpha(1.0f); break; } // we actually don't store this type, it clears ObstacleCharger
      case ENodeContentType::ObstacleUnrecognized   : { color = Anki::NamedColors::MAGENTA;  color.SetAlpha(0.5f); break; }
      case ENodeContentType::Cliff                  : { color = Anki::NamedColors::BLACK;    color.SetAlpha(0.8f); break; }
      case ENodeContentType::InterestingEdge        : { color = Anki::NamedColors::BLUE;     color.SetAlpha(0.5f); break; }
      case ENodeContentType::NotInterestingEdge     : { color = Anki::NamedColors::YELLOW;   color.SetAlpha(0.5f); break; }
    }
    //quadVector.emplace_back(VizManager::MakeSimpleQuad(color, Point3f{_center.x(), _center.y(), _center.z()+_level*100}, _sideLen));
    quadVector.emplace_back(VizManager::MakeSimpleQuad(color, _center, _sideLen));
  }
  else
  {
    // delegate on each child
    for( const auto& childPtr : _childrenPtr ) {
      childPtr->AddQuadsToDraw(quadVector);
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
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()+quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::TopLeft , this) ); // up L
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()+quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::TopRight, this) ); // up R
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()-quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::BotLeft , this) ); // lo L
  _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()-quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::BotRight, this) ); // lo E

  // our children may change later on, but until they do, assume they have our old content
  for ( auto& childPtr : _childrenPtr )
  {
    childPtr->ForceSetDetectedContentType(_content, processor);
  }
  
  // set our content type to subdivided
  NodeContent emptySubdividedContent(ENodeContentType::Subdivided);
  ForceSetDetectedContentType(emptySubdividedContent, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::Merge(const NodeContent& newContent, NavMeshQuadTreeProcessor& processor)
{
  ASSERT_NAMED(IsSubdivided(), "NavMeshQuadTreeNode.Merge.InvalidState");

  // since we are going to destroy the children, notify the processor of all the descendants about to be destroyed
  ClearDescendants(processor);
  
  // set our content to the one we will have after the merge
  ForceSetDetectedContentType(newContent, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::ClearDescendants(NavMeshQuadTreeProcessor& processor)
{
  // iterate all children recursively destroying their children
  for ( auto& childPtr : _childrenPtr ) {
    childPtr->ClearDescendants(processor);
    processor.OnNodeDestroyed(childPtr.get());
  }
  
  // now remove children
  _childrenPtr.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::CanOverrideSelfWithContent(ENodeContentType newContentType, EContentOverlap overlap) const
{
  if ( newContentType == ENodeContentType::Cliff )
  {
    // Cliff can override any other
    return true;
  }
  else if ( _content.type == ENodeContentType::Cliff )
  {
    // Cliff can only be overridden by a full ClearOfCliff (the cliff is gone)
    const bool isTotalClear = (newContentType == ENodeContentType::ClearOfCliff) && (overlap == EContentOverlap::Total);
    return isTotalClear;
  }
  else if ( _content.type == ENodeContentType::ClearOfCliff && newContentType == ENodeContentType::ClearOfObstacle )
  {
    // ClearOfCliff can not be overriden by ClearOfObstacle
    return false;
  }
  else if ( newContentType == ENodeContentType::InterestingEdge )
  {
    // InterestingEdge can only override basic node types, because it would cause data loss otherwise. For example,
    // we don't want to override a recognized marked cube or a cliff with their own border
    if ( ( _content.type == ENodeContentType::ObstacleCube         ) ||
         ( _content.type == ENodeContentType::ObstacleCharger      ) ||
         ( _content.type == ENodeContentType::ObstacleUnrecognized ) ||
         ( _content.type == ENodeContentType::Cliff                ) ||
         ( _content.type == ENodeContentType::NotInterestingEdge   ) )
    {
      return false;
    }
  }
  else if ( newContentType == ENodeContentType::NotInterestingEdge )
  {
    // NotInterestingEdge can only override interesting edges
    if ( _content.type != ENodeContentType::InterestingEdge ) {
      return false;
    }
  }
  else if ( _content.type == ENodeContentType::NotInterestingEdge )
  {
    // NotInterestingEdge should not be overriden by clearOfObstacle or interesting edge. This is probably conservative,
    // but the way interesting edges are currently added to the map would totally destroy non-interesting ones
    // rsam: an improved versino could be to chek that it's a total overlap for ClearOfObstacle. Behaviors could
    // also check that one interesting border very close to not interesting ones should probably be considered not
    // interesting. This is probably necessary anyway due to the errors in visualization/localization
    if ( (newContentType == ENodeContentType::InterestingEdge) ||
         (newContentType == ENodeContentType::ClearOfObstacle) )
    {
      return false;
    }
  }
  else if ( newContentType == ENodeContentType::ObstacleCubeRemoved )
  {
    // ObstacleCubeRemoved can only remove ObstacleCube
    if ( _content.type != ENodeContentType::ObstacleCube ) {
      return false;
    }
  }
  else if ( newContentType == ENodeContentType::ObstacleChargerRemoved )
  {
    // ObstacleChargerRemoved can only remove ObstacleCharger
    if ( _content.type != ENodeContentType::ObstacleCharger ) {
      return false;
    }
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::CanOverrideSelfAndChildrenWithContent(ENodeContentType newContentType, EContentOverlap overlap) const
{
  // ask us
  if ( !CanOverrideSelfWithContent(newContentType, overlap) ) {
    return false;
  }

  // ask children if they can
  for ( const auto& childPtr : _childrenPtr )
  {
    const bool canOverrideChild = childPtr->CanOverrideSelfAndChildrenWithContent(newContentType, overlap);
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

  // check if all children classified the same content
  ENodeContentType childType = _childrenPtr[0]->GetContentType();
  if ( childType == ENodeContentType::Subdivided ) {
    // any subdivided quad prevents the parent from merging
    return;
  }
  
  bool allChildrenEqual = true;
  for(size_t idx1=0; idx1<_childrenPtr.size()-1; ++idx1)
  {
    for(size_t idx2=idx1+1; idx2<_childrenPtr.size(); ++idx2)
    {
      if ( _childrenPtr[idx1]->GetContent() != _childrenPtr[idx2]->GetContent() )
      {
        allChildrenEqual = false;
        break;
      }
    }
  }
  
  // if they did, we can merge and set that type on this parent
  if ( allChildrenEqual )
  {
    NodeContent childContent = _childrenPtr[0]->GetContent(); // do a copy since merging will destroy children
    Merge( childContent, processor );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::TrySetDetectedContentType(const NodeContent& detectedContent, EContentOverlap overlap,
  NavMeshQuadTreeProcessor& processor)
{
  // if we don't want to override with the new content, do not call ForceSet
  if ( !CanOverrideSelfWithContent(detectedContent.type, overlap) ) {
    return;
  }

  // do the change
  ForceSetDetectedContentType(detectedContent, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::ForceSetDetectedContentType(const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  const ENodeContentType oldcontent = _content.type;

  // if we are trying to set a removed type, convert to the type we want to actually set
  NodeContent finalContent = detectedContent;
  {
    const ENodeContentType newContent = detectedContent.type;
    const bool isObstacleRemoved = (newContent == ENodeContentType::ObstacleChargerRemoved) ||
                                   (newContent == ENodeContentType::ObstacleCubeRemoved);
    if ( isObstacleRemoved )
    {
      finalContent.type = ENodeContentType::ClearOfObstacle;
    }
  }
  
  // this is where we can detect changes in content, for example new obstacles or things disappearing
  _content = finalContent;
  
  // notify processor only when content type changes, not if the underlaying info changes
  const bool typeChanged = oldcontent != _content.type;
  if ( typeChanged ) {
    processor.OnNodeContentTypeChanged(this, oldcontent, _content.type);
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
    ( _childrenPtr.empty() ) ?
    ( nullptr ) :
    ( _childrenPtr[(std::underlying_type<EQuadrant>::type)quadrant].get() );
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
    _childrenPtr[(std::underlying_type<EQuadrant>::type)firstChild ]->AddSmallestDescendants(direction, iterationDirection, descendants);
    _childrenPtr[(std::underlying_type<EQuadrant>::type)secondChild]->AddSmallestDescendants(direction, iterationDirection, descendants);
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddSmallestDescendantsDepthFirst(NodeCPtrVector& descendants) const
{
  if ( !IsSubdivided() ) {
    descendants.emplace_back(this);
  } else {
    for( const auto& cPtr : _childrenPtr ) {
      cPtr->AddSmallestDescendantsDepthFirst(descendants);
    }
  }
}

} // namespace Cozmo
} // namespace Anki
