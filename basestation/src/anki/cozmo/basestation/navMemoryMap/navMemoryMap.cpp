/**
 * File: navMemoryMap.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "navMemoryMap.h"

#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapTypes.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// convert between NavMemoryMapTypes::EContentType and NavMeshQuadTreeTypes::ENodeContentType
NavMeshQuadTreeTypes::ENodeContentType ConvertContentType(NavMemoryMapTypes::EContentType contentType )
{
  using namespace NavMemoryMapTypes;
  using namespace NavMeshQuadTreeTypes;

  NavMeshQuadTreeTypes::ENodeContentType nodeContentType = ENodeContentType::Invalid;
  switch (contentType) {
    case EContentType::Unknown: { nodeContentType = ENodeContentType::Unknown; break; }
    case EContentType::Clear: { nodeContentType = ENodeContentType::Clear; break; }
    case EContentType::ObstacleCube: { nodeContentType = ENodeContentType::ObstacleCube; break; }
    case EContentType::ObstacleUnrecognized: { nodeContentType = ENodeContentType::ObstacleUnrecognized; break; }
    case EContentType::Cliff: { nodeContentType = ENodeContentType::Cliff; break; }
  }
  
  CORETECH_ASSERT(nodeContentType != ENodeContentType::Invalid);
  return nodeContentType;
}

}; // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMap
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMemoryMap::NavMemoryMap(VizManager* vizManager)
: _navMesh(vizManager)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::AddQuad(const Quad2f& quad, EContentType type)
{
  const NavMeshQuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(type);
  _navMesh.AddQuad(quad, nodeContentType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMap::HasBorders(EContentType innerType, EContentType outerType) const
{
  const NavMeshQuadTreeTypes::ENodeContentType innerNodeType = ConvertContentType(innerType);
  const NavMeshQuadTreeTypes::ENodeContentType outerNodeType = ConvertContentType(outerType);
  
  const bool hasBorders = _navMesh.GetProcessor().HasBorders(innerNodeType, outerNodeType);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::CalculateBorders(EContentType innerType, EContentType outerType, BorderVector& outBorders)
{
  const NavMeshQuadTreeTypes::ENodeContentType innerNodeType = ConvertContentType(innerType);
  const NavMeshQuadTreeTypes::ENodeContentType outerNodeType = ConvertContentType(outerType);
  
  _navMesh.GetProcessor().GetBorders(innerNodeType, outerNodeType, outBorders);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::Draw() const
{
  _navMesh.Draw();
}

} // namespace Cozmo
} // namespace Anki
