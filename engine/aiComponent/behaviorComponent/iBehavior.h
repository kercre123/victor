/**
* File: IBehavior.h
*
* Author: Kevin M. Karol
* Created: 08/25/17
*
* Description: Interface for "Behavior" elements of the behavior system
* such as activities and behaviors
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_IBehavior_H__
#define __Cozmo_Basestation_BehaviorSystem_IBehavior_H__

#include "coretech/common/shared/types.h"
#include "util/global/globalDefinitions.h"

#include <set>
#include <string>

namespace Anki {
namespace Vector {

// Forward declarations
class BehaviorExternalInterface;
class BehaviorManager;
class BehaviorSystemManager;

class IBehavior : private Util::noncopyable {
public:
  IBehavior(const std::string& debugLabel);
  virtual ~IBehavior(){};

  const std::string& GetDebugLabel() const { return _debugLabel; }
  void SetDebugLabel(const std::string& debugLabel) { _debugLabel = debugLabel; }

  // Function that allows the behavior to initialize variables/subscribe
  // through the behaviorExternalInterface
  void Init(BehaviorExternalInterface& behaviorExternalInterface);

  // Function which informs the Behavior that it may be activated - opportunity
  // to start any processes which need to be running for the Behavior to be activated
  void OnEnteredActivatableScope();

  // Guaranteed to be ticked every tick that the behavior is within activatable scope
  void Update();

  // Check to see if the behavior wants to run right now
  bool WantsToBeActivated() const;

  // Informs the behavior that it has been activated
  void OnActivated();

  // Informs the behavior that it has been deactivated
  void OnDeactivated();

  // Function which informs the Behavior that it has fallen out of scope to be activated
  // the behavior should stop any processes it started on entering selectable scope
  void OnLeftActivatableScope();

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const = 0;

  // Sometimes behaviors (like dispatchers) want to maintain the same scope as one of their delegates
  // even if they are in Activatable scope (i.e. their delegates are still out of scope )
  // This allows them to pass calls to WantsToBeActivated or maintain proper vision mode settings
  // Behaviors returned from this function will be moved into activatable scope in lock step
  // with the behavior that overrides them
  virtual void GetLinkedActivatableScopeBehaviors(std::set<IBehavior*>& delegates) const {};

protected:

  // Called once after this behavior is constructed
  virtual void InitInternal() { }

  // Returns true if this behavior wants to be active, false otherwise
  // TODO:(bn) default to true??
  virtual bool WantsToBeActivatedInternal() const = 0;

  // Called when this behavior has entered activatable scope (it could be delegated to)
  virtual void OnEnteredActivatableScopeInternal() { }

  // Called when this behavior is no longer in activatable scope (no longer valid to be delegated to)
  virtual void OnLeftActivatableScopeInternal() { }

  // Called once per tick with the behavior is in activatable scope
  virtual void UpdateInternal() { }

  // Called when this behavior becomes active and has control
  virtual void OnActivatedInternal() { }

  // Called when this behavior is deactivated (it no longer has control)
  virtual void OnDeactivatedInternal() { }

  // Allow all behavior functions access to the bei after initialization
  BehaviorExternalInterface& GetBEI() const {assert(_beiWrapper); return _beiWrapper->_bei;}

private:
  // string for identifying Behaviors (different than an ID since the latter might just be "Anonymous")
  std::string _debugLabel;
  // Track the number of EnteredScope requests
  uint32_t _currentInScopeCount;
  mutable size_t _lastTickWantsToBeActivatedCheckedOn;
  size_t _lastTickOfUpdate;

  struct BEIWrapper {
    BEIWrapper(BehaviorExternalInterface& bei)
    : _bei(bei){}
    BehaviorExternalInterface& _bei;
  };
  std::unique_ptr<BEIWrapper> _beiWrapper;

  enum class ActivationState {
    NotInitialized,
    OutOfScope,
    InScope,
    Activated
  };

  const char * ActivationStateToString(ActivationState state) const;

  // Functions for ensuring appropriate activation state is maintained
  void SetActivationState_DevOnly(ActivationState newState, const std::string& debugStr);
  void AssertActivationState_DevOnly(ActivationState state, const std::string& debugStr) const;
  void AssertNotActivationState_DevOnly(ActivationState state, const std::string& debugStr) const;


  #if ANKI_DEV_CHEATS
    ActivationState _currentActivationState;
  #endif
};

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_IBehavior_H__
