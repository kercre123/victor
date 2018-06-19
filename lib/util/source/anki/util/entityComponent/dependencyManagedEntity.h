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

#ifndef __Util_EntityComponent_DependencyManagedEntity_H__
#define __Util_EntityComponent_DependencyManagedEntity_H__

#include "util/entityComponent/iDependencyManagedComponent.h"

#include "util/entityComponent/componentTypeEnumMap.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/logging/logging.h"

#include <cassert>
#include <map>
#include <set>

namespace Anki {

// forward declaration - TMP
namespace Cozmo{
class Robot;
}

template<typename EnumType>
class DependencyManagedEntity
{
public:
  using ComponentType = IDependencyManagedComponent<EnumType>;
  using DependentComponents = DependencyManagedEntity<EnumType>;
  
  DependencyManagedEntity() = default;

  // Note: If shouldManage is true ownership of the component is transfered to the entity
  // DO NOT delete or access the pointer directly after this call - access it through the entity's
  // GetComponent or GetComponentPtr calls
  void AddDependentComponent(EnumType enumID, 
                             ComponentType* component,
                             bool shouldManage = true);

  // Allow components to be replaced - dev only since we still have tons of caching all over the place
  void DevReplaceDependentComponent(EnumType enumID,
                                    ComponentType*& component,
                                    bool shouldManage);

  // Remove a component from the entity - if the component is managed it will be deleted
  void RemoveComponent(EnumType enumID);
  void ClearEntity();


  // Init all components in their declared dependency order
  void InitComponents(Cozmo::Robot* robot); //tmp for pass through

  // Update all components in their declared dependency order
  void UpdateComponents();

  template<typename T>
  bool HasComponent() const {
    EnumType enumID = EnumType::Count;
    GetComponentIDForType<EnumType,T>(enumID);
    auto iter = _components.find(enumID);
    return iter != _components.end(); // to do: is valid
  }


  template<typename T>
  T& GetComponent() const {
    EnumType enumID = EnumType::Count;
    GetComponentIDForType<EnumType,T>(enumID);
    return GetComponent(enumID). template GetComponent<T>();
  }

  template<typename T>
  T* GetComponentPtr() const {
    EnumType enumID = EnumType::Count;
    GetComponentIDForType<EnumType,T>(enumID);
    return GetComponent(enumID). template GetComponentPtr<T>();
  }

private:
  // can't store ptr directly in map
  struct ComponentPtrWrapper {
    ComponentPtrWrapper(ComponentType* ptr)
    : _ptr(ptr){}
    ComponentType* _ptr;
  };

  // The components map contains all component ptrs both managed and unmanaged
  std::map<EnumType, ComponentPtrWrapper> _components;
  // The managed components map is responsible for memory managment only - the ptrs
  // in managed component also exist in the _components array as raw ptrs. Only interact
  // with the managed components array when clearing or replacing managed components
  std::map<EnumType, std::unique_ptr<ComponentType>> _managedComponents;
  
  std::vector<std::pair<ComponentPtrWrapper,DependentComponents>> _cachedInitOrder;
  std::vector<std::pair<ComponentPtrWrapper,DependentComponents>> _cachedUpdateOrder;

  using OrderedDependentVector = std::vector<ComponentPtrWrapper>;

  
  const ComponentType& GetComponent(EnumType enumID) const;
  ComponentType& GetComponent(EnumType enumID);

  ComponentType* GetComponentPtr(EnumType enumID);

  // provide the ability to iterate over the entity
  OrderedDependentVector GetComponents();

  // Determine the order of component initialization
  OrderedDependentVector  GetInitDependentOrder();
  
  // Determine the order of component update
  OrderedDependentVector  GetUpdateDependentOrder();

  // Base function which uses a depth first search to topologically sort the order in which components
  // should be ticked based on the dependency map passed in
  OrderedDependentVector  GetDependentOrderBase(const std::map<EnumType, std::set<EnumType>>& dependencyMap);
  
  // Perform a recursive Depth First Search/Topological sort with the output
  // returned through the orderedVector parameter
  void DFS(EnumType baseEnum,
           const std::map<EnumType, std::set<EnumType>>& baseToDependentMap,
           std::set<EnumType>& currentRecursionStack,
           std::set<EnumType>& visitedNodes,
           OrderedDependentVector& orderedVector);

  // Create a component map based on the set of desired components passed in
  DependentComponents BuildDependentComponentsMap(std::set<EnumType>&& desiredComps);
};

////////
// Templated Function Definitions
////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedEntity<EnumType>::AddDependentComponent(EnumType enumID, 
                                                              ComponentType* component,
                                                              bool shouldManage)
{
  auto pair = DependencyManagedEntity<EnumType>::_components.emplace(
    std::make_pair(std::move(enumID), ComponentPtrWrapper(component)));
  ANKI_VERIFY(pair.second,
              "DependencyManagedEntity.AddDependentComponentManaged.FailedToInsert","Failed to insert enum");

  if(shouldManage){
    auto intentionalIDCopy = enumID;
    std::unique_ptr<ComponentType> up(component);
    auto pair = DependencyManagedEntity<EnumType>::_managedComponents.emplace(std::make_pair(std::move(intentionalIDCopy), std::move(up)));
    ANKI_VERIFY(pair.second,
                "DependencyManagedEntity.AddDependentComponentManaged.FailedToInsert","Failed to insert enum");
  }
}
 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedEntity<EnumType>::DevReplaceDependentComponent(EnumType enumID,
                                                                     ComponentType*& component,
                                                                     bool shouldManage)
{
  auto iter = DependencyManagedEntity<EnumType>::_components.find(enumID);
  if(iter != DependencyManagedEntity<EnumType>::_components.end()){
    DependencyManagedEntity<EnumType>::_components.erase(iter);
  }
  
  auto managedIter = DependencyManagedEntity<EnumType>::_managedComponents.find(enumID);
  if(managedIter != DependencyManagedEntity<EnumType>::_managedComponents.end()){
    DependencyManagedEntity<EnumType>::_managedComponents.erase(managedIter);
  }

  AddDependentComponent(enumID, component, shouldManage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedEntity<EnumType>::RemoveComponent(EnumType enumID)
{
  auto compIter = _components.find(enumID);
  if(compIter != _components.end()){
    _components.erase(compIter);
  }
  
  auto managedCompIter = _managedComponents.find(enumID);
  if(managedCompIter != _managedComponents.end()){
    _managedComponents.erase(managedCompIter);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedEntity<EnumType>::ClearEntity()
{
  _components.clear();
  _managedComponents.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType>
auto DependencyManagedEntity<EnumType>::GetComponent(EnumType enumID) const -> const ComponentType&
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY(iter != _components.end(),
              "Entity.GetComponent.InvalidGet","Enum not found");
  return *iter->second._ptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType>
auto DependencyManagedEntity<EnumType>::GetComponent(EnumType enumID) -> ComponentType&
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY(iter != _components.end(),
            "Entity.GetComponent.InvalidGet","Enum not found");
  return *iter->second._ptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType>
auto DependencyManagedEntity<EnumType>::GetComponentPtr(EnumType enumID) ->   ComponentType*
{
  auto iter = _components.find(enumID);
  ANKI_VERIFY(iter != _components.end(),
            "Entity.GetComponent.InvalidGet","Enum not found");
  return iter->second._ptr;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <typename EnumType>
auto DependencyManagedEntity<EnumType>::GetComponents() -> OrderedDependentVector{
  OrderedDependentVector vector;
  for(const auto& entry: _components){
    vector.push_back(entry.second);
  }
  return vector;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedEntity<EnumType>::InitComponents(Cozmo::Robot* robot)
{
  // Build the cache if necessary
  if(_cachedInitOrder.empty()){
    OrderedDependentVector orderedComponents = GetInitDependentOrder();
    PRINT_NAMED_INFO("DependencyManagedEntity.InitComponents.InitOrder",
                     "Components for entity %s will be initialized in the following order:",
                     GetEntityNameForEnumType<EnumType>().c_str());
    int initIdxForPrint = 0;
    for(const auto& ptrWrapper: orderedComponents){
      auto compPtr = ptrWrapper._ptr;
      // Print info
      EnumType enumID = EnumType::Count;
      compPtr->GetTypeDependent(enumID);
      PRINT_NAMED_INFO("DependencyManagedEntity.InitComponents.InitOrder",
                       "Component %d: %s %s",
                       initIdxForPrint, 
                       GetComponentStringForID<EnumType>(enumID).c_str(),
                       compPtr->IsUnreliableComponent() ? "- Unreliable Comp" : "");
      initIdxForPrint++;
      // Add component and all its dependencies to the cached map
      std::set<EnumType> componentNames;
      compPtr->GetInitDependencies(componentNames);
      compPtr->AdditionalInitAccessibleComponents(componentNames);
      DependentComponents comps = BuildDependentComponentsMap(std::move(componentNames));
      _cachedInitOrder.push_back(std::make_pair(ptrWrapper, std::move(comps)));
    }
  }
  // Initialize components;
  for(auto& entry: _cachedInitOrder){
    entry.first._ptr->InitDependent(robot, entry.second);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedEntity<EnumType>::UpdateComponents()
{
  // Build the cache if necessary
  if(_cachedUpdateOrder.empty()){
    OrderedDependentVector orderedComponents = GetUpdateDependentOrder();
    PRINT_NAMED_INFO("DependencyManagedEntity.UpdateComponents.UpdateOrder",
                     "Components for entity %s will be updated in the following order:",
                     GetEntityNameForEnumType<EnumType>().c_str());
    int initIdxForPrint = 0;
    for(const auto& ptrWrapper: orderedComponents){
      auto compPtr = ptrWrapper._ptr;
      // Print info
      EnumType enumID = EnumType::Count;
      compPtr->GetTypeDependent(enumID);
      PRINT_NAMED_INFO("DependencyManagedEntity.UpdateComponents.UpdateOrder",
                       "Component %d: %s %s",
                       initIdxForPrint, 
                       GetComponentStringForID<EnumType>(enumID).c_str(),
                       compPtr->IsUnreliableComponent() ? "- Unreliable Comp" : "");
      initIdxForPrint++;
      // Add component and all its dependencies to the cached map
      std::set<EnumType> componentNames;
      compPtr->GetUpdateDependencies(componentNames);
      compPtr->AdditionalUpdateAccessibleComponents(componentNames);
      DependentComponents comps = BuildDependentComponentsMap(std::move(componentNames));
      _cachedUpdateOrder.push_back(std::make_pair(ptrWrapper, std::move(comps)));
    }
  }
  // Update components;
  for(auto& entry: _cachedUpdateOrder){
    entry.first._ptr->UpdateDependent(entry.second);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
auto DependencyManagedEntity<EnumType>::GetInitDependentOrder() -> OrderedDependentVector
{
  // First convert all components into DependentManagedWrappers
  OrderedDependentVector unorderedWrappers = GetComponents();
  std::map<EnumType, std::set<EnumType>> dependencyMap;
  for(const auto& ptrWrapper: unorderedWrappers){
    // If you hit a memory issue here there's a good chance the class you're attempting
    // to access isn't inherriting from IDependencyManagedComponent correctly, but due to templates and casts it's being hidden
    EnumType type;
    auto compPtr = ptrWrapper._ptr;
    compPtr->GetTypeDependent(type);
    std::set<EnumType> componentNames;
    compPtr->GetInitDependencies(componentNames);
    dependencyMap.insert(std::make_pair(type, componentNames));
  }

  return GetDependentOrderBase(dependencyMap);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
auto DependencyManagedEntity<EnumType>::GetUpdateDependentOrder() -> OrderedDependentVector
{
  // First convert all components into DependentManagedWrappers
  OrderedDependentVector unorderedWrappers = GetComponents();
  std::map<EnumType, std::set<EnumType>> dependencyMap;
  for(const auto& ptrWrapper: unorderedWrappers){
    EnumType type;
    auto compPtr = ptrWrapper._ptr;
    compPtr->GetTypeDependent(type);
    std::set<EnumType> componentNames;
    compPtr->GetUpdateDependencies(componentNames);
    dependencyMap.insert(std::make_pair(type, componentNames));
  }

  return GetDependentOrderBase(dependencyMap);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
auto DependencyManagedEntity<EnumType>::GetDependentOrderBase(
      const std::map<EnumType, std::set<EnumType>>& dependencyMap) -> OrderedDependentVector
{
  // Ensure no components have declared dependence on an unreliable component
  for(const auto& basePair: dependencyMap){
    const auto& baseComponent = GetComponent(basePair.first);
    if(baseComponent.IsUnreliableComponent()){
      ANKI_VERIFY(basePair.second.empty(), 
                  "DependencyManagedEntitiy.GetDependentOrderBase.UnreliableComponentDeclaringDependencies", 
                  "");
    }
    for(auto dependentEnum: basePair.second){
      const auto& dependentComp = GetComponent(dependentEnum);
      ANKI_VERIFY(!dependentComp.IsUnreliableComponent(),
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
template<typename EnumType>
void DependencyManagedEntity<EnumType>::DFS(EnumType baseEnum,
                                                       const std::map<EnumType, std::set<EnumType>>& baseToDependentMap,
                                                       std::set<EnumType>& currentRecursionStack,
                                                       std::set<EnumType>& visitedNodes,
                                                       OrderedDependentVector& orderedVector)
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
  orderedVector.emplace_back(ComponentPtrWrapper(GetComponentPtr(std::move(baseEnum))));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
auto DependencyManagedEntity<EnumType>::BuildDependentComponentsMap(std::set<EnumType>&& desiredComps) ->
  typename DependencyManagedEntity<EnumType>::DependentComponents
{
  DependentComponents components;
  const bool shouldManage = false;
  for(auto entry: desiredComps){
    ComponentType* ptr = GetComponentPtr(entry);
    components.AddDependentComponent(entry, ptr, shouldManage);
  }
  return components;
}


} // namespace Anki

#endif // __Util_EntityComponent_DependencyManagedEntity_H__
