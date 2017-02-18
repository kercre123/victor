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

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/helperHandle.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"

#include <vector>
#include <memory>


namespace Anki {
namespace Cozmo {
  
  
class BehaviorHelperComponent{
public:
  BehaviorHelperComponent();
  
  BehaviorHelperFactory& GetBehaviorHelperFactory(){ assert(_helperFactory); return *_helperFactory;}
  
  // Conceptually relevant function which we currently don't support
  //bool CanRunHelper(HelperHandle helper);
  
  bool DelegateToHelper(Robot& robot,
                        HelperHandle handleToRun,
                        IHelper::SimpleCallbackWithRobot successCallback,
                        IHelper::SimpleCallbackWithRobot failureCallback);
  
  
  bool StopHelper(const HelperHandle& helperToStop);
  
protected:
  friend class AIComponent;
  friend class BehaviorHelperFactory;
  void Update(Robot& robot);
  HelperHandle AddHelperToComponent(IHelper*& helper);
  

private:
  std::unique_ptr<BehaviorHelperFactory> _helperFactory;

  using HelperStack = std::vector<HelperHandle>;
  using HelperIter = HelperStack::iterator;
  
  HelperStack _helperStack;
  IHelper::SimpleCallbackWithRobot _behaviorSuccessCallback;
  IHelper::SimpleCallbackWithRobot _behaviorFailureCallback;
    
  void CheckInactiveStackHelpers(const Robot& robot);
  void UpdateActiveHelper(Robot& robot);
  void PushHelperOntoStackAndUpdate(Robot& robot, HelperHandle helper);

  void ClearStackVars();

  // clear helpers including iter, and everything higher on the stack
  void ClearStackFromTopToIter(HelperIter& iter);

};
  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_BehaviorHelperComponent_H__

