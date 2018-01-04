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

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
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
using namespace MemoryMapTypes;
using namespace ExternalInterface;

ENodeContentTypeEnum ConvertContentType(EContentType contentType)
{
  
  ENodeContentTypeEnum externalContentType = ENodeContentTypeEnum::Unknown;
  switch (contentType) {
    case EContentType::Unknown:               { externalContentType = ENodeContentTypeEnum::Unknown;              break; }
    case EContentType::ClearOfObstacle:       { externalContentType = ENodeContentTypeEnum::ClearOfObstacle;      break; }
    case EContentType::ClearOfCliff:          { externalContentType = ENodeContentTypeEnum::ClearOfCliff;         break; }
    case EContentType::ObstacleCube:          { externalContentType = ENodeContentTypeEnum::ObstacleCube;         break; }
    case EContentType::ObstacleCubeRemoved:   { DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType");      break; } // Should never get this
    case EContentType::ObstacleCharger:       { externalContentType = ENodeContentTypeEnum::ObstacleCharger;      break; }
    case EContentType::ObstacleChargerRemoved:{ DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType");      break; } // Should never get this
    case EContentType::ObstacleProx:          { externalContentType = ENodeContentTypeEnum::ObstacleProx;         break; } 
    case EContentType::ObstacleUnrecognized:  { externalContentType = ENodeContentTypeEnum::ObstacleUnrecognized; break; }
    case EContentType::Cliff:                 { externalContentType = ENodeContentTypeEnum::Cliff;                break; }
    case EContentType::InterestingEdge:       { externalContentType = ENodeContentTypeEnum::InterestingEdge;      break; }
    case EContentType::NotInterestingEdge:    { externalContentType = ENodeContentTypeEnum::NotInterestingEdge;   break; }
    case EContentType::_Count:                { DEV_ASSERT(false, "NavMeshQuadTreeNode._Count"); break; }
  }
  return externalContentType;
}

} // namespace

static_assert( !std::is_copy_assignable<QuadTreeNode>::value, "QuadTreeNode was designed non-copyable" );
static_assert( !std::is_copy_constructible<QuadTreeNode>::value, "QuadTreeNode was designed non-copyable" );
static_assert( !std::is_move_assignable<QuadTreeNode>::value, "QuadTreeNode was designed non-movable" );
static_assert( !std::is_move_constructible<QuadTreeNode>::value, "QuadTreeNode was designed non-movable" );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::QuadTreeNode(const Point3f &center, float sideLength, uint8_t level, EQuadrant quadrant, QuadTreeNode* parent, MemoryMapData& data)
: _center(center)
, _sideLen(sideLength)
, _boundingBox(center - Point3f(sideLength/2, sideLength/2, 0), center + Point3f(sideLength/2, sideLength/2, 0))
, _parent(parent)
, _level(level)
, _quadrant(quadrant)
, _content(data)
{
  DEV_ASSERT(_quadrant <= EQuadrant::Root, "QuadTreeNode.Constructor.InvalidQuadrant");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeNode::AxisAlignedQuad::AxisAlignedQuad(const Point2f& p, const Point2f& q)
: lowerLeft(  Point2f(fmin(p.x(), q.x()), fmin(p.y(), q.y())) )
, upperRight( Point2f(fmax(p.x(), q.x()), fmax(p.y(), q.y())) )
, diagonals{{ 
    LineSegment( lowerLeft, upperRight), 
    LineSegment( Point2f(fmax(p.x(), q.x()), fmin(p.y(), q.y())), Point2f(fmin(p.x(), q.x()), fmax(p.y(), q.y())) ),
  }} {} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::AxisAlignedQuad::Contains(const Point2f& p) const
{
  return FLT_GE(p.x(), lowerLeft.x()) && FLT_LE(p.x(), upperRight.x()) && 
         FLT_GE(p.y(), lowerLeft.y()) && FLT_LE(p.y(), upperRight.y());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::ResetBoundingBox()
{
  Point3f offset(_sideLen/2, _sideLen/2, 0);
  _boundingBox = AxisAlignedQuad(_center - offset, _center + offset );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::Contains(const FastPolygon& poly) const
{
  for (const auto& pt : poly.GetSimplePolygon()) 
  {
    if (!_boundingBox.Contains(pt)) return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Quad2f QuadTreeNode::MakeQuadXY(const float padding_mm) const
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
const std::unique_ptr<QuadTreeNode>& QuadTreeNode::GetChildAt(size_t index) const
{
  if ( index < _childrenPtr.size() ) {
    return _childrenPtr[index];
  }
  else
  {
    PRINT_NAMED_ERROR("QuadTreeNode.GetChildAt.InvalidIndex",
      "Index %zu is greater than number of children %zu. Returning null",
      index, _childrenPtr.size());
    static std::unique_ptr<QuadTreeNode> nullPtr;
    return nullPtr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::Insert(const FastPolygon& poly, const MemoryMapData& data, QuadTreeProcessor& processor)
{
  ANKI_CPU_PROFILE("QuadTreeNode::Insert");
  
  NodeContent detectedContent(data);
  const bool changed = Insert_Recursive(poly, detectedContent, processor);
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::ShiftRoot(const Poly2f& requiredPoints, QuadTreeProcessor& processor)
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
    PRINT_NAMED_WARNING("QuadTreeNode.ShiftRoot.CantShiftPMx", "Current root size can't accomodate given points");
    return false;
  }

  // can't shift +y and -y at the same time
  if ( yPlusAxisReq && yMinusAxisReq ) {
    PRINT_NAMED_WARNING("QuadTreeNode.ShiftRoot.CantShiftPMy", "Current root size can't accomodate given points");
    return false;
  }

  // cache which axes we shift in
  const bool xShift = xPlusAxisReq || xMinusAxisReq;
  const bool yShift = yPlusAxisReq || yMinusAxisReq;
  if ( !xShift && !yShift ) {
    // this means all points are contained in this node, we shouldn't be here
    PRINT_NAMED_ERROR("QuadTreeNode.ShiftRoot.AllPointsIn", "We don't need to shift");
    return false;
  }

  // the new center will be shifted in one or both axes, depending on xyIncrease
  // for example, if we left the root through the right, only the right side will expand, and the left will collapse,
  // but top and bottom will remain the same
  _center.x() = _center.x() + (xShift ? (xPlusAxisReq ? rootHalfLen : -rootHalfLen) : 0.0f);
  _center.y() = _center.y() + (yShift ? (yPlusAxisReq ? rootHalfLen : -rootHalfLen) : 0.0f);
  ResetBoundingBox();
  
  // if the root has children, update them, otherwise no further changes are necessary
  if ( !_childrenPtr.empty() )
  {
    // save my old children so that we can swap them with the new ones
    std::vector< std::unique_ptr<QuadTreeNode> > oldChildren;
    std::swap(oldChildren, _childrenPtr);
    
    // create new children
    const float chHalfLen = rootHalfLen*0.5f;
    const TimeStamp_t timeCreated = _content.data->GetLastObservedTime();
    MemoryMapData newData(EContentType::Unknown, timeCreated);
      
    _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+chHalfLen, _center.y()+chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::TopLeft , this, newData) ); // up L
    _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+chHalfLen, _center.y()-chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::TopRight, this, newData) ); // up R
    _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-chHalfLen, _center.y()+chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::BotLeft , this, newData) ); // lo L
    _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-chHalfLen, _center.y()-chHalfLen, _center.z()}, rootHalfLen, _level-1, EQuadrant::BotRight, this, newData) ); // lo R

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
    MemoryMapData data(EContentType::Unknown, timeCreated);
    NodeContent emptyUnknownContent(data);
    
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
bool QuadTreeNode::UpgradeRootLevel(const Point2f& direction, uint8_t maxRootLevel, QuadTreeProcessor& processor)
{
  DEV_ASSERT(!NEAR_ZERO(direction.x()) || !NEAR_ZERO(direction.y()),
             "QuadTreeNode.UpgradeRootLevel.InvalidDirection");
  
  // reached expansion limit
  if ( _level == std::numeric_limits<uint8_t>::max() || _level >= maxRootLevel) {
    return false;
  }

  // save my old children to store in the child that is taking my spot
  std::vector< std::unique_ptr<QuadTreeNode> > oldChildren;
  std::swap(oldChildren, _childrenPtr);

  const bool xPlus = FLT_GE_ZERO(direction.x());
  const bool yPlus = FLT_GE_ZERO(direction.y());
  const TimeStamp_t timeCreated = _content.data->GetLastObservedTime();
  MemoryMapData newDaata(EContentType::Unknown, timeCreated);
  
  // move to its new center
  const float oldHalfLen = _sideLen * 0.50f;
  _center.x() = _center.x() + (xPlus ? oldHalfLen : -oldHalfLen);
  _center.y() = _center.y() + (yPlus ? oldHalfLen : -oldHalfLen);

  // create new children
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::TopLeft , this, newDaata) ); // up L
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::TopRight, this, newDaata) ); // up R
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-oldHalfLen, _center.y()+oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::BotLeft , this, newDaata) ); // lo L
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-oldHalfLen, _center.y()-oldHalfLen, _center.z()}, _sideLen, _level, EQuadrant::BotRight, this, newDaata) ); // lo R

  // calculate the child that takes my place by using the opposite direction to expansion
  size_t childIdx = 0;
  if      ( !xPlus &&  yPlus ) { childIdx = 1; }
  else if (  xPlus && !yPlus ) { childIdx = 2; }
  else if (  xPlus &&  yPlus ) { childIdx = 3; }
  QuadTreeNode& childTakingMyPlace = *_childrenPtr[childIdx];
  
  // we have to set the new first level children as Unknown, since they are initialized as Invalid
  // except the child that takes my place, since that one is going to inherit my content
  MemoryMapData unknownData(EContentType::Unknown, timeCreated);
  NodeContent emptyUnknownContent(unknownData);
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
  ForceSetDetectedContentType(emptyUnknownContent, processor);
  
  // upgrade my remaining stats
  _sideLen = _sideLen * 2.0f;
  ++_level;
  ResetBoundingBox();

  // log
  PRINT_CH_INFO("QuadTree", "QuadTree.UpdgradeRootLevel", "Root expanded to level %u. Allowing %.2fm", _level, MM_TO_M(_sideLen));
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::AddQuadsToSend(QuadInfoVector& quadInfoVector) const
{
  // if we have children, delegate on them, otherwise add data about ourselves
  if ( _childrenPtr.empty() )
  {
    const auto contentTypeExternal = ConvertContentType(_content.data->type);
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
void QuadTreeNode::Subdivide(QuadTreeProcessor& processor)
{
  DEV_ASSERT(CanSubdivide() && !IsSubdivided(), "QuadTreeNode.Subdivide.InvalidSubdivide");
  DEV_ASSERT(_level > 0, "QuadTreeNode.Subdivide.InvalidLevel");
  
  const float halfLen    = _sideLen * 0.50f;
  const float quarterLen = halfLen * 0.50f;
  const uint8_t cLevel = _level-1;
  const TimeStamp_t timeCreated = _content.data->GetLastObservedTime();
  MemoryMapData newData(EContentType::Unknown, timeCreated);
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::TopLeft , this, newData) ); // up L
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()+quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::TopRight, this, newData) ); // up R
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-quarterLen, _center.y()+quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::BotLeft , this, newData) ); // lo L
  _childrenPtr.emplace_back( new QuadTreeNode(Point3f{_center.x()-quarterLen, _center.y()-quarterLen, _center.z()}, halfLen, cLevel, EQuadrant::BotRight, this, newData) ); // lo E

  // our children may change later on, but until they do, assume they have our old content
  for ( auto& childPtr : _childrenPtr )
  {
    childPtr->ForceSetDetectedContentType(_content, processor);
  }
  
  // set our content type to subdivided
  NodeContent emptySubdividedContent(newData);
  ForceSetDetectedContentType(emptySubdividedContent, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::Merge(const NodeContent& newContent, QuadTreeProcessor& processor)
{
  DEV_ASSERT(IsSubdivided(), "QuadTreeNode.Merge.InvalidState");

  // since we are going to destroy the children, notify the processor of all the descendants about to be destroyed
  ClearDescendants(processor);
  
  // set our content to the one we will have after the merge
  ForceSetDetectedContentType(newContent, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::ClearDescendants(QuadTreeProcessor& processor)
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
bool QuadTreeNode::CanOverrideSelfWithContent(EContentType newContentType, ESetOverlap overlap) const
{
  // TODO To guarantee that the future doesn't break this, we should require a matrix of old vs new to be fully
  // specified here. Some values however depend on overlap, so we could not cache / make const
  
  EContentType dataType = _content.data->type;
  if ( newContentType == EContentType::Cliff )
  {
    // Cliff can override any other
    return true;
  }
  else if ( dataType == EContentType::Cliff )
  {
    // Cliff can only be overridden by a full ClearOfCliff (the cliff is gone)
    const bool isTotalClear = (newContentType == EContentType::ClearOfCliff) && (overlap == ESetOverlap::SupersetOf);
    return isTotalClear;
  }
  else if ( newContentType == EContentType::ClearOfObstacle )
  {
    // ClearOfObstacle can't override ClearOfCliff, since it's less restrictive
    if ( dataType == EContentType::ClearOfCliff ) {
      return false;
    }

    // because quads store information as long as they are touched by something, ClearOfObstacle should
    // not clear basic types unless it has covered them fully. For example, this fixes obstacles or borders
    // being cleared just because we clear from the robot to the marker. We do not want to clear below the marker
    // unless the quad is fully contained (this will prevent lines from destroying content)
    if ( ( dataType == EContentType::ObstacleCube         ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::ObstacleUnrecognized ) ||
         ( dataType == EContentType::InterestingEdge      ) ||
         ( dataType == EContentType::NotInterestingEdge   ) )
    {
      const bool isTotalClear = (overlap == ESetOverlap::SupersetOf);
      return isTotalClear;
    }
  }
  else if ( newContentType == EContentType::InterestingEdge )
  {
    // InterestingEdge can only override basic node types, because it would cause data loss otherwise. For example,
    // we don't want to override a recognized marked cube or a cliff with their own border
    if ( ( dataType == EContentType::ObstacleCube         ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::ObstacleUnrecognized ) ||
         ( dataType == EContentType::Cliff                ) ||
         ( dataType == EContentType::NotInterestingEdge   ) )
    {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleProx )
  {
    if ( ( dataType == EContentType::ObstacleCube         ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::Cliff                ) )
    {
      return false;
    }
  }
  else if ( newContentType == EContentType::NotInterestingEdge )
  {
    // NotInterestingEdge can only override interesting edges
    if ( dataType != EContentType::InterestingEdge ) {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleCubeRemoved )
  {
    // ObstacleCubeRemoved can only remove ObstacleCube
    if ( dataType != EContentType::ObstacleCube ) {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleChargerRemoved )
  {
    // ObstacleChargerRemoved can only remove ObstacleCharger
    if ( dataType != EContentType::ObstacleCharger ) {
      return false;
    }
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::CanOverrideSelfAndChildrenWithContent(EContentType newContentType, ESetOverlap overlap) const
{
  if (ESetOverlap::SupersetOf != overlap)
  {
    return false;
  }
  
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
void QuadTreeNode::TryAutoMerge(QuadTreeProcessor& processor)
{
  DEV_ASSERT(IsSubdivided(), "QuadTreeNode.TryAutoMerge.NotSubdivided");

  // can't merge if any children are subdivided
  for (const auto& child : _childrenPtr) {
    if ( child->IsSubdivided() ) {
      return;
    }
  }
  
  bool allChildrenEqual = true;
  TimeStamp_t firstObservedTime = _childrenPtr[3]->GetData()->GetFirstObservedTime();
  TimeStamp_t lastObservedTime  = _childrenPtr[3]->GetData()->GetLastObservedTime();
  
  // check if all children classified the same content (assumes node content equality is transitive)
  for(size_t i=0; i<_childrenPtr.size()-1; ++i)
  {
    firstObservedTime = fmin(firstObservedTime, _childrenPtr[i]->GetData()->GetFirstObservedTime());
    lastObservedTime  = fmax(lastObservedTime,  _childrenPtr[i]->GetData()->GetLastObservedTime());
    allChildrenEqual &= (_childrenPtr[i]->GetContent() == _childrenPtr[i+1]->GetContent());
  }
  
  // we can merge and set that type on this parent
  if ( allChildrenEqual )
  {
    NodeContent childContent = _childrenPtr[0]->GetContent(); // do a copy since merging will destroy children
    childContent.data->SetFirstObservedTime(firstObservedTime);
    childContent.data->SetLastObservedTime(lastObservedTime);
    
    Merge( childContent, processor );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::TrySetDetectedContentType(const NodeContent& detectedContent, ESetOverlap overlap,
  QuadTreeProcessor& processor)
{
  // if we don't want to override with the new content, do not call ForceSet
  if ( !CanOverrideSelfWithContent(detectedContent.data->type, overlap) ) {
    return;
  }

  // do the change
  ForceSetDetectedContentType(detectedContent, processor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::ForceSetDetectedContentType(const NodeContent& detectedContent, QuadTreeProcessor& processor)
{
  const EContentType oldContentType = _content.data->type;
  const bool wasEmptyType = IsEmptyType();

  // if we are trying to set a removed type, convert to the type we want to actually set
  NodeContent finalContent = detectedContent;
  {
    const EContentType newContent = detectedContent.data->type;
    const bool isObstacleRemoved = (newContent == EContentType::ObstacleChargerRemoved) ||
                                   (newContent == EContentType::ObstacleCubeRemoved);
    if ( isObstacleRemoved )
    {
      MemoryMapData newData(EContentType::ClearOfObstacle, detectedContent.data->GetLastObservedTime());
      finalContent = NodeContent(newData);
    }
  }
  
  // this is where we can detect changes in content, for example new obstacles or things disappearing
  _content = finalContent;
  
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
  NodeContent myPrevContent = _content;
  ForceSetDetectedContentType(otherNode->GetContent(), processor);
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
void QuadTreeNode::AddSmallestDescendantsDepthFirst(NodeCPtrVector& descendants) const
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
QuadTreeNode::ESetOverlap QuadTreeNode::GetOverlapType(const FastPolygon& poly) const
{  
  bool containsOne = false;
  bool containsAll = true;
  
  for (const auto& pt : poly.GetSimplePolygon()) 
  {
    bool containsThis = _boundingBox.Contains(pt); 
    containsOne |= containsThis;
    containsAll &= containsThis;

    // exit early if we know the only case is the two sets are itersecting
    if ( containsOne && !containsAll ) 
    {
      return ESetOverlap::Intersecting;
    }
  }

  // all of the poly points are fully contained within the current node
  if (containsAll)
  {
    return ESetOverlap::SubsetOf;
  }
  
  size_t numVerticies = poly.Size();
  if (numVerticies > 1) // skip for point inserts
  {
    // check all edges of the polygon
    for (const auto& diag : _boundingBox.diagonals)
    {
      for (const auto& curLine : poly.GetEdgeSegments())
      {
        if (curLine.IntersectsWith(diag)) 
        {
          return ESetOverlap::Intersecting;
        }
      }
    }
    
    // there are no line intersections and none of the poly points are in the quad. Therefor, if at least one of
    // the quad points is contained in the poly, then all points of the quad are in the poly.
    bool containsAnyAndAll = poly.Contains( _boundingBox.lowerLeft );
    if ( containsAnyAndAll ) 
    {
      return ESetOverlap::SupersetOf;
    }  
  }

  return ESetOverlap::Disjoint;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::Insert_Recursive(const FastPolygon& poly, 
  const NodeContent& detectedContent,
  QuadTreeProcessor& processor)
{  
  // if we won't gain any new info, no need to process
  const bool isSameInfo = _content.data == detectedContent.data;
  if ( isSameInfo ) 
  {
    return false;
  }
  
  // to check for changes
  bool childChanged = false;
  NodeContent previousContent = _content;
  ESetOverlap overlap = GetOverlapType(poly);
  if ( ESetOverlap::Disjoint != overlap )
  {
    _content.data->SetLastObservedTime(detectedContent.data->GetLastObservedTime());

    if ( IsSubdivided() && CanOverrideSelfAndChildrenWithContent(detectedContent.data->type, overlap) )
    {
      Merge(detectedContent, processor);
    }
    else 
    {
      if (  ESetOverlap::SupersetOf != overlap && !IsSubdivided() && CanSubdivide())
      {
        Subdivide( processor );
      }
      
      if ( IsSubdivided() )
      {
        for( auto& childPtr : _childrenPtr ) 
        {
          childChanged |= childPtr->Insert_Recursive(poly, detectedContent, processor);
        }  
        TryAutoMerge(processor);
      }
      else
      {
        TrySetDetectedContentType( detectedContent, overlap, processor );
      }
    }
  }
  
  const bool ret = (_content != previousContent) || childChanged;
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeNode::Transform(const FastPolygon& poly,
                              NodeTransformFunction transform, 
                              QuadTreeProcessor& processor)
{  
  bool contentChanged = false;
  
  ESetOverlap overlap = GetOverlapType(poly);
  if ( ESetOverlap::Disjoint != overlap )
  {
    if ( IsSubdivided() )
    {
      for ( const auto& cPtr : _childrenPtr )
      {
        cPtr->Transform(poly, transform, processor);
      }
      TryAutoMerge(processor);
    }
    
    // if the node contains any data, attempt to perform the transform
    if (_content.data)
    {
      auto newData = transform(_content.data);
      NodeContent newContent(newData);
      
      // AddContentPoint checks if content has changed for us
      contentChanged = (_content != newContent) && !IsSubdivided();
      if (contentChanged) 
      {
        ForceSetDetectedContentType(newContent, processor);
      }
    }
    
  }
  
  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeNode::FindIf(const FastPolygon& poly, MemoryMapTypes::NodePredicate pred, 
                                    MemoryMapTypes::MemoryMapDataConstList& output) const
{  
  ESetOverlap overlap = GetOverlapType(poly);
  if ( ESetOverlap::Disjoint != overlap )
  {
    if ( IsSubdivided() )
    {
      for ( const auto& cPtr : _childrenPtr )
      {
        cPtr->FindIf(poly, pred, output);
      }
    }
    
    // if the node contains any data, attempt to perform the transform
    if (_content.data)
    {
      if (pred(_content.data)) {
        output.insert(std::const_pointer_cast<MemoryMapData>(_content.data));
      }
    }
  }
}

} // namespace Cozmo
} // namespace Anki
