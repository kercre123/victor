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


#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/noncopyable.h"
#include "util/logging/logging.h"
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
  
class BehaviorContainer : private Util::noncopyable
{
public:
  ~BehaviorContainer();
  
  IBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  IBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;
  
  // Sometimes it's necessary to downcast to a behavior to a specific behavior pointer, e.g. so an Activity
  // can access it's member functions. This function will help with that and provide a few assert checks along
  // the way. It sets outPtr in arguemnts, and returns true if the cast is successful
  template<typename T>
  bool FindBehaviorByIDAndDowncast(BehaviorID behaviorID,
                                   BehaviorClass requiredClass,
                                   std::shared_ptr<T>& outPtr ) const;
  // TODO:(bn) automatically infer requiredClass from T
  
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);
  
protected:
  // AIComponent should be the only clas that directly interacts with the
  // container
  friend class AIComponent;
  
  using BehaviorIDJsonMap = std::unordered_map<BehaviorID,  const Json::Value>;
  BehaviorContainer(const BehaviorIDJsonMap& behaviorData);
  void Init(BehaviorExternalInterface& behaviorExternalInterface,
            const bool shouldAddToActivatableScope);
  
  // Check to ensure that the factory only includes one behavior per executable
  // type
  void VerifyExecutableBehaviors() const;
  
  using BehaviorIDToBehaviorMap = std::map<BehaviorID, IBehaviorPtr>;

  IBehaviorPtr CreateBehavior(BehaviorClass behaviorType, const Json::Value& config);
  IBehaviorPtr CreateBehavior(const Json::Value& behaviorJson);
  
#if ANKI_DEV_CHEATS
  const BehaviorIDToBehaviorMap& GetBehaviorMap() const { return _idToBehaviorMap; }
#endif
  
private:
  IExternalInterface* _robotExternalInterface;

  // ============================== Private Member Funcs ==============================
  
  // NOTE: can modify newBehavior (e.g. on name collision if rule is to reuse existing behavior)
  IBehaviorPtr AddToFactory(IBehaviorPtr newBehavior);
  
  bool RemoveBehaviorFromMap(IBehaviorPtr behavior);
  
  // ============================== Private Member Vars ==============================
  BehaviorIDToBehaviorMap _idToBehaviorMap;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  // helper to avoid including iBehavior.h here
  BehaviorClass GetBehaviorClass(IBehaviorPtr behavior) const;
};

template<typename T>
bool BehaviorContainer::FindBehaviorByIDAndDowncast(BehaviorID behaviorID,
                                                    BehaviorClass requiredClass,
                                                    std::shared_ptr<T>& outPtr) const
{
  
  IBehaviorPtr behavior = FindBehaviorByID(behaviorID);
  if( ANKI_VERIFY(behavior != nullptr,
                  "BehaviorContainer.FindBehaviorByIDAndDowncast.NoBehavior",
                  "BehaviorID: %s requiredClass: %s",
                  BehaviorIDToString(behaviorID),
                  BehaviorClassToString(requiredClass)) &&
     
     ANKI_VERIFY(behavior != nullptr && GetBehaviorClass(behavior) == requiredClass,
                 "BehaviorContainer.FindBehaviorByIDAndDowncast.WrongClass",
                 "BehaviorID: %s requiredClass: %s",
                 BehaviorIDToString(behaviorID),
                 BehaviorClassToString(requiredClass)) ) {
       
       outPtr = std::static_pointer_cast<T>(behavior);
       
       if( ANKI_VERIFY(outPtr != nullptr, "BehaviorContainer.FindBehaviorByIDAndDowncast.CastFailed",
                       "BehaviorID: %s requiredClass: %s",
                       BehaviorIDToString(behaviorID),
                       BehaviorClassToString(requiredClass)) ) {
         return true;
       }
     }
  
  return false;
}
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorContainer_H__

