#ifndef ANKI_CORETECH_COMMON_STRONGTYPEDEF_H
#define ANKI_CORETECH_COMMON_STRONGTYPEDEF_H

#include <limits>
#include <set>
#include <cassert>

namespace Anki {
  
  template<typename StorageType>
  class UniqueEnumeratedValue
  {
  public:
    
    UniqueEnumeratedValue() : _value(UNKNOWN_VALUE) { }
    
    bool operator==(const UniqueEnumeratedValue& other) const {
      return _value == other._value;
    }
    
    bool operator!=(const UniqueEnumeratedValue& other) const {
      return _value != other._value;
    }
    
    bool operator<(const UniqueEnumeratedValue& other) const {
      return _value < other._value;
    }

    UniqueEnumeratedValue<StorageType>& operator=(const UniqueEnumeratedValue<StorageType>& other) {
      _value = other._value;
      return *this;
    }
    
    bool IsUnknown() const {
      return _value == UNKNOWN_VALUE;
    }
    
    bool IsSet() const {
      return _value != UNKNOWN_VALUE;
    }
    
    void SetToUnknown() { _value = UNKNOWN_VALUE; }
    
    void UnSet() { _value = UNKNOWN_VALUE; }
    //void SetToAny() { _value = ANY_VALUE; }
    
    StorageType GetValue() const { return _value; }
    
    operator StorageType() const {
      return _value;
    }
    
  protected:
    
    // Derived classes can choose whether to expose this ability or not
    void SetValue(StorageType newValue) { _value = newValue; }
    
  private:
    
    static const StorageType UNKNOWN_VALUE = -1;
    //static const StorageType ANY_VALUE     = std::numeric_limits<StorageType>::max();

    StorageType _value;

  };
  
  class ObjectType : public UniqueEnumeratedValue<int>
  {
  public:
    using StorageType = int;
    
    ObjectType() {
      const int newType = UniqueTypeCounter++;
      SetValue(newType);
      //printf("Adding new type %d (set size = %lu)\n", newType, GetValidTypes().size());
      GetValidTypes().insert(newType);
    }
    
    ObjectType(int value)
    {
      if(GetValidTypes().count(value) > 0) {
        SetValue(value);
      } else {
        assert(false);
        SetToUnknown();
      }
    }
    
    static int GetNumTypes() {
      return UniqueTypeCounter;
    }
    
  protected:
    
    static StorageType UniqueTypeCounter;
    
    // NOTE: Using static method with static variable inside to ensure the
    // set gets instantiated when needed instead of dangerously relying on static
    // initialization order.
    static std::set<int>& GetValidTypes();
    
//    static std::set<int> ValidTypes;

    
    // Note: No Set() for ObjectType
    
  }; // class ObjectType
  
  
  class ObjectID : public UniqueEnumeratedValue<int>
  {
  public:
    using StorageType = int;
    
    static StorageType UniqueIDCounter;
    
    static void Reset() {
      UniqueIDCounter = 0;
    }
    
    void Set() {
      SetValue(UniqueIDCounter++);
    }
    
  };
  
  /*
template<typename Type>
class TypeWrapper
{
public:
  TypeWrapper()
  : TypeWrapper(0)
  {
    
  }
  
  TypeWrapper(Type value)
  : _value(value)
  {
    
  }
  
  operator Type() {
    return _value;
  }
  
  bool IsAnyValue() const {
    return _value == TypeWrapper<Type>::ANY_VALUE;
  }
  
  bool operator==(const TypeWrapper<Type>& other) const {
    return _value == other._value;
  }
  
  bool operator<(const TypeWrapper<Type>& other) const {
    return _value < other._value;
  }
  
  void Print() const {
    // TODO: Specialize for other types
    PRINT_INFO("%d", _value);
  }
  
private:
  static const Type ANY_VALUE = std::numeric_limits<Type>::max();
  Type _value;
  
};

struct ObjectID : public TypeWrapper<u16>
{
  ObjectID();
  ObjectID(u16 value) : TypeWrapper<u16>(value) { }
};

struct ObjectType : public TypeWrapper<u16>
{
  ObjectType();
  ObjectType(u16 value) : TypeWrapper<u16>(value) { }
};
  */
} // namespace Anki

#endif // ANKI_CORETECH_COMMON_STRONGTYPEDEF_H