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

#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/polygon_impl.h"

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

CONSOLE_VAR(bool, kRenderQuadTree      , "QuadTree", true);
CONSOLE_VAR(bool, kRenderLastAddedQuad , "QuadTree", false);

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
QuadTree::QuadTree(VizManager* vizManager, Robot* robot, MemoryMapData rootData)
: _gfxDirty(true)
, _processor(vizManager)
, _root({0,0,1}, kQuadTreeInitialRootSideLength, kQuadTreeInitialMaxDepth, QuadTreeTypes::EQuadrant::Root, nullptr, rootData)  // Note the root is created at z=1
, _vizManager(vizManager)
, _robot(robot)
{
  _processor.SetRoot( &_root );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTree::~QuadTree()
{
  // we are destroyed, stop our rendering
  ClearDraw();
  
  _processor.SetRoot(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTree::DrawDebugProcessorInfo(size_t mapIdxHint) const
{
  // draw the processor information
  if ( mapIdxHint == 0 ) {
    _processor.Draw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTree::ClearDraw() const
{
  ANKI_CPU_PROFILE("QuadTree::ClearDraw");
  
  std::stringstream instanceId;
  instanceId << "New_QuadTree_" << this;
  _vizManager->EraseQuadVector(instanceId.str());
  
  _gfxDirty = true;
  
  // also clear processor information
  _processor.ClearDraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float QuadTree::GetContentPrecisionMM() const
{
  // return the length of the smallest quad allowed
  const float minSide_mm = kQuadTreeInitialRootSideLength / (1 << kQuadTreeInitialMaxDepth); // 1 << x = pow(2,x)
  return minSide_mm;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTree::Insert(const FastPolygon& poly, const MemoryMapData& data, int shiftAllowedCount)
{
  ANKI_CPU_PROFILE("QuadTree::Insert");
  
  {
    // I have had a unit test send here a NaN quad, probably because a cube pose was busted, detect that case
    // here and ignore the quad so that we don't assert because we expand indefinitely
    bool isNaNPoly = false;
    for (const auto& pt : poly.GetSimplePolygon()) 
    {
      isNaNPoly |= std::isnan(pt.x()) ||   std::isnan(pt.y());
    }
  
    if ( isNaNPoly ) {
      PRINT_NAMED_ERROR("QuadTree.Insert.NaNPoly",
        "Poly is not valid, at least one coordinate is NaN.");
      Util::sDumpCallstack("QuadTree::Insert");
      return;
    }
  }
  
  // if the root does not contain the poly, expand
  if ( !_root.Contains( poly ) )
  {
    // TODO: (mrw) we now remove objects by ID, so we should never be adding the removal type at all?
    
    // if we are 'adding' a removal poly, do not expand, since it would be useless to expand or shift to try
    // to remove data.
    const bool isRemovingContent = MemoryMapTypes::IsRemovalType(data.type);
    if ( isRemovingContent ) {
      PRINT_NAMED_WARNING("QuadTree.Insert.RemovalPolyNotContained",
        "Poly is not fully contained in root, removal does not cause expansion.");
    }
    else
    {
      Expand( poly.GetSimplePolygon(), shiftAllowedCount );
    }
  }

  const bool changed = _root.Insert(poly, data, _processor);
  _gfxDirty |= changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTree::Merge(const QuadTree& other, const Pose3d& transform)
{
  // TODO rsam for the future, when we merge with transform, poses or directions stored as extra info are invalid
  // since they were wrt a previous origin!
  Pose2d transform2d(transform);

  // obtain all leaf nodes from the map we are merging from
  QuadTreeNode::NodeCPtrVector leafNodes;
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
      // TODO: don't copy the data, pass the shared pointer. Also generate the poly directly rather than through a quad
      MemoryMapData* copyOfData(nodeInOther->GetContent().data.get());
      Poly2f transformedPoly;
      transformedPoly.ImportQuad2d(transformedQuad2d);
      
//      AddQuad(transformedQuad2d, *copyOfData, maxNumberOfShifts);
        Insert(transformedPoly, *copyOfData, maxNumberOfShifts);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTree::Expand(const Poly2f& polyToCover, int shiftAllowedCount)
{
  ANKI_CPU_PROFILE("QuadTree::Expand");
  
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

  // if the poly still doesn't fit, see if we can shift
  int shiftsDone = 0;
  bool canShift = (shiftsDone<shiftAllowedCount);
  if ( !fitsInMap && canShift )
  {
    // shift as many times as we can until the poly fits
    do
    {
      // shift the root to try to cover the poly, by removing opposite nodes in the map
      _root.ShiftRoot(polyToCover, _processor);
      ++shiftsDone;
      canShift = (shiftsDone<shiftAllowedCount);

      // check if the poly now fits in the expanded root
      fitsInMap = _root.Contains(polyToCover);
      
    } while( !fitsInMap && canShift );
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
  _gfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTree::Broadcast(uint32_t originID) const
{
  ANKI_CPU_PROFILE("QuadTree::Broadcast");
  
  using namespace ExternalInterface;
  
  // Create and send the start (header) message
  const Point3f& rootCenter = _root.GetCenter();
  MemoryMapMessageBegin msgBegin(originID, _root.GetLevel(), _root.GetSideLen(), rootCenter.x(), rootCenter.y());
  _robot->Broadcast(MessageEngineToGame(std::move(msgBegin)));

  // Ask root to add quad info to be sent (do a DFS of entire tree)
  QuadTreeNode::QuadInfoVector quadInfoVector;
  _root.AddQuadsToSend(quadInfoVector);

  // Now send these packets in clad message(s), respecting the clad message size limit
  const size_t kReservedBytes = 1 + 2; // Message overhead for:  Tag, and vector size
  const size_t kMaxBufferSize = Anki::Comms::MsgPacket::MAX_SIZE;
  const size_t kMaxBufferForQuads = kMaxBufferSize - kReservedBytes;
  size_t quadsPerMessage = kMaxBufferForQuads / sizeof(QuadTreeNode::QuadInfoVector::value_type);
  size_t remainingQuads = quadInfoVector.size();
  
  DEV_ASSERT(quadsPerMessage > 0, "QuadTree.Broadcast.InvalidQuadsPerMessage");
  
  // We can't initialize messages with a range of vectors, so we have to create copies
  QuadTreeNode::QuadInfoVector partQuadInfos;
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
void QuadTree::BroadcastMemoryMapDraw(uint32_t originID, size_t mapIdxHint) const
{
  if ( _gfxDirty && kRenderQuadTree )
  {
    ANKI_CPU_PROFILE("QuadTree::BroadcastMemoryMapDraw");
    
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
    instanceId << "New_QuadTree_" << this;
    ExternalInterface::MemoryMapInfo info(_root.GetLevel(), _root.GetSideLen(), rootCenter.x(), rootCenter.y(), adjustedZ, instanceId.str());
    MemoryMapMessageDebugVizBegin msgBegin(originID, info);
    _robot->Broadcast(MessageViz(std::move(msgBegin)));
    
    // Ask root to add quad info to be sent (do a DFS of entire tree)
    QuadTreeNode::QuadInfoDebugVizVector quadInfoVector;
    _root.AddQuadsToSendDebugViz(quadInfoVector);
    
    // Now send these packets in clad message(s), respecting the clad message size limit
    const size_t kReservedBytes = 1 + 2; // Message overhead for:  Tag, and vector size
    const size_t kMaxBufferSize = Anki::Comms::MsgPacket::MAX_SIZE;
    const size_t kMaxBufferForQuads = kMaxBufferSize - kReservedBytes;
    size_t quadsPerMessage = kMaxBufferForQuads / sizeof(QuadTreeNode::QuadInfoVector::value_type);
    size_t remainingQuads = quadInfoVector.size();
    
    DEV_ASSERT(quadsPerMessage > 0, "QuadTree.BroadcastMemoryMapDraw.InvalidQuadsPerMessage");
    
    // We can't initialize messages with a range of vectors, so we have to create copies
    QuadTreeNode::QuadInfoDebugVizVector partQuadInfos;
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
