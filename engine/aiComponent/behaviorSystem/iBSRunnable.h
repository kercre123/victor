/**
* File: iBSRunnable.h
*
* Author: Kevin M. Karol
* Created: 08/251/17
*
* Description: Interface for "Runnable" elements of the behavior system
* such as activities and behaviors
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_IBSRunnable_H__
#define __Cozmo_Basestation_BehaviorSystem_IBSRunnable_H__

#include "anki/common/types.h"
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior_fwd.h"

#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

// Forward declarations
class BehaviorExternalInterface;
class BehaviorManager;
class BehaviorSystemManager;
  
class IBSRunnable {
public:
  IBSRunnable(const std::string& idString);
  virtual ~IBSRunnable(){};
  
  // Function that allows the behavior to initialize variables/subscribe
  // through the behaviorExternalInterface
  void Init(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Function which informs the Runnable that it may be activated - opportunity
  // to start any processes which need to be running for the Runnable to be runnable
  void EnteredActivatableScope();
  
  // Guaranteed to be ticked every tick that the runnable is within activatable scope
  void Update(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Check to see if the runnable wants to run right now
  bool WantsToBeActivated();
  
  // Informs the runnable that it has been activated
  void OnActivated(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Informs the runnable that it has been deactivated
  void OnDeactivated(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Function which informs the Runnable that it has fallen out of scope to be activated
  // the runnable should stop any processes it started on entering selectable scope
  void LeftActivatableScope();
  
  //virtual std::set<IBSRunnable> GetAllDelegates() = 0;

protected:
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual void EnteredActivatableScopeInternal() = 0;
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual bool WantsToBeActivatedInternal() = 0;
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  virtual void LeftActivatableScopeInternal() = 0;
  
private:
  enum class ActivationState{
    NotInitialized,
    OutOfScope,
    InScope,
    Activated
  };
  
  // tmp string for identifying BSRunnables until IDs are combined
  std::string _idString;
  ActivationState _currentActivationState;
  size_t _lastTickWantsToBeActivatedCheckedOn;
  size_t _lastTickOfUpdate;

  
  std::string ActivationStateToString(ActivationState state);
  
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_IBSRunnable_H__
