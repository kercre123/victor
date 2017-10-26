/**
 * File: behaviorHelperComponent.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: This component has been designed as a short term stand in for the
 * action planner which we'll develop down the line.  As such its functions are designed
 * to model an asyncronous system which allows time for the planner to process data.
 * Behaviors can request a helper handle with the appropriate data, and then
 * (eventually) will be able to check if it can run, which if true they can
 * delegate to with DelegateToHelper
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperComponent_H__

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/helperHandle.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"

#include <vector>
#include <memory>


namespace Anki {
namespace Cozmo {
  
  
class BehaviorHelperComponent : private Util::noncopyable{
public:
  BehaviorHelperComponent();
  
  BehaviorHelperFactory& GetBehaviorHelperFactory(){ assert(_helperFactory); return *_helperFactory;}
  
  // Conceptually relevant function which we currently don't support
  // bool CanRunHelper(HelperHandle helper);
  
  
  bool DelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                        HelperHandle handleToRun,
                        BehaviorSimpleCallbackWithExternalInterface successCallback,
                        BehaviorSimpleCallbackWithExternalInterface failureCallback);
  
  
  bool StopHelperWithoutCallback(const HelperHandle& helperToStop);
  
protected:
  friend class BehaviorComponent;
  friend class BehaviorHelperFactory;
  friend class IHelper;
  void Update(BehaviorExternalInterface& behaviorExternalInterface);
  HelperHandle AddHelperToComponent(IHelper*& helper,BehaviorExternalInterface& behaviorExternalInterface);
  
private:
  std::unique_ptr<BehaviorHelperFactory> _helperFactory;
  
  using HelperStack = std::vector<HelperHandle>;
  using HelperIter = HelperStack::iterator;
  
  HelperStack _helperStack;
  BehaviorSimpleCallbackWithExternalInterface _behaviorSuccessCallback;
  BehaviorSimpleCallbackWithExternalInterface _behaviorFailureCallback;
  
  // TODO: When COZMO-10389 cancels actions on re-jigger
  // it won't be necessary to store/check this value
  // Currently used to track when the block world origin changes
  PoseOriginID_t                  _worldOriginIDAtStart;
  
  void CheckInactiveStackHelpers(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateActiveHelper(BehaviorExternalInterface& behaviorExternalInterface);
  void PushHelperOntoStackAndUpdate(BehaviorExternalInterface& behaviorExternalInterface, HelperHandle helper);

  // Variables which persist for the life of a stack but should be cleared
  // when the stack fully empties or is cleared
  void ClearStackLifetimeVars();
  
  // Clears variables which track the current stack state
  void ClearStackMaintenanceVars();

  // clear helpers including iter, and everything higher on the stack
  void ClearStackFromTopToIter(HelperIter& iter);

};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperComponent_H__

