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
#include <type_traits>
#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool , kRenderContentTypes , "NavMeshQuadTreeProcessor", false); // renders registered content nodes for webots
CONSOLE_VAR(bool , kRenderSeeds        , "NavMeshQuadTreeProcessor", false); // renders seeds differently for debugging purposes
CONSOLE_VAR(bool , kRenderBordersFrom  , "NavMeshQuadTreeProcessor", false); // renders detected borders for webots (origin)
CONSOLE_VAR(bool , kRenderBordersToDot , "NavMeshQuadTreeProcessor", true); // renders detected borders for webots (destination) as dots
CONSOLE_VAR(bool , kRenderBordersToQuad, "NavMeshQuadTreeProcessor", false); // renders detected borders for webots (destination) as neighbor quads
CONSOLE_VAR(float, kRenderZOffset      , "NavMeshQuadTreeProcessor", 20.0f); // adds Z offset to all quads
CONSOLE_VAR(bool , kDebugFindBorders   , "NavMeshQuadTreeProcessor", false); // prints debug information in console

#define DEBUG_FIND_BORDER(format, ...)                                                                          \
if ( kDebugFindBorders ) {                                                                                      \
  do{::Anki::Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, "NMQTProcessor", {}, format, ##__VA_ARGS__);}while(0); \
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeProcessor::NavMeshQuadTreeProcessor()
: _currentBorderCombination(nullptr)
, _root(nullptr)
, _contentGfxDirty(false)
, _borderGfxDirty(false)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::OnNodeContentTypeChanged(const NavMeshQuadTreeNode* node, ENodeContentType oldContent, ENodeContentType newContent)
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
    CORETECH_ASSERT(_nodeSets[oldContent].find(node) != _nodeSets[oldContent].end());
    _nodeSets[oldContent].erase( node );
    
    // flag as dirty
    _contentGfxDirty = true;
  }
  
  // invalidate all borders. Note this is not optimal, we could invalidate only affected borders (if such
  // processing could be done easily), or at least discarding changes by parent quad
  InvalidateBorders();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::GetBorders(ENodeContentType innerType, ENodeContentType outerType, NavMemoryMapTypes::BorderVector& outBorders)
{
  // grab the border combination info
  const uint32_t borderComboKey = GetBorderTypeKey(innerType, outerType);
  const BorderCombination& borderCombination = _bordersPerContentCombination[borderComboKey];
  
  // if it's dirty, recalculate now
  if ( borderCombination.dirty )
  {
    FindBorders(innerType, outerType);
    CORETECH_ASSERT(!borderCombination.dirty);
  }

  outBorders.clear();
  if ( !borderCombination.waypoints.empty() )
  {
    // -- convert from borderCombination info to borderVector
  
    // epsilon to merge waypoints into the same line
    const float kDotBorderEpsilon = 0.9848f; // cos(10deg) = 0.984807...
    EDirection firstNeighborDir = borderCombination.waypoints[0].direction;
    Point3f curOrigin = CalculateBorderWaypointCenter(borderCombination.waypoints[0]);
    Point3f curDest   = curOrigin;
    
    // note: curNeighborDirection could be first, last, average, ... all should yield same result, so I'm doing first
  
    // iterate all waypoints (even 0)
    size_t idx = 0;
    const size_t count = borderCombination.waypoints.size();
    do
    {
      const bool isEnd = (borderCombination.waypoints[idx].isEnd);
      bool canMergeIntoPreviousLine = false;

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
        // add border
        const EDirection lastNeighborDir = borderCombination.waypoints[idx-1].direction;
        outBorders.emplace_back( MakeBorder(curOrigin, curDest, firstNeighborDir, lastNeighborDir) );
        
        // origin <- dest
        curOrigin = curDest;
        firstNeighborDir = lastNeighborDir;
      }
      
      curDest = borderCenter;
      
      if ( isEnd )
      {
        // add border
        const EDirection lastNeighborDir = borderCombination.waypoints[idx].direction;
        outBorders.emplace_back( MakeBorder(curOrigin, curDest, firstNeighborDir, lastNeighborDir) );
        
        // if there are more waypoints, reset current data so that the next waypoint doesn't start on us
        const size_t nextIdx = idx+1;
        if ( nextIdx < count ) {
          firstNeighborDir = borderCombination.waypoints[nextIdx].direction;
          curOrigin = CalculateBorderWaypointCenter(borderCombination.waypoints[nextIdx]);
          curDest   = curOrigin;
        }
      }
      
      ++idx;
    } while ( idx < count );
    
  } // has waypoints
  

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
    
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTreeProcessorContent", quadVector);
    
    _contentGfxDirty = false;
  }

  // borders
  if ( _borderGfxDirty && (kRenderBordersFrom || kRenderBordersToDot || kRenderBordersToQuad) )
  {

    VizManager::SimpleQuadVector quadVector;

    // add quads from borders
    for ( const auto& comboIt : _bordersPerContentCombination )
    {
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
  
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTreeProcessorBorders", quadVector);
    _borderGfxDirty = false;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMeshQuadTreeProcessor::IsCached(ENodeContentType contentType)
{
  const bool isCached = (contentType == ENodeContentType::ObstacleCube        ) ||
                        (contentType == ENodeContentType::ObstacleUnrecognized);
  return isCached;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ColorRGBA NavMeshQuadTreeProcessor::GetDebugColor(ENodeContentType contentType)
{
  ColorRGBA ret = Anki::NamedColors::BLACK;
  switch (contentType) {
    case ENodeContentType::Invalid:              { CORETECH_ASSERT(!"not supported"); break; };
    case ENodeContentType::Subdivided:           { ret = ColorRGBA(0.2f, 0.2f, 0.2f, 0.3f); break; };
    case ENodeContentType::Clear:                { ret = ColorRGBA(1.0f, 0.0f, 1.0f, 0.3f); break; };
    case ENodeContentType::Unknown:              { ret = ColorRGBA(0.2f, 0.2f, 0.6f, 0.3f); break; };
    case ENodeContentType::ObstacleCube:         { ret = ColorRGBA(1.0f, 0.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::ObstacleUnrecognized: { ret = ColorRGBA(0.5f, 0.0f, 0.0f, 0.3f); break; };
    case ENodeContentType::Cliff:                { ret = ColorRGBA(0.0f, 0.0f, 0.0f, 0.3f); break; };
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t NavMeshQuadTreeProcessor::GetBorderTypeKey(ENodeContentType innerType, ENodeContentType outerType)
{
  static_assert( sizeof(std::underlying_type<ENodeContentType>::type) < 2, "Can't fit keys in 4 bytes" );
  
  // key = innerType << X | outerType, where X is sizeof(outerType)
  uint32_t key = 0;
  key |= static_cast<std::underlying_type<ENodeContentType>::type>(innerType);
  key <<= (8*(sizeof(std::underlying_type<ENodeContentType>::type)));
  key |= static_cast<std::underlying_type<ENodeContentType>::type>(outerType);

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
NavMemoryMapTypes::Border NavMeshQuadTreeProcessor::MakeBorder(const Point3f& origin, const Point3f& dest,
  EDirection firstEDirection, EDirection lastEDirection)
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

  NavMemoryMapTypes::Border ret{origin, dest, perpendicular};
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::InvalidateBorders()
{
  // set all borders as dirty
  for ( auto& comboIt : _bordersPerContentCombination )
  {
    comboIt.second.dirty = true;
  }
  
  // all gfx are dirty true
  _borderGfxDirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::AddBorderWaypoint(const NavMeshQuadTreeNode* from, const NavMeshQuadTreeNode* to, EDirection dir)
{
  CORETECH_ASSERT(nullptr != _currentBorderCombination);
  _currentBorderCombination->waypoints.emplace_back( from, to, dir, false );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::FinishBorder()
{
  CORETECH_ASSERT(nullptr != _currentBorderCombination);
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
void NavMeshQuadTreeProcessor::FindBorders(ENodeContentType innerType, ENodeContentType outerType)
{
  using namespace FindBordersHelpers;
  
  DEBUG_FIND_BORDER("------------------------------------------------------");
  DEBUG_FIND_BORDER("Starting FindBorders...");
  const uint32_t borderComboKey = GetBorderTypeKey(innerType, outerType);
  _currentBorderCombination = &_bordersPerContentCombination[borderComboKey];

  CORETECH_ASSERT(IsCached(innerType));
  const NodeSet& innerSet = _nodeSets[innerType];

  _currentBorderCombination->waypoints.clear();
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
  _currentBorderCombination->dirty = false;
  _currentBorderCombination = nullptr;
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
