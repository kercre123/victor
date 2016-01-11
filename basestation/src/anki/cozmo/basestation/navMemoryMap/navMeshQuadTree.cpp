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

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kRenderNavMeshQuadTree, "NavMeshQuadTree", true);
CONSOLE_VAR(bool, kRenderLastAddedQuad  , "NavMeshQuadTree", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTree::NavMeshQuadTree()
: _gfxDirty(true)
, _root({0,0,1}, 256, 4, NavMeshQuadTreeNode::EContentType::Unknown)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Draw() const
{
  if ( _gfxDirty && kRenderNavMeshQuadTree )
  {
    // ask root to add proper quads to be rendered
    VizManager::SimpleQuadVector quadVector;
    _root.AddQuadsToDraw(quadVector);
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTree", quadVector);
  
    _gfxDirty = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddClearQuad(const Quad2f& quad)
{
  // render approx last quad added
  if ( kRenderLastAddedQuad )
  {
    VizManager::SimpleQuadVector quadVector;
    Point3f center(quad.ComputeCentroid().x(), quad.ComputeCentroid().y(), 20);
    float size = Anki::Util::Max((quad.GetMaxX()-quad.GetMinX()), (quad.GetMaxY() - quad.GetMinY()));
    quadVector.push_back(VizManager::MakeSimpleQuad(Anki::NamedColors::DARKGREEN, center, size));
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTree::AddClearQuad", quadVector);
  }

  // if the root does not contain the quad, expand
  if ( !_root.Contains( quad ) )
  {
    Expand( quad );
  }

  // add clear quad now
  _gfxDirty = _root.AddClearQuad(quad) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddObstacle(const Quad2f& quad, NavMemoryMapTypes::EObstacleType obstacleType)
{
  // render approx last quad added
  if ( kRenderLastAddedQuad )
  {
    VizManager::SimpleQuadVector quadVector;
    Point3f center(quad.ComputeCentroid().x(), quad.ComputeCentroid().y(), 20);
    float size = Anki::Util::Max((quad.GetMaxX()-quad.GetMinX()), (quad.GetMaxY() - quad.GetMinY()));
    quadVector.push_back(VizManager::MakeSimpleQuad(Anki::NamedColors::ORANGE, center, size));
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTree::AddObstacle", quadVector);
  }

  // if the root does not contain the quad, expand
  if ( !_root.Contains( quad ) )
  {
    Expand( quad );
  }

  // add the proper obstacle type in the node
  bool changed = false;
  switch( obstacleType ) {
    case NavMemoryMapTypes::Cube: {
      changed = _root.AddObstacleCube(quad);
      break;
    }
    case NavMemoryMapTypes::Unrecognized: {
      changed = _root.AddObstacleUnrecognized(quad);
      break;
    }
    default: {
      ASSERT_NAMED(false, "NavMeshQuadTree.AddObstacle.InvalidObstacleEnum");
    }
  }

  // add obstacle now
  _gfxDirty = changed || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddCliff(const Quad2f& quad)
{
  // render approx last quad added
  if ( kRenderLastAddedQuad )
  {
    VizManager::SimpleQuadVector quadVector;
    Point3f center(quad.ComputeCentroid().x(), quad.ComputeCentroid().y(), 20);
    float size = Anki::Util::Max((quad.GetMaxX()-quad.GetMinX()), (quad.GetMaxY() - quad.GetMinY()));
    quadVector.push_back(VizManager::MakeSimpleQuad(Anki::NamedColors::YELLOW, center, size));
    VizManager::getInstance()->DrawQuadVector("NavMeshQuadTree::AddCliff", quadVector);
  }

  // if the root does not contain the quad, expand
  if ( !_root.Contains( quad ) )
  {
    Expand( quad );
  }

  // add cliff now
  _gfxDirty = _root.AddCliff(quad) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Quad2f& quadToCover)
{
  // Find in which direction we are expanding and upgrade root level in that direction
  const Vec2f& direction = quadToCover.ComputeCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
  _root.UpgradeRootLevel(direction);

  // should be contained, otherwise more expansions are required. This can only happen if quad is too far from
  // the root, ir it is bigger than the current root size. I don't think we'll ever need multiple expansions, but
  // throw error in case it ever happens. If so, the solution might be as easy as changing the calling code to call
  // expand until it's fully contained
  if ( !_root.Contains(quadToCover) ) {
    PRINT_NAMED_ERROR("NavMeshQuadTree.AddCliff.InsufficientExpansion",
      "Quad caused expansion, but expansion was not enough.");
  }
  
  // always flag as dirty since we have modified the root
  _gfxDirty = true;
}

} // namespace Cozmo
} // namespace Anki
