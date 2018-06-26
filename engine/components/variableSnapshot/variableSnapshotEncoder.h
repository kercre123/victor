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


namespace VariableSnapshotEncoder {
  // encoder defines what the keys in the JSON are
  extern const char* kVariableSnapshotIdKey;
  extern const char* kVariableSnapshotKey;

  template <typename T>
  bool Serialize(std::shared_ptr<T>, Json::Value&);

  template <typename T>
  void Deserialize(std::shared_ptr<T>, const Json::Value&);
}

}
}

#endif