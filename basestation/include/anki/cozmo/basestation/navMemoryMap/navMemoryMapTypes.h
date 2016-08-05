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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ContentValueEntry: struct that allows providing an API with type check for algorithms that require combinations
// of content types. It's for example used to make sure that you define a value for all content types, rather
// than including only those you want to be true.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct ContentValueEntry;
using FullContentArray = ContentValueEntry[Util::EnumToUnderlying(EContentType::_Count)];
struct ContentValueEntry {
  
  // returns true if all indices are valid in the given array
  static constexpr bool IsValidArray(const FullContentArray& array);
  
  // constexpr constructor
  constexpr ContentValueEntry(EContentType c, bool v) : content(c), value(v) {}
  
  // constexpr accessors
  constexpr EContentType Content() const { return content; }
  constexpr bool Value() const { return value; }
  
private:

  // returns true if the given index is valid in the given array (the entry's content matches the index)
  static constexpr bool IsValidIndex(const FullContentArray& array, size_t index);

  // returns true if the given index is valid, and all indices after it
  static constexpr bool IsValidAllIndicesFrom(const FullContentArray& array, size_t index);

  enum EContentType content;  // content for this entry
  bool value; // flag: whether you want this content type included in algorithms or not
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Inlines
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr bool ContentValueEntry::IsValidIndex(const FullContentArray& array, size_t index)
{
  return Util::EnumToUnderlying(array[index].content) == index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr bool ContentValueEntry::IsValidAllIndicesFrom(const FullContentArray& array, size_t index)
{
  return (index == Util::EnumToUnderlying(EContentType::_Count)) ? // if we reach the last one
          // true
          true :
          // else check this and +1
          (ContentValueEntry::IsValidIndex(array, index) && IsValidAllIndicesFrom(array, index+1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr bool ContentValueEntry::IsValidArray(const FullContentArray& array)
{
  return ContentValueEntry::IsValidAllIndicesFrom(array, 0);
}


} // namespace
} // namespace
} // namespace

#endif // 
