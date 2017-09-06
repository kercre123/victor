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

#include "engine/behaviorSystem/behaviors/iBehavior_fwd.h"

#include <set>
#include <string>

namespace Anki {
namespace Cozmo {

class IBSRunnable {
public:
  IBSRunnable(const std::string& idString);
  virtual ~IBSRunnable(){};

protected:
  // Functions should only be called by the BehaviorSystemManager, so hidden
  // within protected scope
  
  // Function which informs the Runnable that it may be activated - opportunity
  // to start any processes which need to be running for the Runnable to be runnable
  void EnteredActivatableScope();
  
  // Guaranteed to be ticked every tick that the runnable is within activatable scope
  BehaviorStatus Update(Robot& robot);
  
  // Check to see if the runnable wants to run right now
  bool WantsToBeActivated();
  
  // Informs the runnable that it has been activated
  void OnActivated();
  
  // Informs the runnable that it has been deactivated
  void OnDeactivated();
  
  // Function which informs the Runnable that it has fallen out of scope to be activated
  // the runnable should stop any processes it started on entering selectable scope
  void LeftActivatableScope();
  
  //virtual std::set<IBSRunnable> GetAllDelegates() = 0;
  
  virtual void EnteredActivatableScopeInternal() = 0;
  virtual BehaviorStatus UpdateInternal(Robot& robot) = 0;
  virtual bool WantsToBeActivatedInternal() = 0;
  virtual void OnActivatedInternal() = 0;
  virtual void OnDeactivatedInternal() = 0;
  virtual void LeftActivatableScopeInternal() = 0;
  
private:
  enum class ActivationState{
    OutOfScope,
    InScope,
    Activated
  };
  
  // tmp string for identifying BSRunnables until IDs are combined
  std::string _idString;
  ActivationState _currentActivationState;
  size_t _lastTickWantsToRunCheckedOn;
  size_t _lastTickOfUpdate;

  
  std::string ActivationStateToString(ActivationState state);
  
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_IBSRunnable_H__
