/**
 * File: memoryMap.cpp
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "memoryMap.h"

#include "memoryMapTypes.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{

// number of root shifts that should be made to include new content
static const int numberOfAllowedShiftsToIncludeContent = 1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// convert between MemoryMapTypes::EContentType and QuadTreeTypes::ENodeContentType
QuadTreeTypes::ENodeContentType ConvertContentType(MemoryMapTypes::EContentType contentType )
{
  using namespace MemoryMapTypes;
  using namespace QuadTreeTypes;

  ENodeContentType nodeContentType = ENodeContentType::Invalid;
  switch (contentType) {
    case EContentType::Unknown:               { nodeContentType = ENodeContentType::Unknown;               break; }
    case EContentType::ClearOfObstacle:       { nodeContentType = ENodeContentType::ClearOfObstacle;       break; }
    case EContentType::ClearOfCliff:          { nodeContentType = ENodeContentType::ClearOfCliff;          break; }
    case EContentType::ObstacleCube:          { nodeContentType = ENodeContentType::ObstacleCube;          break; }
    case EContentType::ObstacleCubeRemoved:   { nodeContentType = ENodeContentType::ObstacleCubeRemoved;   break; }
    case EContentType::ObstacleCharger:       { nodeContentType = ENodeContentType::ObstacleCharger;       break; }
    case EContentType::ObstacleChargerRemoved:{ nodeContentType = ENodeContentType::ObstacleChargerRemoved;break; }
    case EContentType::ObstacleProx:          { nodeContentType = ENodeContentType::ObstacleProx;          break; }
    case EContentType::ObstacleUnrecognized:  { nodeContentType = ENodeContentType::ObstacleUnrecognized;  break; }
    case EContentType::Cliff:                 { nodeContentType = ENodeContentType::Cliff;                 break; }
    case EContentType::InterestingEdge:       { nodeContentType = ENodeContentType::InterestingEdge;       break; }
    case EContentType::NotInterestingEdge:    { nodeContentType = ENodeContentType::NotInterestingEdge;    break; }
    case EContentType::_Count:                { DEV_ASSERT(false, "QuadTreeTypes.ConvertContentType.InvalidType._Count"); break; }
  }
  
  DEV_ASSERT(nodeContentType != ENodeContentType::Invalid,
             "QuadTreeTypes.ConvertContentType.InvalidNodeContentType");
  
  return nodeContentType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTreeTypes::ENodeContentTypePackedType ConvertContentArrayToFlags(const MemoryMapTypes::FullContentArray& array)
{
  using namespace MemoryMapTypes;
  using namespace QuadTreeTypes;
  
  DEV_ASSERT(IsSequentialArray(array), "QuadTreeTypes.ConvertContentArrayToFlags.InvalidArray");

  ENodeContentTypePackedType contentTypeFlags = 0;
  for( const auto& entry : array )
  {
    if ( entry.Value() ) {
      const ENodeContentType contentType = ConvertContentType(entry.EnumValue());
      const ENodeContentTypePackedType contentTypeFlag = ENodeContentTypeToFlag(contentType);
      contentTypeFlags = contentTypeFlags | contentTypeFlag;
    }
  }

  return contentTypeFlags;
}

}; // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// MemoryMap
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemoryMap::MemoryMap(VizManager* vizManager, Robot* robot)
: _quadTree(vizManager, robot)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::Merge(const INavMap* other, const Pose3d& transform)
{
  DEV_ASSERT(other != nullptr, "MemoryMap.Merge.NullMap");
  DEV_ASSERT(dynamic_cast<const MemoryMap*>(other), "MemoryMap.Merge.UnsupportedClass");
  const MemoryMap* otherMap = static_cast<const MemoryMap*>(other);
  _quadTree.Merge( otherMap->_quadTree, transform );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::FillBorderInternal(EContentType typeToReplace, const FullContentArray& neighborsToFillFrom, EContentType newTypeSet, TimeStamp_t timeMeasured)
{
  // convert into node types and emtpy (no extra info) node content
  using namespace QuadTreeTypes;
  const ENodeContentType nodeTypeToReplace  = ConvertContentType(typeToReplace);
  const ENodeContentTypePackedType nodeNeighborsToFillFrom = ConvertContentArrayToFlags(neighborsToFillFrom);
  const ENodeContentType newNodeTypeSet = ConvertContentType(newTypeSet);
  NodeContent emptyNewNodeContent(newNodeTypeSet, timeMeasured);

  // ask the processor to do it
  _quadTree.GetProcessor().FillBorder(nodeTypeToReplace, nodeNeighborsToFillFrom, emptyNewNodeContent);
  _quadTree.ForceRedraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::ReplaceContentInternal(const Quad2f& inQuad, EContentType typeToReplace, EContentType newTypeSet, TimeStamp_t timeMeasured)
{
  // convert into node types and emtpy (no extra info) node content
  using namespace QuadTreeTypes;
  const ENodeContentType nodeTypeToReplace = ConvertContentType(typeToReplace);
  const ENodeContentType newNodeTypeSet = ConvertContentType(newTypeSet);
  NodeContent emptyNewNodeContent(newNodeTypeSet, timeMeasured);

  // Implementation note: since we define a quad, we should be directly asking the navMesh to do this. Currently
  // however I do not have a way to do this in AddQuad, and I think it would be slightly more difficult to
  // do in AddQuad (since we have to pass the typeToReplace around and change the logic based on whether a typeToReplace
  // has been specified). Additionally, if I add this to AddQuad, I would also have to add it to AddLine and AddPoint,
  // so the change is bigger than asking the processor to do this.
  // The issue with the processor is that the processor provides fast access to nodes of a given type, but regardless
  // of location (which is exaclty what the quadTree provides), so it may be more or less efficient depending on
  // the number of nodes currently with the given type versus the number of quads that fall within the quad. Since
  // I do not have that metric at the moment for the use cases, I am going with a simpler implementation, unless
  // profile shows that it's not adequate
  _quadTree.GetProcessor().ReplaceContent(inQuad, nodeTypeToReplace, emptyNewNodeContent);
  _quadTree.ForceRedraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::ReplaceContentInternal(EContentType typeToReplace, EContentType newTypeSet, TimeStamp_t timeMeasured)
{
  // convert into node types and emtpy (no extra info) node content
  using namespace QuadTreeTypes;
  const ENodeContentType nodeTypeToReplace = ConvertContentType(typeToReplace);
  const ENodeContentType newNodeTypeSet = ConvertContentType(newTypeSet);
  NodeContent emptyNewNodeContent(newNodeTypeSet, timeMeasured);

  // ask the processor
  _quadTree.GetProcessor().ReplaceContent(nodeTypeToReplace, emptyNewNodeContent);
  _quadTree.ForceRedraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MemoryMap::GetExploredRegionAreaM2() const
{
  // delegate on processor
  const double area = _quadTree.GetProcessor().GetExploredRegionAreaM2();
  return area;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double MemoryMap::GetInterestingEdgeAreaM2() const
{
  // delegate on processor
  const double area = _quadTree.GetProcessor().GetInterestingEdgeAreaM2();
  return area;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float MemoryMap::GetContentPrecisionMM() const
{
  // ask the navmesh
  const float precision = _quadTree.GetContentPrecisionMM();
  return precision;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasBorders(EContentType innerType, const FullContentArray& outerTypes) const
{
  using namespace QuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
  
  // ask processor
  const bool hasBorders = _quadTree.GetProcessor().HasBorders(innerNodeType, outerNodeTypes);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::CalculateBorders(EContentType innerType, const FullContentArray& outerTypes, BorderRegionVector& outBorders)
{
  using namespace QuadTreeTypes;
  const ENodeContentType innerNodeType = ConvertContentType(innerType);
  const ENodeContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
  
  // delegate on processor
  _quadTree.GetProcessor().GetBorders(innerNodeType, outerNodeTypes, outBorders);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, const FullContentArray& types) const
{
  // convert type to quadtree node content and to flag (since processor takes in flags)
  const ENodeContentTypePackedType nodeTypeFlags = ConvertContentArrayToFlags(types);
  
  // ask the processor about the collision with the converted type
  const bool hasCollision = _quadTree.GetProcessor().HasCollisionRayWithTypes(rayFrom, rayTo, nodeTypeFlags);
  return hasCollision;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasContentType(EContentType type) const
{
  // conver to node content type
  using namespace QuadTreeTypes;
  const ENodeContentType nodeType = ConvertContentType(type);
  
  // ask the processor
  const bool hasAny = _quadTree.GetProcessor().HasContentType(nodeType);
  return hasAny;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::DrawDebugProcessorInfo(size_t mapIdxHint) const
{
  _quadTree.DrawDebugProcessorInfo(mapIdxHint);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::ClearDraw() const
{
  _quadTree.ClearDraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::Broadcast(uint32_t originID) const
{
  _quadTree.Broadcast(originID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::BroadcastMemoryMapDraw(uint32_t originID, size_t mapIdxHint) const
{
  _quadTree.BroadcastMemoryMapDraw(originID, mapIdxHint);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddQuadInternal(const Quad2f& quad, EContentType type, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  _quadTree.AddQuad(quad, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddQuadInternal(const Quad2f& quad, const MemoryMapData& content, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(content.type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  nodeContent.typeData.reset( content.Clone() );

  _quadTree.AddQuad(quad, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddLineInternal(const Point2f& from, const Point2f& to, EContentType type, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  _quadTree.AddLine(from, to, nodeContent, numberOfAllowedShiftsToIncludeContent);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddLineInternal(const Point2f& from, const Point2f& to, const MemoryMapData& content, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(content.type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  nodeContent.typeData.reset( content.Clone() );

  _quadTree.AddLine(from, to, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddTriangleInternal(const Triangle2f& tri, EContentType type, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  _quadTree.AddTriangle(tri, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddTriangleInternal(const Triangle2f& tri, const MemoryMapData& content, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(content.type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  nodeContent.typeData.reset( content.Clone() );

  _quadTree.AddTriangle(tri, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddPointInternal(const Point2f& point, EContentType type, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  _quadTree.AddPoint(point, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::AddPointInternal(const Point2f& point, const MemoryMapData& content, TimeStamp_t timeMeasured)
{
  const QuadTreeTypes::ENodeContentType nodeContentType = ConvertContentType(content.type);
  NodeContent nodeContent(nodeContentType, timeMeasured);
  nodeContent.typeData.reset( content.Clone() );

  _quadTree.AddPoint(point, nodeContent, numberOfAllowedShiftsToIncludeContent);
}

} // namespace Cozmo
} // namespace Anki
