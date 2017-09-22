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

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Logic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual IBehaviorPtr GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior) = 0;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


protected:
  // Functions called by iBSRunnable
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  
  // Currently unused overrides of iBSRunnable since no equivalence in old BM system
  void GetAllDelegates(std::set<const IBSRunnable&>& delegates) const override {}
  void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual void OnEnteredActivatableScopeInternal() override {};
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual bool WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const override { return false;};

  virtual void OnLeftActivatableScopeInternal() override {};
  
}; // class IBSRunnableChooser
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorChooser_H__
