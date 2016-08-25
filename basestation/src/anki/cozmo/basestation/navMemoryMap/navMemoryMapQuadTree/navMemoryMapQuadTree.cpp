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

  ENodeContentType nodeContentType = ENodeContentType::Invalid;
  switch (contentType) {
    case EContentType::Unknown:               { nodeContentType = ENodeContentType::Unknown;               break; }
    case EContentType::ClearOfObstacle:       { nodeContentType = ENodeContentType::ClearOfObstacle;       break; }
    case EContentType::ClearOfCliff:          { nodeContentType = ENodeContentType::ClearOfCliff;          break; }
    case EContentType::ObstacleCube:          { nodeContentType = ENodeContentType::ObstacleCube;          break; }
    case EContentType::ObstacleCubeRemoved:   { nodeContentType = ENodeContentType::ObstacleCubeRemoved;   break; }
    case EContentType::ObstacleCharger:       { nodeContentType = ENodeContentType::ObstacleCharger;       break; }
    case EContentType::ObstacleChargerRemoved:{ nodeContentType = ENodeContentType::ObstacleChargerRemoved;break; }
    case EContentType::ObstacleUnrecognized:  { nodeContentType = ENodeContentType::ObstacleUnrecognized;  break; }
    case EContentType::Cliff:                 { nodeContentType = ENodeContentType::Cliff;                 break; }
    case EContentType::InterestingEdge:       { nodeContentType = ENodeContentType::InterestingEdge;       break; }
    case EContentType::NotInterestingEdge:    { nodeContentType = ENodeContentType::NotInterestingEdge;    break; }
    case EContentType::_Count:                { ASSERT_NAMED(false, "NavMeshQuadTreeTypes.ConvertContentType.InvalidType._Count"); break; }
  }
  
  CORETECH_ASSERT(nodeContentType != ENodeContentType::Invalid);
  return nodeContentType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NavMeshQuadTreeTypes::ENodeContentTypePackedType ConvertContentArrayToFlags(const NavMemoryMapTypes::FullContentArray& array)
{
  using namespace NavMemoryMapTypes;
  using namespace NavMeshQuadTreeTypes;
  ASSERT_NAMED( ContentValueEntry::IsValidArray(array), "ConvertContentTypeToFlags.InvalidArray");

  ENodeContentTypePackedType contentTypeFlags = 0;
  for( const auto& entry : array )
  {
    if ( entry.Value() ) {
      const ENodeContentType contentType = ConvertContentType(entry.Content());
      const ENodeContentTypePackedType contentTypeFlag = ENodeContentTypeToFlag(contentType);
      contentTypeFlags = contentTypeFlags | contentTypeFlag;
    }
  }

  return contentTypeFlags;
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
void NavMemoryMapQuadTree::FillBorderInternal(EContentType typeToReplace, const FullContentArray& neighborsToFillFrom, EContentType newTypeSet)
{
  // convert into node types and emtpy (no extra info) node content
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType nodeTypeToReplace  = ConvertContentType(typeToReplace);
  const ENodeContentTypePackedType nodeNeighborsToFillFrom = ConvertContentArrayToFlags(neighborsToFillFrom);
  const ENodeContentType newNodeTypeSet = ConvertContentType(newTypeSet);
  NodeContent emptyNewNodeContent(newNodeTypeSet);

  // ask the processor to do it
  _navMesh.GetProcessor().FillBorder(nodeTypeToReplace, nodeNeighborsToFillFrom, emptyNewNodeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double NavMemoryMapQuadTree::GetExploredRegionAreaM2() const
{
  // delegate on processor
  const double area = _navMesh.GetProcessor().GetExploredRegionAreaM2();
  return area;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadTree::HasBorders(EContentType innerType, const FullContentArray& outerTypes) const
{
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
  
  // ask processor
  const bool hasBorders = _navMesh.GetProcessor().HasBorders(innerNodeType, outerNodeTypes);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NavMemoryMapQuadTree::CalculateBorders(EContentType innerType, const FullContentArray& outerTypes, BorderVector& outBorders)
{
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
  
  // delegate on processor
  _navMesh.GetProcessor().GetBorders(innerNodeType, outerNodeTypes, outBorders);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadTree::HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, const FullContentArray& types) const
{
  // convert type to quadtree node content and to flag (since processor takes in flags)
  const ENodeContentTypePackedType nodeTypeFlags = ConvertContentArrayToFlags(types);
  
  // ask the processor about the collision with the converted type
  const bool hasCollision = _navMesh.GetProcessor().HasCollisionRayWithTypes(rayFrom, rayTo, nodeTypeFlags);
  return hasCollision;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NavMemoryMapQuadTree::HasContentType(EContentType type) const
{
  // conver to node content type
  using namespace NavMeshQuadTreeTypes;
  const ENodeContentType nodeType = ConvertContentType(type);
  
  // ask the processor
  const bool hasAny = _navMesh.GetProcessor().HasContentType(nodeType);
  return hasAny;
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
