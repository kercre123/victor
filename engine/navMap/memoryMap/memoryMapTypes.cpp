/**
 * File: navMemoryMapTypes.cpp
 *
 * Author: Raul
 * Date:   01/11/2016
 *
 * Description: Type definitions for the navMemoryMap.
 *
 * Copyright: Anki, Inc. 2015
 **/
#include "memoryMapTypes.h"

#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"

namespace Anki {
namespace Cozmo {
namespace MemoryMapTypes {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ExpectsAdditionalData(EContentType type)
{
  DEV_ASSERT(type != EContentType::_Count, "MemoryMapTypes.ExpectsAdditionalData.UsingControlTypeIsNotAllowed");
  
  // using switch to force at compilation type to decide on new types
  switch(type)
  {
    case EContentType::Unknown:
    case EContentType::ClearOfObstacle:
    case EContentType::ClearOfCliff:
    case EContentType::ObstacleCharger:
    case EContentType::ObstacleChargerRemoved:
    case EContentType::ObstacleUnrecognized:
    case EContentType::InterestingEdge:
    case EContentType::NotInterestingEdge:
    {
      return false;
    }
    case EContentType::ObstacleObservable:
    case EContentType::Cliff:
    case EContentType::ObstacleProx:
    {
      return true;
    }
    
    case EContentType::_Count:
    {
      return false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* EContentTypeToString(EContentType contentType)
{
  switch (contentType) {
    case EContentType::Unknown: return "Unknown";
    case EContentType::ClearOfObstacle: return "ClearOfObstacle";
    case EContentType::ClearOfCliff: return "ClearOfCliff";
    case EContentType::ObstacleObservable: return "ObstacleObservable";
    case EContentType::ObstacleCharger: return "ObstacleCharger";
    case EContentType::ObstacleChargerRemoved: return "ObstacleChargerRemoved";
    case EContentType::ObstacleProx: return "ObstacleProx";
    case EContentType::ObstacleUnrecognized: return "ObstacleUnrecognized";
    case EContentType::Cliff: return "Cliff";
    case EContentType::InterestingEdge: return "InterestingEdge";
    case EContentType::NotInterestingEdge: return "NotInterestingEdge";
    case EContentType::_Count: return "ERROR_COUNT_SHOULD_NOT_BE_USED";
  }
  return "ERROR";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EContentTypePackedType EContentTypeToFlag(EContentType contentType)
{
  const int contentTypeValue = Util::numeric_cast<int>( contentType );
  DEV_ASSERT(contentTypeValue < sizeof(EContentTypePackedType)*8, "ENodeContentTypeToFlag.InvalidContentType");
  const EContentTypePackedType flag = (1 << contentTypeValue);
  return flag;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IsInEContentTypePackedType(EContentType contentType, EContentTypePackedType contentPackedTypes)
{
  const EContentTypePackedType packedType = EContentTypeToFlag(contentType);
  const bool isIn = (packedType & contentPackedTypes) != 0;
  return isIn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IsRemovalType(EContentType type)
{
  using FullNodeContentToBoolArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<EContentType, bool>;
  constexpr FullNodeContentToBoolArray removalTypes =
  {
    {EContentType::Unknown               , false},
    {EContentType::ClearOfObstacle       , false},
    {EContentType::ClearOfCliff          , false},
    {EContentType::ObstacleObservable    , false},
    {EContentType::ObstacleCharger       , false},
    {EContentType::ObstacleChargerRemoved, true},
    {EContentType::ObstacleProx          , false},
    {EContentType::ObstacleUnrecognized  , false},
    {EContentType::Cliff                 , false},
    {EContentType::InterestingEdge       , false},
    {EContentType::NotInterestingEdge    , false}
  };
  static_assert(Util::FullEnumToValueArrayChecker::IsSequentialArray(removalTypes),
    "This array does not define all types once and only once.");

  // value of entry in array tells if it's a removal type
  const bool isRemoval = removalTypes[ Util::EnumToUnderlying(type) ].Value();
  return isRemoval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// convert between our internal node content type and an external content type
ExternalInterface::ENodeContentTypeEnum ConvertContentType(EContentType contentType)
{
  using namespace MemoryMapTypes;
  using namespace ExternalInterface;

  ENodeContentTypeEnum externalContentType = ENodeContentTypeEnum::Unknown;
  switch (contentType) {
    case EContentType::Unknown:               { externalContentType = ENodeContentTypeEnum::Unknown;              break; }
    case EContentType::ClearOfObstacle:       { externalContentType = ENodeContentTypeEnum::ClearOfObstacle;      break; }
    case EContentType::ClearOfCliff:          { externalContentType = ENodeContentTypeEnum::ClearOfCliff;         break; }
    case EContentType::ObstacleObservable:    { externalContentType = ENodeContentTypeEnum::ObstacleCube;         break; }
    case EContentType::ObstacleCharger:       { externalContentType = ENodeContentTypeEnum::ObstacleCharger;      break; }
    case EContentType::ObstacleChargerRemoved:{ DEV_ASSERT(false, "NavMeshQuadTreeNode.ConvertContentType");      break; } // Should never get this
    case EContentType::ObstacleProx:          { externalContentType = ENodeContentTypeEnum::ObstacleProx;         break; } 
    case EContentType::ObstacleUnrecognized:  { externalContentType = ENodeContentTypeEnum::ObstacleUnrecognized; break; }
    case EContentType::Cliff:                 { externalContentType = ENodeContentTypeEnum::Cliff;                break; }
    case EContentType::InterestingEdge:       { externalContentType = ENodeContentTypeEnum::InterestingEdge;      break; }
    case EContentType::NotInterestingEdge:    { externalContentType = ENodeContentTypeEnum::NotInterestingEdge;   break; }
    case EContentType::_Count:                { DEV_ASSERT(false, "NavMeshQuadTreeNode._Count"); break; }
  }
  return externalContentType;
}

} // namespace
} // namespace
} // namespace
