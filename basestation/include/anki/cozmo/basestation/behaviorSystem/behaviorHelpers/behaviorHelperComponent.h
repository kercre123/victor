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
  // bool CanRunHelper(HelperHandle helper);
  
  // Set a motion profile before delegating to the helper - this motion profile
  // will persist through all actions the helper may perform and will be cleared
  // when the helper completes
  void SetMotionProfile(const PathMotionProfile& profile);
  

  
  bool DelegateToHelper(Robot& robot,
                        HelperHandle handleToRun,
                        BehaviorSimpleCallbackWithRobot successCallback,
                        BehaviorSimpleCallbackWithRobot failureCallback);
  
  
  bool StopHelperWithoutCallback(const HelperHandle& helperToStop);
  
protected:
  friend class AIComponent;
  friend class BehaviorHelperFactory;
  friend class IHelper;
  void Update(Robot& robot);
  HelperHandle AddHelperToComponent(IHelper*& helper);
  
  // For helpers to retreieve the persistant path motion profile
  // if a profile has been set it will be returned via the profile reference
  // and the function will return true - if the function returns false there is
  // currently no path motion profile
  bool GetPathMotionProfile(PathMotionProfile& profile);

private:
  std::unique_ptr<BehaviorHelperFactory> _helperFactory;
  
  PathMotionProfile  _motionProfile;

  using HelperStack = std::vector<HelperHandle>;
  using HelperIter = HelperStack::iterator;
  
  HelperStack _helperStack;
  BehaviorSimpleCallbackWithRobot _behaviorSuccessCallback;
  BehaviorSimpleCallbackWithRobot _behaviorFailureCallback;
  
  // TODO: When COZMO-10389 cancels actions on re-jigger
  // it won't be necessary to store/check this value
  // Currently used to track when the block world origin changes
  const Pose3d*                   _worldOriginAtStart;
  
  void CheckInactiveStackHelpers(const Robot& robot);
  void UpdateActiveHelper(Robot& robot);
  void PushHelperOntoStackAndUpdate(Robot& robot, HelperHandle helper);

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

