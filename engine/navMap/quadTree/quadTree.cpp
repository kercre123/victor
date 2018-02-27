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

#include "engine/viz/vizManager.h"
#include "engine/robot.h"

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"

#include "coretech/messaging/engine/IComms.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/callstack.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <sstream>

namespace Anki {
namespace Cozmo {
  
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
QuadTree::QuadTree(MemoryMapDataPtr rootData)
: _processor()
, _root({0,0,1}, kQuadTreeInitialRootSideLength, kQuadTreeInitialMaxDepth, QuadTreeTypes::EQuadrant::Root, nullptr)  // Note the root is created at z=1
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
bool QuadTree::Insert(const FastPolygon& poly, MemoryMapDataPtr data)
{
  ANKI_CPU_PROFILE("QuadTree::Insert");
  
  // if the root does not contain the poly, expand
  if ( !_root.Contains( poly ) )
  {
    ExpandToFit( poly.GetSimplePolygon());
  }
  
  // run the insert on the expanded QT
  bool contentChanged = false;
  QuadTreeNode::FoldFunctor accumulator = [this, &contentChanged, &data, &poly] (QuadTreeNode& node)
  {
    if ( node.GetData() == data ) { return; }

    QuadTreeNode::NodeContent previousContent = node._content;
    node.GetData()->SetLastObservedTime(data->GetLastObservedTime());

    // split node if we can unsure if the incoming poly will fill the entire area
    if ( !node.IsContainedBy(poly) && !node.IsSubdivided() && node.CanSubdivide())
    {
      node.Subdivide( _processor );
    }
    
    if ( !node.IsSubdivided() )
    {
      if ( node.GetData()->CanOverrideSelfWithContent(data->type) ) {
        node.ForceSetDetectedContentType( data, _processor );
        contentChanged = true;
      }
    } 
  };
  _root.Fold(accumulator, poly);

  // try to cleanup tree
  QuadTreeNode::FoldFunctor merge = [this] (QuadTreeNode& node) { node.TryAutoMerge(_processor); };
  _root.Fold(merge, poly, QuadTreeNode::FoldDirection::DepthFirst);

  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Transform(const Poly2f& poly, NodeTransformFunction transform)
{
  // run the transform
  bool contentChanged = false;
  QuadTreeNode::FoldFunctor trfm = [this, &contentChanged, &transform] (QuadTreeNode& node)
    {
      MemoryMapDataPtr newData = transform(node.GetData());
      if ((node.GetData() != newData) && !node.IsSubdivided()) 
      {
        node.ForceSetDetectedContentType(newData, _processor);
        contentChanged = true;
      }
    };

  _root.Fold(trfm, poly);

  // try to cleanup tree
  QuadTreeNode::FoldFunctor merge = [this] (QuadTreeNode& node) { node.TryAutoMerge(_processor); };
  _root.Fold(merge, poly, QuadTreeNode::FoldDirection::DepthFirst);
  
  return contentChanged;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Transform(NodeTransformFunction transform)
{
  // run the transform
  bool contentChanged = false;
  QuadTreeNode::FoldFunctor trfm = [this, &contentChanged, &transform] (QuadTreeNode& node)
    {
      MemoryMapDataPtr newData = transform(node.GetData());
      if ((node.GetData() != newData) && !node.IsSubdivided()) 
      {
        node.ForceSetDetectedContentType(newData, _processor);
        contentChanged = true;
      }
    };

  _root.Fold(trfm);

  // try to cleanup tree
  QuadTreeNode::FoldFunctor merge = [this] (QuadTreeNode& node) { node.TryAutoMerge(_processor); };
  _root.Fold(merge, QuadTreeNode::FoldDirection::DepthFirst);
  
  return contentChanged;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::Merge(const QuadTree& other, const Pose3d& transform)
{
  // TODO rsam for the future, when we merge with transform, poses or directions stored as extra info are invalid
  // since they were wrt a previous origin!
  Pose2d transform2d(transform);

  // obtain all leaf nodes from the map we are merging from
  QuadTreeNode::NodeCPtrVector leafNodes;
  other.Fold(
    [&leafNodes](const QuadTreeNode& node) {
      if (!node.IsSubdivided()) { leafNodes.push_back(&node); }
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
      // get transformed quad
      Quad2f transformedQuad2d;
      transform2d.ApplyTo(nodeInOther->MakeQuadXY(), transformedQuad2d);
      
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
      
      // add to this
      Poly2f transformedPoly;
      transformedPoly.ImportQuad2d(transformedQuad2d);
      
      changed |= Insert(transformedPoly, nodeInOther->GetData());
    }
  }
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTree::ExpandToFit(const Poly2f& polyToCover)
{
  ANKI_CPU_PROFILE("QuadTree::ExpandToFit");
  
  // allow expanding several times until the poly fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool fitsInMap = false;
  
  do
  {
    // find in which direction we are expanding, upgrade root level in that direction (center moves)
    const Vec2f& direction = polyToCover.ComputeCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);

    // check if the poly now fits in the expanded root
    fitsInMap = _root.Contains(polyToCover);
    
  } while( !fitsInMap && expanded );

  // if the poly still doesn't fit, see if we can shift once
  if ( !fitsInMap )
  {
    // shift the root to try to cover the poly, by removing opposite nodes in the map
    _root.ShiftRoot(polyToCover, _processor);

    // check if the poly now fits in the expanded root
    fitsInMap = _root.Contains(polyToCover);
  }
  
  // the poly should be contained, if it's not, we have reached the limit of expansions and shifts, and the poly does not
  // fit, which will cause information loss
  if ( !fitsInMap ) {
    PRINT_NAMED_WARNING("QuadTree.Expand.InsufficientExpansion",
      "Quad caused expansion, but expansion was not enough PolyCenter(%.2f, %.2f), Root(%.2f,%.2f) with sideLen(%.2f).",
      polyToCover.ComputeCentroid().x(), polyToCover.ComputeCentroid().y(),
      _root.GetCenter().x(), _root.GetCenter().y(),
      _root.GetSideLen() );
  }
  
  // always flag as dirty since we have modified the root (potentially)
  return true;
}
  

} // namespace Cozmo
} // namespace Anki
