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
#include <unordered_map>

namespace Anki {
namespace Cozmo {
namespace VariableSnapshotEncoder {

// constants that are used in every (de)serializer
const char* kVariableSnapshotIdKey = "id";
const char* kVariableSnapshotKey = "data";

// constants that are used in specific (de)serializers
namespace TypeConstants {
// constants for unordered_map: string -> time_t
const char* kStringKey = "mapKey";
const char* kTimeTKey = "mapValue_s";
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

    datum[TypeConstants::kStringKey] = key;
    datum[TypeConstants::kTimeTKey] = (uint32_t) value_s;

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

    std::string key = JsonTools::ParseString(jsonData, TypeConstants::kStringKey, "failed to deserialize key of umap: string -> time_t");
    time_t value_s = JsonTools::ParseUInt32(jsonData, TypeConstants::kTimeTKey, "failed to deserialize value of umap: string -> time_t");

    dataPtr->emplace(key, value_s);
  }
}


}
}
}

#endif