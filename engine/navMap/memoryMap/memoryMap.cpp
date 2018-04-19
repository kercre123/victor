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

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/engine/math/quad.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/fastPolygon2d.h"

#include "util/console/consoleInterface.h"

#include <chrono>
#include <type_traits>
#include <unordered_map>
 

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

CONSOLE_VAR(bool, kMapPerformanceTestsEnabled, "ProxSensorComponent", false);
CONSOLE_VAR(int,  kMapPerformanceTestsSampleWindow, "ProxSensorComponent", 128);

namespace
{
#define MONITOR_PERFORMANCE(eval) (kMapPerformanceTestsEnabled) ? PerformanceMonitor([&]() {return eval;}, __FILE__ ":" + std::string(__func__)) : eval

struct PerformanceRecord { double avgTime_us = 0; u32 samples = 0; };
static std::unordered_map<std::string, PerformanceRecord> sPerformanceRecords;

static void UpdatePerformanceRecord(const double& time_us, const std::string& recordName) {
  auto& record = sPerformanceRecords[recordName];
  if (record.samples > kMapPerformanceTestsSampleWindow ) {
    // approximate rolling window average to save memory
    record.avgTime_us += (time_us - record.avgTime_us) / kMapPerformanceTestsSampleWindow;
  } else {
    record.avgTime_us += time_us / kMapPerformanceTestsSampleWindow;
  }
  
  // print info (faster than modulo for powers of 2)
  if ((++record.samples & kMapPerformanceTestsSampleWindow - 1) == 0) {
    DEV_ASSERT((kMapPerformanceTestsSampleWindow & (kMapPerformanceTestsSampleWindow - 1)) == 0,
      "Performance sample window not a power of 2");
    PRINT_NAMED_INFO("PerformanceMonitor", "Average time for '%s' is %f us", recordName.c_str(), record.avgTime_us);
  }
}

// handle non-void methods
template<typename T>
static auto PerformanceMonitor(T f, const std::string& method, 
  typename std::enable_if<!std::is_void<decltype(f())>::value>::type* = nullptr) -> decltype(f()) {

  const auto start = std::chrono::system_clock::now();
  const auto retv = f();
  const auto time_us = (std::chrono::system_clock::now() - start).count();

  UpdatePerformanceRecord(time_us, method);
  return retv;
}

// handle void methods
template<typename T>
static auto PerformanceMonitor(T f, const std::string& method, 
  typename std::enable_if<std::is_void<decltype(f())>::value>::type* = nullptr) -> decltype(f()) {
    
  const auto start = std::chrono::system_clock::now();
  f();
  const auto time_us = (std::chrono::system_clock::now() - start).count();

  UpdatePerformanceRecord(time_us, method);
}

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
MemoryMap::MemoryMap()
: _quadTree()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::Merge(const INavMap* other, const Pose3d& transform)
{
  DEV_ASSERT(other != nullptr, "MemoryMap.Merge.NullMap");
  DEV_ASSERT(dynamic_cast<const MemoryMap*>(other), "MemoryMap.Merge.UnsupportedClass");
  const MemoryMap* otherMap = static_cast<const MemoryMap*>(other);
  return MONITOR_PERFORMANCE( _quadTree.Merge( otherMap->_quadTree, transform ) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::FillBorder(EContentType typeToReplace,
                           const FullContentArray& neighborsToFillFrom,
                           const MemoryMapDataPtr& newData)
{
  // convert into node types and emtpy (no extra info) node content
  using namespace QuadTreeTypes;
  const EContentTypePackedType nodeNeighborsToFillFrom = ConvertContentArrayToFlags(neighborsToFillFrom);
  
  // ask the processor to do it
  return MONITOR_PERFORMANCE( _quadTree.GetProcessor().FillBorder(typeToReplace, nodeNeighborsToFillFrom, newData) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::FillBorder(const NodePredicate& innerPred, const NodePredicate& outerPred, const MemoryMapDataPtr& newData)
{
  // ask the processor to do it
  return MONITOR_PERFORMANCE( _quadTree.GetProcessor().FillBorder(innerPred, outerPred, newData) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::TransformContent(NodeTransformFunction transform)
{
  return MONITOR_PERFORMANCE( _quadTree.Transform(transform) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::TransformContent(const Poly2f& poly, NodeTransformFunction transform)
{
  return MONITOR_PERFORMANCE( _quadTree.Transform(poly, transform) );
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
  using namespace MemoryMapTypes;
  const EContentTypePackedType outerNodeTypes = ConvertContentArrayToFlags(outerTypes);
  
  // delegate on processor
  MONITOR_PERFORMANCE( _quadTree.GetProcessor().GetBorders(innerType, outerNodeTypes, outBorders) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasCollisionWithTypes(const FastPolygon& poly, const FullContentArray& types) const
{
  // convert type to quadtree node content and to flag (since processor takes in flags)
  const EContentTypePackedType nodeTypeFlags = ConvertContentArrayToFlags(types);

  return Eval( poly, [&nodeTypeFlags] (MemoryMapDataConstPtr data) {
    return IsInEContentTypePackedType(data->type, nodeTypeFlags);
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::Eval(const FastPolygon& poly, const NodePredicate& func) const
{
  bool anyMatch = false;
  if( func == nullptr ) {
    return anyMatch;
  }
  
  QuadTreeTypes::FoldFunctorConst accumulator = [&anyMatch, &func] (const QuadTreeNode& node) {
    MemoryMapDataConstPtr data = node.GetData();
    anyMatch |= func(data);
  };
  
  MONITOR_PERFORMANCE( _quadTree.Fold(accumulator, poly) );
  return anyMatch;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::HasContentType(EContentType type) const
{
  // ask the processor
  const bool hasAny = _quadTree.GetProcessor().HasContentType(type);
  return hasAny;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMap::Insert(const Poly2f& poly, const MemoryMapData& data)
{
  // clone data to make into a shared pointer.
  return MONITOR_PERFORMANCE( _quadTree.Insert(poly, data.Clone()) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::GetBroadcastInfo(MemoryMapTypes::MapBroadcastData& info) const 
{ 
  // get data for each node
  QuadTreeTypes::FoldFunctorConst accumulator = 
    [&info, this] (const QuadTreeNode& node) {
      // if the root done, populate header info
      if ( node.IsRootNode() ){
        std::stringstream instanceId;
        instanceId << "QuadTree_" << this;

        info.mapInfo = ExternalInterface::MemoryMapInfo(
          node.GetLevel(),
          node.GetSideLen(),
          node.GetCenter().x(),
          node.GetCenter().y(),
          node.GetCenter().z(),
          instanceId.str());
      }

      // leaf node
      if ( !node.IsSubdivided() )
      {
        info.quadInfo.emplace_back(
          ExternalInterface::MemoryMapQuadInfo( node.GetData()->GetExternalContentType(), node.GetLevel()));
      }
    };

  _quadTree.Fold(accumulator);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::FindContentIf(NodePredicate pred, MemoryMapDataConstList& output) const
{
  QuadTreeTypes::FoldFunctorConst accumulator = [&output, &pred] (const QuadTreeNode& node) {
    MemoryMapDataPtr data = node.GetData();
    if (pred(data)) {
      output.insert( MemoryMapDataConstPtr(node.GetData()) );
    }
  };

  MONITOR_PERFORMANCE( _quadTree.Fold(accumulator) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemoryMap::FindContentIf(const FastPolygon& poly, NodePredicate pred, MemoryMapDataConstList& output) const
{
  QuadTreeTypes::FoldFunctorConst accumulator = [&output, &pred] (const QuadTreeNode& node) {
    MemoryMapDataPtr data = node.GetData();
    if( pred(data) ) { 
      output.insert( MemoryMapDataConstPtr(node.GetData()) );
    }
  };

  MONITOR_PERFORMANCE( _quadTree.Fold(accumulator, poly) );
}

} // namespace Cozmo
} // namespace Anki
