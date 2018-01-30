/**
*  dependencyManagedEntity.h
*
*  Created by Kevin M. Karol on 12/1/17
*  Copyright (c) 2017 Anki, Inc. All rights reserved.
*
*  Provides base entity class for a version of entity/component model in which the
*  entity manages its components' dependent ordered initialization/update
*  
**/

#ifndef __Engine_DependencyManagedEntity_H__
#define __Engine_DependencyManagedEntity_H__

#include "engine/dependencyManagedComponent.h"

#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/logging/logging.h"

#include <cassert>
#include <map>
#include <set>

//Transforms enum into string
#define COMPONENT_ID_TO_STRING(s) #s

namespace Anki {

// forward declaration - TMP
namespace Cozmo{
class Robot;
}

////////
// Entity - Manages a set of components all mapped to a single enum - calling Init or Update
// will result in the components being initialized/updated according to the dependency order
// they have declared
////////
template<typename EnumType, EnumType CountField>
class DependencyManagedEntity{
public:
  using ComponentType = DependencyManagedComponentWrapper<EnumType>;
  using OrderedDependentVector = std::vector<ComponentType>; 
  using DependentComponents = std::map<EnumType, ComponentType>;

  virtual void AddDependentComponent(EnumType&& enumID, ComponentType&& component);  

  // Allow components to be replaced - dev only since we still have tons of caching all over the place
  void DevReplaceDependentComponent(EnumType enumID, ComponentType component);

  bool HasComponent(EnumType enumID) const;
  
  const ComponentType& GetComponent(EnumType enumID) const;
  ComponentType& GetComponent(EnumType enumID);

  // provide the ability to iterate over the entity
  OrderedDependentVector GetComponents() const;

  // allow derived classes to verify that a GetAccess is valid
  virtual bool VerifyGetAccess(EnumType enumID) const {return true;}

  // Init all components in their declared dependency order
  void InitComponents(Cozmo::Robot* robot); //tmp for pass through

  // Update all components in their declared dependency order
  void UpdateComponents();

protected:
  std::map<EnumType, ComponentType> _components;

private:
  const EnumType _countField = CountField;
  std::vector<std::pair<ComponentType,DependentComponents>> _cachedInitOrder;
  std::vector<std::pair<ComponentType,DependentComponents>> _cachedUpdateOrder;

  // Determine the order of component initialization
  OrderedDependentVector  GetInitDependentOrder() const;
  
  // Determine the order of component update
  OrderedDependentVector  GetUpdateDependentOrder() const;

  // Base function which uses a depth first search to topologically sort the order in which components
  // should be ticked based on the dependency map passed in
  OrderedDependentVector  GetDependentOrderBase(const std::map<EnumType, std::set<EnumType>>& dependencyMap) const;
  
  // Perform a recursive Depth First Search/Topological sort with the output
  // returned through the orderedVector parameter
  void DFS(EnumType baseEnum,
           const std::map<EnumType, std::set<EnumType>>& baseToDependentMap,
           std::set<EnumType>& currentRecursionStack,
           std::set<EnumType>& visitedNodes,
           OrderedDependentVector& orderedVector) const;

  // Create a component map based on the set of desired components passed in
  DependentComponents BuildDependentComponentsMap(std::set<EnumType>&& desiredComps) const;
};


////////
// Entity Full Enumeration - Requires all values in an enum to have an associated dependency
// managed component declared - compile time failure if all enum values are not specified in the constructor
////////
template<typename EnumType, EnumType CountField>
class DependencyManagedEntityFullEnumeration : public DependencyManagedEntity<EnumType,CountField>{
public:
  using FullEntityArray = Util::FullEnumToValueArrayChecker::FullEnumToValueArray<EnumType, DependencyManagedComponentWrapper<EnumType>, CountField>;
  DependencyManagedEntityFullEnumeration(const FullEntityArray& array);
private:
  // All components must be passed into the fully enumerated constructor
  virtual void AddDependentComponent(EnumType&& enumID, 
                                     DependencyManagedComponentWrapper<EnumType>&& component) override {assert(false);}
};

////////
// Templated Function Definitions
////////


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
void DependencyManagedEntity<EnumType,CountField>::AddDependentComponent(EnumType&& enumID, 
                                                                         DependencyManagedComponentWrapper<EnumType>&& component)
{
  auto pair = DependencyManagedEntity<EnumType,CountField>::_components.insert(std::make_pair(std::move(enumID), std::move(component)));

  ANKI_VERIFY(pair.second,
              "DependencyManagedEntity.AddDependentComponent.FailedToInsert","Failed to insert enum %s",
              COMPONENT_ID_TO_STRING(enumID));
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
void DependencyManagedEntity<EnumType,CountField>::DevReplaceDependentComponent(EnumType enumID, ComponentType component)
{
  auto iter = DependencyManagedEntity<EnumType,CountField>::_components.find(enumID);
  if(ANKI_VERIFY((iter != DependencyManagedEntity<EnumType,CountField>::_components.end()),
                 "DependencyManagedEntity.DevReplaceDependentComponent.NotAlreadyInMap","")){
    iter->second = component;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, EnumType CountField>
bool DependencyManagedEntity<EnumType,CountField>::HasComponent(EnumType enumID) const
{
  auto iter = _components.find(enumID);
  return iter != _components.end();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, EnumType CountField>
const DependencyManagedComponentWrapper<EnumType>& DependencyManagedEntity<EnumType,CountField>::GetComponent(EnumType enumID) const
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY((iter != _components.end()) && VerifyGetAccess(enumID),
              "Entity.GetComponent.InvalidGet","Verify Failed for enum %s",
              COMPONENT_ID_TO_STRING(enumID));
  return iter->second;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, EnumType CountField>
DependencyManagedComponentWrapper<EnumType>& DependencyManagedEntity<EnumType,CountField>::GetComponent(EnumType enumID)
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY((iter != _components.end()) && VerifyGetAccess(enumID),
              "Entity.GetComponent.InvalidGet","Verify Failed for enum %s",
              COMPONENT_ID_TO_STRING(enumID));
  return iter->second;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType, EnumType CountField>
typename DependencyManagedEntity<EnumType,CountField>::OrderedDependentVector
 DependencyManagedEntity<EnumType,CountField>::GetComponents() const{
  std::vector<DependencyManagedComponentWrapper<EnumType>> vector;
  for(const auto& entry: _components){
    vector.push_back(entry.second);
  }
  return vector;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
void DependencyManagedEntity<EnumType,CountField>::InitComponents(Cozmo::Robot* robot)
{
  // Build the cache if necessary
  if(_cachedInitOrder.empty()){
    OrderedDependentVector orderedComponents = GetInitDependentOrder();
    for(const DependencyManagedComponentWrapper<EnumType>& entry: orderedComponents){
      const IDependencyManagedComponent<EnumType>& managedComponent = entry.GetManagedComp();
      std::set<EnumType> componentNames;
      managedComponent.GetInitDependencies(componentNames);
      managedComponent.AdditionalInitAccessibleComponents(componentNames);
      DependentComponents comps = BuildDependentComponentsMap(std::move(componentNames));
      _cachedInitOrder.push_back(std::make_pair(entry, comps));
    }
  }
  // Initialize components;
  for(auto& entry: _cachedInitOrder){
    entry.first.GetManagedComp().InitDependent(robot, entry.second);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
void DependencyManagedEntity<EnumType,CountField>::UpdateComponents()
{
  // Build the cache if necessary
  if(_cachedUpdateOrder.empty()){
    OrderedDependentVector orderedComponents = GetUpdateDependentOrder();
    for(const DependencyManagedComponentWrapper<EnumType>& entry: orderedComponents){
      const IDependencyManagedComponent<EnumType>& managedComponent = entry.GetManagedComp();
      std::set<EnumType> componentNames;
      managedComponent.GetUpdateDependencies(componentNames);
      managedComponent.AdditionalUpdateAccessibleComponents(componentNames);
      DependentComponents comps = BuildDependentComponentsMap(std::move(componentNames));
      _cachedUpdateOrder.push_back(std::make_pair(entry, comps));
    }
  }
  // Update components;
  for(auto& entry: _cachedUpdateOrder){
    entry.first.GetManagedComp().UpdateDependent(entry.second);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
std::vector<DependencyManagedComponentWrapper<EnumType>> DependencyManagedEntity<EnumType,CountField>::GetInitDependentOrder() const
{
  // First convert all components into DependentManagedWrappers
  using WrapperType = DependencyManagedComponentWrapper<EnumType>;
  std::vector<WrapperType> unorderedWrappers = GetComponents();
  std::map<EnumType, std::set<EnumType>> dependencyMap;
  for(const auto& entry: unorderedWrappers){
    // If you hit a memory issue here there's a good chance the class you're attempting
    // to access isn't inherriting from IDependencyManagedComponent correctly, but due to templates and casts it's being hidden
    const IDependencyManagedComponent<EnumType>& managedComponent = entry.GetManagedComp();
    EnumType type = _countField;
    managedComponent.GetTypeDependent(type);
    std::set<EnumType> componentNames;
    managedComponent.GetInitDependencies(componentNames);
    dependencyMap.insert(std::make_pair(type, componentNames));
  }

  return GetDependentOrderBase(dependencyMap);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
std::vector<DependencyManagedComponentWrapper<EnumType>> DependencyManagedEntity<EnumType,CountField>::GetUpdateDependentOrder() const
{
  // First convert all components into DependentManagedWrappers
  using WrapperType = DependencyManagedComponentWrapper<EnumType>;
  std::vector<WrapperType> unorderedWrappers = GetComponents();
  std::map<EnumType, std::set<EnumType>> dependencyMap;
  for(const auto& entry: unorderedWrappers){
    const IDependencyManagedComponent<EnumType>& managedComponent = entry.GetManagedComp();
    EnumType type = _countField;
    managedComponent.GetTypeDependent(type);
    std::set<EnumType> componentNames;
    managedComponent.GetUpdateDependencies(componentNames);
    dependencyMap.insert(std::make_pair(type, componentNames));
  }

  return GetDependentOrderBase(dependencyMap);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
std::vector<DependencyManagedComponentWrapper<EnumType>> DependencyManagedEntity<EnumType,CountField>
  ::GetDependentOrderBase(const std::map<EnumType, std::set<EnumType>>& dependencyMap) const
{
  // Ensure no components have declared dependence on an unreliable component
  for(const auto& basePair: dependencyMap){
    const auto& baseComponent = GetComponent(basePair.first);
    if(baseComponent.GetManagedComp().IsUnreliableComponent()){
      ANKI_VERIFY(basePair.second.empty(), 
                  "DependencyManagedEntitiy.GetDependentOrderBase.UnreliableComponentDeclaringDependencies", 
                  "");
    }
    for(auto dependentEnum: basePair.second){
      const auto& dependentComp = GetComponent(dependentEnum);
      ANKI_VERIFY(!dependentComp.GetManagedComp().IsUnreliableComponent(),
                  "DependencyManagedEntity.GetDependentOrderBase.DependencyDeclaringUnreliableComponent",
                  "");
    }
  }

  std::set<EnumType> visitedEnums;
  std::set<EnumType> currentRecursionStack;

  // Depth first search
  OrderedDependentVector orderedVector;
  for(const auto& entry: dependencyMap){
    DFS(entry.first, dependencyMap, currentRecursionStack, visitedEnums, orderedVector);
  }
  
  return orderedVector;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
void DependencyManagedEntity<EnumType,CountField>::DFS(EnumType baseEnum,
                                                       const std::map<EnumType, std::set<EnumType>>& baseToDependentMap,
                                                       std::set<EnumType>& currentRecursionStack,
                                                       std::set<EnumType>& visitedNodes,
                                                       OrderedDependentVector& orderedVector) const
{
  if(visitedNodes.find(baseEnum) != visitedNodes.end()){
    return;
  }

  const bool isBaseAlreadyInStack = (currentRecursionStack.find(baseEnum) != currentRecursionStack.end());
  if(isBaseAlreadyInStack){
    ANKI_VERIFY(false, "DependencyManagedEntity.DFS.CycleDetectedInGraph","");
    return;
  }

  const auto& mapEntry = baseToDependentMap.find(baseEnum);
  if(ANKI_VERIFY(mapEntry != baseToDependentMap.end(),
                 "DependencyManagedEntity.BaseEnumNotInMap","")){
    const std::set<EnumType> iter = mapEntry->second;
    for(const auto& base: iter){
      currentRecursionStack.insert(baseEnum);
      DFS(base, baseToDependentMap, currentRecursionStack, visitedNodes, orderedVector);
      currentRecursionStack.erase(baseEnum);
    }
  }

  visitedNodes.insert(baseEnum);
  orderedVector.push_back(GetComponent(baseEnum));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
typename DependencyManagedEntity<EnumType, CountField>::DependentComponents 
  DependencyManagedEntity<EnumType,CountField>::BuildDependentComponentsMap(std::set<EnumType>&& desiredComps) const
{
  DependentComponents components;
  for(const auto& entry: desiredComps){
    const auto& compWrapper = GetComponent(entry);
    components.insert(std::make_pair(entry, compWrapper));
  }
  return components;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType, EnumType CountField>
DependencyManagedEntityFullEnumeration<EnumType,CountField>::DependencyManagedEntityFullEnumeration(const FullEntityArray& array)
{
  ANKI_VERIFY(IsSequentialArray(array),
              "EntityFullEnumeration.Constructor.ArrayNotSequential","");
  for(auto& entry: array){
    DependencyManagedEntity<EnumType,CountField>::_components.insert(std::make_pair(entry.EnumValue(), entry.Value()));
  }
}



} // namespace Anki

#undef COMPONENT_ID_TO_STRING

#endif // __Engine_DependencyManagedEntity_H__
