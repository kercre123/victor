/**
* File: behaviorSystemManager.h
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Manages and enforces the lifecycle and transitions
* of parts of the behavior system
*
* Copyright: Anki, Inc. 2017
**/

#ifndef COZMO_BEHAVIOR_SYSTEM_MANAGER_H
#define COZMO_BEHAVIOR_SYSTEM_MANAGER_H

#include "anki/common/types.h"
#include "clad/types/behaviorSystem/behaviorTypes.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior_fwd.h"
#include "json/json-forwards.h"

#include <set>
#include <unordered_map>

namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorExternalInterface;
class IBSRunnable;

struct BehaviorRunningInfo;

class BehaviorSystemManager
{
public:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  BehaviorSystemManager();
  ~BehaviorSystemManager();
  
  // initialize this behavior manager from the given Json config
  Result InitConfiguration(BehaviorExternalInterface& behaviorExternalInterface,
                           const Json::Value& behaviorSystemConfig);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // Calls the current behavior's Update() method until it returns COMPLETE or FAILURE.
  void Update(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool IsControlDelegated(const IBSRunnable* delegator);
  bool CanDelegate(IBSRunnable* delegator);
  bool Delegate(IBSRunnable* delegator, IBSRunnable* delegated);
  void CancelDelegates(IBSRunnable* delegator);
  void CancelSelf(IBSRunnable* delegator);
  
  IBehaviorPtr FindBehaviorByID(BehaviorID behaviorID) const;
  IBehaviorPtr FindBehaviorByExecutableType(ExecutableBehaviorType type) const;
  
private:
  enum class InitializationStage{
    SystemNotInitialized,
    StackNotInitialized,
    Initialized
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  InitializationStage _initializationStage;
  
  
  // - - - - - - - - - - - - - - -
  // others/shared
  // - - - - - - - - - - - - - - -
  
  std::vector<IBSRunnable*> _runnableStack;
  std::unordered_map<const IBSRunnable*, int> _runnableToIndexMap;
  std::map<IBSRunnable*,std::set<IBSRunnable*>> _delegatesMap;
  
  BehaviorExternalInterface* _behaviorExternalInterface;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  void PushOntoStack(IBSRunnable* runnable);
  void PopStack();
  
  void UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface, std::set<IBSRunnable*>& tickedInStack);
  void UpdateInActivatableScope(BehaviorExternalInterface& behaviorExternalInterface, const std::set<IBSRunnable*>& tickedInStack);
  
  // calls all appropriate functions to prep the delegates of something about to be added to the stack
  void PrepareDelegatesToEnterScope(IBSRunnable* delegated);
  
  // calls all appropriate functions to prepare a delegate to be removed from the stack
  void PrepareDelegateForRemovalFromStack(IBSRunnable* delegated);
  
  
}; // class BehaviorSystemManager

} // namespace Cozmo
} // namespace Anki


#endif // COZMO_BEHAVIOR_SYSTEM_MANAGER_H
