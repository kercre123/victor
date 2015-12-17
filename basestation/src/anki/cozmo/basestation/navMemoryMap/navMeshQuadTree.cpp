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

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTree::NavMeshQuadTree()
: _gfxDirty(true)
, _root({0,0,1}, 256, 4, NavMeshQuadTreeNode::EContentType::Unknown)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Draw() const
{
  if ( _gfxDirty )
  {
    // ask root to add proper quads to be rendered
    VizManager::SimpleQuadVector quadVector;
    _root.AddQuadsToDraw(quadVector);
    VizManager::getInstance()->DrawQuadVector("NavMemoryMap", quadVector); // TODO set proper name

    PRINT_NAMED_INFO("RSAM", "#quads : %zu", quadVector.size());
  
    _gfxDirty = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddClearQuad(const Quad2f& quad)
{
  // if the root fully contains the quad, then delegate on it
  if ( _root.Contains( quad ) )
  {
    _gfxDirty = _root.AddClearQuad(quad);
  }
  else
  {
    _gfxDirty = true;
    
    // Find in which direction we are expanding and upgrade root level in that direction
    const Vec2f& direction = quad.ComputeCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
    _root.UpgradeToParent(direction);

    // should be contained, otherwise more expansions as required. This can only happen if quad size is too small
    // or direction is wrong
    if ( !_root.Contains(quad) ) {
      PRINT_NAMED_ERROR("NavMeshQuadTree.AddClearQuad.InsufficientExpansion",
        "Quad caused expansion, but expansion was not enough.");
    }

    // treat the quad after expansion
    _root.AddClearQuad(quad);
  }
}


} // namespace Cozmo
} // namespace Anki
