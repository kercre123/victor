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

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

namespace Anki {
namespace Cozmo {

// forward declarations
class BehaviorExternalInterface;
class IBSRunnable;
  
// Defined within .h file so tests can access runnable stack functions
class RunnableStack{
public:
  RunnableStack(BehaviorExternalInterface* behaviorExternalInterface)
  :_behaviorExternalInterface(behaviorExternalInterface){};
  virtual ~RunnableStack(){}
  
  void InitRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                         IBSRunnable* baseOfStack);
  void UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface,
                           std::set<IBSRunnable*>& tickedInStack);
  
  inline IBSRunnable* GetTopOfStack(){ return _runnableStack.empty() ? nullptr : _runnableStack.back();}
  inline bool IsInStack(const IBSRunnable* runnable) { return _runnableToIndexMap.find(runnable) != _runnableToIndexMap.end();}
  
  void PushOntoStack(IBSRunnable* runnable);
  void PopStack();
  
  using DelegatesMap = std::map<IBSRunnable*,std::set<IBSRunnable*>>;
  const DelegatesMap& GetDelegatesMap(){ return _delegatesMap;}
  
  // for debug only, prints stack info
  void DebugPrintStack(const std::string& debugStr) const;
  
private:
  BehaviorExternalInterface* _behaviorExternalInterface;
  std::vector<IBSRunnable*> _runnableStack;
  std::unordered_map<const IBSRunnable*, int> _runnableToIndexMap;
  std::map<IBSRunnable*,std::set<IBSRunnable*>> _delegatesMap;
  
  
  
  // calls all appropriate functions to prep the delegates of something about to be added to the stack
  void PrepareDelegatesToEnterScope(IBSRunnable* delegated);
  
  // calls all appropriate functions to prepare a delegate to be removed from the stack
  void PrepareDelegateForRemovalFromStack(IBSRunnable* delegated);
};


} // namespace Cozmo
} // namespace Anki


#endif // AI_COMPONENT_BEHAVIOR_COMPONENT_RUNNABLE_STACK
