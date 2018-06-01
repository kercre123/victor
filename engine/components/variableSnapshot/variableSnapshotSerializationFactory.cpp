/*
 * File: variableSnapshotSerializationFactory.cpp
 *
 * Author: Hamzah Khan
 * Created: 6/11/2018
 *
 * Description: This file provides static methods for (de)serialization of VariableSnapshotObjects.
 *              A separate pair of (de)serializer must be implemented for each type of VariableSnapshotObject.
 *
 * Copyright: Anki, Inc. 2018
 */

#ifndef __Engine_Component_VariableSnapshot_VariableSnapshotSerializationFactory__
#define __Engine_Component_VariableSnapshot_VariableSnapshotSerializationFactory__

#include "engine/components/variableSnapshot/variableSnapshotSerializationFactory.h"


namespace Anki {
namespace Cozmo {

const char* kVariableSnapshotIdKey = "variableSnapshotId";
const char* kVariableSnapshotKey = "variableSnapshot";

// integers
bool VariableSnapshotSerializationFactory::SerializeInt(std::shared_ptr<int> dataPtr, Json::Value& outJson) {
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

void VariableSnapshotSerializationFactory::DeserializeInt(std::shared_ptr<int> dataPtr, const Json::Value& loadedJson) {
  *dataPtr = loadedJson[kVariableSnapshotKey].asInt();
}


// booleans
bool VariableSnapshotSerializationFactory::SerializeBool(std::shared_ptr<bool> dataPtr, Json::Value& outJson) {
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

void VariableSnapshotSerializationFactory::DeserializeBool(std::shared_ptr<bool> dataPtr, const Json::Value& loadedJson) {
  *dataPtr = loadedJson[kVariableSnapshotKey].asBool();
}


// strings
bool VariableSnapshotSerializationFactory::SerializeString(std::shared_ptr<std::string> dataPtr, Json::Value& outJson) {
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

void VariableSnapshotSerializationFactory::DeserializeString(std::shared_ptr<std::string> dataPtr, const Json::Value& loadedJson) {
  *dataPtr = loadedJson[kVariableSnapshotKey].asString();
}


}
}

#endif