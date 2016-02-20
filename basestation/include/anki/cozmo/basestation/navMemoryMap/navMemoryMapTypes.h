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

#include <cstdint>
#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

struct INavMemoryMapQuadData;

namespace NavMemoryMapTypes {

// content detected in the map
enum class EContentType : uint8_t {
  Unknown,              // not discovered
  ClearOfObstacle,      // an area without obstacles
  ClearOfCliff,         // an area without obstacles or cliffs
  ObstacleCube,         // an area with obstacles we recognize as cubes
  ObstacleUnrecognized, // an area with obstacles we do not recognize
  Cliff,                // an area with cliffs or holes
};

// this function returns true if the given content type expects additional data (iNavMemoryMapQuadData), false otherwise
bool ExpectsAdditionalData(EContentType type);

// struct that defines a border
struct Border
{
  using DataType = std::shared_ptr<INavMemoryMapQuadData>;
  Border() : from{}, to{}, normal{}, extraData(nullptr) {}
  Border(const Point3f& f, const Point3f& t, const Vec3f& n, const DataType& data) : from(f), to(t), normal(n), extraData(data) {}
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
using BorderVector = std::vector<Border>;

} // namespace
} // namespace
} // namespace

#endif // 
