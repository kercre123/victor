/**
 * File: objectTypesAndIDs.cpp
 *
 * Author: Andrew Stein
 * Date:   7/17/2014
 *
 * Description: Base classes for inheritable, strongly-typed, unique values.
 *              Not using regular enums allows them to be used more rigorously
 *              as keys for containers (without being interchangeable).  Not
 *              using C++11 enum classes allows them to be subclassed and
 *              extended with new "enumerated" values more easily.
 *
 *              - ObjectIDs are meant to be unique integer values for storing
 *                an identifier for an instance of an object.
 *
 *              - ObjectTypes are unique values from a given set (generally
 *                created by instantiating static const instances of the classes
 *                with a value coming from the current value of a static counter.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/basestation/objectIDs.h"
#include "util/logging/logging.h"

#include <cassert>

namespace Anki {
  
#if 0
  // Initialize static Type counter:
  ObjectType::StorageType ObjectType::UniqueTypeCounter = 0;
  
  ObjectType ObjectType::GetInvalidType()
  {
    static const ObjectType INVALID("INVALID");
    return INVALID;
  }
  
  std::map<std::string, ObjectType*>& ObjectType::GetTypeNames()
  {
    static std::map<std::string, ObjectType*> names;
    return names;
  }
  
  ObjectType::ObjectType(const std::string& name)
  : _name(name)
  {
    const StorageType newType = UniqueTypeCounter++;
    SetValue(newType);
    
    if(GetTypeNames().count(name) > 0) {
      PRINT_NAMED_WARNING("ObjectType.DuplicateTypeName",
                          "ObjectType named '%s' already exists.\n",
                          name.c_str());
    } else {
      GetTypeNames()[name] = this;
    }
  }

  ObjectType::ObjectType()
  {
    SetToUnknown();
  }
  
  ObjectType::~ObjectType()
  {

  }
  
  ObjectType::StorageType ObjectType::GetNumTypes() {
    return ObjectType::UniqueTypeCounter;
  }  
  
  ObjectType ObjectType::GetTypeByName(const std::string& name)
  {
    auto iter = GetTypeNames().find(name);
    if(iter == GetTypeNames().end() || iter->second == nullptr) {
      return GetInvalidType();
    } else {
      return *iter->second;
    }
  }
#endif
  
#pragma mark ---- ObjectID -----
  
  // Initialize static ID counter:
  ObjectID::StorageType ObjectID::UniqueIDCounter = 0;
  
  void ObjectID::Reset() {
    ObjectID::UniqueIDCounter = 0;
  }
  
  void ObjectID::Set() {
    SetValue(ObjectID::UniqueIDCounter++);
  }
  
} // namespace Anki