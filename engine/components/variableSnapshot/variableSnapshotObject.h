/*
 * File: variableSnapshotObject.h
 *
 * Author: Hamzah Khan
 * Created: 6/7/2018
 *
 * Description: This file defines an interface for objects that used to represent persistent data in the
 *              variable snapshot component. These objects each represent a single type, and the interface 
 *              provides a single function to convert the data to a JSON value.
 *
 *              This file also defines a templated class wrapping a shared_ptr that the variable snapshot
 *              component uses to store objects that enable it to generate JSON for saving persistent data.
 *
 * Copyright: Anki, Inc. 2018
 */

#include "json/json.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"

#include "clad/types/variableSnapshotIds.h"


#ifndef __Cozmo_Components_VariableSnapshotObject_H__
#define __Cozmo_Components_VariableSnapshotObject_H__

namespace Anki {
namespace Cozmo {

using SerializationFnType = std::function<bool(Json::Value&)>;

// This class wraps a function used to serialize the data that will be persisted across boots.
class VariableSnapshotObject {
public:
  VariableSnapshotObject(SerializationFnType serializeFn)
    :  _serializeFn(serializeFn) {};

  // serialization functionality
  bool Serialize(Json::Value& outJsonData) const { return _serializeFn(outJsonData); };

private:
  SerializationFnType _serializeFn;
};


} // Cozmo
} // Anki

#endif