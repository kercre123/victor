/*
 * File: variableSnapshotSerializationFactory.h
 *
 * Author: Hamzah Khan
 * Created: 6/11/2018
 *
 * Description: This file provides static methods for (de)serialization of VariableSnapshotObjects.
 *              A separate pair of (de)serializers must be implemented for each type of VariableSnapshotObject.
 *
 * Copyright: Anki, Inc. 2018
 */

#ifndef __Engine_Component_VariableSnapshot_VariableSnapshotSerializationFactory_H__
#define __Engine_Component_VariableSnapshot_VariableSnapshotSerializationFactory_H__

#include "engine/components/variableSnapshot/variableSnapshotObject.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class VariableSnapshotSerializationFactory : private Util::noncopyable
{
public:
  // integers
  static bool SerializeInt(std::shared_ptr<int>, Json::Value&);
  static void DeserializeInt(std::shared_ptr<int>, const Json::Value&);

  // booleans
  static bool SerializeBool(std::shared_ptr<bool>, Json::Value&);
  static void DeserializeBool(std::shared_ptr<bool>, const Json::Value&);
};

}
}

#endif