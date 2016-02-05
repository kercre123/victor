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
#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/viz/vizManager.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kRenderNavMeshQuadTree         , "NavMeshQuadTree", true);
CONSOLE_VAR(bool, kRenderNavMeshQuadTreeProcessor, "NavMeshQuadTree", true);
CONSOLE_VAR(bool, kRenderLastAddedQuad           , "NavMeshQuadTree", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTree::NavMeshQuadTree(VizManager* vizManager)
: _gfxDirty(true)
, _processor(vizManager)
, _root({0,0,1}, 256, 4, NavMeshQuadTreeTypes::EQuadrant::Root, nullptr)
, _vizManager(vizManager)
{
  _processor.SetRoot( &_root );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTree::~NavMeshQuadTree()
{
  _processor.SetRoot(nullptr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Draw() const
{
  if ( _gfxDirty && kRenderNavMeshQuadTree )
  {
    // ask root to add proper quads to be rendered
    VizManager::SimpleQuadVector quadVector;
    _root.AddQuadsToDraw(quadVector);
    _vizManager->DrawQuadVector("NavMeshQuadTree", quadVector);
    
//    // compare actual size vs max
//    size_t actual = quadVector.size();
//    size_t max = pow(4,_root.GetLevel()) + 1;
//    PRINT_NAMED_INFO("RSAM", "%zu / %zu", actual, max);
  
    _gfxDirty = false;
  }
  
  // draw the processor information
  _processor.Draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::AddQuad(const Quad2f& quad, ENodeContentType nodeType)
{
  // render approx last quad added
  if ( kRenderLastAddedQuad )
  {
    ColorRGBA color = Anki::NamedColors::WHITE;
    const float z = 70.0f;
    Point3f topLeft = {quad[Quad::CornerName::TopLeft].x(), quad[Quad::CornerName::TopLeft].y(), z};
    Point3f topRight = {quad[Quad::CornerName::TopRight].x(), quad[Quad::CornerName::TopRight].y(), z};
    Point3f bottomLeft = {quad[Quad::CornerName::BottomLeft].x(), quad[Quad::CornerName::BottomLeft].y(), z};
    Point3f bottomRight = {quad[Quad::CornerName::BottomRight].x(), quad[Quad::CornerName::BottomRight].y(), z};
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", topLeft, topRight, color, true);
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", topRight, bottomRight, color, false);
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", bottomRight, bottomLeft, color, false);
    _vizManager->DrawSegment("NavMeshQuadTree::AddQuad", bottomLeft, topLeft, color, false);
  }

  // if the root does not contain the quad, expand
  if ( !_root.Contains( quad ) )
  {
    Expand( quad );
  }

  // add quad now
  _gfxDirty = _root.AddQuad(quad, nodeType, _processor) || _gfxDirty;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMeshQuadTree::Expand(const Quad2f& quadToCover)
{
  // Find in which direction we are expanding and upgrade root level in that direction
  const Vec2f& direction = quadToCover.ComputeCentroid() - Point2f{_root.GetCenter().x(), _root.GetCenter().y()};
  _root.UpgradeRootLevel(direction, _processor);

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
