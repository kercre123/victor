/**
 * File: navMeshQuadTreeProcessor.h
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Class for processing a navMeshQuadTree. It relies on the navMeshQuadTree and navMeshQuadTreeNodes to
 * share the proper information for the Processor.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "navMeshQuadTreeProcessor.h"
#include "navMeshQuadTreeNode.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/vision/basestation/profiler.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#include <limits>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool , kRenderContentTypes , "NavMeshQuadTreeProcessor", false); // renders registered content nodes for webots
CONSOLE_VAR(bool , kRenderSeeds        , "NavMeshQuadTreeProcessor", false); // renders seeds differently for debugging purposes
CONSOLE_VAR(bool , kRenderBordersFrom  , "NavMeshQuadTreeProcessor", false); // renders detected borders (origin quad)
CONSOLE_VAR(bool , kRenderBordersToDot , "NavMeshQuadTreeProcessor", false); // renders detected borders (border center) as dots
CONSOLE_VAR(bool , kRenderBordersToQuad, "NavMeshQuadTreeProcessor", false); // renders detected borders (destination quad)
CONSOLE_VAR(bool , kRenderBorder3DLines, "NavMeshQuadTreeProcessor", false); // renders borders returned as 3D lines (instead of quads)
CONSOLE_VAR(bool , kRenderBorderRegions, "NavMeshQuadTreeProcessor", false); // renders borders returned as quads per region (instead of quads)
CONSOLE_VAR(float, kRenderZOffset      , "NavMeshQuadTreeProcessor", 20.0f); // adds Z offset to all quads
CONSOLE_VAR(bool , kDebugFindBorders   , "NavMeshQuadTreeProcessor", false); // prints debug information in console

#define DEBUG_FIND_BORDER(format, ...)                                                                          \
if ( kDebugFindBorders ) {                                                                                      \
  do{::Anki::Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, "NMQTProcessor", {}, format, ##__VA_ARGS__);}while(0); \
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeProcessor::NavMeshQuadTreeProcessor(VizManager* vizManager)
: _currentBorderCombination(nullptr)
, _root(nullptr)
, _contentGfxDirty(false)
, _borderGfxDirty(false)
, _totalExploredArea_m2(0.0)
, _totalInterestingEdgeArea_m2(0.0)
, _vizManager(vizManager)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::SetRoot(NavMeshQuadTreeNode* node)
{
  // grab new root
  _root = node;
  
  // check type if valid root
  if ( nullptr != _root )
  {
    // we expect to set the root before anyone else (and only then)
    DEV_ASSERT(node->GetContentType() == ENodeContentType::Invalid, "NavMeshQuadTreeProcessor.SetRoot.RootIsInitialized");
    // change from invalid to unknown; this is required when Unknown is cached, so that we cache the root as soon as we get it
    NavMeshQuadTreeTypes::NodeContent rootContent(ENodeContentType::Unknown);
    _root->ForceSetDetectedContentType(rootContent, *this);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::OnNodeContentTypeChanged(const NavMeshQuadTreeNode* node, ENodeContentType oldContent, ENodeContentType newContent)
{
  DEV_ASSERT(node->GetContentType() == newContent,
             "NavMeshQuadTreeProcessor.OnNodeContentTypeChanged.MismatchedNewContent");
  DEV_ASSERT(oldContent != newContent, "NavMeshQuadTreeProcessor.OnNodeContentTypeChanged.ContentNotChanged");
  
  // update exploration area based on the content type
  {
    const bool wasOutOld = (oldContent == ENodeContentType::Invalid   ) ||
                           (oldContent == ENodeContentType::Unknown   ) ||
                           (oldContent == ENodeContentType::Subdivided);
    const bool isOutNew  = (newContent == ENodeContentType::Invalid   ) ||
                           (newContent == ENodeContentType::Unknown   ) ||
                           (newContent == ENodeContentType::Subdivided);
    const bool needsToRemove = !wasOutOld &&  isOutNew;
    const bool needsToAdd    =  wasOutOld && !isOutNew;
    if ( needsToRemove )
    {
      const float side_m = MM_TO_M(node->GetSideLen());
      const float area_m2 = side_m*side_m;
      _totalExploredArea_m2 -= area_m2;
    }
    else if ( needsToAdd )
    {
      const float side_m = MM_TO_M(node->GetSideLen());
      const float area_m2 = side_m*side_m;
      _totalExploredArea_m2 += area_m2;
    }
  }

  // update interesting edge
  {
    const bool shouldBeCountedOld = (oldContent == ENodeContentType::InterestingEdge);
    const bool shouldBeCountedNew = (newContent == ENodeContentType::InterestingEdge);
    const bool needsToRemove =  shouldBeCountedOld && !shouldBeCountedNew;
    const bool needsToAdd    = !shouldBeCountedOld &&  shouldBeCountedNew;
    if ( needsToRemove )
    {
      const float side_m = MM_TO_M(node->GetSideLen());
      const float area_m2 = side_m*side_m;
      _totalInterestingEdgeArea_m2 -= area_m2;
    }
    else if ( needsToAdd )
    {
      const float side_m = MM_TO_M(node->GetSideLen());
      const float area_m2 = side_m*side_m;
      _totalInterestingEdgeArea_m2 += area_m2;
    }
  }

  // if old content type is cached
  if ( IsCached(oldContent) )
  {
    // remove the node from that cache
    DEV_ASSERT(_nodeSets[oldContent].find(node) != _nodeSets[oldContent].end(),
               "NavMeshQuadTreeProcessor.OnNodeContentTypeChanged.InvalidRemove");
    _nodeSets[oldContent].erase( node );
    
    // flag as dirty
    _contentGfxDirty = true;
  }

  if ( IsCached(newContent) )
  {
    // add node to that cache
    DEV_ASSERT(_nodeSets[newContent].find(node) == _nodeSets[newContent].end(),
               "NavMeshQuadTreeProcessor.OnNodeContentTypeChanged.InvalidInsert");
    _nodeSets[newContent].insert(node);
    
    // flag as dirty
    _contentGfxDirty = true;
  }
  
  // invalidate all borders. Note this is not optimal, we could invalidate only affected borders (if such
  // processing could be done easily), or at least discarding changes by parent quad
  InvalidateBorders();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::OnNodeDestroyed(const NavMeshQuadTreeNode* node)
{
  // if old content type is cached
  const ENodeContentType oldContent = node->GetContentType();
  if ( IsCached(oldContent) )
  {
    // remove the node from that cache
    DEV_ASSERT(_nodeSets[oldContent].find(node) != _nodeSets[oldContent].end(),
               "NavMeshQuadTreeProcessor.OnNodeDestroyed.InvalidNode");
    _nodeSets[oldContent].erase(node);
    
    // flag as dirty
    _contentGfxDirty = true;
  }

  // remove the area for this node if it was counted before
  {
    const bool wasOutOld = (oldContent == ENodeContentType::Invalid   ) ||
                           (oldContent == ENodeContentType::Unknown   ) ||
                           (oldContent == ENodeContentType::Subdivided);
    const bool needsToRemove = !wasOutOld;
    if ( needsToRemove )
    {
      const float side_m = MM_TO_M(node->GetSideLen());
      const float area_m2 = side_m*side_m;
      _totalExploredArea_m2 -= area_m2;
    }
  }
  
  // remove interesting edge area if it was counted before
  {
    const bool shouldBeCountedOld = (oldContent == ENodeContentType::InterestingEdge);
    const bool needsToRemove =  shouldBeCountedOld;
    if ( needsToRemove )
    {
      const float side_m = MM_TO_M(node->GetSideLen());
      const float area_m2 = side_m*side_m;
      _totalInterestingEdgeArea_m2 -= area_m2;
    }
  }
  
  // invalidate all borders. Note this is not optimal, we could invalidate only affected borders (if such
  // processing could be done easily), or at least discarding changes by parent quad
  InvalidateBorders();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::HasBorders(ENodeContentType innerType, ENodeContentTypePackedType outerTypes) const
{
  // check cached version if available and current
  const BorderKeyType borderComboKey = GetBorderTypeKey(innerType, outerTypes);
  const auto comboMatchIt = _bordersPerContentCombination.find( borderComboKey );
  if ( comboMatchIt != _bordersPerContentCombination.end() )
  {
    if ( !comboMatchIt->second.dirty ) {
      const bool hasBorders = !comboMatchIt->second.waypoints.empty();
      return hasBorders;
    }
  }
  
  // no cached version, pick in the cached nodes if any would be seed
  const bool hasSeed = HasBorderSeed(innerType, outerTypes);
  return hasSeed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::GetBorders(ENodeContentType innerType, ENodeContentTypePackedType outerTypes, NavMemoryMapTypes::BorderRegionVector& outBorders)
{
  #if ANKI_DEV_CHEATS
  // container for debug quads
  VizManager::SimpleQuadVector allRegionQuads;
  static size_t curColorIdx = 0;
  curColorIdx=0; // reset so that regions have same color as previous calculation (useful when I call this per frame)
  #endif

  // grab the border combination info
  const BorderCombination& borderCombination = RefreshBorderCombination(innerType, outerTypes);

  outBorders.clear();
  if ( !borderCombination.waypoints.empty() )
  {
    // -- convert from borderCombination info to borderVector
    
    // container to calculate area of a region by checking which nodes belong to this region
    std::unordered_set<const NavMeshQuadTreeNode*> nodesInCurrentRegion;

    // epsilon to merge waypoints into the same line
    const float kDotBorderEpsilon = 0.9848f; // cos(10deg) = 0.984807...
    EDirection firstNeighborDir = borderCombination.waypoints[0].direction;
    Point3f curOrigin = CalculateBorderWaypointCenter(borderCombination.waypoints[0]);
    Point3f curDest   = curOrigin;
    const NodeContent* curBorderContentPtr = &(borderCombination.waypoints[0].from->GetContent());
    
    // note: curNeighborDirection could be first, last, average, ... all should yield same result, so I'm doing first
  
    // iterate all waypoints (even 0)
    size_t idx = 0;
    const size_t count = borderCombination.waypoints.size();
    do
    {
      const bool isEnd = (borderCombination.waypoints[idx].isEnd);
      bool canMergeIntoPreviousLine = false;
      
      // add from node to region
      nodesInCurrentRegion.insert( borderCombination.waypoints[idx].from );
      
      // get border center for this waypoint
      Point3f borderCenter  = CalculateBorderWaypointCenter( borderCombination.waypoints[idx] );
    
      Vec3f curDir = curDest - curOrigin;
    
      // if we currently don't have a line (but a point)
      if ( NEAR_ZERO( curDir.LengthSq() ) )
      {
        // we can continue this line
        canMergeIntoPreviousLine = true;
      }
      else
      {
        // there's already a line, can we continue it?
        curDir.MakeUnitLength();
        Vec3f stepDir = borderCenter - curDest;
        stepDir.MakeUnitLength();
        // calculate dotProduct and see if they are close enough in the same direction
        const float dotProduct = DotProduct(curDir, stepDir);
        canMergeIntoPreviousLine = dotProduct >= kDotBorderEpsilon;
      }
      
      // check if the extra data can be shared between the current border and the waypoint we intend to add
      if ( canMergeIntoPreviousLine ) {
        const NodeContent& waypointContent = borderCombination.waypoints[idx].from->GetContent();
        const bool canShareData = ((*curBorderContentPtr) == waypointContent);
        canMergeIntoPreviousLine = canShareData;
      }
      
      //  canMergeIntoPrev ; isEnd ; expected
      //               0        0   ==> AddLine(), origin <- dest, dest <- center
      //               0        1   ==> AddLine(), origin <- dest, dest <- center, AddLine(), resetDir
      //               1        0   ==>                            dest <- center
      //               1        1   ==>                            dest <- center, AddLine(), resetDir
      // -->
      // if ( !canMerge ) { AddLine(), origin <- dest }
      // dest <- center
      // if ( isEnd ) { AddLine() }
      
      if ( !canMergeIntoPreviousLine )
      {
        // add segment to the current region, add region if there's no current one
        const EDirection lastNeighborDir = borderCombination.waypoints[idx-1].direction;
        if ( outBorders.empty() || outBorders.back().IsFinished() ) {
          outBorders.emplace_back();
        }
        NavMemoryMapTypes::BorderRegion& curRegion = outBorders.back();
        curRegion.segments.emplace_back( MakeBorderSegment(curOrigin, curDest, curBorderContentPtr->data, firstNeighborDir, lastNeighborDir) );
        
        // origin <- dest
        curOrigin = curDest;
        firstNeighborDir = lastNeighborDir;
        curBorderContentPtr = &(borderCombination.waypoints[idx].from->GetContent());
      }
      
      curDest = borderCenter;
      
      if ( isEnd )
      {
        // add border
        const EDirection lastNeighborDir = borderCombination.waypoints[idx].direction;
        if ( outBorders.empty() || outBorders.back().IsFinished() ) {
          outBorders.emplace_back();
        }
        NavMemoryMapTypes::BorderRegion& curRegion = outBorders.back();
        curRegion.segments.emplace_back( MakeBorderSegment(curOrigin, curDest, curBorderContentPtr->data, firstNeighborDir, lastNeighborDir) );
        
        // calculate the area of the border by adding all node's area
        float totalArea_m2 = 0.0f;
        for ( const auto& nodeInBorder : nodesInCurrentRegion )
        {
          const float side_m = MM_TO_M(nodeInBorder->GetSideLen());
          const float area_m2 = side_m*side_m;
          totalArea_m2 += area_m2;
          
          #if ANKI_DEV_CHEATS
          {
            if ( kRenderBorderRegions ) {
              // colors per region (to change colors and see them better)
              constexpr size_t colorCount = 9;
              ColorRGBA regionColors[colorCount] =
              { NamedColors::RED, NamedColors::GREEN, NamedColors::BLUE,
                NamedColors::YELLOW, NamedColors::CYAN, NamedColors::ORANGE,
                NamedColors::MAGENTA, NamedColors::WHITE, NamedColors::BLACK };
              // add quad
              const ColorRGBA& regionColor = regionColors[curColorIdx%colorCount];
              Vec3f center = nodeInBorder->GetCenter();
              center.z() += 50.f; // z offset to render
              allRegionQuads.emplace_back(VizManager::MakeSimpleQuad(regionColor, center, nodeInBorder->GetSideLen()));
            }
          }
          #endif
        }
        
        #if ANKI_DEV_CHEATS
        ++curColorIdx;
        #endif
        
        // clear nodes since we'll start a new region
        nodesInCurrentRegion.clear();
        
        // finish the region
        curRegion.Finish(totalArea_m2);
        
        // if there are more waypoints, reset current data so that the next waypoint doesn't start on us
        const size_t nextIdx = idx+1;
        if ( nextIdx < count ) {
          firstNeighborDir = borderCombination.waypoints[nextIdx].direction;
          curOrigin = CalculateBorderWaypointCenter(borderCombination.waypoints[nextIdx]);
          curDest   = curOrigin;
          curBorderContentPtr = &(borderCombination.waypoints[nextIdx].from->GetContent());
        }
      }
      
      ++idx;
    } while ( idx < count );
    
  } // has waypoints
  
  // debug render all quads
  #if ANKI_DEV_CHEATS
  {
    // debug render of lines returned to systems quering for borders
    if ( kRenderBorder3DLines )
    {
      _vizManager->EraseSegments("NavMeshQuadTreeProcessorBorderSegments");
      for ( const auto& region : outBorders )
      {
        for( const auto& segment : region.segments )
        {
          Vec3f centerLine = (segment.from + segment.to)*0.5f;
          _vizManager->DrawSegment("NavMeshQuadTreeProcessorBorderSegments",
            segment.from, segment.to, Anki::NamedColors::YELLOW, false, 50.0f);
          _vizManager->DrawSegment("NavMeshQuadTreeProcessorBorderSegments",
            centerLine, centerLine+segment.normal*5.0f, Anki::NamedColors::BLUE, false, 50.0f);
        }
      }
    }
    
    if ( kRenderBorderRegions ) {
      _vizManager->DrawQuadVector("NavMeshQuadTreeProcessorBorderSegments", allRegionQuads);
    }
  }
  #endif
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, ENodeContentTypePackedType types) const
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeProcessor::HasCollisionRayWithTypesNonRecursive");

  // search from root
  const bool ret = HasCollisionRayWithTypes(_root, rayFrom, rayTo, types);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::HasCollisionRayWithTypes(const NavMeshQuadTreeNode* node, const Point2f& rayFrom, const Point2f& rayTo, ENodeContentTypePackedType types) const
{
  // this function could be easily optimized by using axis-aligned math available in NavMeshQuadTreeNode. Adding profile
  // here to prove that we need to
  // ANKI_CPU_PROFILE("NavMeshQuadTreeProcessor::HasCollisionRayWithTypesRecursive"); // recursive functions are not properly tracked. Moving to caller

  // does this node match the types we are looking for?
  const ENodeContentType curNodeType = node->GetContentType();
  const bool matchesType = IsInENodeContentTypePackedType(curNodeType, types);
  if ( !node->IsSubdivided() && !matchesType ) {
    // if it's a leaf quad and the type is not interesting, do not care if the quad collides
    return false;
  }

  // if a quad contains any of the points, or the ray intersects with the quad, then the quad is relevant
  const Quad2f& curQuad = node->MakeQuadXY();
  bool doesLineCrossQuad = false;
  const bool isQuadRelevant = QTOptimizations::OverlapsOrContains(curQuad, NavMeshQuadTreeNode::SegmentLineEquation(rayFrom, rayTo), doesLineCrossQuad);
  if ( isQuadRelevant )
  {
    // the quad is relevant, let's check type
    
    // if it matches any type specified, then we found collision return true
    if ( matchesType ) {
      return true;
    }
    
    // if it has children, depth search first
    size_t numChildren = node->GetNumChildren();
    for(size_t index=0; index<numChildren; ++index)
    {
      // grab children at index and ask for collision/type check
      const std::unique_ptr<NavMeshQuadTreeNode>& childPtr = node->GetChildAt(index);
      DEV_ASSERT(childPtr, "NavMeshQuadTreeProcessor.HasCollisionRayWithTypes.NullChild");
      const bool childMatches = HasCollisionRayWithTypes(childPtr.get(), rayFrom, rayTo, types);
      if ( childMatches ) {
        // child said yes, we are also a yes
        return true;
      }
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::FillBorder(ENodeContentType filledType, ENodeContentTypePackedType fillingTypeFlags, const NodeContent& newContent)
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeProcessor.FillBorder");

  DEV_ASSERT_MSG(IsCached(filledType),
                 "NavMeshQuadTreeProcessor.FillBorder.FilledTypeNotCached",
                 "%s is not cached, which is needed for fast processing operations",
                 ENodeContentTypeToString(filledType));

  // should this timer be a member variable? It's normally desired to time all processors
  // together, but beware when merging stats from different maps (always current is the only one processing)
  static Vision::Profiler timer;
  // timer.SetPrintChannelName("Unfiltered");
  timer.SetPrintFrequency(5000);
  timer.Tic("NavMeshQuadTreeProcessor.FillBorder");

  // calculate nodes being flooded directly. Note that we are not going to cause filled nodes to flood forward
  // into others. A second call to FillBorder would be required for that (consider for local fills when we have them,
  // since they'll be significally faster).
  
  // The reason why we cache points instead of nodes is because adding a point can cause change and destruction of nodes,
  // for example through automerges in parents whose children all become the new content. To prevent having to
  // update an iterator from this::OnNodeX events, cache centers and apply. The result algorithm should be
  // slightly slower, but much simpler to understand, debug and profile
  std::vector<Point2f> floodedQuadCenters;
  std::unordered_set<const NavMeshQuadTreeNode*> doneQuads;

  // grab borders
  const BorderCombination& borderInfo = RefreshBorderCombination(filledType, fillingTypeFlags);
  for( auto& waypoint : borderInfo.waypoints )
  {
    const auto& retPair = doneQuads.emplace(waypoint.from);
    const bool isNew = retPair.second;
    if ( isNew ) {
      floodedQuadCenters.emplace_back(waypoint.from->GetCenter());
    }
  }
  
  // add flooded centers to the tree (not this does not cause flood filling)
  for( const auto& center : floodedQuadCenters ) {
    _root->AddContentPoint(center, newContent, *this);
  }
  
  timer.Toc("NavMeshQuadTreeProcessor.FillBorder");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::ReplaceContent(const Quad2f& inQuad, ENodeContentType typeToReplace, const NodeContent& newContent)
{
  DEV_ASSERT_MSG(IsCached(typeToReplace), "NavMeshQuadTreeProcessor.ReplaceContent",
                 "%s is not cached", ENodeContentTypeToString(typeToReplace));
  
  auto nodeSetMatch = _nodeSets.find(typeToReplace);
  if (nodeSetMatch != _nodeSets.end())
  {
    const NodeSet& nodesToReplaceSet = nodeSetMatch->second;
    if ( !nodesToReplaceSet.empty() )
    {
      // when we change the content type, nodeSet for that type changes (erases nodes). Instead of caching iterators
      // in different functions, grab their center and readd content
      std::vector<Point2f> affectedNodeCenters;
      affectedNodeCenters.reserve( nodesToReplaceSet.size() );
      
      // iterate all nodes adding centers
      for( const auto& node : nodesToReplaceSet )
      {
        // check if the node and the quad overlap. In some cases this will cause this implementation to be slower. In
        // others it will be faster (depends on number of quads in inQuad, vs number of nodes with typeToReplace content)
        const bool isAffected = node->ContainsOrOverlapsQuad(inQuad);
        if ( isAffected ) {
          affectedNodeCenters.emplace_back( node->GetCenter() );
        }
      }
      
      // re-add all centers with the new type
      for( const Point2f& point : affectedNodeCenters ) {
        _root->AddContentPoint(point, newContent, *this);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::ReplaceContent(ENodeContentType typeToReplace, const NodeContent& newContent)
{
  DEV_ASSERT_MSG(IsCached(typeToReplace), "NavMeshQuadTreeProcessor.ReplaceContent",
                 "%s is not cached", ENodeContentTypeToString(typeToReplace));
  
  // note: NavMeshQuadTreeProcessor.FillBorder uses quads instead of nodes, and re-adds those quads as detected
  // content in order to change the memory map. That approach is a good way of having the processor not modify
  // nodes directly, but allow QuadTreeNodes to change their content and automerge parents properly. It
  // however can be slow, since it adds every quad without any other spatial information, having to start
  // from the root and explore down
  // Additionally, content override requirements may not meet, which I'm not sure it's a good or a bad thing:
  // did the caller of ReplaceContent actually mean "replace" or "tenative set"? At this moment I consider it
  // an actual replace, regardless of regular requirements for content overriding, which means we should bypass
  // the QTNodes' checks
  // For now, I am going to try the same approach but using AddPoint, which should be the 2nd fastest way of doing this,
  // since directly changing nodes would be faster, but probably the fastest one providing automerge without becoming
  // difficult to understand
  
  auto nodeSetMatch = _nodeSets.find(typeToReplace);
  if (nodeSetMatch != _nodeSets.end())
  {
    const NodeSet& nodesToReplaceSet = nodeSetMatch->second;
    if ( !nodesToReplaceSet.empty() )
    {
      // when we change the content type, nodeSet for that type changes (erases nodes). Instead of caching iterators
      // in different functions, grab their center and readd content
      std::vector<Point2f> affectedNodeCenters;
      affectedNodeCenters.reserve( nodesToReplaceSet.size() );
      
      // iterate all nodes adding centers
      for( const auto& node : nodesToReplaceSet )
      {
        affectedNodeCenters.emplace_back( node->GetCenter() );
      }
      
      // re-add all centers with the new type
      for( const Point2f& point : affectedNodeCenters ) {
        _root->AddContentPoint(point, newContent, *this);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::HasContentType(ENodeContentType nodeType) const
{
  DEV_ASSERT_MSG(IsCached(nodeType), "NavMeshQuadTreeProcessor.HasContentType", "%s is not cached",
                 ENodeContentTypeToString(nodeType));
  
  // check if any nodes for that type are cached currently
  auto nodeSetMatch = _nodeSets.find(nodeType);
  const bool hasAny = (nodeSetMatch != _nodeSets.end()) && !nodeSetMatch->second.empty();
  return hasAny;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::Draw() const
{
  // content quads
  if ( _contentGfxDirty && kRenderContentTypes )
  {
    VizManager::SimpleQuadVector quadVector;

    // add registered types
    for ( const auto& it : _nodeSets )
    {
      ENodeContentType type = it.first;
      AddQuadsToDraw(type, quadVector, GetDebugColor(type), kRenderZOffset);
    }
    
    _vizManager->DrawQuadVector("NavMeshQuadTreeProcessorContent", quadVector);
    
    _contentGfxDirty = false;
  }

  // borders
  if ( _borderGfxDirty && (kRenderBordersFrom || kRenderBordersToDot || kRenderBordersToQuad) )
  {

    VizManager::SimpleQuadVector quadVector;

    // add quads from borders
    for ( const auto& comboIt : _bordersPerContentCombination )
    {
      // if a border combination is dirty it means the quads that formed the borders have been modified
      // and hence we can't use them
      if ( comboIt.second.dirty ) {
        continue;
      }
    
      for ( const auto& it : comboIt.second.waypoints )
      {
        // from quad
        if ( kRenderBordersFrom || (kRenderSeeds && it.isSeed) )
        {
          Anki::ColorRGBA fromColor = GetDebugColor( it.from->GetContentType() );
          fromColor.SetAlpha(0.6f);
          const float fromSideLen = it.from->GetSideLen();
          Point3f from3D = it.from->GetCenter();
          from3D.z() += (kRenderZOffset + .1f);
          quadVector.emplace_back(VizManager::MakeSimpleQuad(fromColor, from3D, fromSideLen));
        }
      
        // to quad
        if ( kRenderBordersToQuad )
        {
          Anki::ColorRGBA toColor = GetDebugColor( it.to->GetContentType() );
          if ( kRenderSeeds && it.isSeed ) { toColor = Anki::NamedColors::YELLOW; }
          const float alpha = comboIt.second.dirty ? 0.2f : 0.8f;
          toColor.SetAlpha(alpha);
          const float toSideLen = it.to->GetSideLen();
          Point3f to3D = it.to->GetCenter();
          const float renderOffset = comboIt.second.dirty ? (kRenderZOffset*0.5f) : kRenderZOffset;
          to3D.z() += (renderOffset + .1f);
          if ( kRenderSeeds && it.isSeed) { to3D.z() += 1.0f; };
          quadVector.emplace_back(VizManager::MakeSimpleQuad(toColor, to3D, toSideLen));
        }

        // to dot
        if ( kRenderBordersToDot )
        {
          Anki::ColorRGBA toColor = GetDebugColor( it.to->GetContentType() );
          if ( kRenderBordersToQuad ) { toColor = Anki::NamedColors::WHITE; }
          if ( kRenderSeeds && it.isSeed ) { toColor = Anki::NamedColors::YELLOW; }
          if ( it.isEnd ) { toColor = Anki::NamedColors::MAGENTA; }
          if ( kRenderSeeds && it.isSeed && it.isEnd ) { toColor = Anki::NamedColors::ORANGE; }
          const float alpha = comboIt.second.dirty ? 0.2f : 0.8f;
          toColor.SetAlpha(alpha);
          const float toSideLen = 5.0f;
          Point3f border3D = CalculateBorderWaypointCenter( it );
          const float renderOffset = comboIt.second.dirty ? (kRenderZOffset*0.5f) : kRenderZOffset;
          border3D.z() += (renderOffset + .5f);
          if ( kRenderSeeds && it.isSeed) { border3D.z() += 1.0f; };
          quadVector.emplace_back(VizManager::MakeSimpleQuad(toColor, border3D, toSideLen));
        }

      }
    }
  
    _vizManager->DrawQuadVector("NavMeshQuadTreeProcessorBorders", quadVector);
    _borderGfxDirty = false;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::ClearDraw() const
{
  // clear content and borders
  _vizManager->EraseQuadVector("NavMeshQuadTreeProcessorContent");
  _vizManager->EraseQuadVector("NavMeshQuadTreeProcessorBorders");
  
  // flag as dirty for next draw
  _contentGfxDirty = true;
  _borderGfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::IsCached(ENodeContentType contentType)
{
  const bool isCached = (contentType == ENodeContentType::ObstacleCube         ) ||
                        (contentType == ENodeContentType::ObstacleUnrecognized ) ||
                        (contentType == ENodeContentType::InterestingEdge      ) ||
                        (contentType == ENodeContentType::NotInterestingEdge   ) ||
                        (contentType == ENodeContentType::Cliff );
  return isCached;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorRGBA NavMeshQuadTreeProcessor::GetDebugColor(ENodeContentType contentType)
{
  ColorRGBA ret = Anki::NamedColors::BLACK;
  switch (contentType) {
    case ENodeContentType::Invalid:               { DEV_ASSERT(false, "NavMeshQuadTreeProcessor.Invalid.NotSupported"); break; };
    case ENodeContentType::Subdivided:            { ret = ColorRGBA(0.2f, 0.2f, 0.2f, 0.3f); break; };
    case ENodeContentType::ClearOfObstacle:       { ret = ColorRGBA(0.0f, 1.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::ClearOfCliff:          { ret = ColorRGBA(0.0f, 0.5f, 0.0f, 0.3f); break; };
    case ENodeContentType::Unknown:               { ret = ColorRGBA(0.2f, 0.2f, 0.6f, 0.3f); break; };
    case ENodeContentType::ObstacleCube:          { ret = ColorRGBA(1.0f, 0.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::ObstacleCubeRemoved:   { ret = ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f); break; };
    case ENodeContentType::ObstacleCharger:       { ret = ColorRGBA(1.0f, 1.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::ObstacleChargerRemoved:{ ret = ColorRGBA(1.0f, 1.0f, 0.0f, 1.0f); break; };
    case ENodeContentType::ObstacleUnrecognized:  { ret = ColorRGBA(0.5f, 0.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::Cliff:                 { ret = ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::InterestingEdge:       { ret = ColorRGBA(0.0f, 0.0f, 0.5f, 0.3f); break; };
    case ENodeContentType::NotInterestingEdge:    { ret = ColorRGBA(0.0f, 0.5f, 0.5f, 0.3f); break; };
    case ENodeContentType::_Count:                { DEV_ASSERT(false, "NavMeshQuadTreeProcessor._Count.NotSupported"); break; };
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeProcessor::BorderKeyType NavMeshQuadTreeProcessor::GetBorderTypeKey(ENodeContentType innerType, ENodeContentTypePackedType outerTypes)
{
  using UnderlyingContentType = std::underlying_type<ENodeContentType>::type;

  static_assert( (sizeof(ENodeContentTypePackedType)+sizeof(UnderlyingContentType)) <= (sizeof(BorderKeyType)),
  "BorderKeyType should hold 2 ENodeContentTypePackedType" );
  
  // key = innerTypes << X | outerType, where X is sizeof(ENodeContentTypePackedType)
  BorderKeyType key = 0;
  key |= static_cast<UnderlyingContentType>(innerType);
  key <<= (8*(sizeof(ENodeContentTypePackedType)));
  key |= (outerTypes);

  return key;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vec3f NavMeshQuadTreeProcessor::CalculateBorderWaypointCenter(const BorderWaypoint& waypoint)
{
  Point3f borderCenter {};

  // get direction in 3D
  const Vec3f& borderDir = EDirectionToNormalVec3f(waypoint.direction);
  // find center from the smaller node towards the other one
  if ( waypoint.from->GetLevel() <= waypoint.to->GetLevel() ) {
    borderCenter = waypoint.from->GetCenter() + (borderDir * waypoint.from->GetSideLen() * 0.5f);
  } else {
    borderCenter = waypoint.to->GetCenter() - (borderDir * waypoint.to->GetSideLen()  * 0.5f);
  }
  return borderCenter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMemoryMapTypes::BorderSegment NavMeshQuadTreeProcessor::MakeBorderSegment(const Point3f& origin, const Point3f& dest,
  const NavMemoryMapTypes::BorderSegment::DataType& data,
  EDirection firstEDirection,
  EDirection lastEDirection)
{
  // Border.from   = origin
  // Border.to     = dest
  // Border.normal = perpendicular to (dest-origin) in same side as neighborDir (first or last, both should be in the same)
  const Vec3f& normalFromEDirection = EDirectionToNormalVec3f(firstEDirection);

  Vec3f perpendicular{};
  
  // create a perpendicular to the border line, and check if it's the one in the same side as the neighbor direction
  Vec3f borderLine = dest - origin;
  const float length = borderLine.MakeUnitLength();
  if ( NEAR_ZERO(length) )
  {
    // origin and dest fall in the same place, use average of the neighbor directions
    const Vec3f& normalFromEDirectionLast = EDirectionToNormalVec3f(lastEDirection);
    perpendicular = (normalFromEDirection + normalFromEDirectionLast) * 0.5f;
  }
  else
  {
    perpendicular = {-borderLine.y(), borderLine.x(), borderLine.z()};
    const float dotPerpendicularAndNeighbor = DotProduct(perpendicular, normalFromEDirection);
    if ( dotPerpendicularAndNeighbor <= 0.0f )
    {
      // the dot product between the perpendicular we chose randomly and the expected direction from
      // the neighbor direction is <0. This means it's the wrong perpendicular, so we just correct
      // the sign of x,y (2d)
      perpendicular.x() = -perpendicular.x();
      perpendicular.y() = -perpendicular.y();
    }
  }

  NavMemoryMapTypes::BorderSegment ret{origin, dest, perpendicular, data};
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::IsInENodeContentTypePackedType(ENodeContentType contentType, ENodeContentTypePackedType contentPackedTypes)
{
  const ENodeContentTypePackedType packedType = ENodeContentTypeToFlag(contentType);
  const bool isIn = (packedType & contentPackedTypes) != 0;
  return isIn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::InvalidateBorders()
{
  // set all borders as dirty
  for ( auto& comboIt : _bordersPerContentCombination )
  {
    comboIt.second.dirty = true;
    
    // if we don't clear the waypoints, the processor may be tempted to use them (for example to render them),
    // but the pointers are not valid, so it crashes or renders wrong information. Make sure we don't use dirty data
    comboIt.second.waypoints.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::AddBorderWaypoint(const NavMeshQuadTreeNode* from, const NavMeshQuadTreeNode* to, EDirection dir)
{
  DEV_ASSERT(nullptr != _currentBorderCombination, "NavMeshQuadTreeProcessor.AddBorderWaypoint.InvalidBorderCombination");
  _currentBorderCombination->waypoints.emplace_back( from, to, dir, false );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::FinishBorder()
{
  DEV_ASSERT(nullptr != _currentBorderCombination, "NavMeshQuadTreeProcessor.FinishBorder.InvalidBorderCombination");
  if ( !_currentBorderCombination->waypoints.empty() ) {
    _currentBorderCombination->waypoints.back().isEnd = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// FindBordersHelpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace FindBordersHelpers {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CheckedDirectionInfo
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// keeps track of visisted neighbors for a node and direction
struct CheckedDirectionInfo
{
  // constructor
  CheckedDirectionInfo();
  // allocate flags if not initialized yet (otherwise ignored)
  void Init(size_t neighborCount);
  // check the given neighbor (calling more than once is allowed)
  void MarkChecked(size_t neighborIdx);
  // return whether the given neighbor is checked
  bool IsChecked(size_t neighborIdx) const;
  // return whether this direction's neighbors are all checked. False if not initialized yet
  bool IsComplete() const;

  // attributes
  private:
    std::vector<bool> neighborVisited;
    size_t neighborsLeft;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckedDirectionInfo::CheckedDirectionInfo()
: neighborsLeft(std::numeric_limits< decltype(neighborsLeft) >::max())
{
  // do not initialize to 0 because we use that to know whether it's complete.
  // directions with 0 neighbors are allowed
  DEV_ASSERT(neighborsLeft > 0, "CheckedDirectionInfo.Constructor.InvalidNeighborsLeft" );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckedDirectionInfo::Init(size_t neighborCount)
{
  if ( neighborVisited.empty() ) {
    neighborVisited.resize(neighborCount, false);
    neighborsLeft = neighborCount;
  } else {
    DEV_ASSERT(neighborCount == neighborVisited.size(), "CheckedDirectionInfo.Init.InvalidNeighborCount");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckedDirectionInfo::MarkChecked(size_t neighborIdx)
{
  DEV_ASSERT(neighborIdx < neighborVisited.size(), "CheckedDirectionInfo.MarkChecked.InvalidNeighbor");
  if ( !neighborVisited[neighborIdx] )
  {
    neighborVisited[neighborIdx] = true;
    --neighborsLeft;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckedDirectionInfo::IsChecked(size_t neighborIdx) const
{
  if ( !neighborVisited.empty() ) {
    DEV_ASSERT(neighborIdx < neighborVisited.size(), "CheckedDirectionInfo.IsChecked.InvalidNeighbor");
    return neighborVisited[neighborIdx];
  } else {
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckedDirectionInfo::IsComplete() const
{
  const bool ret = (neighborsLeft == 0);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// CheckedInfo
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// this struct allows to iterate nodes only once and also verify proper node completion
struct CheckedInfo
{
  CheckedInfo() {}
  
  CheckedDirectionInfo& GetDirInfo(const EDirection dir )
  {
    DEV_ASSERT((size_t)dir < 4, "CheckedInfo.GetDirInfo.InvalidDirection");
    return directions[(size_t)dir];
  }
  
  const CheckedDirectionInfo& GetDirInfo(const EDirection dir ) const
  {
    DEV_ASSERT((size_t)dir < 4, "CheckedInfo.constGetDirInfo.InvalidDirection");
    return directions[(size_t)dir];
  }
  
  // allocate flags if not initialized yet (otherwise ignored)
  void InitDirection(const EDirection dir, size_t neighborCount) { GetDirInfo(dir).Init(neighborCount); }
  // check the given neighbor (calling more than once is allowed)
  void MarkChecked(const EDirection dir, size_t neighborIndex) { GetDirInfo(dir).MarkChecked(neighborIndex); }
  // return whether the given neighbor is checked
  bool IsChecked(const EDirection dir, size_t neighborIndex) const { return GetDirInfo(dir).IsChecked(neighborIndex); }
  // return whether the direction's neighbors are all checked. False if not initialized yet
  bool IsDirectionComplete(EDirection dir) const { return GetDirInfo(dir).IsComplete(); }
  // returns whether all directions are complete
  bool AreAllDirectionsComplete() const;
  
  // attributes
  private:
    std::array<CheckedDirectionInfo, 4> directions;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CheckedInfo::AreAllDirectionsComplete() const
{
  const bool ret = IsDirectionComplete(EDirection::North) &&
                   IsDirectionComplete(EDirection::East ) &&
                   IsDirectionComplete(EDirection::South) &&
                   IsDirectionComplete(EDirection::West );
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// End namespace: FindBordersHelpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::FindBorders(ENodeContentType innerType, ENodeContentTypePackedType outerTypes)
{
  ANKI_CPU_PROFILE("NavMeshQuadTreeProcessor.FindBorders");
  using namespace FindBordersHelpers;
  
  DEBUG_FIND_BORDER("------------------------------------------------------");
  DEBUG_FIND_BORDER("Starting FindBorders...");
  
  const BorderKeyType borderComboKey = GetBorderTypeKey(innerType, outerTypes);
  _currentBorderCombination = &_bordersPerContentCombination[borderComboKey];

  DEV_ASSERT(IsCached(innerType), "NavMeshQuadTreeProcessor.FindBorders.InvalidType");
  
  const NodeSet& innerSet = _nodeSets[innerType];

  _currentBorderCombination->waypoints.clear();
  _borderGfxDirty = true;
  
  // map with what is visited for every inner node
  std::unordered_map<const NavMeshQuadTreeNode*, CheckedInfo> checkedNodes;

  // hope this doesn't grow too quickly
  DEV_ASSERT(_root->GetLevel() < 32, "NavMeshQuadTreeProcessor.FindBorders.InvalidLevel");
  
  // reserve space for this node's neighbors
  NavMeshQuadTreeNode::NodeCPtrVector neighbors;
  neighbors.reserve( 1 << _root->GetLevel() );
  
  const EClockDirection clockDir = EClockDirection::CW;
    
  // for every node, try to see if they are an innerContent with at least one outerContent neighbor
  for( const auto& candidateSeed : innerSet )
  {
    DEBUG_FIND_BORDER("[%p] Checking node for seeds", candidateSeed);
    
    // check if this node is complete
    CheckedInfo& candidateCheckedInfo = checkedNodes[candidateSeed];
    while ( !candidateCheckedInfo.AreAllDirectionsComplete() )
    {
      DEBUG_FIND_BORDER("[%p] Candidate not complete, looking for outer seed", candidateSeed);

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // 1) find out if this node can be a seed (has outerType as neighbor)
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      /*
        NOTE: Because we start at [North, 0] by default, there is a chance that the borders to the left of that
        (eg: west) are actually disconnected from the right side. It should not be an issue, but it's fixable by
        iterating first to the left if seed[N,0] is an outerType, and then iterating to the right, adding the left
        border backwards to the front of the right side. I would rather keep the algorithm more simple, since having
        separate border sides should not be a problem, it will just use 2 lines instead of 1 to define a border.
      */

      // find direction to start
      EDirection candidateDir = EDirection::North;
      size_t candidateOuterIdx = 0;
      
      // find outer
      bool foundOuter = false;
      do
      {
        // check if current direction is complete
        if ( !candidateCheckedInfo.IsDirectionComplete(candidateDir) )
        {
          // grab neighbors of current direction
          neighbors.clear();
          candidateSeed->AddSmallestNeighbors(candidateDir, clockDir, neighbors);
          candidateCheckedInfo.InitDirection(candidateDir, neighbors.size());
      
          // look for an outer neighbor within this direction
          const size_t neighborCount = neighbors.size();
          while ( candidateOuterIdx < neighborCount )
          {
            if ( !candidateCheckedInfo.IsChecked(candidateDir, candidateOuterIdx) )
            {
              // flag as checked (we are about to check for outer)
              candidateCheckedInfo.MarkChecked(candidateDir, candidateOuterIdx);
              
              // if it's an outer, we have found a seed
              const bool isOuter = IsInENodeContentTypePackedType( neighbors[candidateOuterIdx]->GetContentType(), outerTypes);
              if ( isOuter ) {
                foundOuter = true;
                break;
              }
            }
            
            ++candidateOuterIdx;
          }
          
          DEV_ASSERT(foundOuter || candidateCheckedInfo.IsDirectionComplete(candidateDir),
                     "NavMeshQuadTreeProcessor.FindBorders.DirectionNotComplete");
        }

        // if we didn't find an outer, try next direction
        if ( !foundOuter ) {
          candidateDir = GetNextDirection( candidateDir, clockDir );
          candidateOuterIdx = 0;
        }
      
      } while ( !candidateCheckedInfo.AreAllDirectionsComplete() && !foundOuter );

      // if we found outer, we will use this seed to detect a border
      if ( foundOuter )
      {
        DEBUG_FIND_BORDER("[%p] Found outer for seed %zu in dir %s", candidateSeed, candidateOuterIdx, EDirectionToString(candidateDir));
      
        // add first waypoint
        AddBorderWaypoint( candidateSeed, neighbors[candidateOuterIdx], candidateDir );
        _currentBorderCombination->waypoints.back().isSeed = true;

        // this node can be seed of the group, prepare the variables for iteration:
        // neighbors = already set
        EDirection curDir = candidateDir;
        size_t nextNeighborIdx = candidateOuterIdx + 1;
        const NavMeshQuadTreeNode* curInnerNode = candidateSeed;
        
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // 2) iterate inner group from seed
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        // iterate every move direction
        bool continueSeeding = true;
        do
        {
          // iterate all neighbors in this direction
          while( nextNeighborIdx < neighbors.size() && continueSeeding )
          {
            CheckedInfo& curNodeCheckedInfo = checkedNodes[curInnerNode];
            
            {
              // next node we are checking
              const NavMeshQuadTreeNode* nextNode = neighbors[nextNeighborIdx];
              
              // check content of neighbor
              // - - - - - - - - - - - -
              // InnerType
              // - - - - - - - - - - - -
              if ( nextNode->GetContentType() == innerType )
              {
                DEBUG_FIND_BORDER("[%p] It's 'InnerType', iterating through it", nextNode);
                
                // the new node will have to start in the opposite direction we come from, for example:
                // if we move towards East, this node will start exploring its West
                EDirection fromDir = GetOppositeDirection(curDir);
                
                // we should start by the neighborIdx we come from, so get the neighbors we are about to process
                // and find where we come from
                // find all neighbors
                neighbors.clear();
                nextNode->AddSmallestNeighbors(fromDir, clockDir, neighbors);
              
                // find the neighbor we came from
                size_t fromNeighborIdx = neighbors.size();
                for ( size_t idx=0; idx<neighbors.size(); ++idx) {
                  if ( neighbors[idx] == curInnerNode ) {
                    fromNeighborIdx = idx; // we pretend to point at the same node we come from
                    break;
                  }
                }
                
                // should always find it
                DEV_ASSERT(fromNeighborIdx < neighbors.size(), "NavMeshQuadTreeProcessor.FindBorders.InvalidNeighbor");
                
                // print visit debug
                DEBUG_FIND_BORDER(">> [%p] Visiting %zu towards %s ", curInnerNode, nextNeighborIdx, EDirectionToString(curDir));
                DEBUG_FIND_BORDER(".. [%p] Comes from %zu from %s ", nextNode, fromNeighborIdx, EDirectionToString(fromDir));

                // init direction in new node if needed
                // it is ok if that direction is complete, we could have traveled in that direction and now it comes
                // back, either for a new border or the same one
                CheckedInfo& nextCheckedInfo = checkedNodes[nextNode];
                nextCheckedInfo.InitDirection(fromDir, neighbors.size());
                
                // Flag where we come from, cause we know that it's not an outer
                nextCheckedInfo.MarkChecked(fromDir, fromNeighborIdx);

                // jump to the new node
                curDir = fromDir;
                curInnerNode = nextNode;
                nextNeighborIdx = fromNeighborIdx+1;
              }
              // - - - - - - - - - - - -
              // OuterType
              // - - - - - - - - - - - -
              else if ( IsInENodeContentTypePackedType( nextNode->GetContentType(), outerTypes) )
              {
                // if we have already visisted this node, we have looped back to a previous border or the seed
                if ( checkedNodes[curInnerNode].IsChecked(curDir, nextNeighborIdx) )
                {
                  DEBUG_FIND_BORDER("[%p] (%s,%zu) 'OuterType' looped iterating neighbors. Finishing border and seed",
                  curInnerNode, EDirectionToString(curDir), nextNeighborIdx);
                  
                  // also add the point. Even though it belongs now to two borders, it will create the proper normals
                  // add it only if it would add to a running segment. If it would create a segment on its own, then
                  // we can discard it, since it's already in the other one
                  if ( !_currentBorderCombination->waypoints.empty() && !_currentBorderCombination->waypoints.back().isEnd )
                  {
                    AddBorderWaypoint( curInnerNode, nextNode, curDir );
                  }
              
                  // in any case, a border ends here
                  FinishBorder();

                  // we are no longer seeding, we have to go back to find a seeding point
                  continueSeeding = false;
                }
                else
                {
                  DEBUG_FIND_BORDER("[%p] It's 'OuterType'. Adding border", nextNode);
              
                  // it's an outer node, add as border
                  AddBorderWaypoint( curInnerNode, nextNode, curDir );
                }

                // flag checked
                curNodeCheckedInfo.MarkChecked(curDir, nextNeighborIdx);
                
                // go to next neighbor
                ++nextNeighborIdx;
              }
              // - - - - - - - - - - - - - - - -
              // Neither InnerType nor OuterType
              // - - - - - - - - - - - - - - - -
              else
              {
                DEBUG_FIND_BORDER("[%p] It's neither 'InnerType' nor 'OuterType'. Border finished here", nextNode);

                FinishBorder();

                // flag checked
                curNodeCheckedInfo.MarkChecked(curDir, nextNeighborIdx);
                
                // go to next neighbor
                ++nextNeighborIdx;
              }
              
            } // if-else
            
          } // while iterating neighbors in same direction
          
          // we are no longer iterating neighbors, check if we want to continue next direction
          if ( continueSeeding )
          {
            // go to next direction
            curDir = GetNextDirection(curDir, clockDir);
            nextNeighborIdx = 0;

            // we still want to iterate, go to next direction
            DEBUG_FIND_BORDER("[%p] Moving to new direction %s", curInnerNode, EDirectionToString(curDir) );
            
            // need to grab this again in case we jumped to next node
            CheckedInfo& curNodeCheckedInfo = checkedNodes[curInnerNode];
          
            // grab neighbors and init checkedInfo
            neighbors.clear();
            curInnerNode->AddSmallestNeighbors(curDir, clockDir, neighbors);
            curNodeCheckedInfo.InitDirection(curDir, neighbors.size());
          }
          
        } while ( continueSeeding );
      } // found outer
      else
      {
        DEBUG_FIND_BORDER("[%p] No more seeds found", candidateSeed);
      
        DEV_ASSERT(checkedNodes[candidateSeed].AreAllDirectionsComplete(),
                   "NavMeshQuadTreeProcessor.FindBorders.DirectionsNotComplete");
      }
      
    } // while !complete
    
  } // for every candidate seed
  
  // we should have visited all inner nodes, and only those
  DEV_ASSERT(checkedNodes.size() == innerSet.size(), "NavMeshQuadTreeProcessor.FindBorders.InvalidSize");
  
  DEBUG_FIND_BORDER("FINISHED FindBorders!");
  _currentBorderCombination->dirty = false;
  _currentBorderCombination = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::HasBorderSeed(ENodeContentType innerType, ENodeContentTypePackedType outerTypes) const
{
  DEBUG_FIND_BORDER("------------------------------------------------------");
  DEBUG_FIND_BORDER("Starting HasBorderSeed...");

  DEV_ASSERT_MSG(IsCached(innerType), "NavMeshQuadTreeProcessor.HasBorderSeed", "%s is not cached",
                 ENodeContentTypeToString(innerType));
  
  const auto innerSetMatchIt = _nodeSets.find(innerType);
  if ( innerSetMatchIt == _nodeSets.end() ) {
    // we don't have any nodes of innerType, there can't be any seeds
    return false;
  }
  
  DEV_ASSERT(_root->GetLevel() < 32, "NavMeshQuadTreeProcessor.HasBorderSeed.InvalidRootLevel");
  
  // reserve space for any node's neighbors
  NavMeshQuadTreeNode::NodeCPtrVector neighbors;
  neighbors.reserve( 1 << _root->GetLevel() ); // hope this doesn't grow too quickly
  
  // for every node, try to see if they are an innerContent with at least one outerContent neighbor
  const NodeSet& innerSet = innerSetMatchIt->second;
  for( const auto& candidateSeed : innerSet )
  {
    DEBUG_FIND_BORDER("[%p] Checking node for seeds", candidateSeed);
    
    // start from a given neighbor
    EDirection candidateDir = EDirection::North;
    const EClockDirection clockDir = EClockDirection::CW;
    
    do
    {
      DEBUG_FIND_BORDER("[%p] Checking direction '%s'", candidateSeed, EDirectionToString(candidateDir));

      // grab neighbors of current direction
      neighbors.clear();
      candidateSeed->AddSmallestNeighbors(candidateDir, clockDir, neighbors);

      size_t candidateOuterIdx = 0;
      
      // look for an outer neighbor within this direction
      const size_t neighborCount = neighbors.size();
      while ( candidateOuterIdx < neighborCount )
      {
          // if it's an outer, we have found a seed
          const bool isOuter = IsInENodeContentTypePackedType( neighbors[candidateOuterIdx]->GetContentType(), outerTypes);
          if (  isOuter ) {
            break;
          }
        
        ++candidateOuterIdx;
      }
      
      const bool foundOuter = (candidateOuterIdx < neighborCount);
      if ( foundOuter ) {
        DEBUG_FIND_BORDER("[%p] Found outer in '%s/%zu'", candidateSeed, EDirectionToString(candidateDir), candidateOuterIdx);
        return true;
      }
      
      candidateDir = GetNextDirection( candidateDir, clockDir );
      
    } while ( candidateDir != EDirection::North ); // until iterated all directions
    
  } // for every seed
  
  DEBUG_FIND_BORDER("Finished HasBorders without finding any seed");
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeProcessor::BorderCombination& NavMeshQuadTreeProcessor::RefreshBorderCombination(ENodeContentType innerType, ENodeContentTypePackedType outerTypes)
{
  const BorderKeyType borderComboKey = GetBorderTypeKey(innerType, outerTypes);
  BorderCombination& borderCombination = _bordersPerContentCombination[borderComboKey];
  
  // if it's dirty, recalculate now
  if ( borderCombination.dirty )
  {
    FindBorders(innerType, outerTypes);
    DEV_ASSERT(!borderCombination.dirty, "NavMeshQuadTreeProcessor.RefreshBorderCombination.DirtyAfterFindBorders");
  }
  
  return borderCombination;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::AddQuadsToDraw(ENodeContentType contentType,
  VizManager::SimpleQuadVector& quadVector, const ColorRGBA& color, float zOffset) const
{
  // find the set of quads for that content type
  auto match = _nodeSets.find(contentType);
  if ( match != _nodeSets.end() )
  {
    // iterate the set
    for ( const auto& nodePtr : match->second )
    {
      // add a quad for this ndoe
      Point3f center = nodePtr->GetCenter();
      center.z() += zOffset;
      const float sideLen = nodePtr->GetSideLen();
      quadVector.emplace_back(VizManager::MakeSimpleQuad(color, center, sideLen));
    }
  }
}


} // namespace Cozmo
} // namespace Anki
