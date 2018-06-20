/*
 * File: variableSnapshotEncoder.h
 *
 * Author: Hamzah Khan
 * Created: 6/11/2018
 *
 * Description: This file provides static methods for (de)serialization of VariableSnapshotObjects.
 *              A separate pair of (de)serializers must be implemented for each type of VariableSnapshotObject.
 *
 * Copyright: Anki, Inc. 2018
 */

#ifndef __Engine_Component_VariableSnapshot_VariableSnapshotEncoder_H__
#define __Engine_Component_VariableSnapshot_VariableSnapshotEncoder_H__


#include "json/json.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {


class VariableSnapshotEncoder : private Util::noncopyable {
public:

  // encoder defines what the keys in the JSON are
  static const char* kVariableSnapshotIdKey;
  static const char* kVariableSnapshotKey;

  // integers
  static bool SerializeInt(std::shared_ptr<int>, Json::Value&);
  static void DeserializeInt(std::shared_ptr<int>, const Json::Value&);

  // booleans
  static bool SerializeBool(std::shared_ptr<bool>, Json::Value&);
  static void DeserializeBool(std::shared_ptr<bool>, const Json::Value&);

  // strings
  static bool SerializeString(std::shared_ptr<std::string>, Json::Value&);
  static void DeserializeString(std::shared_ptr<std::string>, const Json::Value&);
};

}
}

#endif