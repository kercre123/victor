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
#include "anki/common/basestation/math/polygon_impl.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

CONSOLE_VAR(bool, kDisableNavMap, "MemoryMap", false);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{

// number of root shifts that should be made to include new content
static const int numberOfAllowedShiftsToIncludeContent = 1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EContentTypePackedType ConvertContentArrayToFlags(const MemoryMapTypes::FullContentArray& array)
{
  using namespace MemoryMapTypes;
  using namespace QuadTreeTypes;
  
  DEV_ASSERT(IsSequentialArray(array), "QuadTreeTypes.ConvertContentArrayToFlags.InvalidArray");

  EContentTypePackedType contentTypeFlags = 0;
  for( const auto& entry : array )
  {
    if ( entry.Value() ) {
      const EContentTypePackedType contentTypeFlag = EContentTypeToFlag(entry.EnumValue());
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
: _quadTree(vizManager, robot, MemoryMapData(EContentType::Unknown, robot->GetLastMsgTimestamp()))
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
  if (!kDisableNavMap) {
    // convert into node types and emtpy (no extra info) node content
    using namespace QuadTreeTypes;
    const EContentTypePackedType nodeNeighborsToFillFrom = ConvertContentArrayToFlags(neighborsToFillFrom);
    MemoryMapData data(newTypeSet, timeMeasured);

    // ask the processor to do it
    _quadTree.GetProcessor().FillBorder(typeToReplace, nodeNeighborsToFillFrom, data);
    _quadTree.ForceRedraw();
  }
}

void MemoryMap::TransformContent(NodeTransformFunction transform)
{
  if (!kDisableNavMap) {
    _quadTree.GetProcessor().Transform(transform);
  }
}

void MemoryMap::TransformContent(const Poly2f& poly, NodeTransformFunction transform)
{
  if (!kDisableNavMap) {
    _quadTree.Transform(poly, transform);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::FindContentIf(NodePredicate pred, MemoryMapDataConstList& output)
{
  if (!kDisableNavMap) {
    _quadTree.GetProcessor().FindIf(pred, output);
  }
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
  const EContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
  
  // ask processor
  const bool hasBorders = _quadTree.GetProcessor().HasBorders(innerType, outerNodeTypes);
  return hasBorders;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::CalculateBorders(EContentType innerType, const FullContentArray& outerTypes, BorderRegionVector& outBorders)
{
  if (!kDisableNavMap) {
    using namespace MemoryMapTypes;
    const EContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
    
    // delegate on processor
    _quadTree.GetProcessor().GetBorders(innerType, outerNodeTypes, outBorders);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, const FullContentArray& types) const
{
  // convert type to quadtree node content and to flag (since processor takes in flags)
  const EContentTypePackedType nodeTypeFlags = ConvertContentArrayToFlags(types);
  
  // ask the processor about the collision with the converted type
  const bool hasCollision = _quadTree.GetProcessor().HasCollisionRayWithTypes(rayFrom, rayTo, nodeTypeFlags);
  return hasCollision;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasContentType(EContentType type) const
{
  // ask the processor
  const bool hasAny = _quadTree.GetProcessor().HasContentType(type);
  return hasAny;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::DrawDebugProcessorInfo(size_t mapIdxHint) const
{
  if (!kDisableNavMap) {
    _quadTree.DrawDebugProcessorInfo(mapIdxHint);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::ClearDraw() const
{
  if (!kDisableNavMap) {
    _quadTree.ClearDraw();
  }
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
void MemoryMap::Insert(const Poly2f& poly, const MemoryMapData& data)
{
  if (!kDisableNavMap) {
    _quadTree.Insert(poly, data, numberOfAllowedShiftsToIncludeContent);
  }
}

} // namespace Cozmo
} // namespace Anki
