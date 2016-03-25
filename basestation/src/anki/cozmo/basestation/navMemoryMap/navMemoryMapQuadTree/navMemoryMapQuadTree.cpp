/**
 * File: navMemoryMapQuadTree.cpp
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "navMemoryMapQuadTree.h"

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
    case EContentType::InterestingEdge: { nodeContentType = ENodeContentType::InterestingEdge; break; }
  }
  
  CORETECH_ASSERT(nodeContentType != ENodeContentType::Invalid);
  return nodeContentType;
}

}; // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMapQuadTree
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMemoryMapQuadTree::NavMemoryMapQuadTree(VizManager* vizManager)
: _navMesh(vizManager)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMapQuadTree::Merge(const INavMemoryMap* other, const Pose3d& transform)
{
  ASSERT_NAMED(other != nullptr, "NavMemoryMapQuadTree.Merge.NullMap");
  ASSERT_NAMED(dynamic_cast<const NavMemoryMapQuadTree*>(other), "NavMemoryMapQuadTree.Merge.UnsupportedClass");
  const NavMemoryMapQuadTree* otherMap = static_cast<const NavMemoryMapQuadTree*>(other);
  _navMesh.Merge( otherMap->_navMesh, transform );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadTree::HasBorders(EContentType innerType, EContentType outerType) const
{
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentType outerNodeType = ConvertContentType(outerType);
  const ENodeContentTypePackedType outerNodeTypes = ENodeContentTypeToFlag( outerNodeType );
  
  const bool hasBorders = _navMesh.GetProcessor().HasBorders(innerNodeType, outerNodeTypes);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadTree::HasBorders(EContentType innerType, const std::set<EContentType>& outerTypes) const
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
void NavMemoryMapQuadTree::CalculateBorders(EContentType innerType, EContentType outerType, BorderVector& outBorders)
{
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentType outerNodeType = ConvertContentType(outerType);
  const ENodeContentTypePackedType outerNodeTypes = ENodeContentTypeToFlag( outerNodeType );
  
  _navMesh.GetProcessor().GetBorders(innerNodeType, outerNodeTypes, outBorders);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMapQuadTree::CalculateBorders(EContentType innerType, const std::set<EContentType>& outerTypes, BorderVector& outBorders)
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
void NavMemoryMapQuadTree::Draw(size_t mapIdxHint) const
{
  _navMesh.Draw(mapIdxHint);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMapQuadTree::ClearDraw() const
{
  _navMesh.ClearDraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMapQuadTree::AddQuadInternal(const Quad2f& quad, EContentType type)
{
  const NavMeshQuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(type);
  NodeContent nodeContent(nodeContentType);
  _navMesh.AddQuad(quad, nodeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMapQuadTree::AddQuadInternal(const Quad2f& quad, const INavMemoryMapQuadData& content)
{
  const NavMeshQuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(content.type);
  NodeContent nodeContent(nodeContentType);
  nodeContent.data.reset( content.Clone() );

  _navMesh.AddQuad(quad, nodeContent);
}

} // namespace Cozmo
} // namespace Anki
