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

  // if the root fully contains the quad, then delegate on it
  if ( _root.Contains( quad ) )
  {
    _gfxDirty = _root.AddClearQuad(quad) || _gfxDirty;
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddObstacle(const Quad2f& quad)
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

  // if the root fully contains the quad, then delegate on it
  if ( _root.Contains( quad ) )
  {
    _gfxDirty = _root.AddObstacle(quad) || _gfxDirty;
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
      PRINT_NAMED_ERROR("NavMeshQuadTree.AddObstacle.InsufficientExpansion",
        "Quad caused expansion, but expansion was not enough.");
    }

    // treat the quad after expansion
    _root.AddObstacle(quad);
  }
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

  // if the root fully contains the quad, then delegate on it
  if ( _root.Contains( quad ) )
  {
    _gfxDirty = _root.AddCliff(quad) || _gfxDirty;
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
      PRINT_NAMED_ERROR("NavMeshQuadTree.AddCliff.InsufficientExpansion",
        "Quad caused expansion, but expansion was not enough.");
    }

    // treat the quad after expansion
    _root.AddCliff(quad);
  }
}



} // namespace Cozmo
} // namespace Anki
