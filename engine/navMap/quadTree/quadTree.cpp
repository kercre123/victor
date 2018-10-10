/**
 * File: quadTree.cpp
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Mesh representation of known geometry and obstacles for/from navigation with quad trees.
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "quadTree.h"

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"

#include "coretech/messaging/engine/IComms.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/callstack.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <sstream>

namespace Anki {
namespace Vector {
  
class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
// rsam note: I tweaked this to Initial=160mm, maxDepth=8 to get 256cm max area. With old the 200 I had to choose
// between 160cm (too small) or 320cm (too big). Incidentally we have gained 2mm per leaf node. I think performance-wise
// it will barely impact even slowest devices, but we need to keep an eye an all these numbers as we get data from
// real users
constexpr float kQuadTreeInitialRootSideLength = 160.0f;
constexpr uint8_t kQuadTreeInitialMaxDepth = 4;
constexpr uint8_t kQuadTreeMaxRootDepth = 8;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTree::QuadTree()
: QuadTreeNode({0,0}, kQuadTreeInitialRootSideLength, kQuadTreeInitialMaxDepth, QuadTreeTypes::EQuadrant::Root, ParentPtr::Nothing())  // Note the root is created at z=1
{
  _processor.SetRoot( this );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTree::~QuadTree()
{
  // we are destroyed, stop our rendering
  _processor.SetRoot(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float QuadTree::GetContentPrecisionMM() const
{
  // return the length of the smallest quad allowed
  const float minSide_mm = kQuadTreeInitialRootSideLength / (1 << kQuadTreeInitialMaxDepth); // 1 << x = pow(2,x)
  return minSide_mm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Insert(const FoldableRegion& region, NodeTransformFunction transform)
{
  // if the root does not contain the region, expand
  const AxisAlignedQuad aabb = region.GetBoundingBox();
  if ( !_boundingBox.Contains( aabb ) )
  {
    ExpandToFit( aabb );
  }
  
  // run the insert on the expanded QT
  bool contentChanged = false;
  FoldFunctor accumulator = [&] (QuadTreeNode& node)
  {
    auto newData = transform(node.GetData());
    if ( node.GetData() == newData ) { return; }

    node.GetData()->SetLastObservedTime(newData->GetLastObservedTime());

    // split node if we are unsure if the incoming region will fill the entire area
    if ( !region.ContainsQuad(node.GetBoundingBox()) )
    {
      node.Subdivide();
      node.MoveDataToChildren(_processor);
    }
    
    if ( !node.IsSubdivided() )
    {
      if ( node.GetData()->CanOverrideSelfWithContent(newData) ) {
        node.ForceSetDetectedContentType( newData, _processor );
        contentChanged = true;
      }
    } 
  };
  Fold(accumulator, region);

  // try to cleanup tree
  FoldFunctor merge = [this] (QuadTreeNode& node) { node.TryAutoMerge(_processor); };
  Fold(merge, region, FoldDirection::DepthFirst);

  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Transform(const FoldableRegion& region, NodeTransformFunction transform)
{
  // run the transform
  bool contentChanged = false;
  FoldFunctor trfm = [&] (QuadTreeNode& node)
    {
      MemoryMapDataPtr newData = transform(node.GetData());
      if ((node.GetData() != newData) && !node.IsSubdivided()) 
      {
        node.ForceSetDetectedContentType(newData, _processor);
        contentChanged = true;
      }
    };

  Fold(trfm, region);

  // try to cleanup tree
  FoldFunctor merge = [this] (QuadTreeNode& node) { node.TryAutoMerge(_processor); };
  Fold(merge, region, FoldDirection::DepthFirst);
  
  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Transform(const NodeAddress& address, NodeTransformFunction transform)
{
  // run the transform
  bool contentChanged = false;
  auto trfm = [&] (NodePtr node) {
    MemoryMapDataPtr newData = transform(node->GetData());
    if ((node->GetData() != newData) && !node->IsSubdivided()) 
    {
      node->ForceSetDetectedContentType(newData, _processor);
      contentChanged = true;
    }
  };

  GetNodeAtAddress(address).FMap(trfm);
  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Transform(NodeTransformFunction transform)
{
  // run the transform
  bool contentChanged = false;
  FoldFunctor trfm = [&] (QuadTreeNode& node)
    {
      MemoryMapDataPtr newData = transform(node.GetData());
      if ((node.GetData() != newData) && !node.IsSubdivided()) 
      {
        node.ForceSetDetectedContentType(newData, _processor);
        contentChanged = true;
      }
    };

  Fold(trfm);

  // try to cleanup tree
  FoldFunctor merge = [this] (QuadTreeNode& node) { node.TryAutoMerge(_processor); };
  Fold(merge, FoldDirection::DepthFirst);
  
  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Merge(const QuadTree& other, const Pose3d& transform)
{
  // TODO rsam for the future, when we merge with transform, poses or directions stored as extra info are invalid
  // since they were wrt a previous origin!
  Pose2d transform2d(transform);

  // obtain all leaf nodes from the map we are merging from
  NodeCPtrVector leafNodes;
  other.Fold(
    [&leafNodes](const QuadTreeNode& node) {
      if (!node.IsSubdivided()) { leafNodes.emplace_back(&node); }
    });
  
  // note regarding quad size limit: when we merge one map into another, this map can expand or shift the root
  // to accomodate the information that we are receiving from 'other'. 'other' is considered to have more up to
  // date information than 'this', so it should be ok to let it destroy as much info as it needs by shifting the root
  // towards them. In an ideal world, it would probably come to a compromise to include as much information as possible.
  // This I expect to happen naturally, since it's likely that 'other' won't be fully expanded in the opposite direction.
  // It can however happen in Cozmo during explorer mode, and it's debatable which information is more relevant.
  // A simple idea would be to limit leafNodes that we add back to 'this' by some distance, for example, half the max
  // root length. That would allow 'this' to keep at least half a root worth of information with respect the new one
  // we are bringing in.
  
  // iterate all those leaf nodes, adding them to this tree
  bool changed = false;
  for( const auto& nodeInOther : leafNodes ) {
  
    // if the leaf node is unkown then we don't need to add it
    const bool isUnknown = ( nodeInOther->GetData()->type == EContentType::Unknown );
    if ( !isUnknown ) {
      
      // NOTE: there's a precision problem when we add back the quads; when we add a non-axis aligned quad to the map,
      // we modify (if applicable) all quads that intersect with that non-aa quad. When we merge this information into
      // a different map, we have lost precision on how big the original non-aa quad was, since we have stored it
      // with the resolution of the memory map quad size. In general, when merging information from the past, we should
      // not rely on precision, but there ar things that we could do to mitigate this issue, for example:
      // a) reducing the size of the aaQuad being merged by half the size of the leaf nodes
      // or
      // b) scaling down aaQuad to account for this error
      // eg: transformedQuad2d.Scale(0.9f);
      // At this moment is just a known issue

      std::vector<Point2f> corners = nodeInOther->GetBoundingBox().GetVertices();
      for (Point2f& p : corners) {
        p = transform2d.GetTransform() * p;
      }
      
      // grab CH to sort verticies into CW order
      const ConvexPolygon poly = ConvexPolygon::ConvexHull( std::move(corners) );
      changed |= Insert(FastPolygon(poly), [&nodeInOther] (auto) { return nodeInOther->GetData(); });
    }
  }
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::ExpandToFit(const AxisAlignedQuad& region)
{
  ANKI_CPU_PROFILE("QuadTree::ExpandToFit");
  
  // allow expanding several times until the poly fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool fitsInMap = false;
  
  do
  {
    // find in which direction we are expanding, upgrade root level in that direction (center moves)
    const Vec2f& direction = region.GetCentroid() - GetCenter();
    expanded = UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);

    // check if the region now fits in the expanded root
    fitsInMap = _boundingBox.ContainsAll( {region.GetMinVertex(), region.GetMaxVertex()} );
    
  } while( !fitsInMap && expanded );

  // if the poly still doesn't fit, see if we can shift once
  if ( !fitsInMap )
  {
    // shift the root to try to cover the poly, by removing opposite nodes in the map
    ShiftRoot(region, _processor);

    // check if the poly now fits in the expanded root
    fitsInMap = _boundingBox.ContainsAll( {region.GetMinVertex(), region.GetMaxVertex()} );
  }
  
  // the poly should be contained, if it's not, we have reached the limit of expansions and shifts, and the poly does not
  // fit, which will cause information loss
  if ( !fitsInMap ) {
    PRINT_NAMED_WARNING("QuadTree.Expand.InsufficientExpansion",
      "Quad caused expansion, but expansion was not enough PolyCenter(%.2f, %.2f), Root(%.2f,%.2f) with sideLen(%.2f).",
      region.GetCentroid().x(), region.GetCentroid().y(),
      GetCenter().x(), GetCenter().y(),
      GetSideLen() );
  }
  
  // always flag as dirty since we have modified the root (potentially)
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::ShiftRoot(const AxisAlignedQuad& region, QuadTreeProcessor& processor)
{
  const float rootHalfLen = _sideLen * 0.5f;

  const bool xPlusAxisReq  = FLT_GE(region.GetMaxVertex().x(), _center.x()+rootHalfLen);
  const bool xMinusAxisReq = FLT_LE(region.GetMinVertex().x(), _center.x()-rootHalfLen);
  const bool yPlusAxisReq  = FLT_GE(region.GetMaxVertex().y(), _center.y()+rootHalfLen);
  const bool yMinusAxisReq = FLT_LE(region.GetMinVertex().y(), _center.y()-rootHalfLen);
  
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
  _boundingBox = AxisAlignedQuad(_center - Point2f(_sideLen * .5f), _center + Point2f(_sideLen * .5f) );
  
  // if the root has children, update them, otherwise no further changes are necessary
  if ( !_childrenPtr.empty() )
  {
    // save my old children so that we can swap them with the new ones
    ChildrenVector oldChildren;
    std::swap(oldChildren, _childrenPtr);
    
    // create new children
    Subdivide();
      
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
       how PlusXPlusY becomes BottomRight in the new root. We want to preserve the children of that direct child (old TL), but
       we need to hook them to a different child (new BR). That's essentially what the rest of this method does.
     
    */
    
    // this content is set to the children that don't inherit old children
    
    // calculate which children are brought over from the old ones
    if ( xShift && yShift )
    {
      // double move, only one child is preserved, which is the one in the same direction top the expansion one
      if ( xPlusAxisReq ) {
        if ( yPlusAxisReq ) {
          // we are moving along +x +y axes, top left becomes bottom right of the new root
          _childrenPtr[(Q2N)EQuadrant::MinusXMinusY]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::PlusXPlusY].get(), processor);
        } else {
          // we are moving along +x -y axes, top right becomes bottom left of the new root
          _childrenPtr[(Q2N)EQuadrant::MinusXPlusY ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::PlusXMinusY].get(), processor);
        }
      }
      else
      {
        if ( yPlusAxisReq ) {
          // we are moving along -x +y axes, bottom left becomes top right of the new root
          _childrenPtr[(Q2N)EQuadrant::PlusXMinusY]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::MinusXPlusY].get(), processor);
        } else {
          // we are moving along -x -y axes, bottom right becomes top left of the new root
          _childrenPtr[(Q2N)EQuadrant::PlusXPlusY ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::MinusXMinusY].get(), processor);
        }
      }
    }
    else if ( xShift )
    {
      // move only in one axis, two children are preserved, top or bottom
      if ( xPlusAxisReq )
      {
        // we are moving along +x axis, top children are preserved, but they become the bottom ones
        _childrenPtr[(Q2N)EQuadrant::MinusXPlusY ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::PlusXPlusY].get(), processor );
        _childrenPtr[(Q2N)EQuadrant::MinusXMinusY]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::PlusXMinusY].get(), processor);
      }
      else
      {
        // we are moving along -x axis, bottom children are preserved, but they become the top ones
        _childrenPtr[(Q2N)EQuadrant::PlusXPlusY ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::MinusXPlusY].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::PlusXMinusY]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::MinusXMinusY].get(), processor);
      }
    }
    else if ( yShift )
    {
      // move only in one axis, two children are preserved, left or right
      if ( yPlusAxisReq )
      {
        // we are moving along +y axis, left children are preserved, but they become the right ones
        _childrenPtr[(Q2N)EQuadrant::PlusXMinusY]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::PlusXPlusY].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::MinusXMinusY]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::MinusXPlusY].get(), processor);
      }
      else
      {
        // we are moving along -y axis, right children are preserved, but they become the left ones
        _childrenPtr[(Q2N)EQuadrant::PlusXPlusY ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::PlusXMinusY].get(), processor);
        _childrenPtr[(Q2N)EQuadrant::MinusXPlusY ]->SwapChildrenAndContent(oldChildren[(Q2N)EQuadrant::MinusXMinusY].get(), processor);
      }
    }
    
    // destroy the nodes that are going away because we shifted away from them
    DestroyNodes(oldChildren, processor);
  }

  // update address of all children
  Fold([] (QuadTreeNode& node) { node.ResetAddress(); });
  
  // log
  PRINT_CH_INFO("QuadTree", "QuadTree.ShiftRoot", "Root level is still %u, root shifted. Allowing %.2fm", _level, MM_TO_M(_sideLen));
  
  // successful shift
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::UpgradeRootLevel(const Point2f& direction, uint8_t maxRootLevel, QuadTreeProcessor& processor)
{
  // reached expansion limit
  if ( _level == std::numeric_limits<uint8_t>::max() || _level >= maxRootLevel) {
    return false;
  }

  // take this nodes children to store in the new node that is taking my spot
  ChildrenVector oldChildren;
  std::swap(oldChildren, _childrenPtr);
  
  // reset this nodes parameters
  _center += Quadrant2Vec( Vec2Quadrant(direction) ) * _sideLen * 0.5f;
  _boundingBox = AxisAlignedQuad(_center - Point2f(_sideLen), _center + Point2f(_sideLen) );
  _sideLen *= 2.0f;
  ++_level;

  // since we stole its children before, subdivide this node again
  Subdivide();

  // calculate the child that takes my place by using the opposite direction to expansion
  auto childTakingMyPlace = GetChild( Vec2Quadrant(-direction) );  
  childTakingMyPlace.FMap( 
    [&](auto node) { 
      std::swap(node->_childrenPtr, oldChildren); 
      node->ForceSetDetectedContentType( _content.data, processor);
    }   
  );

  // set the new parent in my old children
  using namespace Util::MaybeOperators;
  ParentPtr newParent = [](auto sharedPtr) { return (const QuadTreeNode*) sharedPtr.get(); } *= childTakingMyPlace;
  for ( auto& childPtr : oldChildren ) {
    childPtr->ChangeParent( newParent );
  }

  // reset my data
  ForceSetDetectedContentType(MemoryMapDataPtr(), processor);

  // update address of all children
  Fold([] (QuadTreeNode& node) { node.ResetAddress(); });
  
  PRINT_CH_INFO("QuadTree", "QuadTree.UpdgradeRootLevel", "Root expanded to level %u. Allowing %.2fm", _level, MM_TO_M(_sideLen));
  
  return true;
}

  

} // namespace Vector
} // namespace Anki
