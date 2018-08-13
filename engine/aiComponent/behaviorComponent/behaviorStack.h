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

#ifndef AI_COMPONENT_BEHAVIOR_COMPONENT_BEHAVIOR_STACK
#define AI_COMPONENT_BEHAVIOR_COMPONENT_BEHAVIOR_STACK

#include "clad/types/robotCompletedAction.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include <list>

namespace Json {
class Value;
}

namespace Anki {
namespace Vector {

// forward declarations
class AsyncMessageGateComponent;
class BehaviorExternalInterface;
class CozmoContext;
class IBehavior;
class IExternalInterface;
class IStackMonitor;
  
namespace ExternalInterface{
struct RobotCompletedAction;
}
  
class BehaviorStack : private Util::noncopyable {
public:
  BehaviorStack();
  virtual ~BehaviorStack();

  // Convert a stack to a standardized string format
  // Currently only used for development output (e.g. audio). Don't use this internally for anything important
  static std::string StackToBehaviorString(std::vector<IBehavior*> stack);

  
  void InitBehaviorStack(IBehavior* baseOfStack, IExternalInterface* externalInterface);
  // Clear the stack if it needs to be re-initialized
  void ClearStack();
  
  void UpdateBehaviorStack(BehaviorExternalInterface& behaviorExternalInterface,
                           std::vector<ExternalInterface::RobotCompletedAction>& actionsCompletedThisTick,
                           AsyncMessageGateComponent& asyncMessageGateComp,
                           std::set<IBehavior*>& tickedInStack);
  
  inline IBehavior* GetTopOfStack(){ return _behaviorStack.empty() ? nullptr : _behaviorStack.back();}
  inline const IBehavior* GetTopOfStack() const { return _behaviorStack.empty() ? nullptr : _behaviorStack.back();}
  inline IBehavior* GetBottomOfStack() const { return _behaviorStack.empty() ? nullptr : _behaviorStack.front();}
  inline bool IsInStack(const IBehavior* behavior) { return _stackMetadataMap.find(behavior) != _stackMetadataMap.end();}
  
  // if the passed in behavior is in the stack, return a pointer to the behavior which is above it in the
  // stack, or null if it is at the top or not in the stack
  const IBehavior* GetBehaviorInStackAbove(const IBehavior* behavior) const;
  const IBehavior* GetBehaviorInStackBelow(const IBehavior* behavior) const;

  void PushOntoStack(IBehavior* behavior);
  void PopStack();
  
  std::set<IBehavior*> GetBehaviorsInActivatableScope();

  bool IsValidDelegation(IBehavior* delegator, IBehavior* delegated);
  
  // for debug only, prints stack info
  void DebugPrintStack(const std::string& debugStr) const;
  // builds and returns the behavior tree as a flat array in json
  Json::Value BuildDebugBehaviorTree(BehaviorExternalInterface& behaviorExternalInterface) const;
  
private:
  struct StackMetadataEntry{
    IBehavior* behavior = nullptr;
    int        indexInStack = -1; // negative = not in stack
    std::set<IBehavior*> delegates;
    std::set<IBehavior*> linkedActivationScope;
    
    StackMetadataEntry(){}
    StackMetadataEntry(IBehavior* behavior, int indexInStack);
    // Recursively grab all behaviors that need to have linked scope
    static void RecursivelyGatherLinkedBehaviors(IBehavior* baseBehavior,
                                                 std::set<IBehavior*>& linkedBehaviors);
  };
  
  std::vector<IBehavior*> _behaviorStack;
  std::unordered_map<const IBehavior*, StackMetadataEntry> _stackMetadataMap;
  IExternalInterface* _externalInterface = nullptr;
  
  bool _behaviorStackDirty = false;
  std::list<std::unique_ptr<IStackMonitor>> _stackMonitors;

  // Let the audio system know that a branch has been activated/deactivated
  void BroadcastAudioBranch(bool activated);
  
  // calls all appropriate functions to prep the delegates of something about to be added to the stack
  void PrepareDelegatesToEnterScope(IBehavior* delegated);
  
  // calls all appropriate functions to prepare a delegate to be removed from the stack
  void PrepareDelegatesForRemovalFromStack(IBehavior* delegated);
  
  void NotifyOfChange(BehaviorExternalInterface& bei);
  
};


} // namespace Vector
} // namespace Anki


#endif // AI_COMPONENT_BEHAVIOR_COMPONENT_BEHAVIOR_STACK
