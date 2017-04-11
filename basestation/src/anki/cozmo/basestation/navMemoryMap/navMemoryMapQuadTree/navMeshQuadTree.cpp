/**
 * File: navMeshQuadTree.cpp
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
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/messaging/basestation/IComms.h"

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
NavMeshQuadTree::NavMeshQuadTree(VizManager* vizManager, Robot* robot)
: _gfxDirty(true)
, _processor(vizManager)
, _root({0,0,1}, kQuadTreeInitialRootSideLength, kQuadTreeInitialMaxDepth, NavMeshQuadTreeTypes::EQuadrant::Root, nullptr)  // Note the root is created at z=1
, _vizManager(vizManager)
, _robot(robot)
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
void NavMeshQuadTree::DrawDebugProcessorInfo(size_t mapIdxHint) const
{
  // draw the processor information
  if ( mapIdxHint == 0 ) {
    _processor.Draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::ClearDraw() const
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ClearDraw");
  
  std::stringstream instanceId;
  instanceId << "New_NavMeshQuadTree_" << this;
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
void NavMeshQuadTree::AddQuad(const Quad2f& quad, const NodeContent& nodeContent, int shiftAllowedCount)
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
    // if we are 'adding' a removal quad, do not expand, since it would be useless to expand or shift to try
    // to remove data.
    const bool isRemovingContent = NavMeshQuadTreeTypes::IsRemovalType(nodeContent.type);
    if ( isRemovingContent ) {
      PRINT_NAMED_INFO("NavMeshQuadTree.AddQuad.RemovalQuadNotContained",
        "Quad is not fully contained in root, removal does not cause expansion.");
    }
    else
    {
      Expand( quad, shiftAllowedCount );
    }
  }

  // add quad now
  _gfxDirty = _root.AddContentQuad(quad, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddLine(const Point2f& from, const Point2f& to, const NodeContent& nodeContent, int shiftAllowedCount)
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
    // if we are 'adding' a removal line, do not expand, since it would be useless to expand or shift to try
    // to remove data.
    const bool isRemovingContent = NavMeshQuadTreeTypes::IsRemovalType(nodeContent.type);
    if ( isRemovingContent ) {
      PRINT_NAMED_INFO("NavMeshQuadTree.AddLine.RemovalLineFromNotContained",
        "Line 'from' point is not fully contained in root, removal does not cause expansion.");
    }
    else
    {
      Expand( from, shiftAllowedCount );
    }
  }

  // if the root does not contain destination, we need to expand in that direction
  if ( !_root.Contains( to ) )
  {
    // if we are 'adding' a removal line, do not expand, since it would be useless to expand or shift to try
    // to remove data.
    const bool isRemovingContent = NavMeshQuadTreeTypes::IsRemovalType(nodeContent.type);
    if ( isRemovingContent ) {
      PRINT_NAMED_INFO("NavMeshQuadTree.AddLine.RemovalLineToNotContained",
        "Line 'to' point is not fully contained in root, removal does not cause expansion.");
    }
    else
    {
      Expand( to, shiftAllowedCount );
    }
  }

  // add segment now
  _gfxDirty = _root.AddContentLine(from, to, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddTriangle(const Triangle2f& tri, const NodeContent& nodeContent, int shiftAllowedCount)
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
    // if we are 'adding' a removal triangle, do not expand, since it would be useless to expand or shift to try
    // to remove data.
    const bool isRemovingContent = NavMeshQuadTreeTypes::IsRemovalType(nodeContent.type);
    if ( isRemovingContent ) {
      PRINT_NAMED_INFO("NavMeshQuadTree.AddTriangle.RemovalTriangleNotContained",
        "Triangle is not fully contained in root, removal does not cause expansion.");
    }
    else
    {
      Expand( tri, shiftAllowedCount );
    }
  }

  // add triangle now
  _gfxDirty = _root.AddContentTriangle(tri, nodeContent, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddPoint(const Point2f& point, const NodeContent& nodeContent, int shiftAllowedCount)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::AddPoint");
  
  // if the root does not contain the point, we need to expand in that direction
  if ( !_root.Contains( point ) )
  {
    // if we are 'adding' a removal point, do not expand, since it would be useless to expand or shift to try
    // to remove data.
    const bool isRemovingContent = NavMeshQuadTreeTypes::IsRemovalType(nodeContent.type);
    if ( isRemovingContent ) {
      PRINT_NAMED_INFO("NavMeshQuadTree.AddPoint.RemovalPointNotContained",
        "Point is not contained in root, removal does not cause expansion.");
    }
    else
    {
      Expand( point, shiftAllowedCount );
    }
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
  
  // note regarding quad size limit: when we merge one map into another, this map can expand or shift the root
  // to accomodate the information that we are receiving from 'other'. 'other' is considered to have more up to
  // date information than 'this', so it should be ok to let it destroy as much info as it needs by shifting the root
  // towards them. In an ideal world, it would probably come to a compromise to include as much information as possible.
  // This I expect to happen naturally, since it's likely that 'other' won't be fully expanded in the opposite direction.
  // It can however happen in Cozmo during explorer mode, and it's debatable which information is more relevant.
  // A simple idea would be to limit leafNodes that we add back to 'this' by some distance, for example, half the max
  // root length. That would allow 'this' to keep at least half a root worth of information with respect the new one
  // we are bringing in.
  
  // the expected number of shifts should be less than :
  // halfMaxSize = rootMaxSize*0.5
  // distance = distance between root centers
  // maxNumberOfShifts = ceil( distance / halfMaxSize)
  const Vec3f centerFromOtherInThisFrame = transform * other._root.GetCenter();
  const float centerDistance = (centerFromOtherInThisFrame - _root.GetCenter()).Length();
  const float minNodeSize = kQuadTreeInitialRootSideLength / (2 << (kQuadTreeInitialMaxDepth-1));
  const float maxNodeSize = minNodeSize * (2 << (kQuadTreeMaxRootDepth-1));
  const float halfRootSize = maxNodeSize * 0.5f;
  const int maxNumberOfShifts = std::ceil( centerDistance / halfRootSize );
  
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
      AddQuad(transformedQuad2d, copyOfContent, maxNumberOfShifts);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Quad2f& quadToCover, int shiftAllowedCount)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ExpandByQuad");
  
  // allow expanding several times until the quad fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool quadFitsInMap = false;
  
  do
  {
    // find in which direction we are expanding, upgrade root level in that direction (center moves)
    const Vec2f& direction = quadToCover.ComputeCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);

    // check if the quad now fits in the expanded root
    quadFitsInMap = _root.Contains(quadToCover);
    
  } while( !quadFitsInMap && expanded );

  // if the quad still doesn't fit, see if we can shift
  int shiftsDone = 0;
  bool canShift = (shiftsDone<shiftAllowedCount);
  if ( !quadFitsInMap && canShift )
  {
    // calculate points for shift
    std::vector<Point2f> requiredPoints;
    requiredPoints.reserve(4);
    for( const Point2f& p : quadToCover ) {
      requiredPoints.emplace_back(p);
    }
    
    // shift as many times as we can until the quad fits
    do
    {
      // shift the root to try to cover the quad, by removing opposite nodes in the map
      _root.ShiftRoot(requiredPoints, _processor);
      ++shiftsDone;
      canShift = (shiftsDone<shiftAllowedCount);

      // check if the quad now fits in the expanded root
      quadFitsInMap = _root.Contains(quadToCover);
      
    } while( !quadFitsInMap && canShift );
  }
  
  // the quad should be contained, if it's not, we have reached the limit of expansions and shifts, and the quad does not
  // fit, which will cause information loss
  if ( !quadFitsInMap ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.ExpandByQuad.InsufficientExpansion",
      "Quad caused expansion, but expansion was not enough QuadCenter(%.2f, %.2f), Root(%.2f,%.2f) with sideLen(%.2f).",
      quadToCover.ComputeCentroid().x(), quadToCover.ComputeCentroid().y(),
      _root.GetCenter().x(), _root.GetCenter().y(),
      _root.GetSideLen() );
  }
  
  // always flag as dirty since we have modified the root (potentially)
  _gfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Point2f& pointToInclude, int shiftAllowedCount)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ExpandByPoint");
  
  // allow expanding several times until the point fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool pointInMap = false;
  
  do {

    // find in which direction we are expanding and upgrade root level in that direction (center moves)
    const Vec2f& direction = pointToInclude - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);
    
    // check if the point now fits in the expanded root
    pointInMap = _root.Contains(pointToInclude);
    
  } while( !pointInMap && expanded );
  
  // if the point still doesn't fit, see if we can shift
  int shiftsDone = 0;
  bool canShift = (shiftsDone<shiftAllowedCount);
  if ( !pointInMap && canShift )
  {
    // calculate points for shift
    std::vector<Point2f> requiredPoints;
    requiredPoints.reserve(1);
    requiredPoints.emplace_back(pointToInclude);
    
    // shift as many times as we can until the quad fits
    do
    {
      // shift the root to try to cover the point, by removing opposite nodes in the map
      _root.ShiftRoot(requiredPoints, _processor);
      ++shiftsDone;
      canShift = (shiftsDone<shiftAllowedCount);

      // check if the point now fits in the expanded root
      pointInMap = _root.Contains(pointToInclude);
      
    } while( !pointInMap && canShift );
  }
  
  // the point should be contained, if it's not, we have reached the limit of expansions and shifts, and the point does not
  // fit, which will cause information loss
  if ( !pointInMap ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.ExpandByPoint.InsufficientExpansion",
      "Point caused expansion, but expansion was not enough Point(%.2f, %.2f), Root(%.2f,%.2f) with sideLen(%.2f).",
      pointToInclude.x(), pointToInclude.y(), _root.GetCenter().x(), _root.GetCenter().y(), _root.GetSideLen() );
  }
  
  // always flag as dirty since we have modified the root (potentially)
  _gfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Triangle2f& triangleToCover, int shiftAllowedCount)
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::ExpandByTriangle");
  
  // allow expanding several times until the triangle fits in the tree, as long as we can expand, we keep trying,
  // relying on the root to tell us if we reached a limit
  bool expanded = false;
  bool triangleInMap = false;
  
  do {

    // find in which direction we are expanding and upgrade root level in that direction (center moves)
    const Vec2f& direction = triangleToCover.GetCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    expanded = _root.UpgradeRootLevel(direction, kQuadTreeMaxRootDepth, _processor);
    
    // check if the point now fits in the expanded root
    triangleInMap = _root.Contains(triangleToCover);
    
  } while( !triangleInMap && expanded );
  
  // if the triangle still doesn't fit, see if we can shift
  int shiftsDone = 0;
  bool canShift = (shiftsDone<shiftAllowedCount);
  if ( !triangleInMap && canShift )
  {
    // calculate points for shift
    std::vector<Point2f> requiredPoints;
    requiredPoints.reserve(3);
    for( const Point2f& p : triangleToCover ) {
      requiredPoints.emplace_back(p);
    }
    
    // shift as many times as we can until the triangle fits
    do
    {
      // shift the root to try to cover the triangle, by removing opposite nodes in the map
      _root.ShiftRoot(requiredPoints, _processor);
      ++shiftsDone;
      canShift = (shiftsDone<shiftAllowedCount);

      // check if the triangle now fits in the expanded root
      triangleInMap = _root.Contains(triangleToCover);
      
    } while( !triangleInMap && canShift );
  }
  
  // the point should be contained, if it's not, we have reached the limit of expansions and shifts, and the point does not
  // fit, which will cause information loss
  if ( !triangleInMap ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.ExpandByTriangle.InsufficientExpansion",
      "Triangle caused expansion, but expansion was not enough TriCenter(%.2f, %.2f), Root(%.2f,%.2f) with sideLen(%.2f).",
      triangleToCover.GetCentroid().x(), triangleToCover.GetCentroid().y(),
      _root.GetCenter().x(), _root.GetCenter().y(),
      _root.GetSideLen() );
  }
  
  // always flag as dirty since we have modified the root (potentially)
  _gfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Broadcast(uint32_t originID) const
{
  ANKI_CPU_PROFILE("NavMeshQuadTree::Broadcast");
  
  using namespace ExternalInterface;
  
  // Create and send the start (header) message
  const Point3f& rootCenter = _root.GetCenter();
  MemoryMapMessageBegin msgBegin(originID, _root.GetLevel(), _root.GetSideLen(), rootCenter.x(), rootCenter.y());
  _robot->Broadcast(MessageEngineToGame(std::move(msgBegin)));

  // Ask root to add quad info to be sent (do a DFS of entire tree)
  NavMeshQuadTreeNode::QuadInfoVector quadInfoVector;
  _root.AddQuadsToSend(quadInfoVector);

  // Now send these packets in clad message(s), respecting the clad message size limit
  const size_t kReservedBytes = 1 + 2; // Message overhead for:  Tag, and vector size
  const size_t kMaxBufferSize = Anki::Comms::MsgPacket::MAX_SIZE;
  const size_t kMaxBufferForQuads = kMaxBufferSize - kReservedBytes;
  size_t quadsPerMessage = kMaxBufferForQuads / sizeof(NavMeshQuadTreeNode::QuadInfoVector::value_type);
  size_t remainingQuads = quadInfoVector.size();
  
  DEV_ASSERT(quadsPerMessage > 0, "NavMeshQuadTree.Broadcast.InvalidQuadsPerMessage");
  
  // We can't initialize messages with a range of vectors, so we have to create copies
  NavMeshQuadTreeNode::QuadInfoVector partQuadInfos;
  partQuadInfos.reserve( quadsPerMessage );
  
  // while we have quads to send
  while ( remainingQuads > 0 )
  {
    // how many are we sending in this message?
    quadsPerMessage = Anki::Util::Min(quadsPerMessage, remainingQuads);
    
    // clear the destination vector and insert as many as we are sending, from where we left off
    partQuadInfos.clear();
    partQuadInfos.insert( partQuadInfos.end(), quadInfoVector.end() - remainingQuads, quadInfoVector.end() - remainingQuads + quadsPerMessage );
    
    remainingQuads -= quadsPerMessage;
    
    // send message
    _robot->Broadcast(MessageEngineToGame(MemoryMapMessage(partQuadInfos)));
  }
  
  // Send the end message
  _robot->Broadcast(MessageEngineToGame(MemoryMapMessageEnd()));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::BroadcastMemoryMapDraw(uint32_t originID, size_t mapIdxHint) const
{
  if ( _gfxDirty && kRenderNavMeshQuadTree )
  {
    ANKI_CPU_PROFILE("NavMeshQuadTree::BroadcastMemoryMapDraw");
    
    _gfxDirty = false;
    
    using namespace ExternalInterface;
    using namespace VizInterface;
    
    // Create and send the start (header) message
    const Point3f& rootCenter = _root.GetCenter();
    float adjustedZ = rootCenter.z();
    // The mapIdx hint reveals that we are not the current active map, but an old memory. Apply some offset
    // so that we don't render on top of any other map
    if (mapIdxHint > 0)
    {
      const float offSetPerIdx = -250.0f;
      adjustedZ += (mapIdxHint * offSetPerIdx);
    }
    else
    {
      // small offset to not clip with floor
      adjustedZ += 10.0f;
    }
    
    std::stringstream instanceId;
    instanceId << "New_NavMeshQuadTree_" << this;
    ExternalInterface::MemoryMapInfo info(_root.GetLevel(), _root.GetSideLen(), rootCenter.x(), rootCenter.y(), adjustedZ, instanceId.str());
    MemoryMapMessageDebugVizBegin msgBegin(originID, info);
    _robot->Broadcast(MessageViz(std::move(msgBegin)));
    
    // Ask root to add quad info to be sent (do a DFS of entire tree)
    NavMeshQuadTreeNode::QuadInfoDebugVizVector quadInfoVector;
    _root.AddQuadsToSendDebugViz(quadInfoVector);
    
    // Now send these packets in clad message(s), respecting the clad message size limit
    const size_t kReservedBytes = 1 + 2; // Message overhead for:  Tag, and vector size
    const size_t kMaxBufferSize = Anki::Comms::MsgPacket::MAX_SIZE;
    const size_t kMaxBufferForQuads = kMaxBufferSize - kReservedBytes;
    size_t quadsPerMessage = kMaxBufferForQuads / sizeof(NavMeshQuadTreeNode::QuadInfoVector::value_type);
    size_t remainingQuads = quadInfoVector.size();
    
    DEV_ASSERT(quadsPerMessage > 0, "NavMeshQuadTree.BroadcastMemoryMapDraw.InvalidQuadsPerMessage");
    
    // We can't initialize messages with a range of vectors, so we have to create copies
    NavMeshQuadTreeNode::QuadInfoDebugVizVector partQuadInfos;
    partQuadInfos.reserve(quadsPerMessage);
    
    // while we have quads to send
    while (remainingQuads > 0)
    {
      // how many are we sending in this message?
      quadsPerMessage = Anki::Util::Min(quadsPerMessage, remainingQuads);
      
      // clear the destination vector and insert as many as we are sending, from where we left off
      partQuadInfos.clear();
      partQuadInfos.insert( partQuadInfos.end(), quadInfoVector.end() - remainingQuads, quadInfoVector.end() - remainingQuads + quadsPerMessage );
      
      remainingQuads -= quadsPerMessage;
      
      // send message
      _robot->Broadcast(MessageViz(MemoryMapMessageDebugViz(originID, partQuadInfos)));
    }
    
    // Send the end message
    _robot->Broadcast(MessageViz(MemoryMapMessageDebugVizEnd(originID)));
  }
}
  

} // namespace Cozmo
} // namespace Anki
