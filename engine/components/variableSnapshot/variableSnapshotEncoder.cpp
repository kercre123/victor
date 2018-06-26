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


namespace Anki {
namespace Cozmo {
namespace VariableSnapshotEncoder {

const char* kVariableSnapshotIdKey = "id";
const char* kVariableSnapshotKey = "data";

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


}
}
}

#endif