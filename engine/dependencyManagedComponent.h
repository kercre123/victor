/**
*  dependencyManagedComponent.h
*
*  Created by Kevin M. Karol on 12/1/17
*  Copyright (c) 2017 Anki, Inc. All rights reserved.
*
*  Provides base component classes for a version of entity/component model in which the
*  components are dependent on each other and require a specific update/initialization order
*  
**/

#ifndef __Engine_DependencyManagedComponent_H__
#define __Engine_DependencyManagedComponent_H__

#include "util/logging/logging.h"

#include <map>
#include <set>

namespace Anki {

// forward declaration - TMP
namespace Cozmo{
class Robot;
}

// forward declaration
template<typename EnumType>
class DependencyManagedComponentWrapper;

////////
// Interface for components that have their dependencies managed
////////
template<typename EnumType>
class IDependencyManagedComponent {
public:
  IDependencyManagedComponent<EnumType>(EnumType type)
  : _type(type){}
  virtual ~IDependencyManagedComponent(){};
  // Get the type of the managed component
  void GetTypeDependent(EnumType& type) const { type = _type;}
  
  // Only override if the component can't be depended on since it is updated
  // through some other process
  virtual bool IsUnreliableComponent() const { return false;}

  // Init dependencies are guaranteed to be initialized before this component
  virtual void GetInitDependencies(std::set<EnumType>& dependencies) const = 0;

  // Additional components will be passed into init but may not have been initialized
  virtual void AdditionalInitAccessibleComponents(std::set<EnumType>& components) const{};

  // TMP - add in robot to make transition to new system easier for comps that previously
  // received robot - theoretically only dependent components should be necessary in the future
  virtual void InitDependent(Cozmo::Robot* robot, 
                             const std::map<EnumType, DependencyManagedComponentWrapper<EnumType>>& dependentComponents) = 0;

  // Update dependencies are guaranteed to be updated before this component
  virtual void GetUpdateDependencies(std::set<EnumType>& dependencies) const = 0;

  // Additional components will be passed into update but may not have been updated
  virtual void AdditionalUpdateAccessibleComponents(std::set<EnumType>& components) const{};

  virtual void UpdateDependent(const std::map<EnumType, DependencyManagedComponentWrapper<EnumType>>& dependentComponents) {};

private:
  EnumType _type;
};

// Component that can't be relied on by other components
// Often used for components that exist within multiple dependency managed entities
// If the top level entity determines update/init order, the nested level would be an
// unreliable component
template<typename EnumType>
class UnreliableComponent : public IDependencyManagedComponent<EnumType> {
public:
  UnreliableComponent<EnumType>(EnumType type)
  : IDependencyManagedComponent<EnumType>(type){}

  virtual bool IsUnreliableComponent() const override final { return true;}
  virtual void GetInitDependencies(std::set<EnumType>& dependencies) const override final {}
  virtual void AdditionalInitAccessibleComponents(std::set<EnumType>& components) const override final {};
  virtual void InitDependent(Cozmo::Robot* robot, 
                             const std::map<EnumType, DependencyManagedComponentWrapper<EnumType>>& dependentComponents) override final {};
  virtual void GetUpdateDependencies(std::set<EnumType>& dependencies) const override final {};
  virtual void AdditionalUpdateAccessibleComponents(std::set<EnumType>& components) const override final {};
  virtual void UpdateDependent(const std::map<EnumType, DependencyManagedComponentWrapper<EnumType>>& dependentComponents) override final {};
};


////////
// Wrapper - allows DependencyManagedComponents of different types to be mapped to the same enum
// and stored in the same entity since this class has its constructor templated on type rather than
// the full class
////////
template<typename EnumType>
class DependencyManagedComponentWrapper{
public:
  // To avoid bizarre memory bugs that can arise from converting back and forth
  // from void* to base vs derived classes (particularly when multiple inheritance is involved)
  // this class stores two versions of the ptr passed in
  // 1) the void* ptr comes directly from the templated type T and is returned directly
  // through a static cast to T in GetValue - so long as the base class is used in both cases all memory should be fine
  // 2) A ptr to the up-cast IDependencyManagedComponent. For simplicity this is always stored as a shared ptr
  // and a null deleter is passed in if the class has specified it doesn't want its memory managed.
  // Having an explicit up cast means a) a compile time failure if the component being passed in is not a valid
  // dependency managed component and b) no memory/cast issues when accessing IDependenceManagedComponent functions
  template<typename T>
  DependencyManagedComponentWrapper(T* componentPtr, const bool shouldManage = false)
  : _componentPtr(componentPtr){
    auto explicitUpCast = static_cast<IDependencyManagedComponent<EnumType>*>(componentPtr);
    if(shouldManage){
      _dependencyComponentPtr.reset(explicitUpCast);
    }else{
      auto nullDeleter = [](IDependencyManagedComponent<EnumType>* p){};
      _dependencyComponentPtr.reset(explicitUpCast, nullDeleter);
    }
  }

  virtual ~DependencyManagedComponentWrapper(){}

  void Init();
  void Update();

  bool IsValueValid() const {return (_componentPtr != nullptr) && IsValueValidInternal(); }
  virtual bool IsValueValidInternal() const {return true;}

  const IDependencyManagedComponent<EnumType>& GetManagedComp() const { return *_dependencyComponentPtr.get();}
  IDependencyManagedComponent<EnumType>& GetManagedComp(){ return *_dependencyComponentPtr.get();}

  template<typename T>
  T& GetValue() const { 
    ANKI_VERIFY(IsValueValid(),"DependencyManagedComponentWrapper.GetValue.ValueIsNotValid",""); 
    auto castPtr = static_cast<T*>(_componentPtr); 
    return *castPtr;
  }
  
private:
  void* _componentPtr;
  std::shared_ptr<IDependencyManagedComponent<EnumType>> _dependencyComponentPtr;
};

////////
// Templated Function Definitions
////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedComponentWrapper<EnumType>::Init()
{
  _dependencyComponentPtr->Init();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename EnumType>
void DependencyManagedComponentWrapper<EnumType>::Update()
{
  _dependencyComponentPtr->Update();
}


} // namespace Anki

#endif // __Engine_DependencyManagedComponent_H__
