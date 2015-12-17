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
  
    _gfxDirty = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddClearQuad(const Quad2f& quad)
{
  _gfxDirty = true;

  // if the root fully contains the quad, then delegate on it
  if ( _root.Contains( quad ) )
  {
    _root.AddClearQuad(quad);
  }
  else
  {
    // otherwise, we need to expand the root, but that means either rebuilding the quadtree or finding
    // a smart way to merge the current root into a new parent. Note that increases the depth and the size
    // of the root, so I'll think about it later (rsam 12/15/2015)
//    ASSERT_NAMED(false, "NavMeshQuadTree.AddClearQuad.NotImplemented");
  }
}


} // namespace Cozmo
} // namespace Anki
