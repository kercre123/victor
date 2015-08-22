/**
 * File: objectTypesAndIDs.h
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

#ifndef ANKI_CORETECH_COMMON_OBJECTTYPESANDIDS_H
#define ANKI_CORETECH_COMMON_OBJECTTYPESANDIDS_H

#include <limits>
#include <map>
#include <set>
#include <string>

namespace Anki {
  
  template<typename StorageType_>
  class UniqueEnumeratedValue
  {
  public:
    using StorageType = StorageType_;
    
    UniqueEnumeratedValue() : _value(UNKNOWN_VALUE) { }
    
    // Comparison and sorting (for use as key value)
    bool operator==(const UniqueEnumeratedValue& other) const { return _value == other._value; }
    bool operator!=(const UniqueEnumeratedValue& other) const { return _value != other._value; }
    bool operator< (const UniqueEnumeratedValue& other) const { return _value <  other._value; }
    
    bool operator==(const StorageType_ value) const { return _value == value; }
    bool operator!=(const StorageType_ value) const { return _value != value; }
    bool operator< (const StorageType value) const { return _value <  value; }

    //
    // The ability to assign from a value or from another UniqueEnumeratedValue
    // is a bit weird since it means you can have another one of these objects
    // in existance with the same ID (so it isn't really unique). This sort of
    // assignment should only be used as a means of selecting/finding a specific
    // UniqueEnumeratedValue.
    //
    
    // Assignment
    UniqueEnumeratedValue<StorageType>& operator=(const UniqueEnumeratedValue<StorageType>& other) {
      _value = other._value;
      return *this;
    }
    
    UniqueEnumeratedValue<StorageType>& operator=(const StorageType value) {
      _value = value;
      return *this;
    }
    
    bool IsUnknown() const { return _value == UNKNOWN_VALUE; }
    
    bool IsSet() const { return _value != UNKNOWN_VALUE; }
    
    void SetToUnknown() { _value = UNKNOWN_VALUE; }
    
    void UnSet() { _value = UNKNOWN_VALUE; }
    
    //void SetToAny() { _value = ANY_VALUE; }
    
    StorageType GetValue() const { return _value; }
    
    virtual operator StorageType() const { return _value; }
    
  protected:
    
    // Derived classes can choose whether to expose this ability or not
    void SetValue(StorageType newValue) { _value = newValue; }
    
  private:
    
    static const StorageType UNKNOWN_VALUE = -1;
    //static const StorageType ANY_VALUE     = std::numeric_limits<StorageType>::max();

    StorageType _value;

  }; // class UniqueEnumeratedValue
  
#if 0
  class ObjectType : public UniqueEnumeratedValue<int>
  {
  public:
    
    ObjectType();
    ~ObjectType();
    
    const std::string& GetName() const { return _name; }
    
    static StorageType GetNumTypes();
    
    static ObjectType GetInvalidType();
    
    static ObjectType GetTypeByName(const std::string& name);
    
  protected:
    
    // Derived classes can use this, but otherwise we do not want to provide a
    // way to construct a named ObjectType dynamically at runtime. That's the
    // whole point of this class (to be static unique named types).
    ObjectType(const std::string& name);
    
    static StorageType UniqueTypeCounter;
    
    // NOTE: Using static method with static variable inside to ensure the
    // set gets instantiated when needed instead of dangerously relying on static
    // initialization order.
    static std::map<std::string, ObjectType*>& GetTypeNames();
    
    std::string _name;
    
    // Note: No Set() for ObjectType
    
  }; // class ObjectType
#endif
  
  class ObjectID : public UniqueEnumeratedValue<int>
  {
  public:
    
    static StorageType UniqueIDCounter;
    
    static void Reset();
    
    void Set();
    
    using UniqueEnumeratedValue<int>::operator=;
    
  }; // class ObjectID
  
} // namespace Anki

#endif // ANKI_CORETECH_COMMON_OBJECTTYPESANDIDS_H