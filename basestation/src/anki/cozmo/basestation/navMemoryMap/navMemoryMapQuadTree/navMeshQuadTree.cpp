/**
 * File: navMeshQuadTree.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Mesh representation of known geometry and obstacles for/from navigation with quad trees.
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "navMeshQuadTree.h"
#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/callstack.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <sstream>

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kRenderNavMeshQuadTree         , "NavMeshQuadTree", true);
CONSOLE_VAR(bool, kRenderLastAddedQuad           , "NavMeshQuadTree", false);

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
NavMeshQuadTree::NavMeshQuadTree(VizManager* vizManager)
: _gfxDirty(true)
, _processor(vizManager)
, _root({0,0,1}, kQuadTreeInitialRootSideLength, kQuadTreeInitialMaxDepth, NavMeshQuadTreeTypes::EQuadrant::Root, nullptr)
, _vizManager(vizManager)
{
  _processor.SetRoot( &_root );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTree::~NavMeshQuadTree()
{
  // we are destroyed, stop our rendering
  ClearDraw();
  
  _processor.SetRoot(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Draw(size_t mapIdxHint) const
{
  if ( _gfxDirty && kRenderNavMeshQuadTree )
  {
    ANKI_CPU_PROFILE("NavMeshQuadTree::Draw");
    
    // ask root to add proper quads to be rendered
    VizManager::SimpleQuadVector quadVector;
    _root.AddQuadsToDraw(quadVector);
    
    // the mapIdx hint reveals that we are not the current active map, but an old memory. Apply some offset
    // so that we don't render on top of any other map
    if ( mapIdxHint > 0 )
    {
      const float offSetPerIdx = MM_TO_M(-250.0f);
      for( auto& q : quadVector ) {
        q.center[2] += (mapIdxHint*offSetPerIdx);
      }
    }
    else
    {
      // small offset to not clip with floor
      for( auto& q : quadVector ) {
        q.center[2] += MM_TO_M(10.0f);
      }
    }
    
    // since we have several maps rendering, each one render with its own id
    std::stringstream instanceId;
    instanceId << "NavMeshQuadTree_" << this;
    _vizManager->DrawQuadVector(instanceId.str(), quadVector);
    
//    // compare actual size vs max
//    size_t actual = quadVector.size();
//    size_t max = pow(4,_root.GetLevel()) + 1;
//    PRINT_NAMED_INFO("RSAM", "%zu / %zu", actual, max);
    
    _gfxDirty = false;
  }
  
  // draw the processor information
  if ( mapIdxHint == 0 ) {
    _processor.Draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::ClearDraw() const
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ClearDraw");
  
  // clear previous quads
  std::stringstream instanceId;
  instanceId << "NavMeshQuadTree_" << this;
  _vizManager->EraseQuadVector(instanceId.str());

  _gfxDirty = true;
  
  // also clear processor information
  _processor.ClearDraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float NavMeshQuadTree::GetContentPrecisionMM() const
{
  // return the length of the smallest quad allowed
  const float minSide_mm = kQuadTreeInitialRootSideLength / (1 << kQuadTreeInitialMaxDepth); // 1 << x = pow(2,x)
  return minSide_mm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddQuad(const Quad2f& quad, const NodeContent& nodeContent)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::AddQuad");
  
  {
    // I have had a unit test send here a NaN quad, probably because a cube pose was busted, detect that case
    // here and ignore the quad so that we don't assert because we expand indefinitely
    const bool isNaNQuad =
      std::isnan(quad.GetTopLeft().x()    ) ||
      std::isnan(quad.GetTopLeft().y()    ) ||
      std::isnan(quad.GetTopRight().x()   ) ||
      std::isnan(quad.GetTopRight().y()   ) ||
      std::isnan(quad.GetBottomLeft().x() ) ||
      std::isnan(quad.GetBottomLeft().y() ) ||
      std::isnan(quad.GetBottomRight().x()) ||
      std::isnan(quad.GetBottomRight().y());
    if ( isNaNQuad ) {
      PRINT_NAMED_ERROR("NavMeshQuadTree.AddQuad.NaNQuad",
        "Quad is not valid, at least one coordinate is NaN.");
      Util::sDumpCallstack("NavMeshQuadTree::AddQuad");
      return;
    }
  }
  
  // render approx last quad added
  if ( kRenderLastAddedQuad )
  {
    ANKI_CPU_PROFILE("NavMeshQuadTree::AddQuad.Render");
    
    ColorRGBA color = Anki::NamedColors::WHITE;
    const float z = 70.0f;
    Point3f topLeft = {quad[Quad::CornerName::TopLeft].x(), quad[Quad::CornerName::TopLeft].y(), z};
    Point3f topRight = {quad[Quad::CornerName::TopRight].x(), quad[Quad::CornerName::TopRight].y(), z};
    Point3f bottomLeft = {quad[Quad::CornerName::BottomLeft].x(), quad[Quad::CornerName::BottomLeft].y(), z};
    Point3f bottomRight = {quad[Quad::CornerName::BottomRight].x(), quad[Quad::CornerName::BottomRight].y(), z};
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", topLeft, topRight, color, true);
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", topRight, bottomRight, color, false);
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", bottomRight, bottomLeft, color, false);
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", bottomLeft, topLeft, color, false);
  }

  // if the root does not contain the quad, expand
  if ( !_root.Contains( quad ) )
  {
    Expand( quad );
  }

  // add quad now
  _gfxDirty = _root.AddContentQuad(quad, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddLine(const Point2f& from, const Point2f& to, const NodeContent& nodeContent)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::AddLine");
  
  {
    // I have had a unit test send here a NaN quad, probably because a cube pose was busted, detect here
    // if a line is also nan
    const bool isNaNLine =
      std::isnan(from.x()) ||
      std::isnan(from.y()) ||
      std::isnan(to.x()  ) ||
      std::isnan(to.y()  );
    if ( isNaNLine ) {
      PRINT_NAMED_ERROR("NavMeshQuadTree.AddLine.NaNQuad",
        "Line is not valid, at least one coordinate is NaN.");
      Util::sDumpCallstack("NavMeshQuadTree::AddLine");
      return;
    }
  }

  // if the root does not contain origin, we need to expand in that direction
  if ( !_root.Contains( from ) )
  {
    Expand( from );
  }

  // if the root does not contain destination, we need to expand in that direction
  if ( !_root.Contains( to ) )
  {
    Expand( to );
  }

  // add segment now
  _gfxDirty = _root.AddContentLine(from, to, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddTriangle(const Triangle2f& tri, const NodeContent& nodeContent)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::AddTriangle");
  
  {
    // I have had a unit test send here a NaN quad, probably because a cube pose was busted, detect here if
    // a triangle becomes NaN ever to prevent expanding indefinitely
    const bool isNaNTri =
      std::isnan(tri[0].x()) ||
      std::isnan(tri[0].y()) ||
      std::isnan(tri[1].x()) ||
      std::isnan(tri[1].y()) ||
      std::isnan(tri[2].x()) ||
      std::isnan(tri[2].y());
    if ( isNaNTri ) {
      PRINT_NAMED_ERROR("NavMeshQuadTree.AddTriangle.NaNQuad",
        "Triangle is not valid, at least one coordinate is NaN.");
      Util::sDumpCallstack("NavMeshQuadTree::AddTriangle");
      return;
    }
  }

  // if the root does not contain the triangle, we need to expand in that direction
  if ( !_root.Contains( tri ) )
  {
    Expand( tri );
  }

  // add triangle now
  _gfxDirty = _root.AddContentTriangle(tri, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddPoint(const Point2f& point, const NodeContent& nodeContent)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::AddPoint");
  
  // if the root does not contain the point, we need to expand in that direction
  if ( !_root.Contains( point ) )
  {
    Expand( point );
  }
  
  // add point now
  _gfxDirty = _root.AddContentPoint(point, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Merge(const NavMeshQuadTree& other, const Pose3d& transform)
{
  // TODO rsam for the future, when we merge with transform, poses or directions stored as extra info are invalid
  // since they were wrt a previous origin!
  Pose2d transform2d(transform);

  // obtain all leaf nodes from the map we are merging from
  NavMeshQuadTreeNode::NodeCPtrVector leafNodes;
  other._root.AddSmallestDescendantsDepthFirst(leafNodes);
  
  // note regarding quad size limit: when we mege one map into another, this map can expand or shift the root
  // to accomodate the information that we are receiving from 'other'. 'other' is considered to have more up to
  // date information than 'this', so it should be ok to let it destroy as much info as it needs by shifting the root
  // towards them. In an ideal world, it would probably come to a compromise to include as much information as possible.
  // This I expect to happen naturally, since it's likely that 'other' won't be fully expanded in the opposite direction.
  // It can however happen in Cozmo during explorer mode, and it's debatable which information is more relevant.
  // A simple idea would be to limit leafNodes that we add back to 'this' by some distance, for example, half the max
  // root length. That would allow 'this' to keep at least half a root worth of information with respect the new one
  // we are bringing in.
  
  // iterate all those leaf nodes, adding them to this tree
  for( const auto& nodeInOther : leafNodes ) {
  
    // if the leaf node is unkown then we don't need to add it
    const bool isUnknown = ( nodeInOther->IsContentTypeUnknown() );
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
      NodeContent copyOfContent = nodeInOther->GetContent();
      AddQuad(transformedQuad2d, copyOfContent); // TODO how good it's to pass a const shared_ptr? the ctrl block is modified
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Quad2f& quadToCover)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ExpandByQuad");
  
  // allow expanding several times until the quad fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool quadFitsInMap = false;
  
  do {

    // Find in which direction we are expanding and upgrade root level in that direction
    const Vec2f& direction = quadToCover.ComputeCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);
    
    // if we didn't expand, see if we can shift the root to cover the quad, by removing other nodes in the map
    if ( !expanded ) {
      std::vector<Point2f> requiredPoints;
      requiredPoints.reserve(4);
      for( const Point2f& p : quadToCover ) {
        requiredPoints.emplace_back(p);
      }
      _root.ShiftRoot(requiredPoints, _processor);
    }
    
    // check if the quad now fits in the expanded root
    quadFitsInMap = _root.Contains(quadToCover);
    
  } while( !quadFitsInMap && expanded );
  
  // the quad should be contained, if it's not, we have reached the limit of expansions and the quad does not
  // fit, which will cause information loss
  if ( !quadFitsInMap ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.Expand.InsufficientExpansion",
      "Quad caused expansion, but expansion was not enough.");
  }
  
  // always flag as dirty since we have modified the root
  _gfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Point2f& pointToInclude)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ExpandByPoint");
  
  // allow expanding several times until the point fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool pointInMap = false;
  
  do {

    // Find in which direction we are expanding and upgrade root level in that direction
    const Vec2f& direction = pointToInclude - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);
    
    // if we didn't expand, see if we can shift the root to cover the point, by removing other nodes in the map
    if ( !expanded ) {
      std::vector<Point2f> requiredPoints;
      requiredPoints.reserve(1);
      requiredPoints.emplace_back(pointToInclude);
      _root.ShiftRoot(requiredPoints, _processor);
    }
    
    // check if the point now fits in the expanded root
    pointInMap = _root.Contains(pointToInclude);
    
  } while( !pointInMap && expanded );
  
  // the point should be contained, if it's not, we have reached the limit of expansions and the point does not
  // fit, which will cause information loss
  if ( !pointInMap ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.Expand.InsufficientExpansion",
      "Point caused expansion, but expansion was not enough.");
  }
  
  // always flag as dirty since we have modified the root
  _gfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Triangle2f& triangleToCover)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ExpandByTriangle");
  
  // allow expanding several times until the triangle fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool triangleInMap = false;
  
  do {

    // Find in which direction we are expanding and upgrade root level in that direction
    const Vec2f& direction = triangleToCover.GetCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);
    
    // if we didn't expand, see if we can shift the root to cover the triangle, by removing other nodes in the map
    if ( !expanded ) {
      std::vector<Point2f> requiredPoints;
      requiredPoints.reserve(3);
      for( const Point2f& p : triangleToCover ) {
        requiredPoints.emplace_back(p);
      }
      _root.ShiftRoot(requiredPoints, _processor);
    }
    
    // check if the point now fits in the expanded root
    triangleInMap = _root.Contains(triangleToCover);
    
  } while( !triangleInMap && expanded );
  
  // the point should be contained, if it's not, we have reached the limit of expansions and the point does not
  // fit, which will cause information loss
  if ( !triangleInMap ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.Expand.InsufficientExpansion",
      "Triangle caused expansion, but expansion was not enough.");
  }
  
  // always flag as dirty since we have modified the root
  _gfxDirty = true;
}


} // namespace Cozmo
} // namespace Anki
