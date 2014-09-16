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
  
  template<typename StorageType>
  class UniqueEnumeratedValue
  {
  public:
    
    UniqueEnumeratedValue() : _value(UNKNOWN_VALUE) { }
    
    // Comparison and sorting (for use as key value)
    bool operator==(const UniqueEnumeratedValue& other) const { return _value == other._value; }
    bool operator!=(const UniqueEnumeratedValue& other) const { return _value != other._value; }
    bool operator< (const UniqueEnumeratedValue& other) const { return _value <  other._value; }

    // Assignment
    UniqueEnumeratedValue<StorageType>& operator=(const UniqueEnumeratedValue<StorageType>& other) {
      _value = other._value;
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
  
  class ObjectType : public UniqueEnumeratedValue<int>
  {
  public:
    using StorageType = int;
    
    ObjectType();
    ObjectType(const std::string& name);
    ObjectType(int value);
    
    ~ObjectType();
    
    const std::string& GetName() const { return _name; }
    
    static int GetNumTypes();
    
    static ObjectType GetInvalidType();
    
    static ObjectType GetTypeByName(const std::string& name);
    
  protected:
    
    static StorageType UniqueTypeCounter;
    
    // NOTE: Using static method with static variable inside to ensure the
    // set gets instantiated when needed instead of dangerously relying on static
    // initialization order.
    static std::set<int>& GetValidTypes();
    
    std::string _name;
    
    // Note: No Set() for ObjectType
    
  }; // class ObjectType
  
  
  class ObjectID : public UniqueEnumeratedValue<int>
  {
  public:
    using StorageType = int;
    
    static StorageType UniqueIDCounter;
    
    static void Reset();
    
    void Set();
    
  }; // class ObjectID
  
} // namespace Anki

#endif // ANKI_CORETECH_COMMON_OBJECTTYPESANDIDS_H