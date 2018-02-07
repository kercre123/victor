/**
*  entity.h
*
*  Created by Kevin M. Karol on 12/1/17
*  Copyright (c) 2017 Anki, Inc. All rights reserved.
*
*  Defines an entity class which maps enums to components of 
*  an arbitrary type
*  
**/

#ifndef __Util_EntityComponent_Entity_H__
#define __Util_EntityComponent_Entity_H__

#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/logging/logging.h"

#include <cassert>
#include <map>

namespace Anki {

template<typename EnumType, typename ValueType>
class Entity{
public:
  virtual void AddComponent(EnumType enumID, ValueType component);
  ValueType& GetComponent(EnumType enumID);

  // provide the ability to iterate over the entity
  std::vector<ValueType&> GetComponents();

protected:
  std::map<EnumType, ValueType> _components;
};

/////////////
/// A class which requires full enumeration of all components at construction
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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, typename ValueType>
void Entity<EnumType,ValueType>::AddComponent(EnumType enumID, ValueType component)
{
  auto pair = _components.insert(std::make_pair(enumID, component));
  ANKI_VERIFY(pair.second,
              "Entity.AddComponent.FailedToInsert","Failed to insert enum");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, typename ValueType>
ValueType& Entity<EnumType,ValueType>::GetComponent(EnumType enumID)
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY(iter != _components.end(),
              "Entity.GetComponent.InvalidGet","Verify Failed for enum");
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

#endif // __Util_EntityComponent_Entity_H__
