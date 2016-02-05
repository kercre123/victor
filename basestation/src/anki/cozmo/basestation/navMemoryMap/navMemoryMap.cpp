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
    case EContentType::ClearOfObstacle: { nodeContentType = ENodeContentType::ClearOfObstacle; break; }
    case EContentType::ClearOfCliff: { nodeContentType = ENodeContentType::ClearOfCliff; break; }
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
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentType outerNodeType = ConvertContentType(outerType);
  const ENodeContentTypePackedType outerNodeTypes = ENodeContentTypeToFlag( outerNodeType );
  
  const bool hasBorders = _navMesh.GetProcessor().HasBorders(innerNodeType, outerNodeTypes);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMap::HasBorders(EContentType innerType, std::set<EContentType> outerTypes) const
{
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  ENodeContentTypePackedType outerNodeTypes = 0;
  
  for( const auto& outerTypeIt : outerTypes ) {
    const ENodeContentType outerNodeType = ConvertContentType(outerTypeIt);
    const ENodeContentTypePackedType outerNodeTypeFlag = ENodeContentTypeToFlag( outerNodeType );
    outerNodeTypes |= outerNodeTypeFlag;
  }
  
  const bool hasBorders = _navMesh.GetProcessor().HasBorders(innerNodeType, outerNodeTypes);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::CalculateBorders(EContentType innerType, EContentType outerType, BorderVector& outBorders)
{
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentType outerNodeType = ConvertContentType(outerType);
  const ENodeContentTypePackedType outerNodeTypes = ENodeContentTypeToFlag( outerNodeType );
  
  _navMesh.GetProcessor().GetBorders(innerNodeType, outerNodeTypes, outBorders);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::CalculateBorders(EContentType innerType, std::set<EContentType> outerTypes, BorderVector& outBorders)
{
  ASSERT_NAMED(!outerTypes.empty(), "No outerTypes provided");
  
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  ENodeContentTypePackedType outerNodeTypes = 0;
  
  for( const auto& outerTypeIt : outerTypes ) {
    const ENodeContentType outerNodeType = ConvertContentType(outerTypeIt);
    const ENodeContentTypePackedType outerNodeTypeFlag = ENodeContentTypeToFlag( outerNodeType );
    outerNodeTypes |= outerNodeTypeFlag;
  }
  
  _navMesh.GetProcessor().GetBorders(innerNodeType, outerNodeTypes, outBorders);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMap::Draw() const
{
  _navMesh.Draw();
}

} // namespace Cozmo
} // namespace Anki
