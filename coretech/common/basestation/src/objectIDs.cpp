/**
 * File: objectTypesAndIDs.cpp
 *
 * Author: Andrew Stein
 * Date:   7/17/2014
 *
 * Description: Base class for inheritable, strongly-typed, unique values, used
 *              for unique ObjectIDs for now. Using a full class instead of just
 *              an int provides for strong typing and runtime ID generation.
 *              Still, this is a bit gross and could probably be refactored/removed.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/basestation/objectIDs.h"
#include "util/logging/logging.h"

#include <cassert>

namespace Anki {
  
  // Initialize static ID counter:
  ObjectID::StorageType ObjectID::UniqueIDCounter = 0;
  
  void ObjectID::Reset() {
    ObjectID::UniqueIDCounter = 0;
  }
  
  void ObjectID::Set() {
    SetValue(ObjectID::UniqueIDCounter++);
  }
  
} // namespace Anki