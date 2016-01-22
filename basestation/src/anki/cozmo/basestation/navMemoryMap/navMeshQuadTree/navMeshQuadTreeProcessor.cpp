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
#include <typeinfo>
#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool , kRenderContentTypes , "NavMeshQuadTreeProcessor", false); // renders registered content nodes for webots
CONSOLE_VAR(bool , kRenderSeeds        , "NavMeshQuadTreeProcessor", false); // renders seeds differently for debugging purposes
CONSOLE_VAR(bool , kRenderBordersFrom  , "NavMeshQuadTreeProcessor", false); // renders detected borders for webots (origin)
CONSOLE_VAR(bool , kRenderBordersTo    , "NavMeshQuadTreeProcessor", true); // renders detected borders for webots (destination)
CONSOLE_VAR(float, kRenderZOffset      , "NavMeshQuadTreeProcessor", 20.0f); // adds Z offset to all quads
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
void NavMeshQuadTreeProcessor::OnNodeContentTypeChanged(const NavMeshQuadTreeNode* node, EContentType oldContent, EContentType newContent)
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
void NavMeshQuadTreeProcessor::OnNodeDestroyed(const NavMeshQuadTreeNode* node)
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
      if ( kRenderBordersTo )
      {
        Anki::ColorRGBA toColor = GetDebugColor( it.to->GetContentType() );
        if ( kRenderSeeds && it.isSeed ) { toColor = Anki::NamedColors::YELLOW; }
        toColor.SetAlpha(0.6f);
        const float toSideLen = it.to->GetSideLen();
        Point3f to3D = it.to->GetCenter();
        to3D.z() += (kRenderZOffset + .1f);
        if ( kRenderSeeds && it.isSeed) { to3D.z() += 1.0f; };
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
    case EContentType::Clear:                { ret = ColorRGBA(0.2f, 0.6f, 0.2f, 0.3f); break; };
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
  CORETECH_ASSERT(neighborsLeft > 0 );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckedDirectionInfo::Init(size_t neighborCount)
{
  if ( neighborVisited.empty() ) {
    neighborVisited.resize(neighborCount, false);
    neighborsLeft = neighborCount;
  } else {
    CORETECH_ASSERT(neighborCount == neighborVisited.size());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckedDirectionInfo::MarkChecked(size_t neighborIdx)
{
  CORETECH_ASSERT(neighborIdx < neighborVisited.size());
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
    CORETECH_ASSERT(neighborIdx < neighborVisited.size());
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
  
  CheckedDirectionInfo& GetDirInfo(const EDirection dir ) { CORETECH_ASSERT((size_t)dir < 4); return directions[(size_t)dir]; }
  const CheckedDirectionInfo& GetDirInfo(const EDirection dir ) const { CORETECH_ASSERT((size_t)dir < 4); return directions[(size_t)dir]; }
  
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
void NavMeshQuadTreeProcessor::FindBorders(EContentType innerType, EContentType outerType)
{
  using namespace FindBordersHelpers;
  
  DEBUG_FIND_BORDER("------------------------------------------------------");
  DEBUG_FIND_BORDER("Starting FindBorders...");

  CORETECH_ASSERT(IsCached(innerType));
  const NodeSet& innerSet = _nodeSets[innerType];

  _borders.clear();
  _borderGfxDirty = true;
  
  // map with what is visited for every inner node
  std::unordered_map<const NavMeshQuadTreeNode*, CheckedInfo> checkedNodes;

  // reserve space for this node's neighbors
  NavMeshQuadTreeNode::NodeCPtrVector neighbors;
  CORETECH_ASSERT( _root->GetLevel() < 32 );
  neighbors.reserve( 1 << _root->GetLevel() ); // hope this doesn't grow too quickly
  
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
              if ( neighbors[candidateOuterIdx]->GetContentType() == outerType ) {
                foundOuter = true;
                break;
              }
            }
            
            ++candidateOuterIdx;
          }
          
          CORETECH_ASSERT(foundOuter || candidateCheckedInfo.IsDirectionComplete(candidateDir));
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
        AddBorderWaypoint( candidateSeed, neighbors[candidateOuterIdx] );
        _borders.back().isSeed = true;

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
                CORETECH_ASSERT(fromNeighborIdx < neighbors.size()); // should always find it
                
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
              else if ( nextNode->GetContentType() == outerType )
              {
                // if we have already visisted this node, we have looped back to a previous border or the seed
                if ( checkedNodes[curInnerNode].IsChecked(curDir, nextNeighborIdx) )
                {
                  DEBUG_FIND_BORDER("[%p] (%s,%zu) 'OuterType' looped iterating neighbors. Finishing border and seed",
                  curInnerNode, EDirectionToString(curDir), nextNeighborIdx);
                  
                  // we could add also this node as a waypoint, but then it belongs to two borders; consider.
                  // AddBorderWaypoint( curInnerNode, nextNode );
              
                  // in any case, a border ends here
                  FinishBorder();

                  // we are no longer seeding, we have to go back to find a seeding point
                  continueSeeding = false;
                }
                else
                {
                  DEBUG_FIND_BORDER("[%p] It's 'OuterType'. Adding border", nextNode);
              
                  // it's an outer node, add as border
                  AddBorderWaypoint( curInnerNode, nextNode );
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
            // we still want to iterate, go to next direction
            DEBUG_FIND_BORDER("[%p] Moving to new direction %s", curInnerNode, EDirectionToString(curDir) );

            // go to next direction
            curDir = GetNextDirection(curDir, clockDir);
            nextNeighborIdx = 0;
          
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
      
        CORETECH_ASSERT(checkedNodes[candidateSeed].AreAllDirectionsComplete());
      }
      
    } // while !complete
    
  } // for every candidate seed
  
  // we should have visited all inner nodes, and only those
  CORETECH_ASSERT(checkedNodes.size() == innerSet.size());
  
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
