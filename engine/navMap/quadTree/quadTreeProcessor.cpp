/**
 * File: quadTreeProcessor.cpp
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Class for processing a quadTree. It relies on the quadTree and quadTreeNodes to
 * share the proper information for the Processor.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "quadTreeProcessor.h"
#include "quadTree.h"

#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/vision/engine/profiler.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#include <limits>
#include <memory>
#include <typeinfo>
#include <type_traits>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool , kRenderSeeds        , "QuadTreeProcessor", false); // renders seeds differently for debugging purposes
CONSOLE_VAR(bool , kRenderBordersFrom  , "QuadTreeProcessor", false); // renders detected borders (origin quad)
CONSOLE_VAR(bool , kRenderBordersToDot , "QuadTreeProcessor", false); // renders detected borders (border center) as dots
CONSOLE_VAR(bool , kRenderBordersToQuad, "QuadTreeProcessor", false); // renders detected borders (destination quad)
CONSOLE_VAR(bool , kRenderBorder3DLines, "QuadTreeProcessor", false); // renders borders returned as 3D lines (instead of quads)
CONSOLE_VAR(bool , kRenderBorderRegions, "QuadTreeProcessor", false); // renders borders returned as quads per region (instead of quads)
CONSOLE_VAR(float, kRenderZOffset      , "QuadTreeProcessor", 20.0f); // adds Z offset to all quads
CONSOLE_VAR(bool , kDebugFindBorders   , "QuadTreeProcessor", false); // prints debug information in console

#define DEBUG_FIND_BORDER(format, ...)                                                                          \
if ( kDebugFindBorders ) {                                                                                      \
  do{::Anki::Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, "NMQTProcessor", {}, format, ##__VA_ARGS__);}while(0); \
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeProcessor::QuadTreeProcessor()
: _currentBorderCombination(nullptr)
, _quadTree(nullptr)
, _totalExploredArea_m2(0.0)
, _totalInterestingEdgeArea_m2(0.0)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeProcessor::OnNodeContentTypeChanged(const QuadTreeNode* node, const EContentType& oldType, const bool wasEmpty)
{
  
  using namespace MemoryMapTypes;
  EContentType newType = node->GetContent().data->type;

  DEV_ASSERT(oldType != newType, "QuadTreeProcessor.OnNodeContentTypeChanged.ContentNotChanged");

  // update exploration area based on the content type
  {
    const bool needsToRemove = !wasEmpty &&  node->IsEmptyType();
    const bool needsToAdd    =  wasEmpty && !node->IsEmptyType();
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
    const bool shouldBeCountedOld = (oldType == EContentType::InterestingEdge);
    const bool shouldBeCountedNew = (newType == EContentType::InterestingEdge);
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
  if ( IsCached(oldType) )
  {
    // remove the node from that cache
    DEV_ASSERT(_nodeSets[oldType].find(node) != _nodeSets[oldType].end(),
               "QuadTreeProcessor.OnNodeContentTypeChanged.InvalidRemove");
    _nodeSets[oldType].erase( node );
  }

  if ( IsCached(newType) )
  {
    // add node to that cache
    DEV_ASSERT(_nodeSets[newType].find(node) == _nodeSets[newType].end(),
               "QuadTreeProcessor.OnNodeContentTypeChanged.InvalidInsert");
    _nodeSets[newType].insert(node);
  }
  
  // invalidate all borders. Note this is not optimal, we could invalidate only affected borders (if such
  // processing could be done easily), or at least discarding changes by parent quad
  InvalidateBorders();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeProcessor::OnNodeDestroyed(const QuadTreeNode* node)
{
  // if old content type is cached
  const EContentType oldContent = node->GetData()->type;
  if ( IsCached(oldContent) )
  {
    // remove the node from that cache
    DEV_ASSERT(_nodeSets[oldContent].find(node) != _nodeSets[oldContent].end(),
               "QuadTreeProcessor.OnNodeDestroyed.InvalidNode");
    _nodeSets[oldContent].erase(node);
  }

  // remove the area for this node if it was counted before
  {
    const bool wasOutOld = node->IsEmptyType();
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
    const bool shouldBeCountedOld = (node->GetData()->type == EContentType::InterestingEdge);
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
bool QuadTreeProcessor::HasBorders(EContentType innerType, EContentTypePackedType outerTypes) const
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
void QuadTreeProcessor::GetBorders(EContentType innerType, EContentTypePackedType outerTypes, BorderRegionVector& outBorders)
{
  // grab the border combination info
  const BorderCombination& borderCombination = RefreshBorderCombination(innerType, outerTypes);

  outBorders.clear();
  if ( !borderCombination.waypoints.empty() )
  {
    // -- convert from borderCombination info to borderVector
    
    // container to calculate area of a region by checking which nodes belong to this region
    std::unordered_set<const QuadTreeNode*> nodesInCurrentRegion;

    // epsilon to merge waypoints into the same line
    const float kDotBorderEpsilon = 0.9848f; // cos(10deg) = 0.984807...
    EDirection firstNeighborDir = borderCombination.waypoints[0].direction;
    Point3f curOrigin = CalculateBorderWaypointCenter(borderCombination.waypoints[0]);
    Point3f curDest   = curOrigin;
    auto curBorderContent = borderCombination.waypoints[0].from->GetContent();

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
        const auto& waypointContent = borderCombination.waypoints[idx].from->GetContent();
        const bool canShareData = (curBorderContent == waypointContent);
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
        MemoryMapTypes::BorderRegion& curRegion = outBorders.back();
        curRegion.segments.emplace_back( MakeBorderSegment(curOrigin, curDest, curBorderContent.data, firstNeighborDir, lastNeighborDir) );
        
        // origin <- dest
        curOrigin = curDest;
        firstNeighborDir = lastNeighborDir;
        curBorderContent = borderCombination.waypoints[idx].from->GetContent();
      }
      
      curDest = borderCenter;
      
      if ( isEnd )
      {
        // add border
        const EDirection lastNeighborDir = borderCombination.waypoints[idx].direction;
        if ( outBorders.empty() || outBorders.back().IsFinished() ) {
          outBorders.emplace_back();
        }
        MemoryMapTypes::BorderRegion& curRegion = outBorders.back();
        curRegion.segments.emplace_back( MakeBorderSegment(curOrigin, curDest, curBorderContent.data, firstNeighborDir, lastNeighborDir) );

        
        // calculate the area of the border by adding all node's area
        float totalArea_m2 = 0.0f;
        for ( const auto& nodeInBorder : nodesInCurrentRegion )
        {
          const float side_m = MM_TO_M(nodeInBorder->GetSideLen());
          const float area_m2 = side_m*side_m;
          totalArea_m2 += area_m2;
        }

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
          curBorderContent = borderCombination.waypoints[nextIdx].from->GetContent();
        }
      }
      
      ++idx;
    } while ( idx < count );
    
  } // has waypoints
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeProcessor::GetNodesToFill(const NodePredicate& innerPred, const NodePredicate& outerPred, NodeSet& output)
{
  // search direction constants
  static const EClockDirection clockDir = EClockDirection::CW;
  static const std::array<EDirection, 4> cwDirs{{ EDirection::North, EDirection::East, EDirection::South, EDirection::West }};

  // find any node of typeToFill that satisfies pred(node, neighbor)
  std::deque<const QuadTreeNode*> unexpandedNodes;
  FoldFunctorConst fillTypeFilter = [&] (const QuadTreeNode& node) {
    // first check if node is typeToFill
    if ( innerPred( node.GetData() ) ) {
      QuadTreeNode::NodeCPtrVector neighbors;
      neighbors.reserve( 1 << node.GetLevel() );

      // check if this nodes has a neighbor of any typesToFillFrom
      for(const EDirection candidateDir : cwDirs) {
        neighbors.clear(); // AddSmallestNeighbors does not clear the output list itself
        node.AddSmallestNeighbors(candidateDir, clockDir, neighbors);

        for(const auto neighbor : neighbors) {
          if( outerPred( neighbor->GetData() ) ) {
            unexpandedNodes.push_back( &node );
            return;
          }
        }
      }
    }
  };
  ((const QuadTree*) _quadTree)->Fold(fillTypeFilter);

  // expand all nodes for fill
  while(!unexpandedNodes.empty()) {
    // get the next node and add it to the output list
    const QuadTreeNode* node = unexpandedNodes.front();
    unexpandedNodes.pop_front();
    output.insert(node);

    // get all of this nodes neighbors of the same type
    QuadTreeNode::NodeCPtrVector neighbors;
    neighbors.reserve( 1 << node->GetLevel() );

    for(const EDirection candidateDir : cwDirs) {
      neighbors.clear();
      node->AddSmallestNeighbors(candidateDir, clockDir, neighbors);

      // for any neighbor of the same type, if it has not already been expanded, add it the unexpanded list
      for(const auto neighbor : neighbors) {
        MemoryMapDataConstPtr neighborData = neighbor->GetData();
        if ( innerPred( neighbor->GetData() ) && (output.find(neighbor) == output.end()) ) {
          unexpandedNodes.push_back( neighbor );
        }
      } // done adding neighbors for this side
    } // finished all sides
  } // all nodes expanded
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeProcessor::FillBorder(EContentType filledType, EContentTypePackedType fillingTypeFlags, const MemoryMapDataPtr& data)
{
  DEV_ASSERT_MSG(IsCached(filledType),
                 "QuadTreeProcessor.FillBorder.FilledTypeNotCached",
                 "%s is not cached, which is needed for fast processing operations",
                 EContentTypeToString(filledType));
  NodePredicate innerCheckFunc = [&filledType](MemoryMapDataConstPtr inside) {
    return (filledType == inside->type);
  };
  NodePredicate outerCheckFunc = [&fillingTypeFlags](MemoryMapDataConstPtr outside) {
    return IsInEContentTypePackedType(outside->type, fillingTypeFlags);
  };
  return FillBorder( innerCheckFunc, outerCheckFunc, data );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeProcessor::FillBorder(const NodePredicate& innerPred, const NodePredicate& outerPred, const MemoryMapDataPtr& data)
{
  ANKI_CPU_PROFILE("QuadTreeProcessor.FillBorder");

  // should this timer be a member variable? It's normally desired to time all processors
  // together, but beware when merging stats from different maps (always current is the only one processing)
  static Vision::Profiler timer;
  // timer.SetPrintChannelName("Unfiltered");
  timer.SetPrintFrequency(5000);
  timer.Tic("QuadTreeProcessor.FillBorder");

  // calculate nodes being flooded directly. Note that we are not going to cause filled nodes to flood forward
  // into others. A second call to FillBorder would be required for that (consider for local fills when we have them,
  // since they'll be significally faster).
  
  // The reason why we cache points instead of nodes is because adding a point can cause change and destruction of nodes,
  // for example through automerges in parents whose children all become the new content. To prevent having to
  // update an iterator from this::OnNodeX events, cache centers and apply. The result algorithm should be
  // slightly slower, but much simpler to understand, debug and profile
  std::vector<Point2f> floodedQuadCenters;

  // grab nodes to fill and find their centers
  NodeSet nodesToFill;
  GetNodesToFill(innerPred, outerPred, nodesToFill);
  for( const auto& node : nodesToFill ) {
    floodedQuadCenters.emplace_back( node->GetCenter() );
  }
  
  // add flooded centers to the tree (note this does not cause flood filling)
  bool changed = false;
  MemoryMapDataPtr dataPtr = data->Clone();
  for( const auto& center : floodedQuadCenters ) {
    changed |= _quadTree->Insert(Poly2f({center}), [&dataPtr] (auto _) { return dataPtr; });
  }
  
  timer.Toc("QuadTreeProcessor.FillBorder");
  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeProcessor::HasContentType(EContentType nodeType) const
{
  DEV_ASSERT_MSG(IsCached(nodeType), "QuadTreeProcessor.HasContentType", "%s is not cached",
                 EContentTypeToString(nodeType));
  
  // check if any nodes for that type are cached currently
  auto nodeSetMatch = _nodeSets.find(nodeType);
  const bool hasAny = (nodeSetMatch != _nodeSets.end()) && !nodeSetMatch->second.empty();
  return hasAny;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeProcessor::IsCached(EContentType contentType)
{
  switch( contentType ) {
    case EContentType::ObstacleObservable:
    case EContentType::ObstacleProx:
    case EContentType::ObstacleUnrecognized:
    case EContentType::ObstacleCharger:
    case EContentType::InterestingEdge:
    case EContentType::NotInterestingEdge:
    case EContentType::Cliff:
    {
      return true;
    }
    case EContentType::Unknown:
    case EContentType::ClearOfObstacle:
    case EContentType::ClearOfCliff:
    case EContentType::ObstacleChargerRemoved:
    case EContentType::_Count:
    {
      return false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeProcessor::BorderKeyType QuadTreeProcessor::GetBorderTypeKey(EContentType innerType, EContentTypePackedType outerTypes)
{
  using UnderlyingContentType = std::underlying_type<EContentType>::type;

  static_assert( (sizeof(EContentTypePackedType)+sizeof(UnderlyingContentType)) <= (sizeof(BorderKeyType)),
  "BorderKeyType should hold 2 EContentTypePackedType" );
  
  // key = innerTypes << X | outerType, where X is sizeof(EContentTypePackedType)
  BorderKeyType key = 0;
  key |= static_cast<UnderlyingContentType>(innerType);
  key <<= (8*(sizeof(EContentTypePackedType)));
  key |= (outerTypes);

  return key;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vec3f QuadTreeProcessor::CalculateBorderWaypointCenter(const BorderWaypoint& waypoint)
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
BorderSegment QuadTreeProcessor::MakeBorderSegment(const Point3f& origin, const Point3f& dest,
  const BorderSegment::DataType& data,
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

  MemoryMapTypes::BorderSegment ret{origin, dest, perpendicular, data};
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeProcessor::InvalidateBorders()
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
void QuadTreeProcessor::AddBorderWaypoint(const QuadTreeNode* from, const QuadTreeNode* to, EDirection dir)
{
  DEV_ASSERT(nullptr != _currentBorderCombination, "QuadTreeProcessor.AddBorderWaypoint.InvalidBorderCombination");
  _currentBorderCombination->waypoints.emplace_back( from, to, dir, false );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTreeProcessor::FinishBorder()
{
  DEV_ASSERT(nullptr != _currentBorderCombination, "QuadTreeProcessor.FinishBorder.InvalidBorderCombination");
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
void QuadTreeProcessor::FindBorders(EContentType innerType, EContentTypePackedType outerTypes)
{
  ANKI_CPU_PROFILE("QuadTreeProcessor.FindBorders");
  using namespace FindBordersHelpers;
  
  DEBUG_FIND_BORDER("------------------------------------------------------");
  DEBUG_FIND_BORDER("Starting FindBorders...");
  
  const BorderKeyType borderComboKey = GetBorderTypeKey(innerType, outerTypes);
  _currentBorderCombination = &_bordersPerContentCombination[borderComboKey];

  DEV_ASSERT(IsCached(innerType), "QuadTreeProcessor.FindBorders.InvalidType");
  DEV_ASSERT(!IsInEContentTypePackedType(innerType, outerTypes), "QuadTreeProcessor.FindBorders.InnerTypeAlsoInOuter");
  
  const NodeSet& innerSet = _nodeSets[innerType];

  _currentBorderCombination->waypoints.clear();
  
  // map with what is visited for every inner node
  std::unordered_map<const QuadTreeNode*, CheckedInfo> checkedNodes;

  // hope this doesn't grow too quickly
  DEV_ASSERT(_quadTree->GetLevel() < 32, "QuadTreeProcessor.FindBorders.InvalidLevel");
  
  // reserve space for this node's neighbors
  QuadTreeNode::NodeCPtrVector neighbors;
  neighbors.reserve( 1 << _quadTree->GetLevel() );
  
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
              const bool isOuter = IsInEContentTypePackedType( neighbors[candidateOuterIdx]->GetData()->type, outerTypes);
              if ( isOuter ) {
                foundOuter = true;
                break;
              }
            }
            
            ++candidateOuterIdx;
          }
          
          DEV_ASSERT(foundOuter || candidateCheckedInfo.IsDirectionComplete(candidateDir),
                     "QuadTreeProcessor.FindBorders.DirectionNotComplete");
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
        const QuadTreeNode* curInnerNode = candidateSeed;
        
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
              const QuadTreeNode* nextNode = neighbors[nextNeighborIdx];
              
              // check content of neighbor
              // - - - - - - - - - - - -
              // InnerType
              // - - - - - - - - - - - -
              if ( nextNode->GetData()->type == innerType )
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
                DEV_ASSERT(fromNeighborIdx < neighbors.size(), "QuadTreeProcessor.FindBorders.InvalidNeighbor");
                
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
              else if ( IsInEContentTypePackedType( nextNode->GetData()->type, outerTypes) )
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
                   "QuadTreeProcessor.FindBorders.DirectionsNotComplete");
      }
      
    } // while !complete
    
  } // for every candidate seed
  
  // we should have visited all inner nodes, and only those
  DEV_ASSERT(checkedNodes.size() == innerSet.size(), "QuadTreeProcessor.FindBorders.InvalidSize");
  
  DEBUG_FIND_BORDER("FINISHED FindBorders!");
  _currentBorderCombination->dirty = false;
  _currentBorderCombination = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool QuadTreeProcessor::HasBorderSeed(EContentType innerType, EContentTypePackedType outerTypes) const
{
  DEBUG_FIND_BORDER("------------------------------------------------------");
  DEBUG_FIND_BORDER("Starting HasBorderSeed...");

  DEV_ASSERT_MSG(IsCached(innerType), "QuadTreeProcessor.HasBorderSeed", "%s is not cached",
                 EContentTypeToString(innerType));
  
  const auto innerSetMatchIt = _nodeSets.find(innerType);
  if ( innerSetMatchIt == _nodeSets.end() ) {
    // we don't have any nodes of innerType, there can't be any seeds
    return false;
  }
  
  DEV_ASSERT(_quadTree->GetLevel() < 32, "QuadTreeProcessor.HasBorderSeed.InvalidRootLevel");
  
  // reserve space for any node's neighbors
  QuadTreeNode::NodeCPtrVector neighbors;
  neighbors.reserve( 1 << _quadTree->GetLevel() ); // hope this doesn't grow too quickly
  
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
          const bool isOuter = IsInEContentTypePackedType( neighbors[candidateOuterIdx]->GetData()->type, outerTypes);
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
QuadTreeProcessor::BorderCombination& QuadTreeProcessor::RefreshBorderCombination(EContentType innerType, EContentTypePackedType outerTypes)
{
  const BorderKeyType borderComboKey = GetBorderTypeKey(innerType, outerTypes);
  BorderCombination& borderCombination = _bordersPerContentCombination[borderComboKey];
  
  // if it's dirty, recalculate now
  if ( borderCombination.dirty )
  {
    FindBorders(innerType, outerTypes);
    DEV_ASSERT(!borderCombination.dirty, "QuadTreeProcessor.RefreshBorderCombination.DirtyAfterFindBorders");
  }
  
  return borderCombination;
}

} // namespace Cozmo
} // namespace Anki
