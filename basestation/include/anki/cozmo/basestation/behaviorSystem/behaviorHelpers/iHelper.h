/**
 * File: iHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Interface for BehaviorHelpers
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_IHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_IHelper_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperParameters.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/helperHandle.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior_fwd.h"
#include "clad/types/actionResults.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/userFacingResults.h"
#include <functional>

namespace Anki {
namespace Cozmo {

// Forward Declarations
class Robot;
class IActionRunner;
class BehaviorHelperFactory;

class IHelper{
  // Enforce creation/interaction through HelperComponent
  friend class BehaviorHelperComponent;
  friend class BehaviorHelperFactory;

public:
  virtual ~IHelper(){};

  const std::string& GetName() const { return _name; }
  
protected:
  using UserFacingActionResultMapFunc = std::function<UserFacingActionResult(ActionResult)>;
  
  struct DelegateProperties{
  public:
    HelperHandle GetDelegateToSet() const { return _delegateToSet;}
    BehaviorStatusCallbackWithRobot GetOnSuccessFunction() const {return _onSuccessFunction;}
    BehaviorStatusCallbackWithRobot GetOnFailureFunction() const {return _onFailureFunction;}
    
    void SetDelegateToSet(HelperHandle delegate){ _delegateToSet = delegate;}
    void SetOnSuccessFunction(BehaviorStatusCallbackWithRobot onSuccess){_onSuccessFunction = onSuccess;}
    void SetOnFailureFunction(BehaviorStatusCallbackWithRobot onFailure){_onFailureFunction = onFailure;}

    // utility functions to automatically pass or fail when the delegate does
    void FailImmediatelyOnDelegateFailure();
    void SucceedImmediatelyOnDelegateFailure();
    
    void ClearDelegateProperties();
    
  private:
    HelperHandle _delegateToSet;
    BehaviorStatusCallbackWithRobot _onSuccessFunction = nullptr;
    BehaviorStatusCallbackWithRobot _onFailureFunction = nullptr;
  };
  
  // Initialize the helper with the behavior to start actions on, and a reference to the factory to delegate
  // out to new helpers
  IHelper(const std::string& name, Robot& robot, IBehavior& behavior, BehaviorHelperFactory& _helperFactory);

  void SetName(const std::string& name) { _name = name; }
  
  // Initialize variables when placed on stack
  void InitializeOnStack();
  virtual void InitializeOnStackInternal() {};
  
  BehaviorStatus OnDelegateSuccess(Robot& robot);
  BehaviorStatus OnDelegateFailure(Robot& robot);
  
  bool IsActing();
  
  // Helper is being stopped externally
  void Stop(bool isActive);

  // override-able by helpers to handle cleanup when they are stopped
  virtual void StopInternal(bool isActive) {};
  
  // Called each tick when the helper is not active to determine if
  // the stack of delegates this helper has created should be canceled
  // and this helper should become active
  virtual bool ShouldCancelDelegates(const Robot& robot) const = 0;
  
  // Called each tick when the helper is at the top of the helper stack
  BehaviorStatus UpdateWhileActive(Robot& robot, HelperHandle& delegateToSet);
 
  // Called on the first time a helper is ticked while active
  // UpdateWhileActive will be called immediately after on the same tick if no
  // delegate is set
  virtual BehaviorStatus Init(Robot& robot) = 0;
  
  // Allows sub classes to pass back a delegate, success and failure function for IHelper to manage. If a
  // delegate is set in the delegate properties, then it will be pushed onto the stack, and the callbacks from
  // the properties will be used
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot) = 0;
  

  
  ///////
  /// Convenience functions
  //////
  
  ActionResult IsAtPreActionPoseWithVisualVerification(Robot& robot,
                                                       const ObjectID& targetID,
                                                       PreActionPose::ActionType actionType,
                                                       const f32 offsetX_mm = 0,
                                                       const f32 offsetY_mm = 0);
  
  void DelegateAfterUpdate(const DelegateProperties& properties){ _delegateAfterUpdate = properties;}
  
  // Convenience accessor to the behaviorHelperComponent's motion profile ref
  // returns true if a profile is set, false otherwise
  bool GetPathMotionProfile(Robot& robot, PathMotionProfile& profile);
  
  
  // Use the set behavior's start acting to perform an action
  template <typename T>
  bool StartActing(IActionRunner* action, void(T::*callback)(ActionResult,Robot&));
  
  // Pass in a function that maps action results to UserFacingACtionResults so that the
  // BehaviorEventAnimationResponseDirector knows how to respond to the failure with the
  // appropriate animation
  template <typename T>
  bool StartActingWithResponseAnim(
            IActionRunner* action,
            void(T::*callback)(ActionResult,Robot&),
            UserFacingActionResultMapFunc mapFunc = nullptr);

  bool StartActing(IActionRunner* action, BehaviorActionResultWithRobotCallback callback);

  // Stop the behavior acting so that the delegate can issue a new action
  bool StopActing(bool allowCallback);
  
  // Helpers to access the HelperFactory without needing access to the underlying behavior
  HelperHandle CreatePickupBlockHelper(Robot& robot, const ObjectID& targetID,
                                       const PickupBlockParamaters& parameters = {});
  HelperHandle CreatePlaceBlockHelper(Robot& robot);
  HelperHandle CreateRollBlockHelper(Robot& robot, const ObjectID& targetID,
                                     bool rollToUpright, const RollBlockParameters& parameters = {});
  HelperHandle CreateDriveToHelper(Robot& robot,
                                   const ObjectID& targetID,
                                   const DriveToParameters& parameters = {});
  HelperHandle CreatePlaceRelObjectHelper(Robot& robot,
                                          const ObjectID& targetID,
                                          const bool placingOnGround = false,
                                          const PlaceRelObjectParameters& parameters = {});
  
  HelperHandle CreateSearchForBlockHelper(Robot& robot,
                                          const SearchParameters& params = {});

  
  BehaviorStatus _status;
  
private:

  void LogStopEvent(bool isActive);
  
  std::string _name;
  bool _hasStarted;
  BehaviorStatusCallbackWithRobot _onSuccessFunction;
  BehaviorStatusCallbackWithRobot _onFailureFunction;
  DelegateProperties _delegateAfterUpdate;
  float _timeStarted_s = 0.0f;


  IBehavior& _behaviorToCallActionsOn;
  BehaviorHelperFactory& _helperFactory;
  
  // Functions for responding to action results with StartActingWithResponseAnim
  UserFacingActionResultMapFunc _actionResultMapFunc = nullptr;
  BehaviorActionResultWithRobotCallback _callbackAfterResponseAnim = nullptr;
  
  AnimationTrigger AnimationResponseToActionResult(Robot& robot, UserFacingActionResult result);
  void RespondToResultWithAnim(ActionResult result, Robot& robot);

};

  
template<typename T>
bool IHelper::StartActing(IActionRunner* action, void(T::*callback)(ActionResult,Robot&))
{
  return StartActing(action, 
          std::bind(callback, static_cast<T*>(this), std::placeholders::_1,
                                                     std::placeholders::_2));
}
  
template<typename T>
bool IHelper::StartActingWithResponseAnim(
            IActionRunner* action,
            void(T::*callback)(ActionResult,Robot&),
            UserFacingActionResultMapFunc mapFunc)
{
  _callbackAfterResponseAnim = std::bind(callback, static_cast<T*>(this), std::placeholders::_1,
                                         std::placeholders::_2);
  _actionResultMapFunc = mapFunc;
  
  DEV_ASSERT(_callbackAfterResponseAnim != nullptr,
             "IHelper.StartActingWithResponseAnim.NullCllback");
  return StartActing(action, &IHelper::RespondToResultWithAnim);
}
  
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_IHelper_H__
