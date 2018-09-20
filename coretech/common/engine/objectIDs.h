/**
 * File: objectIDs.h
 *
 * Author: Andrew Stein
 * Date:   7/17/2014
 *
 * Description: Base class for inheritable, strongly-typed, unique values, used
 *              for unique ObjectIDs for now. Using a full class instead of just
 *              an int provides for strong typing and runtime ID generation.
 *              Still, this is a bit gross and could probably be refactored/removed.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Anki_Common_ObjectIDs_H__
#define __Anki_Common_ObjectIDs_H__

#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>

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

    template<typename FwdType>
    explicit UniqueEnumeratedValue(FwdType&& value) : _value(std::forward<FwdType>(value)) { }

    bool IsUnknown() const { return _value == UNKNOWN_VALUE; }

    bool IsSet() const { return _value != UNKNOWN_VALUE; }

    void SetToUnknown() { _value = UNKNOWN_VALUE; }

    void UnSet() { _value = UNKNOWN_VALUE; }

    //void SetToAny() { _value = ANY_VALUE; }

    StorageType GetValue() const { return _value; }

    operator StorageType() const { return _value; }

  protected:

    // Derived classes can choose whether to expose this ability or not
    void SetValue(StorageType newValue) { _value = newValue; }

  private:

    static const StorageType UNKNOWN_VALUE = -1;
    //static const StorageType ANY_VALUE     = std::numeric_limits<StorageType>::max();

    StorageType _value;

  }; // class UniqueEnumeratedValue

  void ResetObjectIDCounter();

  class ObjectID : public UniqueEnumeratedValue<int>
  {
    friend void ResetObjectIDCounter();
  public:

    void Set();

    using UniqueEnumeratedValue<StorageType>::operator=;

    ObjectID() = default;
    ObjectID(StorageType value) : UniqueEnumeratedValue<StorageType>(value) { }

  private:
    static StorageType UniqueIDCounter;

  }; // class ObjectID

} // namespace Anki

#endif // __Anki_Common_ObjectIDs_H__
