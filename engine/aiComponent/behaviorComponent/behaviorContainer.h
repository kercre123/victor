/**
 * File: behaviorContainer
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Container which creates and stores behaviors by ID
 * which were generated from data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <unordered_map>


namespace Json {
  class Value;
}


namespace Anki {
namespace Cozmo {

class BehaviorExternalInterface;
class IExternalInterface;
class IReactionaryBehavior;
class Robot;

class BehaviorContainer : public IDependencyManagedComponent<BCComponentID>, private Util::noncopyable
{
public:
  virtual ~BehaviorContainer();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override;
  virtual void UpdateDependent(const BCCompMap& dependentComps) override {};
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {
    dependencies.insert(BCComponentID::BehaviorExternalInterface);
  }
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////


  ICozmoBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  ICozmoBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;
  
  std::set<ICozmoBehaviorPtr> FindBehaviorsByClass(BehaviorClass behaviorClass) const;

  // Sometimes it's necessary to downcast to a behavior to a specific behavior pointer, e.g. so an Activity
  // can access its member functions. This function will help with that and provide a few assert checks along
  // the way. It sets outPtr in arguments, and returns true if the cast is successful
  template<typename T>
  bool FindBehaviorByIDAndDowncast(BehaviorID behaviorID,
                                   BehaviorClass requiredClass,
                                   std::shared_ptr<T>& outPtr ) const;
  // TODO:(bn) automatically infer requiredClass from T

  // Create a behavior from the given config. Config must specify a behavior ID and class. If the behavior is
  // successfully created and added to the container, True is returned. Errors include malformed JSON or
  // duplicate behavior IDs
  bool CreateAndStoreBehavior(const Json::Value& behaviorConfig);

  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);


  // BehaviorComponent should be the only class that directly interacts with the
  // container
  using BehaviorIDJsonMap = std::unordered_map<BehaviorID,  const Json::Value>;
  BehaviorContainer(const BehaviorIDJsonMap& behaviorData);

  void Init(BehaviorExternalInterface& behaviorExternalInterface);
protected:
  friend class BehaviorComponentMessageHandler;
  // Check to ensure that the factory only includes one behavior per executable
  // type
  void VerifyExecutableBehaviors() const;

  using BehaviorIDToBehaviorMap = std::map<BehaviorID, ICozmoBehaviorPtr>;

#if ANKI_DEV_CHEATS
  const BehaviorIDToBehaviorMap& GetBehaviorMap() const { return _idToBehaviorMap; }
#endif

private:
  IExternalInterface* _robotExternalInterface;

  // ============================== Private Member Funcs ==============================

  bool RemoveBehaviorFromMap(ICozmoBehaviorPtr behavior);

  // helper to avoid including ICozmoBehavior.h here
  BehaviorClass GetBehaviorClass(ICozmoBehaviorPtr behavior) const;

  // hide behaviorTypes.h file in .cpp
  std::string GetClassString(BehaviorClass behaviorClass) const;

  // ============================== Private Member Vars ==============================
  BehaviorIDToBehaviorMap _idToBehaviorMap;
  std::vector<Signal::SmartHandle> _signalHandles;


};

template<typename T>
bool BehaviorContainer::FindBehaviorByIDAndDowncast(BehaviorID behaviorID,
                                                    BehaviorClass requiredClass,
                                                    std::shared_ptr<T>& outPtr) const
{

  ICozmoBehaviorPtr behavior = FindBehaviorByID(behaviorID);
  if( ANKI_VERIFY(behavior != nullptr,
                  "BehaviorContainer.FindBehaviorByIDAndDowncast.NoBehavior",
                  "BehaviorID: %s requiredClass: %s",
                  BehaviorTypesWrapper::BehaviorIDToString(behaviorID),
                  GetClassString(requiredClass).c_str()) &&

     ANKI_VERIFY(behavior != nullptr && GetBehaviorClass(behavior) == requiredClass,
                 "BehaviorContainer.FindBehaviorByIDAndDowncast.WrongClass",
                 "BehaviorID: %s requiredClass: %s",
                 BehaviorTypesWrapper::BehaviorIDToString(behaviorID),
                 GetClassString(requiredClass).c_str()) ) {

       outPtr = std::static_pointer_cast<T>(behavior);

       if( ANKI_VERIFY(outPtr != nullptr, "BehaviorContainer.FindBehaviorByIDAndDowncast.CastFailed",
                       "BehaviorID: %s requiredClass: %s",
                       BehaviorTypesWrapper::BehaviorIDToString(behaviorID),
                       GetClassString(requiredClass).c_str()) ) {
         return true;
       }
     }

  return false;
}


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__
