/*
 * File: variableSnapshotEncoder.cpp
 *
 * Author: Hamzah Khan
 * Created: 6/11/2018
 *
 * Description: This file provides static methods for (de)serialization of VariableSnapshotObjects.
 *              A separate pair of (de)serializer must be implemented for each type of VariableSnapshotObject.
 *
 * Copyright: Anki, Inc. 2018
 */

#ifndef __Engine_Component_VariableSnapshot_VariableSnapshotEncoder__
#define __Engine_Component_VariableSnapshot_VariableSnapshotEncoder__

#include "engine/components/variableSnapshot/variableSnapshotEncoder.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/logging/logging.h"

#include <unordered_map>

#include "clad/types/behaviorComponent/behaviorIDs.h"
#include "clad/types/behaviorComponent/beiConditionTypes.h"


namespace Anki {
namespace Cozmo {
namespace VariableSnapshotEncoder {

// constants that are used in every (de)serializer
const char* kVariableSnapshotIdKey = "id";
const char* kVariableSnapshotKey = "data";

// constants that are used in specific (de)serializers
namespace TypeConstants {
// constants for maps
const char* kMapKey = "mapKey";
const char* kMapValueKey = "mapValue";
}

// integers
template<>
bool Serialize<int>(std::shared_ptr<int> dataPtr, Json::Value& outJson)
{
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

template<>
void Deserialize<int>(std::shared_ptr<int> dataPtr, const Json::Value& loadedJson)
{
  *dataPtr = loadedJson[kVariableSnapshotKey].asInt();
}


// booleans
template<>
bool Serialize<bool>(std::shared_ptr<bool> dataPtr, Json::Value& outJson)
{
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

template<>
void Deserialize<bool>(std::shared_ptr<bool> dataPtr, const Json::Value& loadedJson)
{
  *dataPtr = loadedJson[kVariableSnapshotKey].asBool();
}


// strings
template<>
bool Serialize<std::string>(std::shared_ptr<std::string> dataPtr, Json::Value& outJson)
{
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

template<>
void Deserialize<std::string>(std::shared_ptr<std::string> dataPtr, const Json::Value& loadedJson)
{
  *dataPtr = loadedJson[kVariableSnapshotKey].asString();
}


// unordered_map: string -> time_t
template<>
bool Serialize<std::unordered_map<std::string, time_t>>(std::shared_ptr<std::unordered_map<std::string, time_t>> dataPtr, 
                                                        Json::Value& outJson)
{
  Json::Value dataList;
  for(const auto& dataIter : *dataPtr) {
    Json::Value datum;
    std::string key = dataIter.first;
    time_t value_s = dataIter.second;

    datum[TypeConstants::kMapKey] = key;
    datum[TypeConstants::kMapValueKey] = (uint32_t) value_s;

    dataList.append(datum);
  }

  outJson[kVariableSnapshotKey] = std::move(dataList);  
  return true;
}

template<>
void Deserialize<std::unordered_map<std::string, time_t>>(std::shared_ptr<std::unordered_map<std::string, time_t>> dataPtr, 
                                                          const Json::Value& loadedJson)
{
  for(const auto& jsonData : loadedJson[kVariableSnapshotKey]) {

    std::string key = JsonTools::ParseString(jsonData, TypeConstants::kMapKey, "failed to deserialize key of umap: string -> time_t");
    time_t value_s = JsonTools::ParseUInt32(jsonData, TypeConstants::kMapValueKey, "failed to deserialize value of umap: string -> time_t");

    dataPtr->emplace(key, value_s);
  }
}


// unordered_map: BEIConditionType -> BehaviorID
template<>
bool Serialize<std::unordered_map<BEIConditionType, BehaviorID>>(std::shared_ptr<std::unordered_map<BEIConditionType, BehaviorID>> dataPtr, 
                                                                 Json::Value& outJson)
{
  Json::Value dataList;
  for(const auto& dataIter : *dataPtr) {
    Json::Value datum;
    BEIConditionType beiCondition = dataIter.first;
    BehaviorID behaviorId = dataIter.second;

    // save data as strings
    datum[TypeConstants::kMapKey] = BEIConditionTypeToString(beiCondition);
    datum[TypeConstants::kMapValueKey] = BehaviorIDToString(behaviorId);

    dataList.append(datum);
  }

  outJson[kVariableSnapshotKey] = std::move(dataList);  
  return true;
}

template<>
void Deserialize<std::unordered_map<BEIConditionType, BehaviorID>>(std::shared_ptr<std::unordered_map<BEIConditionType, BehaviorID>> dataPtr, 
                                                                   const Json::Value& loadedJson)
{
  for(const auto& jsonData : loadedJson[kVariableSnapshotKey]) {
    BEIConditionType key = BEIConditionType::Invalid;
    const std::string loadedBEICondString = JsonTools::ParseString(jsonData,
                                                                   TypeConstants::kMapKey,
                                                                   "failed to deserialize key of umap: BEIConditionType -> BehaviorID");
    const bool beiCondLoaded = BEIConditionTypeFromString(loadedBEICondString, key);
    if( !beiCondLoaded ) {
      PRINT_NAMED_WARNING("VariableSnapshotEncoder.DeserializeUMapBEICondToBehaviorID.UnknownStringinJson",
                          "Key %s was not recognized as a valid BEIConditionType value, will be dropped", loadedBEICondString.c_str());
    }

    BehaviorID value = BehaviorID::Anonymous;
    const std::string loadedBehaviorIdString = JsonTools::ParseString(jsonData,
                                                                      TypeConstants::kMapValueKey,
                                                                      "failed to deserialize value of umap: BEIConditionType -> BehaviorID");
    const bool behaviorIdLoaded = BehaviorIDFromString(loadedBehaviorIdString, value);
    if( !behaviorIdLoaded ) {
      PRINT_NAMED_WARNING("VariableSnapshotEncoder.DeserializeUMapBEICondToBehaviorID.UnknownStringinJson",
                          "Key %s was not recognized as a valid BehaviorID value, will be dropped", loadedBehaviorIdString.c_str());
    }


    if( beiCondLoaded && behaviorIdLoaded) {
      dataPtr->emplace(key, value);
    }
  }
}


}
}
}

#endif