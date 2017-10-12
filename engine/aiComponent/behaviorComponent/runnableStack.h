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

#ifndef AI_COMPONENT_BEHAVIOR_COMPONENT_RUNNABLE_STACK
#define AI_COMPONENT_BEHAVIOR_COMPONENT_RUNNABLE_STACK

#include "util/helpers/noncopyable.h"

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {

// forward declarations
class AsyncMessageGateComponent;
class BehaviorExternalInterface;
class IBehavior;
  
namespace ExternalInterface{
struct RobotCompletedAction;
}
  
// Defined within .h file so tests can access runnable stack functions
class RunnableStack : private Util::noncopyable {
public:
  RunnableStack(BehaviorExternalInterface* behaviorExternalInterface)
  :_behaviorExternalInterface(behaviorExternalInterface){};
  virtual ~RunnableStack(){}
  
  void InitRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                         IBehavior* baseOfStack);
  void UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                          std::vector<ExternalInterface::RobotCompletedAction>& actionsCompletedThisTick,
                           AsyncMessageGateComponent& asyncMessageGateComp,
                           std::set<IBehavior*>& tickedInStack);
  
  inline IBehavior* GetTopOfStack(){ return _runnableStack.empty() ? nullptr : _runnableStack.back();}
  inline bool IsInStack(const IBehavior* runnable) { return _runnableToIndexMap.find(runnable) != _runnableToIndexMap.end();}
  
  void PushOntoStack(IBehavior* runnable);
  void PopStack();
  
  using DelegatesMap = std::map<IBehavior*,std::set<IBehavior*>>;
  const DelegatesMap& GetDelegatesMap(){ return _delegatesMap;}
  
  // for debug only, prints stack info
  void DebugPrintStack(const std::string& debugStr) const;
  
private:
  BehaviorExternalInterface* _behaviorExternalInterface;
  std::vector<IBehavior*> _runnableStack;
  std::unordered_map<const IBehavior*, int> _runnableToIndexMap;
  std::map<IBehavior*,std::set<IBehavior*>> _delegatesMap;
  
  
  
  // calls all appropriate functions to prep the delegates of something about to be added to the stack
  void PrepareDelegatesToEnterScope(IBehavior* delegated);
  
  // calls all appropriate functions to prepare a delegate to be removed from the stack
  void PrepareDelegatesForRemovalFromStack(IBehavior* delegated);
};


} // namespace Cozmo
} // namespace Anki


#endif // AI_COMPONENT_BEHAVIOR_COMPONENT_RUNNABLE_STACK
