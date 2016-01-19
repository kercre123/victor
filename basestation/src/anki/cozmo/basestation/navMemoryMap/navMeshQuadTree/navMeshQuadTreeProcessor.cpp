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

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/constantsAndMacros.h"

#include <limits>
#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool , kRenderContentTypes , "NavMeshQuadTreeProcessor", false); // renders registered content nodes for webots
CONSOLE_VAR(bool , kRenderBordersFrom  , "NavMeshQuadTreeProcessor", false); // renders detected borders for webots (origin)
CONSOLE_VAR(bool , kRenderBordersTo    , "NavMeshQuadTreeProcessor", true); // renders detected borders for webots (destination)
CONSOLE_VAR(float, kRenderZOffset      , "NavMeshQuadTreeProcessor", 50.0f); // adds Z offset to all quads
CONSOLE_VAR(bool , kDebugFindBorders   , "NavMeshQuadTreeProcessor", false); // prints debug information in console

#define DEBUG_FIND_BORDER(format, ...)                                                                          \
if ( kDebugFindBorders ) {                                                                                      \
  do{::Anki::Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, "NMQTProcessor", {}, format, ##__VA_ARGS__);}while(0); \
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeProcessor::NavMeshQuadTreeProcessor()
: _root(nullptr)
, _contentGfxDirty(false)
, _borderGfxDirty(false)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::OnNodeContentTypeChanged(NavMeshQuadTreeNode* node, EContentType oldContent, EContentType newContent)
{
  CORETECH_ASSERT(node->GetContentType() == newContent);

  // if old content type is cached
  if ( IsCached(oldContent) )
  {
    // remove the node from that cache
    CORETECH_ASSERT(_nodeSets[oldContent].find(node) != _nodeSets[oldContent].end());
    _nodeSets[oldContent].erase( node );
    
    // flag as dirty
    _contentGfxDirty = true;
  }

  if ( IsCached(newContent) )
  {
    // remove the node from that cache
    CORETECH_ASSERT(_nodeSets[newContent].find(node) == _nodeSets[newContent].end());
    _nodeSets[newContent].insert(node);
    
    // flag as dirty
    _contentGfxDirty = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::OnNodeDestroyed(NavMeshQuadTreeNode* node)
{
  // if old content type is cached
  const EContentType oldContent = node->GetContentType();
  if ( IsCached(oldContent) )
  {
    // remove the node from that cache
    CORETECH_ASSERT(_nodeSets[oldContent].find(node) != _nodeSets[oldContent].end());
    _nodeSets[oldContent].erase( node );
    
    // flag as dirty
    _contentGfxDirty = true;
  }
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
      EContentType type = it.first;
      AddQuadsToDraw(type, quadVector, GetDebugColor(type), kRenderZOffset);
    }
    
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTreeProcessorContent", quadVector);
    
    _contentGfxDirty = false;
  }

  // borders
  if ( _borderGfxDirty && (kRenderBordersFrom || kRenderBordersTo) )
  {

    VizManager::SimpleQuadVector quadVector;

    // add quads from borders
    for ( const auto& it : _borders )
    {
      // from quad
      if ( kRenderBordersFrom )
      {
        Anki::ColorRGBA fromColor = GetDebugColor( it.from->GetContentType() );
        fromColor.SetAlpha(0.6f);
        const float fromSideLen = it.from->GetSideLen();
        Point3f from3D = it.from->GetCenter();
        from3D.z() += (kRenderZOffset + .1f);
        quadVector.emplace_back(VizManager::MakeSimpleQuad(fromColor, from3D, fromSideLen));
      }
    
      // to quad
      if ( kRenderBordersTo )
      {
        Anki::ColorRGBA toColor = GetDebugColor( it.to->GetContentType() );
        toColor.SetAlpha(0.6f);
        const float toSideLen = it.to->GetSideLen();
        Point3f to3D = it.to->GetCenter();
        to3D.z() += (kRenderZOffset + .1f);
        quadVector.emplace_back(VizManager::MakeSimpleQuad(toColor, to3D, toSideLen));
      }
    }
  
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTreeProcessorBorders", quadVector);
    _borderGfxDirty = false;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::IsCached(EContentType contentType)
{
  const bool isCached = (contentType == EContentType::ObstacleCube        ) ||
                        (contentType == EContentType::ObstacleUnrecognized);
  return isCached;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorRGBA NavMeshQuadTreeProcessor::GetDebugColor(EContentType contentType)
{
  ColorRGBA ret = Anki::NamedColors::BLACK;
  switch (contentType) {
    case EContentType::Unknown:              { ret = ColorRGBA(0.2f, 0.2f, 0.6f, 0.3f); break; };
    case EContentType::ObstacleCube:         { ret = ColorRGBA(1.0f, 0.0f, 0.0f, 0.3f); break; };
    case EContentType::ObstacleUnrecognized: { ret = ColorRGBA(0.5f, 0.0f, 0.0f, 0.3f); break; };
    default: {
      CORETECH_ASSERT(!"supported");
    }
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::AddBorderWaypoint(const NavMeshQuadTreeNode* from, const NavMeshQuadTreeNode* to)
{
  _borders.emplace_back( from, to, false );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::FinishBorder()
{
  if ( !_borders.empty() ) {
    _borders.back().isEnd = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::FindBorders(EContentType innerType, EContentType outerType)
{
  DEBUG_FIND_BORDER("Starting FindBorders...");

  CORETECH_ASSERT(IsCached(innerType));
  NodeSet& innerSet = _nodeSets[innerType];

  _borders.clear();
  _borderGfxDirty = true;

  // this struct allows to iterate nodes only once and also verify proper node completion
  struct VisitedInfo
  {
    VisitedInfo() : isComplete(false), fromDir(EDirection::Invalid), fromIndex(0) {}
    // setters
    void VisitFrom(EDirection dir, size_t idx) { (fromDir = dir); (fromIndex = idx); }
    void SetComplete() { isComplete = true; }
    // getters
    bool       IsComplete() const { return isComplete; }
    EDirection GetFromDir() const { return fromDir;    }
    size_t     GetFromIdx() const { return fromIndex;  }
    // visit query
    bool IsVisited() const { return fromDir != EDirection::Invalid; }
    bool IsVisitedFrom(EDirection dir, size_t idx) const { return (fromDir == dir) && (fromIndex == idx); }
    // attributes
    private:
      bool       isComplete;
      EDirection fromDir;
      size_t     fromIndex;
  };
  
  // map with what is visited for every inner node
  std::map<const NavMeshQuadTreeNode*, VisitedInfo> visitedNodes;
  
  // set of nodes visited from the current seed
  std::set<const NavMeshQuadTreeNode*> nodesSeededInIteration;

  // reserve space for this node's neighbors
  NavMeshQuadTreeNode::NodeCPtrVector neighbors;
  CORETECH_ASSERT( _root->GetLevel() < 32 );
  neighbors.reserve( 1 << _root->GetLevel() ); // hope this doesn't grow too quickly
  
  const EClockDirection clockDir = EClockDirection::CW;
    
  // for every node, try to see if they are an innerContent with at least one outerContent neighbor
  for( auto& node : innerSet )
  {
    DEBUG_FIND_BORDER("[%p] Checking node for seed", node);
    
    // nodes are either not visited or fully visited at this step; check conditions
    auto visitedInfoIt = visitedNodes.find( node );
    const bool visited = ( visitedInfoIt != visitedNodes.end() );
    CORETECH_ASSERT( !visited || visitedInfoIt->second.IsComplete() );
    if ( visited ) {
      DEBUG_FIND_BORDER("[%p] Node already visited", node);
      continue;
    }

    // clear nodes visited by this seed
    nodesSeededInIteration.clear();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // 1) find out if this node can be a seed (has outerType as neighbor)
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    /*
      NOTE: Because we start at [North, 0] on every seed, there is a chance that the borders to the left of that
      (eg: west) are actually disconnected from the right side. It should not be an issue, but it's fixable by
      iterating first to the left if seed[N,0] is an outerType, and then iterating to the right, adding the left
      border backwards to the front of the right side. I would rather keep the algorithm more simple, since having
      separate border sides should not be a problem, it will just use 2 lines instead of 1 to define a border.
    */

    // pretend we have been visisted from the N,0 corner
    const EDirection seedStartDir = EDirection::North;
    visitedNodes[node].VisitFrom(seedStartDir, 0);
    EDirection seedDir = seedStartDir;
    size_t seedOuterIdx = 0;

    // find outer
    bool foundOuter = false;
    do
    {
      // grab neighbors of current direction
      neighbors.clear();
      node->AddSmallestNeighbors(seedDir, clockDir, neighbors);
    
      // look for an outer neighbor
      const size_t neighborCount = neighbors.size();
      while ( seedOuterIdx < neighborCount )
      {
        if ( neighbors[seedOuterIdx]->GetContentType() == outerType ) {
          break;
        }
        ++seedOuterIdx;
      }

      // if we didn't find an outer, try next direction
      foundOuter = seedOuterIdx < neighborCount;
      if ( !foundOuter ) {
        seedDir = GetNextDirection( seedDir, clockDir );
        seedOuterIdx = 0;
      }
    
    } while ( (seedDir != seedStartDir) && !foundOuter );

    // if we found outer, we will use this node to detect a border
    if ( foundOuter )
    {
      DEBUG_FIND_BORDER("[%p] Found outer for seed %zu in dir %s", node, seedOuterIdx, EDirectionToString(seedDir));
    
      // insert the first outer node because we will use it later as end condition
      AddBorderWaypoint( node, neighbors[seedOuterIdx] );

      // this node can be seed of the group, prepare the variables for iteration:
      // neighbors = already set
      EDirection curDir = seedDir;
      size_t nextNeighborIdx = seedOuterIdx + 1;
      const NavMeshQuadTreeNode* curInnerNode = node;
      
      nodesSeededInIteration.insert( curInnerNode );

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // 2) iterate inner group from seed
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // iterate every move direction
      do
      {
        bool foundSeed = false;
      
        // iterate all neighbors in this direction
        while( nextNeighborIdx < neighbors.size() && !foundSeed )
        {
          DEBUG_FIND_BORDER("[%p] Iterating %zu/%zu in dir %s", curInnerNode, nextNeighborIdx, neighbors.size(), EDirectionToString(curDir));

          // if the current node is the seed node, and the current index is the seedOuter index, it means
          // we looped back to the original node, in this case finish the border
          foundSeed = ((curInnerNode == node) && (curDir == seedDir) && (nextNeighborIdx == seedOuterIdx));
          if ( foundSeed )
          {
            DEBUG_FIND_BORDER("Finished loop at seed index (iterating neighbors). Finishing border");
          
            // done with this node
            VisitedInfo& curVisitedInfo = visitedNodes[curInnerNode];
            curVisitedInfo.SetComplete();
          
            FinishBorder();
          }
          else
          {
            // next node we are checking
            const NavMeshQuadTreeNode* nextNode = neighbors[nextNeighborIdx];
          
            // check content of neighbor
            // - - - - - - - - - - - -
            // INNER TYPE
            // - - - - - - - - - - - -
            if ( nextNode->GetContentType() == innerType )
            {
              DEBUG_FIND_BORDER("[%p] It's 'InnerType'", nextNode);

              // if we are leaving the way we came in the first place then we are finally complete
              VisitedInfo& curVisitedInfo = visitedNodes[curInnerNode];
              if ( curVisitedInfo.IsVisitedFrom(curDir, nextNeighborIdx) )
              {
                DEBUG_FIND_BORDER("[%p] Leaving the way we started. Node is complete", curInnerNode);
                curVisitedInfo.SetComplete();
              }
              else
              {
                DEBUG_FIND_BORDER("[%p] Leaving open, was previously visited from %zu dir %s",
                  curInnerNode, curVisitedInfo.GetFromIdx(), EDirectionToString(curVisitedInfo.GetFromDir()));
              }
            
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
                }
              }
              CORETECH_ASSERT(fromNeighborIdx < neighbors.size()); // should always find it
              
              // flag the new one about where we come from
              VisitedInfo& nextVisitedInfo = visitedNodes[nextNode];
              if ( !nextVisitedInfo.IsVisited() )
              {
                // first time visit
                nextVisitedInfo.VisitFrom(fromDir, fromNeighborIdx);
              }

              // inform we are visiting nextNode
              DEBUG_FIND_BORDER(">> [%p] Visiting %zu towards %s ", curInnerNode, nextNeighborIdx, EDirectionToString(curDir));
              DEBUG_FIND_BORDER(".. [%p] Comes from %zu from %s ", nextNode, fromNeighborIdx, EDirectionToString(fromDir));
              nodesSeededInIteration.insert( nextNode );

              // advance to the new node
              curDir = fromDir;
              curInnerNode = nextNode;
              nextNeighborIdx = fromNeighborIdx;
            }
            // - - - - - - - - - - - -
            // OUTER TYPE
            // - - - - - - - - - - - -
            else if ( nextNode->GetContentType() == outerType )
            {
              DEBUG_FIND_BORDER("[%p] It's 'OuterType'. Adding border", nextNode);
            
              // it's an outer node, add as border
              AddBorderWaypoint( curInnerNode, nextNode );
            }
            else
            {
              DEBUG_FIND_BORDER("[%p] It's neither 'InnerType' nor 'OuterType'. Border finished here", nextNode);

              FinishBorder();
            }
            
            // go to next neighbor, if we jumped from one node, this will advance the pointer to the next one
            ++nextNeighborIdx;
          }
        }
        
        if ( !foundSeed )
        {
          // go to next direction
          curDir = GetNextDirection(curDir, clockDir);
          nextNeighborIdx = 0;
          
          // check if this index is the seed start
          foundSeed = ((curInnerNode == node) && (curDir == seedDir) && (nextNeighborIdx == seedOuterIdx));
          if ( foundSeed )
          {
            DEBUG_FIND_BORDER("Finished loop at seed index (starting new direction). Finishing border");
          
            // done with this node
            VisitedInfo& curVisitedInfo = visitedNodes[curInnerNode];
            curVisitedInfo.SetComplete();
          
            FinishBorder();
          }
          else
          {
            // we did not loop back to the seed, grab the neighbors of this direction and start processing
            DEBUG_FIND_BORDER("[%p] Moving to new direction %s", curInnerNode, EDirectionToString(curDir) );
          
            neighbors.clear();
            curInnerNode->AddSmallestNeighbors(curDir, clockDir, neighbors);
          }
        }
        else
        {
          CORETECH_ASSERT(visitedNodes[curInnerNode].IsComplete());
        }
        
      } while ( !visitedNodes[curInnerNode].IsComplete() );

      DEBUG_FIND_BORDER("[%p] Finished node", curInnerNode);

      // Flag all nodes visisted this iteration as complete, since they closed a loop
      for( auto& innerNode : nodesSeededInIteration ) {
        CORETECH_ASSERT( visitedNodes[innerNode].IsVisited() );
        visitedNodes[innerNode].SetComplete();
      }
    }
    else
    {
      DEBUG_FIND_BORDER("[%p] Could not seed", node);
      
      // we iterated the whole node and did not become seed, flag as complete
      visitedNodes[node].SetComplete();
    }
  }
  
  // we should have visited all inner nodes, and only those
  CORETECH_ASSERT(visitedNodes.size() == innerSet.size());
  
  DEBUG_FIND_BORDER("FINISHED FindBorders!");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::AddQuadsToDraw(EContentType contentType,
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
