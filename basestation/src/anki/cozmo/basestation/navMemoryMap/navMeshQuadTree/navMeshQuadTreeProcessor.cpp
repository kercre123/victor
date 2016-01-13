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
#include "util/math/constantsAndMacros.h"

#include <unordered_map>
#include <limits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeProcessor::NavMeshQuadTreeProcessor()
: _gfxDirty(false)
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
    CORETECH_ASSERT(_nodeCache[oldContent].find(node) != _nodeCache[oldContent].end());
    _nodeCache[oldContent].erase( node );
    
    // flag as dirty
    _gfxDirty = true;
  }

  if ( IsCached(newContent) )
  {
    // remove the node from that cache
    CORETECH_ASSERT(_nodeCache[newContent].find(node) == _nodeCache[newContent].end());
    _nodeCache[newContent].insert(node);
    
    // flag as dirty
    _gfxDirty = true;
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
    CORETECH_ASSERT(_nodeCache[oldContent].find(node) != _nodeCache[oldContent].end());
    _nodeCache[oldContent].erase( node );
    
    // flag as dirty
    _gfxDirty = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTreeProcessor::Draw() const
{
  if ( _gfxDirty )
  {
    // Add quads
    VizManager::SimpleQuadVector quadVector;
    
    AddQuadsToDraw(EContentType::ObstacleCube, quadVector, ColorRGBA( 128, 128, 255, (u8)200));
    AddQuadsToDraw(EContentType::ObstacleUnrecognized, quadVector, ColorRGBA(   0,   0, 128, (u8)200));

    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTreeProcessor", quadVector);
    
    _gfxDirty = false;
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
void NavMeshQuadTreeProcessor::AddQuadsToDraw(EContentType contentType,
  VizManager::SimpleQuadVector& quadVector, const ColorRGBA& color, float zOffset) const
{
  // find the set of quads for that content type
  auto match = _nodeCache.find(contentType);
  if ( match != _nodeCache.end() )
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
