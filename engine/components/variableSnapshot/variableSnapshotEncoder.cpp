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

// encoder defines what the keys in the JSON are
const char* VariableSnapshotEncoder::kVariableSnapshotIdKey = "variableSnapshotId";
const char* VariableSnapshotEncoder::kVariableSnapshotKey = "variableSnapshot";


// integers
bool VariableSnapshotEncoder::SerializeInt(std::shared_ptr<int> dataPtr, Json::Value& outJson)
{
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

void VariableSnapshotEncoder::DeserializeInt(std::shared_ptr<int> dataPtr, const Json::Value& loadedJson)
{
  *dataPtr = loadedJson[kVariableSnapshotKey].asInt();
}


// booleans
bool VariableSnapshotEncoder::SerializeBool(std::shared_ptr<bool> dataPtr, Json::Value& outJson)
{
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

void VariableSnapshotEncoder::DeserializeBool(std::shared_ptr<bool> dataPtr, const Json::Value& loadedJson)
{
  *dataPtr = loadedJson[kVariableSnapshotKey].asBool();
}


// strings
bool VariableSnapshotEncoder::SerializeString(std::shared_ptr<std::string> dataPtr, Json::Value& outJson)
{
  outJson[kVariableSnapshotKey] = *dataPtr;
  return true;
}

void VariableSnapshotEncoder::DeserializeString(std::shared_ptr<std::string> dataPtr, const Json::Value& loadedJson)
{
  *dataPtr = loadedJson[kVariableSnapshotKey].asString();
}


}
}

#endif