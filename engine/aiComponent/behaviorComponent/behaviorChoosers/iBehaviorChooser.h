/**
 * File: IBehaviorChooser.h
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

#include "engine/aiComponent/behaviorComponent/behaviors/ICozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
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
class IBehaviorChooser : public IBehavior, private Util::noncopyable
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Initialization
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // constructor/destructor
  IBehaviorChooser(BehaviorExternalInterface& behaviorExternalInterface,
                     const Json::Value& config):IBehavior("chooser"){};
  virtual ~IBehaviorChooser() {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Logic
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // chooses the next behavior to run (could be the same we are currently running or null if none are desired)
  virtual ICozmoBehaviorPtr GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior) = 0;
  
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Accessors
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  

protected:
  // Functions called by IBehavior
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};

  // Currently unused overrides of IBehavior since no equivalence in old BM system
  void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual void OnEnteredActivatableScopeInternal() override {};
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual bool WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const override { return false;};

  virtual void OnLeftActivatableScopeInternal() override {};
  
}; // class IBehaviorChooser
  
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorChooser_H__
