/**
 * File: memoryMapData.cpp
 *
 * Author: rpss
 * Date:   apr 12 2018
 *
 * Description: Base for data structs that will be held in every node depending on their content type.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "engine/navMap/memoryMap/data/memoryMapData.h"
#include "engine/navMap/memoryMap/data/memoryMapData_ProxObstacle.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemoryMapData::CanOverrideSelfWithContent(MemoryMapDataConstPtr newContent, bool centerContainedByROI) const
{
  EContentType newContentType = newContent->type;
  EContentType dataType = type;
  if ( newContentType == EContentType::Cliff )
  {
    // Cliff can override any other
    return true;
  }
  else if ( dataType == EContentType::Cliff )
  {
    // Cliff can only be overridden by a full ClearOfCliff (the cliff is gone)
    const bool isTotalClear = (newContentType == EContentType::ClearOfCliff);
    return isTotalClear;
  }
  else if ( newContentType == EContentType::ClearOfObstacle )
  {
    // ClearOfObstacle currently comes from vision or prox sensor having a direct line of sight
    // to some object, so it can't clear negative space obstacles (IE cliffs). Additionally,
    // ClearOfCliff is currently a superset of Clear of Obstacle, so trust ClearOfCliff flags.
    const bool isTotalClear = ( dataType != EContentType::Cliff ) &&
                              ( dataType != EContentType::ClearOfCliff );
    // only mark prox obstacles as clear if their centers are within the insertion area. this is
    // because we clear prox obstacles within a poly drawn from the robot to the sensor reading
    // position, and then place a prox obstacle on the other side of that poly. the lowest level
    // quads may not be fully contained within the clearing poly. if we allow both clearing and
    // placing of prox obstacles without this restriction, those on/near the border will flicker
    // between being clear and new. this prevents that
    const bool nonContainedProx = (dataType == EContentType::ObstacleProx) && !centerContainedByROI;
    return isTotalClear && !nonContainedProx;
  }
  else if ( newContentType == EContentType::InterestingEdge )
  {
    // InterestingEdge can only override basic node types, because it would cause data loss otherwise. For example,
    // we don't want to override a recognized marked cube or a cliff with their own border
    if ( ( dataType == EContentType::ObstacleObservable   ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::ObstacleUnrecognized ) ||
         ( dataType == EContentType::Cliff                ) ||
         ( dataType == EContentType::NotInterestingEdge   ) )
    {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleProx )
  {
    if ( ( dataType == EContentType::ObstacleObservable   ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::Cliff                ) )
    {
      return false;
    }
    // an unexplored prox obstacle shouldnt replace an explored prox obstacle
    if( dataType == EContentType::ObstacleProx ) {
      DEV_ASSERT( dynamic_cast<const MemoryMapData_ProxObstacle*>(this) != nullptr, "MemoryMapData.CanOverride.InvalidCast" );
      auto castPtr = static_cast<const MemoryMapData_ProxObstacle*>(this);
      return castPtr->_explored != MemoryMapData_ProxObstacle::EXPLORED;
    }
  }
  else if ( newContentType == EContentType::NotInterestingEdge )
  {
    // NotInterestingEdge can only override interesting edges
    if ( dataType != EContentType::InterestingEdge ) {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleChargerRemoved )
  {
    // ObstacleChargerRemoved can only remove ObstacleCharger
    if ( dataType != EContentType::ObstacleCharger ) {
      return false;
    }
  }
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ExternalInterface::ENodeContentTypeEnum MemoryMapData::GetExternalContentType() const
{
  using namespace MemoryMapTypes;
  using namespace ExternalInterface;
  
  ENodeContentTypeEnum externalContentType = ENodeContentTypeEnum::Unknown;
  switch (type) {
    case EContentType::Unknown:               { externalContentType = ENodeContentTypeEnum::Unknown;              break; }
    case EContentType::ClearOfObstacle:       { externalContentType = ENodeContentTypeEnum::ClearOfObstacle;      break; }
    case EContentType::ClearOfCliff:          { externalContentType = ENodeContentTypeEnum::ClearOfCliff;         break; }
    case EContentType::ObstacleCharger:       { externalContentType = ENodeContentTypeEnum::ObstacleCharger;      break; }
    case EContentType::ObstacleChargerRemoved:{ DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType");      break; } // Should never get this
    case EContentType::ObstacleUnrecognized:  { externalContentType = ENodeContentTypeEnum::ObstacleUnrecognized; break; }
    case EContentType::InterestingEdge:       { externalContentType = ENodeContentTypeEnum::InterestingEdge;      break; }
    case EContentType::NotInterestingEdge:    { externalContentType = ENodeContentTypeEnum::NotInterestingEdge;   break; }
    // handled elsewhere
    case EContentType::ObstacleObservable:
    case EContentType::Cliff:
    case EContentType::ObstacleProx:
    case EContentType::_Count:
    {
      DEV_ASSERT(false, "NavMeshQuadTreeNode._Count");
    }
      break;
  }
  return externalContentType;
}


} // namespace Cozmo
} // namespace Anki
