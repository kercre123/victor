/**
 * File: navMemoryMapTypes.h
 *
 * Author: Raul
 * Date:   01/11/2016
 *
 * Description: Type definitions for the navMemoryMap.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_TYPES_H
#define ANKI_COZMO_NAV_MEMORY_MAP_TYPES_H

#include "anki/common/basestation/math/point.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/helpers/templateHelpers.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

struct INavMemoryMapQuadData;

namespace NavMemoryMapTypes {

// content detected in the map
enum class EContentType : uint8_t {
  Unknown,               // not discovered
  ClearOfObstacle,       // an area without obstacles
  ClearOfCliff,          // an area without obstacles or cliffs
  ObstacleCube,          // an area with obstacles we recognize as cubes
  ObstacleCubeRemoved,   // an area that used to have a cube and now the cube has moved somewhere else
  ObstacleCharger,       // an area with obstacles we recognize as a charger
  ObstacleChargerRemoved,// an area that used to have a charger and now the charger has moved somewhere else
  ObstacleUnrecognized,  // an area with obstacles we do not recognize
  Cliff,                 // an area with cliffs or holes
  InterestingEdge,       // a border/edge detected by the camera
  NotInterestingEdge,    // a border/edge detected by the camera that we have already explored and it's not interesting anymore
  _Count // Flag, not a type
};

// this function returns true if the given content type expects additional data (iNavMemoryMapQuadData), false otherwise
bool ExpectsAdditionalData(EContentType type);

// each segment in a border region
struct BorderSegment
{
  using DataType = std::shared_ptr<const INavMemoryMapQuadData>;
  BorderSegment() : from{}, to{}, normal{}, extraData(nullptr) {}
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

using BorderRegionVector = std::vector<BorderRegion>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Array of content that provides an API with compilation checks for algorithms that require combinations
// of content types. It's for example used to make sure that you define a value for all content types, rather
// than including only those you want to be true.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
using FullContentArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<EContentType, bool>;
using Util::FullEnumToValueArrayChecker::IsSequentialArray; // import IsSequentialArray to this namespace

} // namespace
} // namespace
} // namespace

#endif // 
