/**
 * File: IBSRunnableChooser.h
 *
 * Author: Lee
 * Created: 08/20/15, raul 05/03/16
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorChooser_H__
#define __Cozmo_Basestation_BehaviorChooser_H__

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "engine/aiComponent/behaviorSystem/iBSRunnable.h"
#include "anki/common/types.h"
#include "util/helpers/noncopyable.h"
#include <vector>


namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {
  
//Forward declarations
class Robot;

// Interface for the container and logic associated with holding and choosing behaviors
class IBSRunnableChooser : public IBSRunnable, private Util::noncopyable
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // constructor/destructor
  IBSRunnableChooser(BehaviorExternalInterface& behaviorExternalInterface,
                     const Json::Value& config):IBSRunnable("chooser"){};
  virtual ~IBSRunnableChooser() {}
  
  virtual void OnSelected() {};
  virtual void OnDeselected() {};

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Logic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehaviorPtr GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior) = 0;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


protected:
  // IBSRunnable methods - TO BE IMPLEMENTED - these will be used by the new BSM
  // as a uniform interface across Activities and Behaviors, but they will be
  // wired up in a seperate PR
  //virtual std::set<IBSRunnable> GetAllDelegates() override { return std::set<IBSRunnable>();}
  void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual void EnteredActivatableScopeInternal() override {};
  virtual BehaviorStatus UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override { return BehaviorStatus::Complete;};
  virtual bool WantsToBeActivatedInternal() override { return false;};
  virtual Result OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {OnSelected(); return RESULT_OK;};
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {OnDeselected();};
  virtual void LeftActivatableScopeInternal() override {};
  
}; // class IBSRunnableChooser
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorChooser_H__
