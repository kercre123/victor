/**
 * File: navMeshQuadTreeTypes.cpp
 *
 * Author: Raul
 * Date:   01/13/2016
 *
 * Description: Type definitions for navMeshQuadTree.
 *
 * Copyright: Anki, Inc. 2016
 **/
#include "navMeshQuadTreeTypes.h"

#include "anki/cozmo/basestation/navMemoryMap/quadData/iNavMemoryMapQuadData.h"

#include "anki/common/basestation/exceptions.h"

#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {
namespace NavMeshQuadTreeTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator==(const NodeContent& other) const
{
  const bool equals = (type == other.type) &&
    ( (data == other.data) ||
      ((data != nullptr) && (other.data != nullptr) && (data->Equals(other.data.get())))
    );
  
  return equals;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NodeContent::operator!=(const NodeContent& other) const
{
  const bool ret = !(this->operator==(other));
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ENodeContentTypePackedType ENodeContentTypeToFlag(ENodeContentType nodeContentType)
{
  int contentTypeValue = Util::numeric_cast<int>( nodeContentType );
  CORETECH_ASSERT( contentTypeValue < sizeof(ENodeContentTypePackedType)*8 );
  const ENodeContentTypePackedType flag = 1 << contentTypeValue;
  return flag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ENodeContentTypeToString(ENodeContentType nodeContentType)
{
  switch (nodeContentType) {
    case ENodeContentType::Invalid: return "Invalid";
    case ENodeContentType::Subdivided: return "Subdivided";
    case ENodeContentType::Unknown: return "Unknown";
    case ENodeContentType::ClearOfObstacle: return "ClearOfObstacle";
    case ENodeContentType::ClearOfCliff: return "ClearOfCliff";
    case ENodeContentType::ObstacleCube: return "ObstacleCube";
    case ENodeContentType::ObstacleCubeRemoved: return "ObstacleCubeRemoved";
    case ENodeContentType::ObstacleCharger: return "ObstacleCharger";
    case ENodeContentType::ObstacleChargerRemoved: return "ObstacleChargerRemoved";
    case ENodeContentType::ObstacleUnrecognized: return "ObstacleUnrecognized";
    case ENodeContentType::Cliff: return "Cliff";
    case ENodeContentType::InterestingEdge: return "InterestingEdge";
    case ENodeContentType::NotInterestingEdge: return "NotInterestingEdge";
  }
  return "ERROR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* EDirectionToString(EDirection dir)
{
  switch (dir) {
    case NavMeshQuadTreeTypes::EDirection::North:   { return "N"; };
    case NavMeshQuadTreeTypes::EDirection::East:    { return "E"; };
    case NavMeshQuadTreeTypes::EDirection::South:   { return "S"; };
    case NavMeshQuadTreeTypes::EDirection::West:    { return "W"; };
    case NavMeshQuadTreeTypes::EDirection::Invalid: { return "Invalid"; };
  }
  return "Error";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Vec3f EDirectionToNormalVec3f(EDirection dir)
{
  switch (dir) {
    case NavMeshQuadTreeTypes::EDirection::North:   { return Vec3f{ 1.0f,  0.0f, 0.0f}; };
    case NavMeshQuadTreeTypes::EDirection::East:    { return Vec3f{ 0.0f, -1.0f, 0.0f}; };
    case NavMeshQuadTreeTypes::EDirection::South:   { return Vec3f{-1.0f,  0.0f, 0.0f}; };
    case NavMeshQuadTreeTypes::EDirection::West:    { return Vec3f{ 0.0f,  1.0f, 0.0f}; };
    case NavMeshQuadTreeTypes::EDirection::Invalid: {};
  }
  
  CORETECH_ASSERT(!"Invalid direction");
  return Vec3f{0.0f, 0.0f, 0.0f};
}


} // namespace
} // namespace
} // namespace
