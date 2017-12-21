/**
*  entity.h
*
*  Created by Kevin M. Karol on 12/1/17
*  Copyright (c) 2017 Anki, Inc. All rights reserved.
*
*  Provides base classes for entity/component systems that encorporate enums
*  
**/

#ifndef __Engine_Entity_H__
#define __Engine_Entity_H__

#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/logging/logging.h"

#include <cassert>
#include <map>

//Transforms enum into string
#define COMPONENT_ID_TO_STRING(s) #s

namespace Anki {

/////////////
/// Container that maintains an enum -> component map
/////////////
template<typename EnumType, typename ValueType>
class Entity{
public:
  virtual void AddComponent(EnumType enumID, ValueType component);
  ValueType& GetComponent(EnumType enumID);

  // provide the ability to iterate over the entity
  std::vector<ValueType&> GetComponents();

  // allow derrived classes to verify that a GetAccess is valid
  virtual bool VerifyGetAccess(EnumType enumID){return true;}

protected:
  std::map<EnumType, ValueType> _components;
};

/////////////
/// A class which requires full enumeration of all components
/////////////
template<typename EnumType, typename ValueType, EnumType CountField>
class EntityFullEnumeration : public Entity<EnumType,ValueType>{
public:
  using FullEntityArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<EnumType, ValueType, CountField>;
  EntityFullEnumeration(const FullEntityArray& array);

private:
  // All entities have already been set in the constructor
  virtual void AddComponent(EnumType enumID, ValueType component) override final { assert(false);}
};


/////////////
// Decorator for managed components
/////////////
class ManageableComponent{
public:
  virtual ~ManageableComponent(){};
};

/////////////
// A wrapper that allows arbitrary data to be stored as a component
// Wrapper can either manage its own memory or just maintain a reference
/////////////
class ComponentWrapper{
public:
  template<typename T>
  ComponentWrapper(T* componentPtr)
  : _componentPtr(componentPtr){}

  template<typename T>
  ComponentWrapper(T*& componentPtr, const bool shouldManage)
  : _componentPtr(componentPtr){
    if(shouldManage){
      _manageableComponent.reset(static_cast<ManageableComponent*>(componentPtr));
      componentPtr = nullptr;
    }else{
      ANKI_VERIFY(false,
                  "ComponentWrapper.ManagedConstructor.UsedWrongConstructor","");
    }
  }

  ~ComponentWrapper(){
  }

  bool IsValueValid() const {return (_componentPtr != nullptr) && IsValueValidInternal(); }
  virtual bool IsValueValidInternal() const {return true;}

  template<typename T>
  T& GetValue() const { 
    ANKI_VERIFY(IsValueValid(),"ComponentWrapper.GetValue.ValueIsNotValid",""); 
    auto castPtr = static_cast<T*>(_componentPtr); 
    return *castPtr;
  }

protected:
  // allow derrived classes to leave ptr uninitialized
  ComponentWrapper(){
  
  };

  void* _componentPtr = nullptr;

  std::shared_ptr<ManageableComponent> _manageableComponent;
}; // ComponentWrapper


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, typename ValueType>
void Entity<EnumType,ValueType>::AddComponent(EnumType enumID, ValueType component)
{
  auto pair = _components.insert(std::make_pair(enumID, component));
  ANKI_VERIFY(pair.second,
              "Entity.AddComponent.FailedToInsert","Failed to insert enum %s",
              COMPONENT_ID_TO_STRING(enumID));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, typename ValueType>
ValueType& Entity<EnumType,ValueType>::GetComponent(EnumType enumID)
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY((iter != _components.end()) && VerifyGetAccess(enumID),
              "Entity.GetComponent.InvalidGet","Verify Failed for enum %s",
              COMPONENT_ID_TO_STRING(enumID));
  return iter->second;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, typename ValueType>
std::vector<ValueType&> Entity<EnumType,ValueType>::GetComponents(){
  std::vector<ValueType&> vector;
  for(auto& entry: _components){
    vector.push_back(entry.second);
  }
  return vector;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, typename ValueType, EnumType CountField>
EntityFullEnumeration<EnumType,ValueType,CountField>::EntityFullEnumeration(const FullEntityArray& array)
{
  ANKI_VERIFY(IsSequentialArray(array),
              "EntityFullEnumeration.Constructor.ArrayNotSequential","");
  for(auto& entry: array){
    Entity<EnumType,ValueType>::_components.insert(std::make_pair(entry.EnumValue(), entry.Value()));
  }
}

} // namespace Anki

#endif // __Engine_Entity_H__
