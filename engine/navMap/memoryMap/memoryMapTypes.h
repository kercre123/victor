/**
 * File: memoryMapTypes.h
 *
 * Author: Raul
 * Date:   01/11/2016
 *
 * Description: Type definitions for the MemoryMap.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_MEMORY_MAP_TYPES_H
#define ANKI_COZMO_MEMORY_MAP_TYPES_H

#include "engine/navMap/quadTree/quadTreeTypes.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/helpers/templateHelpers.h"
#include "clad/types/memoryMap.h"

#include <cstdint>
#include <vector>
#include <unordered_set>

namespace Anki {
namespace Vector {

class MemoryMapData;

namespace MemoryMapTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// structs
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// content detected in the map
enum class EContentType : uint8_t {
  Unknown,               // not discovered
  ClearOfObstacle,       // an area without obstacles
  ClearOfCliff,          // an area without obstacles or cliffs
  ObstacleObservable,    // an area with obstacles we recognize as observable
  ObstacleProx,          // an area with an obstacle found with the prox sensor
  ObstacleUnrecognized,  // an area with obstacles we do not recognize
  Cliff,                 // an area with cliffs or holes
  InterestingEdge,       // a border/edge detected by the camera
  NotInterestingEdge,    // a border/edge detected by the camera that we have already explored and it's not interesting anymore
  _Count // Flag, not a type
};

// each segment in a border region
struct BorderSegment
{
  using DataType = MemoryMapDataWrapper<MemoryMapData>;
  BorderSegment(const Point3f& f, const Point3f& t, const Vec3f& n, const DataType& data) :
    from(f), to(t), normal(n), extraData(data) {}
  
  // -- attributes
  Point3f from;
  Point3f to;
  // Note the normal could be embedded in the order 'from->to', but a separate variable makes it easier to use
  Vec3f normal; // perpendicular to the segment, in outwards direction with respect to the content.
  // additional information for this segment. Can be null if no additional data is available
  DataType extraData;
  
  // calculate segment center point
  inline Point3f GetCenter() const { return (from + to) * 0.5f; }
};

// each region detected between content types
struct BorderRegion {
  using BorderSegmentList = std::vector<BorderSegment>;
  BorderRegion() : area_m2(-1.0f) {}

  // when a region is finished (no more segments) we need to specify the area
  void Finish(float area) { area_m2 = area; };
  // deduct if the region is finished by checking the area
  bool IsFinished() const { return area_m2 >= 0.0f; }
  
  // -- attributes
  // area of the region in square meters
  float area_m2;
  // all the segments that define the given region (do not necessarily define a closed region)
  BorderSegmentList segments;
};

struct MapBroadcastData {
  MapBroadcastData() : mapInfo(), quadInfo() {}
  ExternalInterface::MemoryMapInfo                  mapInfo;
  std::vector<ExternalInterface::MemoryMapQuadInfo> quadInfo;
};

// Provide a custom hasher for unordered sets
template<class T>
struct MemoryMapDataHasher
{
  size_t operator()(const MemoryMapDataWrapper<T> & obj) const {
    return std::hash<std::shared_ptr<T>>()(obj.GetSharedPtr());
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Common Aliases
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using MemoryMapRegion        = QuadTreeTypes::FoldableRegion;

using MemoryMapDataPtr       = MemoryMapDataWrapper<MemoryMapData>;
using MemoryMapDataConstPtr  = MemoryMapDataWrapper<const MemoryMapData>;

using MemoryMapDataList      = std::unordered_set<MemoryMapDataPtr, MemoryMapDataHasher<MemoryMapData>>;
using MemoryMapDataConstList = std::unordered_set<MemoryMapDataConstPtr, MemoryMapDataHasher<const MemoryMapData>>;

using BorderRegionVector     = std::vector<BorderRegion>;
using NodeTransformFunction  = std::function<MemoryMapDataPtr (MemoryMapDataPtr)>;
using NodePredicate          = std::function<bool (MemoryMapDataConstPtr)>;

using QuadInfoVector         = std::vector<ExternalInterface::MemoryMapQuadInfo>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper Functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// returns false if the base constructor for MemoryMapData can be used with content type, and true if a derived 
// class constructor must be called, forcing additional data to be provided on instantiation
bool ExpectsAdditionalData(EContentType type);

// String representing ENodeContentType for debugging purposes
const char* EContentTypeToString(EContentType contentType);

// convert between our internal node content type and an external content type
ExternalInterface::ENodeContentTypeEnum ConvertContentType(EContentType contentType);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Array of content that provides an API with compilation checks for algorithms that require combinations
// of content types. It's for example used to make sure that you define a value for all content types, rather
// than including only those you want to be true.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

using FullContentArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<EContentType, bool>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray; // import IsSequentialArray to this namespace

// variable type in which we can pack EContentType as flags. Check ENodeContentTypeToFlag
using EContentTypePackedType = uint32_t;

// Converts EContentType values into flag bits. This is handy because I want to store EContentType in
// the smallest type possible since we have a lot of quad nodes, but I want to pass groups as bit flags in one
// packed variable
EContentTypePackedType EContentTypeToFlag(EContentType nodeContentType);

// returns true if contentType is in PackedTypes
bool IsInEContentTypePackedType(EContentType contentType, EContentTypePackedType contentPackedTypes);
  
  
static const MemoryMapTypes::FullContentArray kTypesThatAreObstacles =
{
  {MemoryMapTypes::EContentType::Unknown               , false},
  {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
  {MemoryMapTypes::EContentType::ObstacleObservable    , true },
  {MemoryMapTypes::EContentType::ObstacleProx          , true },
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
  {MemoryMapTypes::EContentType::Cliff                 , true },
  {MemoryMapTypes::EContentType::InterestingEdge       , false},
  {MemoryMapTypes::EContentType::NotInterestingEdge    , false}
};

} // namespace MemoryMapTypes
} // namespace Vector
} // namespace Anki

#endif // 
