/**
*  iDependencyManagedComponent.h
*
*  Created by Kevin M. Karol on 12/1/17
*  Copyright (c) 2017 Anki, Inc. All rights reserved.
*
*  Provides base component classes for a version of entity/component model in which the
*  components are dependent on each other and require a specific update/initialization order
*  
**/

#ifndef __Util_EntityComponent_iDependencyManagedComponent_H__
#define __Util_EntityComponent_iDependencyManagedComponent_H__


#include "util/entityComponent/componentTypeEnumMap.h"
#include "util/logging/logging.h"

#include <map>
#include <set>

namespace Anki {

// forward declaration - TMP
namespace Cozmo{
class Robot;
}
template<typename EnumType>
class DependencyManagedEntity;

////////
// Interface for components that have their dependencies managed
////////
template<typename EnumType>
class IDependencyManagedComponent 
{
protected:
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
  IDependencyManagedComponent(T* derivedPtr,
                              EnumType componentEnum)
  : _derivedPtr(derivedPtr)
  , _type(componentEnum)
  {
    EnumType enumForTypeT = _type;
    GetComponentIDForType<EnumType,T>(enumForTypeT);
    ANKI_VERIFY(_type == enumForTypeT, 
                "IDependencyManagedComponent.IncorrectTypeToEnumMapping",
                "Data will be corrupted if you try to get this value");
  }
  
public:

  virtual ~IDependencyManagedComponent(){};
  // Get the type of the managed component
  void GetTypeDependent(EnumType& type) const { type = _type;}
  
  // Only override if the component can't be depended on since it is updated
  // through some other process
  virtual bool IsUnreliableComponent() const { return false;}

  // Init dependencies are guaranteed to be initialized before this component
  virtual void GetInitDependencies(std::set<EnumType>& dependencies) const {};

  // Additional components will be passed into init but may not have been initialized
  virtual void AdditionalInitAccessibleComponents(std::set<EnumType>& components) const{};

  // TMP - add in robot to make transition to new system easier for comps that previously
  // received robot - theoretically only dependent components should be necessary in the future
  virtual void InitDependent(Cozmo::Robot* robot, 
                             const DependencyManagedEntity<EnumType>& dependentComps) {};

  // Update dependencies are guaranteed to be updated before this component
  virtual void GetUpdateDependencies(std::set<EnumType>& dependencies) const {};

  // Additional components will be passed into update but may not have been updated
  virtual void AdditionalUpdateAccessibleComponents(std::set<EnumType>& components) const{};

  virtual void UpdateDependent(const DependencyManagedEntity<EnumType>& dependentComps) {};

  template<typename T>
  T& GetValue() const {
    EnumType enumID = EnumType::Count;
    GetComponentIDForType<EnumType,T>(enumID);
    ANKI_VERIFY(enumID == _type, 
                "DependencyManagedComponentWrapper.GetValue.CastingToIncorrectType", "");
    ANKI_VERIFY(IsValueValid(),"DependencyManagedComponentWrapper.GetValue.ValueIsNotValid",""); 
    auto* castPtr = static_cast<T*>(_derivedPtr);
    return *castPtr;
  }

  template<typename T>
  T* GetBasePtr() const {
    EnumType enumID = EnumType::Count;
    GetComponentIDForType<EnumType,T>(enumID);
    ANKI_VERIFY(enumID == _type, 
                "DependencyManagedComponentWrapper.GetBasePtr.CastingToIncorrectType", "");
    ANKI_VERIFY(IsValueValid(),"DependencyManagedComponentWrapper.GetBasePtr.ValueIsNotValid",""); 
    auto* castPtr = static_cast<T*>(_derivedPtr);
    return castPtr;
  }

  bool IsValueValid() const {return (_derivedPtr != nullptr) && IsValueValidInternal(); }
  virtual bool IsValueValidInternal() const {return true;}


private:
  void* _derivedPtr;
  EnumType _type;
};

// Component that can't be relied on by other components
// Often used for components that exist within multiple dependency managed entities
// If the top level entity determines update/init order, the nested level would be an
// unreliable component
template<typename EnumType>
class UnreliableComponent : public IDependencyManagedComponent<EnumType> {
protected:
  template<typename T>
  UnreliableComponent<EnumType>(T* derivedPtr, EnumType type)
  : IDependencyManagedComponent<EnumType>(derivedPtr, type){}

public:
  virtual bool IsUnreliableComponent() const override final { return true;}
  virtual void GetInitDependencies(std::set<EnumType>& dependencies) const override final {}
  virtual void AdditionalInitAccessibleComponents(std::set<EnumType>& components) const override final {};
  virtual void InitDependent(Cozmo::Robot* robot, 
                             const DependencyManagedEntity<EnumType>& dependentComps) override final {};
  virtual void GetUpdateDependencies(std::set<EnumType>& dependencies) const override final {};
  virtual void AdditionalUpdateAccessibleComponents(std::set<EnumType>& components) const override final {};
  virtual void UpdateDependent(const DependencyManagedEntity<EnumType>& dependentComps) override final {};
};

} // namespace Anki

#endif // __Util_EntityComponent_iDependencyManagedComponent_H__
