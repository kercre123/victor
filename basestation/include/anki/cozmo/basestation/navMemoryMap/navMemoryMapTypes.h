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
#include <vector>

namespace Anki {
namespace Cozmo {
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

// struct that defines a border
struct Border {
  Border() : from{}, to{}, normal{} {}
  Border(const Point3f& f, const Point3f& t, const Vec3f& n) : from(f), to(t), normal(n) {}
  Point3f from;
  Point3f to;
  Vec3f normal; // perpendicular to the segment, in outwards direction with respect to the content.
  // Note the normal could be embedded in the order 'from->to', but a separate variable makes it easier to use
  Point3f GetCenter() const { return (from + to) * 0.5f; }
};
using BorderVector = std::vector<Border>;


} // namespace
} // namespace
} // namespace

#endif // 
