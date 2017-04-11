/**
 * File: navMeshQuadTreeNode.cpp
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
#include "util/math/math.h"

#include "util/cpuProfiler/cpuProfiler.h"

#include <unordered_map>
#include <limits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// convert between our internal node content type and an external content type
ExternalInterface::ENodeContentTypeEnum ConvertContentType(ENodeContentType contentType )
{
  using namespace ExternalInterface;
  
  ExternalInterface::ENodeContentTypeEnum externalContentType = ExternalInterface::ENodeContentTypeEnum::Unknown;
  switch (contentType) {
    case ENodeContentType::Invalid:               { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType"); break; } // Should never get this
    case ENodeContentType::Subdivided:            { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType"); break; } // Should never get this (we only send leaves)
    case ENodeContentType::Unknown:               { externalContentType = ENodeContentTypeEnum::Unknown;         break; }
    case ENodeContentType::ClearOfObstacle:       { externalContentType = ENodeContentTypeEnum::ClearOfObstacle; break; }
    case ENodeContentType::ClearOfCliff:          { externalContentType = ENodeContentTypeEnum::ClearOfCliff;    break; }
    case ENodeContentType::ObstacleCube:          { externalContentType = ENodeContentTypeEnum::ObstacleCube;    break; }
    case ENodeContentType::ObstacleCubeRemoved:   { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType"); break; } // Should never get this
    case ENodeContentType::ObstacleCharger:       { externalContentType = ENodeContentTypeEnum::ObstacleCharger; break; }
    case ENodeContentType::ObstacleChargerRemoved:{ DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType"); break; } // Should never get this
    case ENodeContentType::ObstacleUnrecognized:  { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType"); break; } // Should never get this (unsupported)
    case ENodeContentType::Cliff:                 { externalContentType = ENodeContentTypeEnum::Cliff;           break; }
    case ENodeContentType::InterestingEdge:       { externalContentType = ENodeContentTypeEnum::VisionBorder;    break; }
    case ENodeContentType::NotInterestingEdge:    { externalContentType = ENodeContentTypeEnum::VisionBorder;    break; }
    case ENodeContentType::_Count:                { DEV_ASSERT(false, "NavMeshQuadTreeNode._Count"); break; }
  }
  return externalContentType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// convert between our internal node content type and an internal content type
ExternalInterface::ENodeContentTypeDebugVizEnum ConvertContentTypeDebugViz(ENodeContentType contentType )
{
  using namespace ExternalInterface;
  
  ExternalInterface::ENodeContentTypeDebugVizEnum internalContentType = ExternalInterface::ENodeContentTypeDebugVizEnum::Unknown;
  switch (contentType) {
    case ENodeContentType::Invalid:               { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentTypeDebugViz");        break; } // Should never get this
    case ENodeContentType::Subdivided:            { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentTypeDebugViz");        break; } // Should never get this (we only send leaves)
    case ENodeContentType::Unknown:               { internalContentType = ENodeContentTypeDebugVizEnum::Unknown;                break; }
    case ENodeContentType::ClearOfObstacle:       { internalContentType = ENodeContentTypeDebugVizEnum::ClearOfObstacle;        break; }
    case ENodeContentType::ClearOfCliff:          { internalContentType = ENodeContentTypeDebugVizEnum::ClearOfCliff;           break; }
    case ENodeContentType::ObstacleCube:          { internalContentType = ENodeContentTypeDebugVizEnum::ObstacleCube;           break; }
    case ENodeContentType::ObstacleCubeRemoved:   { internalContentType = ENodeContentTypeDebugVizEnum::ObstacleCubeRemoved;    break; }
    case ENodeContentType::ObstacleCharger:       { internalContentType = ENodeContentTypeDebugVizEnum::ObstacleCharger;        break; }
    case ENodeContentType::ObstacleChargerRemoved:{ internalContentType = ENodeContentTypeDebugVizEnum::ObstacleChargerRemoved; break; }
    case ENodeContentType::ObstacleUnrecognized:  { internalContentType = ENodeContentTypeDebugVizEnum::ObstacleUnrecognized;   break; }
    case ENodeContentType::Cliff:                 { internalContentType = ENodeContentTypeDebugVizEnum::Cliff;                  break; }
    case ENodeContentType::InterestingEdge:       { internalContentType = ENodeContentTypeDebugVizEnum::InterestingEdge;        break; }
    case ENodeContentType::NotInterestingEdge:    { internalContentType = ENodeContentTypeDebugVizEnum::NotInterestingEdge;     break; }
    case ENodeContentType::_Count:                { DEV_ASSERT(false, "NavMeshQuadTreeNode._Count"); break; }
  }
  return internalContentType;
}
  
} // namespace

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
, _content(ENodeContentType::Invalid)
{
  DEV_ASSERT(_quadrant <= EQuadrant::Root, "NavMeshQuadTreeNode.Constructor.InvalidQuadrant");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::Contains(const Quad2f& quad) const
{
  // this should be using the new faster checks. It's only called from outside during expansion atm though, so
  // not worth changing atm (no way of testing speed impact)
  const bool ret = MakeQuadXY().Contains(quad);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::Contains(const Point2f& point) const
{
  // this should be using the new faster checks. It's only called from outside during expansion atm though, so
  // not worth changing atm (no way of testing speed impact)
  const bool ret = MakeQuadXY().Contains(point);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::Contains(const Triangle2f& tri) const
{
  // this should be using the new faster checks. It's only called from outside during expansion atm though, so
  // not worth changing atm (no way of testing speed impact)
  const Quad2f& myQuad = MakeQuadXY();
  const bool ret = myQuad.Contains(tri[0]) && myQuad.Contains(tri[1]) && myQuad.Contains(tri[2]);
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
bool NavMeshQuadTreeNode::AddContentPoint(const Point2f& point, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddContentPoint");
  
  // set up optimized triangle checks
  bool changed = AddPoint_Recursive(point, detectedContent, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddContentTriangle(const Triangle2f& tri, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddContentTriangle");
  
  // set up optimized triangle checks
  bool changed = AddTriangle_Setup(tri, detectedContent, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddContentLine(const Point2f& from, const Point2f& to,
  const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddContentLine");

  // add line recursively
  SegmentLineEquation segmentLine(from, to);
  bool changed = AddLine_Recursive(segmentLine, detectedContent, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddContentQuad(const Quad2f& quad, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddContentQuad");
  
  // delegate on optimized implementation (under testing)
  bool changed = AddQuad_NewSetup(quad, detectedContent, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddQuad_OldRecursive(const Quad2f& quad, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  // ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddQuad_OldRecursive"); // recursive functions don't properly show averages
  
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
            childChanged = childPtr->AddQuad_OldRecursive(quad, detectedContent, processor) || childChanged;
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
          childChanged = childPtr->AddQuad_OldRecursive(quad, detectedContent, processor) || childChanged;
        }
        
        // try to automerge (if it does, our content type will change from subdivided to the merged type)
        if ( childChanged ) {
          TryAutoMerge(processor);
        }
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
bool NavMeshQuadTreeNode::ShiftRoot(const std::vector<Point2f>& requiredPoints, NavMeshQuadTreeProcessor& processor)
{
  bool xPlusAxisReq  = false;
  bool xMinusAxisReq = false;
  bool yPlusAxisReq  = false;
  bool yMinusAxisReq = false;
  const float rootHalfLen = _sideLen * 0.5f;

  // iterate every point and see what direction they need the root to shift towards
  for( const Point2f& p : requiredPoints )
  {
    xPlusAxisReq  = xPlusAxisReq  || FLT_GE(p.x(), _center.x()+rootHalfLen);
    xMinusAxisReq = xMinusAxisReq || FLT_LE(p.x(), _center.x()-rootHalfLen);
    yPlusAxisReq  = yPlusAxisReq  || FLT_GE(p.y(), _center.y()+rootHalfLen);
    yMinusAxisReq = yMinusAxisReq || FLT_LE(p.y(), _center.y()-rootHalfLen);
  }
  
  // can't shift +x and -x at the same time
  if ( xPlusAxisReq && xMinusAxisReq ) {
    PRINT_NAMED_WARNING("NavMeshQuadTreeNode.ShiftRoot.CantShiftPMx", "Current root size can't accomodate given points");
    return false;
  }

  // can't shift +y and -y at the same time
  if ( yPlusAxisReq && yMinusAxisReq ) {
    PRINT_NAMED_WARNING("NavMeshQuadTreeNode.ShiftRoot.CantShiftPMy", "Current root size can't accomodate given points");
    return false;
  }

  // cache which axes we shift in
  const bool xShift = xPlusAxisReq || xMinusAxisReq;
  const bool yShift = yPlusAxisReq || yMinusAxisReq;
  if ( !xShift && !yShift ) {
    // this means all points are contained in this node, we shouldn't be here
    PRINT_NAMED_ERROR("NavMeshQuadTreeNode.ShiftRoot.AllPointsIn", "We don't need to shift");
    return false;
  }

  // the new center will be shifted in one or both axes, depending on xyIncrease
  // for example, if we left the root through the right, only the right side will expand, and the left will collapse,
  // but top and bottom will remain the same
  _center.x() = _center.x() + (xShift ? (xPlusAxisReq ? rootHalfLen : -rootHalfLen) : 0.0f);
  _center.y() = _center.y() + (yShift ? (yPlusAxisReq ? rootHalfLen : -rootHalfLen) : 0.0f);
  
  // if the root has children, update them, otherwise no further changes are necessary
  if ( !_childrenPtr.empty() )
  {
    // save my old children so that we can swap them with the new ones
    std::vector< std::unique_ptr<NavMeshQuadTreeNode> > oldChildren;
    std::swap(oldChildren, _childrenPtr);
    
    // create new children
    const float chHalfLen = rootHalfLen*0.5f;
    _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()+chHalfLen, _center.y()+chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::TopLeft , this) ); // up L
    _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()+chHalfLen, _center.y()-chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::TopRight, this) ); // up R
    _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()-chHalfLen, _center.y()+chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::BotLeft , this) ); // lo L
    _childrenPtr.emplace_back( new NavMeshQuadTreeNode(Point3f{_center.x()-chHalfLen, _center.y()-chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::BotRight, this) ); // lo R

    // typedef to cast quadrant enum to the underlaying type (that can be assigned to size_t)
    using Q2N = std::underlying_type<EQuadrant>::type; // Q2N stands for "Quadrant To Number", it makes code below easier to read
    static_assert( sizeof(Q2N) < sizeof(size_t), "UnderlyingTypeIsBiggerThanSizeType");
    
    /* 
      Example of shift along both axes +x,+y
    
                      ^                                           ^ +y
                      | +y                                        |---- ----
                                                                  |    | TL |
                  ---- ----                                        ---- ----
        -x       | BL | TL |     +x               -x              | BR |    |  +x
       < ---      ---- ----      --->              < ---           ---- ----  --->
                 | BR | TR |
                  ---- ----
     
                      | -y                                        | -y
                      v                                           v
     
       Since the root can't expand anymore, we move it in the direction we would want to expand. Note in the example
       how TopLeft becomes BottomRight in the new root. We want to preserve the children of that direct child (old TL), but
       we need to hook them to a different child (new BR). That's essentially what the rest of this method does.
     
    */
    
    // this content is set to the children that don't inherit old children
    NodeContent emptyUnknownContent(ENodeContentType::Unknown);
    
    // calculate which children are brought over from the old ones
    if ( xShift && yShift )
    {
      // double move, only one child is preserved, which is the one in the same direction top the expansion one
      if ( xPlusAxisReq ) {
        if ( yPlusAxisReq ) {
          // we are moving along +x +y axes, top left becomes bottom right of the new root
          _childrenPtr[(Q2N)EQuadrant::TopLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::TopRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::BotLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::BotRight]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::TopLeft].get(), processor);
        } else {
          // we are moving along +x -y axes, top right becomes bottom left of the new root
          _childrenPtr[(Q2N)EQuadrant::TopLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::TopRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::BotLeft ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::TopRight].get(), processor);
          _childrenPtr[(Q2N)EQuadrant::BotRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        }
      }
      else
      {
        if ( yPlusAxisReq ) {
          // we are moving along -x +y axes, bottom left becomes top right of the new root
          _childrenPtr[(Q2N)EQuadrant::TopLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::TopRight]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::BotLeft].get(), processor);
          _childrenPtr[(Q2N)EQuadrant::BotLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::BotRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        } else {
          // we are moving along -x -y axes, bottom right becomes top left of the new root
          _childrenPtr[(Q2N)EQuadrant::TopLeft ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::BotRight].get(), processor);
          _childrenPtr[(Q2N)EQuadrant::TopRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::BotLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
          _childrenPtr[(Q2N)EQuadrant::BotRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        }
      }
    }
    else if ( xShift )
    {
      // move only in one axis, two children are preserved, top or bottom
      if ( xPlusAxisReq )
      {
        // we are moving along +x axis, top children are preserved, but they become the bottom ones
        _childrenPtr[(Q2N)EQuadrant::TopLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        _childrenPtr[(Q2N)EQuadrant::TopRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        _childrenPtr[(Q2N)EQuadrant::BotLeft ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::TopLeft].get(), processor );
        _childrenPtr[(Q2N)EQuadrant::BotRight]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::TopRight].get(), processor);
      }
      else
      {
        // we are moving along -x axis, bottom children are preserved, but they become the top ones
        _childrenPtr[(Q2N)EQuadrant::TopLeft ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::BotLeft].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::TopRight]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::BotRight].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::BotLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        _childrenPtr[(Q2N)EQuadrant::BotRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
      }
    }
    else if ( yShift )
    {
      // move only in one axis, two children are preserved, left or right
      if ( yPlusAxisReq )
      {
        // we are moving along +y axis, left children are preserved, but they become the right ones
        _childrenPtr[(Q2N)EQuadrant::TopLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        _childrenPtr[(Q2N)EQuadrant::TopRight]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::TopLeft].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::BotLeft ]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        _childrenPtr[(Q2N)EQuadrant::BotRight]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::BotLeft].get(), processor);
      }
      else
      {
        // we are moving along -y axis, right children are preserved, but they become the left ones
        _childrenPtr[(Q2N)EQuadrant::TopLeft ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::TopRight].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::TopRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
        _childrenPtr[(Q2N)EQuadrant::BotLeft ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::BotRight].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::BotRight]->ForceSetDetectedContentType(emptyUnknownContent, processor);
      }
    }
    
    // destroy the nodes that are going away because we shifted away from them
    DestroyNodes(oldChildren, processor);
  }
  
  // log
  PRINT_CH_INFO("QuadTree", "QuadTree.ShiftRoot", "Root level is still %u, root shifted. Allowing %.2fm", _level, MM_TO_M(_sideLen));
  
  // successful shift
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::UpgradeRootLevel(const Point2f& direction, uint8_t maxRootLevel, NavMeshQuadTreeProcessor& processor)
{
  DEV_ASSERT(!NEAR_ZERO(direction.x()) || !NEAR_ZERO(direction.y()),
             "NavMeshQuadTreeNode.UpgradeRootLevel.InvalidDirection");
  
  // reached expansion limit
  if ( _level == std::numeric_limits<uint8_t>::max() || _level >= maxRootLevel) {
    return false;
  }

  // save my old children to store in the child that is taking my spot
  std::vector< std::unique_ptr<NavMeshQuadTreeNode> > oldChildren;
  std::swap(oldChildren, _childrenPtr);

  const bool xPlus = FLT_GE_ZERO(direction.x());
  const bool yPlus = FLT_GE_ZERO(direction.y());
  
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
  
  // we have to set the new first level children as Unknown, since they are initialized as Invalid
  // except the child that takes my place, since that one is going to inherit my content
  NodeContent emptyUnknownContent(ENodeContentType::Unknown);
  for(size_t idx=0; idx<_childrenPtr.size(); ++idx) {
    if ( idx != childIdx ) {
      _childrenPtr[idx]->ForceSetDetectedContentType(emptyUnknownContent, processor);
    }
  }
  
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

  // log
  PRINT_CH_INFO("QuadTree", "QuadTree.UpdgradeRootLevel", "Root expanded to level %u. Allowing %.2fm", _level, MM_TO_M(_sideLen));
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddQuadsToSend(QuadInfoVector& quadInfoVector) const
{
  // if we have children, delegate on them, otherwise add data about ourselves
  if ( _childrenPtr.empty() )
  {
    const auto contentTypeExternal = ConvertContentType(_content.type);
    quadInfoVector.emplace_back(ExternalInterface::MemoryMapQuadInfo(contentTypeExternal, _level));
  }
  else
  {
    // delegate on each child
    for( const auto& childPtr : _childrenPtr ) {
      childPtr->AddQuadsToSend(quadInfoVector);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::AddQuadsToSendDebugViz(QuadInfoDebugVizVector& quadInfoVector) const
{
  // if we have children, delegate on them, otherwise add data about ourselves
  if ( _childrenPtr.empty() )
  {
    const auto contentTypeDebugViz = ConvertContentTypeDebugViz(_content.type);
    quadInfoVector.emplace_back(ExternalInterface::MemoryMapQuadInfoDebugViz(contentTypeDebugViz, _level));
  }
  else
  {
    // delegate on each child
    for( const auto& childPtr : _childrenPtr ) {
      childPtr->AddQuadsToSendDebugViz(quadInfoVector);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::Subdivide(NavMeshQuadTreeProcessor& processor)
{
  DEV_ASSERT(CanSubdivide() && !IsSubdivided(), "NavMeshQuadTreeNode.Subdivide.InvalidSubdivide");
  DEV_ASSERT(_level > 0, "NavMeshQuadTreeNode.Subdivide.InvalidLevel");
  
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
  DEV_ASSERT(IsSubdivided(), "NavMeshQuadTreeNode.Merge.InvalidState");

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
  // TODO To guarantee that the future doesn't break this, we should require a matrix of old vs new to be fully
  // specified here. Some values however depend on overlap, so we could not cache / make const

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
  else if ( newContentType == ENodeContentType::ClearOfObstacle )
  {
    // ClearOfObstacle can't override ClearOfCliff, since it's less restrictive
    if ( _content.type == ENodeContentType::ClearOfCliff ) {
      return false;
    }

    // because quads store information as long as they are touched by something, ClearOfObstacle should
    // not clear basic types unless it has covered them fully. For example, this fixes obstacles or borders
    // being cleared just because we clear from the robot to the marker. We do not want to clear below the marker
    // unless the quad is fully contained (this will prevent lines from destroying content)
    if ( ( _content.type == ENodeContentType::ObstacleCube         ) ||
         ( _content.type == ENodeContentType::ObstacleCharger      ) ||
         ( _content.type == ENodeContentType::ObstacleUnrecognized ) ||
         ( _content.type == ENodeContentType::InterestingEdge      ) ||
         ( _content.type == ENodeContentType::NotInterestingEdge   ) )
    {
      const bool isTotalClear = (overlap == EContentOverlap::Total);
      return isTotalClear;
    }
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
  DEV_ASSERT(IsSubdivided(), "NavMeshQuadTreeNode.TryAutoMerge.NotSubdivided");

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
void NavMeshQuadTreeNode::SwapChildrenAndContent(NavMeshQuadTreeNode* otherNode, NavMeshQuadTreeProcessor& processor )
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
  NodeContent myPrevContent = _content;
  ForceSetDetectedContentType(otherNode->GetContent(), processor);
  otherNode->ForceSetDetectedContentType(myPrevContent, processor);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeNode::DestroyNodes(ChildrenVector& nodes, NavMeshQuadTreeProcessor& processor)
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
  
  DEV_ASSERT(from <= EQuadrant::Root, "NavMeshQuadTreeNode.GetDestination.InvalidQuadrant");
  DEV_ASSERT(direction <= EDirection::West, "NavMeshQuadTreeNode.GetDestination.InvalidDirection");
  
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
        DEV_ASSERT(false, "NavMeshQuadTreeNode.AddSmallDescendants.InvalidDirection");
      }
    }
    
    DEV_ASSERT(firstChild != EQuadrant::Invalid, "NavMeshQuadTreeNode.AddSmallDescendants.InvalidFirstChild");
    DEV_ASSERT(secondChild != EQuadrant::Invalid, "NavMeshQuadTreeNode.AddSmallDescendants.InvalidSecondChild");
    
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
      DEV_ASSERT(_parent, "NavMeshQuadTreeNode.FindSingleNeighbor.InvalidParent");
      neighbor = _parent->GetChild( moveInfo->neighborQuadrant );
      DEV_ASSERT(neighbor, "NavMeshQuadTreeNode.FindSingleNeighbor.InvalidNeighbor");
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Optimizations
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace QTOptimizations {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This function was useful when Contains was slow for nonAAQuads, but now that is faster it doesn't gain much in the
// best case, and it does weigh for the worse case (bad happens more often than good)
//inline void GetAABBox(const Quad2f& nonAAQuad, Point2f& outMin, Point2f& outMax)
//{
//  const Point2f& cornerTL = nonAAQuad.GetTopLeft();
//  const Point2f& cornerTR = nonAAQuad.GetTopRight();
//  const Point2f& cornerBL = nonAAQuad.GetBottomLeft();
//  const Point2f& cornerBR = nonAAQuad.GetBottomRight();
//  outMin.x() = MIN( MIN(cornerTL.x(), cornerTR.x()), MIN(cornerBL.x(), cornerBR.x()));
//  outMin.y() = MIN( MIN(cornerTL.y(), cornerTR.y()), MIN(cornerBL.y(), cornerBR.y()));
//  outMax.x() = MAX( MAX(cornerTL.x(), cornerTR.x()), MAX(cornerBL.x(), cornerBR.x()));
//  outMax.y() = MAX( MAX(cornerTL.y(), cornerTR.y()), MAX(cornerBL.y(), cornerBR.y()));
//}

// Standard AA Quad:
// Top    = max X
// Bottom = min X
// Left   = max Y
// Right  = min Y
// PositiveX is Bottom -> Top
// PositiveY is Right -> Left
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
__attribute__ ((used))
inline bool IsStandardAA(const Quad2f& quad)
{
  ANKI_CPU_PROFILE("NewQuadCheck::IsStandardAA");
  const Point2f& cornerTL = quad.GetTopLeft();
  const Point2f& cornerTR = quad.GetTopRight();
  const Point2f& cornerBL = quad.GetBottomLeft();
  const Point2f& cornerBR = quad.GetBottomRight();
  const bool ret =
    Anki::Util::IsFltNear(cornerTL.x(), cornerTR.x() ) && // TopLeft and TopRight have same X
    Anki::Util::IsFltNear(cornerBL.x(), cornerBR.x() ) && // BottomLeft and BottomRight have same X
    Anki::Util::IsFltGT  (cornerTL.x(), cornerBL.x() ) && // TopLeft has greater X than BottomLeft (and *right because of previous checks)
    Anki::Util::IsFltNear(cornerTL.y(), cornerBL.y() ) && // TopLeft and BottomLeft have same Y
    Anki::Util::IsFltNear(cornerTR.y(), cornerBR.y() ) && // TopRight and BottomRight have same Y
    Anki::Util::IsFltGT  (cornerTL.y(), cornerTR.y() ) ;  // TopLeft has greater Y than TopRight (and bottom* because of previous checks)
  return ret;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// is point within aaBBox
inline bool Contains(const Point2f& min, const Point2f& max, const Point2f& point)
{
  const bool contains =
    FLT_GE(point.x(), min.x()) &&
    FLT_LE(point.x(), max.x()) &&
    FLT_GE(point.y(), min.y()) &&
    FLT_LE(point.y(), max.y());
  return contains;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool IntersectsX(const float atX,
  const float minY, const float maxY,
  const NavMeshQuadTreeNode::SegmentLineEquation& line )
{
  if ( line.isXAligned )
  {
    // if the line is X aligned, it intersects if they overlap. If they overlap however, this should be
    // caught by Contains, so as an optimization, do not bother rechecking here.
    return false;
  }
  else
  {
    // line is not aligned with X
    
    // true if the segment intersects with the segment that goes from (atX,minY) to (atX,maxY)
    // 1) check segment.x crosses atX
    // 1.1) or at least one is atX, but since we call this after Contains, there is not need to check that
    const bool crosses = ( FLT_GE(line.from.x(), atX) != FLT_GE(line.to.x(), atX) ) ;
    if ( !crosses ) {
      return false;
    }
  
    // 2) find segment.y at 'atX'
    const float segYatX = line.isYAligned ?
      (line.from.y()) :    // from.y == to.y, and all other Ys
      (line.segM * atX + line.segB); // y = mx + b
    // 2.1) check that segmentAtX.y() is between minY and maxY
    const bool intersectsBetweenMinMaxY = ( FLT_GE(segYatX, minY) && FLT_LE(segYatX, maxY) );
    return intersectsBetweenMinMaxY;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool IntersectsY(const float atY,
  const float minX, const float maxX,
  const NavMeshQuadTreeNode::SegmentLineEquation& line)
{
  if ( line.isYAligned )
  {
    // if the line is Y aligned, it intersects if they overlap. If they overlap however, this should be
    // caught by Contains, so as an optimization, do not bother rechecking here.
    return false;
  }
  else
  {
    // true if the segment intersects with the segment that goes from (minX,atY) to (maxX,atY)
    // 1) check segment.y crosses atY
    // 1.1) or at least one is atY, but since we call this after Contains, there is not need to check that
    const bool crosses = ( FLT_GE(line.from.y(), atY) != FLT_GE(line.to.y(), atY) ) ;
    if ( !crosses ) {
      return false;
    }
    
    // 2) find segment.x at 'atY'
    const float segXatY = line.isXAligned ?
    (line.from.x()):                    // from.x == to.x, and all other Xs
    (atY - line.segB) * line.oneOverM;  // y = mx + b -> x = (y-b)/m
    // 2.1) check that segmentAtX.y() is between minY and maxY
    const bool intersectsBetweenMinMaxX = ( FLT_GE(segXatY, minX) && FLT_LE(segXatY, maxX) );
    return intersectsBetweenMinMaxX;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Calculates the sign of p1 with respect to p2 and p3; the sign being which halfPlane it falls into, the positive
// or negative with respect to the line p2<->p3
float HalfPlaneSign(const Point2f& p1, const Point2f& p2, const Point2f& p3)
{
  return (p1.x() - p3.x()) * (p2.y() - p3.y()) - (p2.x() - p3.x()) * (p1.y() - p3.y());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// IsPointInTri If the point in on the same side of the three half-planes. Alternatively, if we know the direction
// in which the tri edges are defined (CW vs CCW), we know what Sign to expect, so we could rule out as soon as the point
// is out of any of the planes. This version allows Tris to come CW or CCW
bool IsPointInTri(const Point2f& pt, const Point2f& v1, const Point2f& v2, const Point2f& v3)
{
  const bool b1 = FLT_LE(HalfPlaneSign(pt, v1, v2), 0.0f);
  const bool b2 = FLT_LE(HalfPlaneSign(pt, v2, v3), 0.0f);
  const bool b3 = FLT_LE(HalfPlaneSign(pt, v3, v1), 0.0f);

  return ((b1 == b2) && (b2 == b3));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IsPointInNonAAQuad(const Quad2f& nonAAQuad, const Point2f& pt)
{
  return
    IsPointInTri(pt, nonAAQuad.GetBottomLeft(), nonAAQuad.GetTopLeft(), nonAAQuad.GetBottomRight()) ||
    IsPointInTri(pt, nonAAQuad.GetBottomRight(), nonAAQuad.GetTopLeft(), nonAAQuad.GetTopRight());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// This function is a little helper for 1 point, but do not call several times if you ahve a set of points, since it can
// be slightly slower than callin Contains directly with cached aaMin/aaMax
inline bool IsPointInAAQuad(const Quad2f& aaQuad, const Point2f& pt)
{
  assert(IsStandardAA(aaQuad));
  
  // get aa bbox and check against point
  const Point2f& aaMax = aaQuad.GetTopLeft();
  const Point2f& aaMin = aaQuad.GetBottomRight();
  const bool isInside = Contains(aaMin, aaMax, pt);
  return isInside;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// intersection check that is optimized when one quad is axis aligned. In the case that there is intersection,
// it also stores in isAAQuadContainedInNonAAQuad whether the axisAligned quad is fully inside the nonAA one.
__attribute__ ((used))
inline bool OverlapsOrContains(const Quad2f& axisAlignedQuad, const Quad2f& nonAAQuad,
  const NavMeshQuadTreeNode::QuadSegmentArray& nonAAQuadSegments,
  bool& isAAQuadContainedInNonAAQuad)
{
  assert(IsStandardAA(axisAlignedQuad));
  
  // if any of the corners in the nonAAQuad is inside the quad
  const Point2f& aaMax = axisAlignedQuad.GetTopLeft();
  const Point2f& aaMin = axisAlignedQuad.GetBottomRight();
  const bool aaContainsSomePoint =
    Contains(aaMin, aaMax, nonAAQuad.GetTopLeft()    ) ||
    Contains(aaMin, aaMax, nonAAQuad.GetTopRight()   ) ||
    Contains(aaMin, aaMax, nonAAQuad.GetBottomLeft() ) ||
    Contains(aaMin, aaMax, nonAAQuad.GetBottomRight());
  if ( aaContainsSomePoint ) {
    // the aa quad is not contained, since aaQuad contains at least one point from non-aa
    isAAQuadContainedInNonAAQuad = false;
    // however they do overlap
    return true;
  }
  
  // iterate all segments from the nonAAQuad
  for(const NavMeshQuadTreeNode::SegmentLineEquation& curLine : nonAAQuadSegments)
  {
    // if any segment intersects with the AA lines, the quads intersect
    const bool segmentIntersects =
      IntersectsX(aaMin.x(), aaMin.y(), aaMax.y(), curLine) ||
      IntersectsX(aaMax.x(), aaMin.y(), aaMax.y(), curLine) ||
      IntersectsY(aaMin.y(), aaMin.x(), aaMax.x(), curLine) ||
      IntersectsY(aaMax.y(), aaMin.x(), aaMax.x(), curLine);
    if ( segmentIntersects ) {
      // the aa quad is not contained, since edges intersect (consider overlapping edges as not containment as optimization)
      isAAQuadContainedInNonAAQuad = false;
      // however they do overlap
      return true;
    }
  }
  
  // In order to calculate whether we overlap we need to check whether at least one point is contained.
  // Note that since we checked segment collisions before, and we are here, we know there are NO edge collisions. That
  // means that either all points from the aaQuad are inside the nonAAQuad, or none are. We only need to check one
  // (We could do the same to check whether nonAA points are inside the aaQuad, but that's a faster check).
  bool containsAnyAndAll = IsPointInNonAAQuad(nonAAQuad, axisAlignedQuad.GetTopLeft() );
  if ( containsAnyAndAll ) {
    // at least one point is inside, and there's no collision, all are
    isAAQuadContainedInNonAAQuad = containsAnyAndAll;
    // quads overlap
    return true;
  }
  
  // quads don't overlap
  isAAQuadContainedInNonAAQuad = false;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// intersection check that is optimized when the quad is axis aligned. In the case that there is intersection,
// it also stores in isQuadContainedInTriangle whether the axisAligned quad is fully inside the triangle
__attribute__ ((used))
inline bool OverlapsOrContains(const Quad2f& axisAlignedQuad,
  const Triangle2f& triangle,
  const NavMeshQuadTreeNode::TriangleSegmentArray& triSegments,
  bool& isQuadContainedInTriangle)
{
  assert(IsStandardAA(axisAlignedQuad));
  
  // if any of the corners in the nonAAQuad is inside the quad
  const Point2f& aaMax = axisAlignedQuad.GetTopLeft();
  const Point2f& aaMin = axisAlignedQuad.GetBottomRight();
  const bool aaContainsSomePoint =
    Contains(aaMin, aaMax, triangle[0] ) ||
    Contains(aaMin, aaMax, triangle[1] ) ||
    Contains(aaMin, aaMax, triangle[2] );
  if ( aaContainsSomePoint ) {
    // the aa quad is not contained, since aaQuad contains at least one point from non-aa
    isQuadContainedInTriangle = false;
    // however they do overlap
    return true;
  }
  
  // iterate all segments from the triangle
  for(const NavMeshQuadTreeNode::SegmentLineEquation& curLine : triSegments)
  {
    // if any segment intersects with the AA lines, the quads intersect
    const bool segmentIntersects =
      IntersectsX(aaMin.x(), aaMin.y(), aaMax.y(), curLine) ||
      IntersectsX(aaMax.x(), aaMin.y(), aaMax.y(), curLine) ||
      IntersectsY(aaMin.y(), aaMin.x(), aaMax.x(), curLine) ||
      IntersectsY(aaMax.y(), aaMin.x(), aaMax.x(), curLine);
    if ( segmentIntersects ) {
      // the aa quad is not contained, since edges intersect (consider overlapping edges as not containment as optimization)
      isQuadContainedInTriangle = false;
      // however they do overlap
      return true;
    }
  }
  
  // In order to calculate whether we intersect we need to check whether at least one point is contained.
  // Note that since we checked segment collisions before, and we are here, we know there are NO edge collisions. That
  // means that either all points from the aaQuad are inside the nonAAQuad, or none are. We only need to check one
  // (We could do the same to check whether nonAA points are inside the aaQuad, but that's a faster check).
  bool containsAnyAndAll = IsPointInTri(axisAlignedQuad.GetTopLeft(), triangle[0], triangle[1], triangle[2]);
  if ( containsAnyAndAll ) {
    // at least one point is inside, and there's no collision, all are
    isQuadContainedInTriangle = containsAnyAndAll;
    // quads intersect
    return true;
  }
  
  // quads don't overlap
  isQuadContainedInTriangle = false;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// intersection check for line that is optimized when the quad is axis aligned
// doesLineCrossQuad: true if the line crosses the quad (two intersections), false if one end stays inside the quad
__attribute__ ((used))
inline bool OverlapsOrContains(const Quad2f& axisAlignedQuad,
  const NavMeshQuadTreeNode::SegmentLineEquation& line,
  bool& doesLineCrossQuad)
{
  assert(IsStandardAA(axisAlignedQuad));
  
  // if any of the points from the line is inside the aaQuad
  const Point2f& aaMax = axisAlignedQuad.GetTopLeft();
  const Point2f& aaMin = axisAlignedQuad.GetBottomRight();
  const bool aaContainsSomePoint =
    Contains(aaMin, aaMax, line.from ) ||
    Contains(aaMin, aaMax, line.to   );
  if ( aaContainsSomePoint ) {
    // at least one end is inside the quad
    doesLineCrossQuad = false;
    return true;
  }
  
  // if any segment intersects with the AA lines, the quads intersect
  const bool segmentIntersects =
    IntersectsX(aaMin.x(), aaMin.y(), aaMax.y(), line) ||
    IntersectsX(aaMax.x(), aaMin.y(), aaMax.y(), line) ||
    IntersectsY(aaMin.y(), aaMin.x(), aaMax.x(), line) ||
    IntersectsY(aaMax.y(), aaMin.x(), aaMax.x(), line);
  if ( segmentIntersects ) {
    // they intersect, and since the line points are not inside the quad, the line crossed the quad
    doesLineCrossQuad = true;
    return true;
  }

  // does not contain or intersect with the segment
  doesLineCrossQuad = false;
  return false;
}

};
  
using namespace QTOptimizations;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::ContainsOrOverlapsQuad(const Quad2f& inQuad) const
{
  // get my quad
  const Quad2f& myQuad = MakeQuadXY();
  
  // break the quad into segments to fast-compute m and b for them
  QuadSegmentArray nonAAQuadSegments = {
    SegmentLineEquation(inQuad.GetTopLeft(), inQuad.GetTopRight()),
    SegmentLineEquation(inQuad.GetTopRight(), inQuad.GetBottomRight()),
    SegmentLineEquation(inQuad.GetBottomRight(), inQuad.GetBottomLeft()),
    SegmentLineEquation(inQuad.GetBottomLeft(), inQuad.GetTopLeft()) };
  
  // call optimized version of addquad
  bool containmentFlag;
  const bool ret = OverlapsOrContains(myQuad, inQuad, nonAAQuadSegments, containmentFlag);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddQuad_NewSetup(const Quad2f &quad, const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  // break the quad into segments to fast-compute m and b for them
  QuadSegmentArray nonAAQuadSegments = {
    SegmentLineEquation(quad.GetTopLeft(), quad.GetTopRight()),
    SegmentLineEquation(quad.GetTopRight(), quad.GetBottomRight()),
    SegmentLineEquation(quad.GetBottomRight(), quad.GetBottomLeft()),
    SegmentLineEquation(quad.GetBottomLeft(), quad.GetTopLeft()) };
  
  // call optimized version of addquad
  const bool ret = AddQuad_NewRecursive(quad, nonAAQuadSegments, detectedContent, processor);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddQuad_NewRecursive(const Quad2f& quad,
  const QuadSegmentArray& nonAAQuadSegments,
  const NodeContent& detectedContent,
  NavMeshQuadTreeProcessor& processor)
{
  // ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddQuad_NewRecursive"); // recursive functions don't properly show averages
  
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
  bool isMyQuadContained = false;
  const bool quadsOverlap = OverlapsOrContains(myQuad, quad, nonAAQuadSegments, isMyQuadContained);
  if ( quadsOverlap )
  {
    const EContentOverlap overlap = isMyQuadContained ? EContentOverlap::Total : EContentOverlap::Partial;
  
    // am I fully contained within the quad?
    if ( isMyQuadContained )
    {
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
            childChanged = childPtr->AddQuad_NewRecursive(quad, nonAAQuadSegments, detectedContent, processor) || childChanged;
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
          childChanged = childPtr->AddQuad_NewRecursive(quad, nonAAQuadSegments, detectedContent, processor) || childChanged;
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
bool NavMeshQuadTreeNode::AddTriangle_Setup(const Triangle2f& triangle,
  const NodeContent& detectedContent,
  NavMeshQuadTreeProcessor& processor)
{
  // break the quad into segments to fast-compute m and b for them
  TriangleSegmentArray triangleSegments = {
    SegmentLineEquation(triangle[0], triangle[1]),
    SegmentLineEquation(triangle[1], triangle[2]),
    SegmentLineEquation(triangle[2], triangle[0])};

  // call optimized version of addquad
  const bool ret = AddTriangle_Recursive(triangle, triangleSegments, detectedContent, processor);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddTriangle_Recursive(const Triangle2f& triangle,
  const TriangleSegmentArray& triangleSegments,
  const NodeContent& detectedContent,
  NavMeshQuadTreeProcessor& processor)
{
  // ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddTriangle_Recursive"); // recursive functions don't properly show averages

  // if we won't gain any new info, no need to process
  const bool isSameInfo = _content == detectedContent;
  if ( isSameInfo ) {
    return false;
  }

  // to check for changes
  NodeContent previousContent = _content;
  bool childChanged = false;

  // check if the triangle affects us
  const Quad2f& myQuad = MakeQuadXY();
  bool isMyQuadContained = false;
  const bool quadsOverlap = OverlapsOrContains(myQuad, triangle, triangleSegments, isMyQuadContained);
  if ( quadsOverlap )
  {
    const EContentOverlap overlap = isMyQuadContained ? EContentOverlap::Total : EContentOverlap::Partial;
  
    // am I fully contained within the quad?
    if ( isMyQuadContained )
    {
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
            childChanged = childPtr->AddTriangle_Recursive(triangle, triangleSegments, detectedContent, processor) || childChanged;
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
          childChanged = childPtr->AddTriangle_Recursive(triangle, triangleSegments, detectedContent, processor) || childChanged;
        }
        
        // try to automerge (if it does, our content type will change from subdivided to the merged type)
        if ( childChanged ) {
          TryAutoMerge(processor);
        }
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
bool NavMeshQuadTreeNode::AddLine_Recursive(const SegmentLineEquation& segmentLine,
  const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  // ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddLine_Recursive"); // recursive functions don't properly show averages
  
  // if we won't gain any new info, no need to process
  const bool isSameInfo = _content == detectedContent;
  if ( isSameInfo ) {
    return false;
  }
  
  // to check for changes
  NodeContent previousContent = _content;
  bool childChanged = false;

  // check if the line affects us
  bool doesLineCrossQuad = false;
  const Quad2f& myQuad = MakeQuadXY();
  const bool isQuadAffected = OverlapsOrContains(myQuad, segmentLine, doesLineCrossQuad);
  if ( isQuadAffected )
  {
    // for any given node, unless the line crosses the exact center of my quad, at least a child won't be affected.
    // In the general case this won't happen, so instead of checking whether lines cross centers, simply try to
    // subdivide. If it turned out to cross it, the children will automerge after checking themselves.
    // For content override, consider that if the line fully crosses the quad, the overlap is total,
    // but if the line stays in, the overlap is partial (this can prevent removing some borders at the edge of
    // the line, while fully crossing a quad means that the new detected border is beyond)
    // const EContentOverlap overlap = doesLineCrossQuad ? EContentOverlap::Total : EContentOverlap::Partial;
    
    // rsam 09/11/2016 note: the comment above is not always right. What I intended was to clear interesting edges
    // that are created from far away with a bad estimation of the ground plane distance. By getting closer and
    // detecting the proper edge, we would be able to clear the previous misleading information. Unfortunately,
    // this can destroy information in other cases by not seeing a partial border in a quad, and allowing a line
    // to clear it just because it's passing by a corner of the quad to a border we detected behind (also would happen
    // with curves). For that reason and until I find a better way to represent this (or to trust timestamps, distance
    // to detected content, etc, as a measurement of trust), I am going to revert lines to being partial
    const EContentOverlap overlap = EContentOverlap::Partial;
  
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
        childChanged = childPtr->AddLine_Recursive(segmentLine, detectedContent, processor) || childChanged;
      }
      
      // try to automerge (if it does, our content type will change from subdivided to the merged type)
      if ( childChanged ) {
        TryAutoMerge(processor);
      }
    }
    else
    {
      TrySetDetectedContentType(detectedContent, overlap, processor);
    }
  }
  
  const bool ret = (_content != previousContent) || childChanged;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeNode::AddPoint_Recursive(const Point2f& point,
  const NodeContent& detectedContent, NavMeshQuadTreeProcessor& processor)
{
  // ANKI_CPU_PROFILE("NavMeshQuadTreeNode::AddPoint_Recursive"); // recursive functions don't properly show averages
  
  // if we won't gain any new info, no need to process
  const bool isSameInfo = _content == detectedContent;
  if ( isSameInfo ) {
    return false;
  }

  // if we won't be overriding any types, there's no reason to process down anymore
  // todo optimization: I could be checking this in all recursive functions, not just on recursive calls. It earlies
  // out if the given content won't have any effect on the children. The problem however is that
  // CanOverrideSelfAndChildrenWithContent currently returns if it can override ALL children, whereas here I want to
  // know if it can override ANY of them. While it can be trivial to implement, it is not trivial to understand
  // whether it's an optimization. Note that I would have to go down to the lowest level every time, checking if it can
  // be overridden. For the positive cases, this means at least N-1 explorations at level N, O(n^2) superflous checks.
  //  const bool canOverride = CanOverrideSelfAndChildrenWithContent(detectedContent.type, EContentOverlap::Partial);
  //  if ( !canOverride ) {
  //    return false;
  //  }
  
  // to check for changes
  NodeContent previousContent = _content;
  bool childChanged = false;

  // check if the point is in this quad
  const Quad2f& myQuad = MakeQuadXY();
  const bool quadContainsPoint = IsPointInAAQuad(myQuad, point);
  if ( quadContainsPoint )
  {
    const EContentOverlap overlap = EContentOverlap::Partial; // makes sense
  
    // for any given node, a point can only be in one of their children, so we should always subdivide, since potentially
    // only one of the children will inherit the type
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
        childChanged = childChanged = childPtr->AddPoint_Recursive(point, detectedContent, processor) || childChanged;
      }
      
      // try to automerge (if it does, our content type will change from subdivided to the merged type)
      if ( childChanged ) {
        TryAutoMerge(processor);
      }
    }
    else
    {
      TrySetDetectedContentType(detectedContent, overlap, processor);
    }
  }
  
  const bool ret = (_content != previousContent) || childChanged;
  return ret;
}

} // namespace Cozmo
} // namespace Anki
