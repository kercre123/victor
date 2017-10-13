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
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperParameters.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/helperHandle.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
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

class IHelper : public IBehavior{
  // Enforce creation/interaction through HelperComponent
  friend class BehaviorHelperComponent;
  friend class BehaviorHelperFactory;

public:
  virtual ~IHelper(){};

  const std::string& GetName() const { return _name; }
  
protected:
  // VIC-322: For the time being the behavior helper component ticks helpers instead of
  // the BSM. While this is true it will be impossible to delegate to helpers in the way
  // other BSRunnables are delegated to - so don't tie this UpdateInternal in just yet
  
  // Currently unused overrides of IBehavior since no equivalence in old BM system
  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  virtual bool WantsToBeActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) const override { return false;};
  virtual void OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override {};
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void OnEnteredActivatableScopeInternal() override {};
  virtual void OnLeftActivatableScopeInternal() override {};
  virtual void UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface) override {
  DEV_ASSERT("IHelper.UpdateInternal.TickedImproperly",
             "Helpers are still managed by the BehaviorHelperComponent and this function doesn't work");
  };
  
  using UserFacingActionResultMapFunc = std::function<UserFacingActionResult(ActionResult)>;
  
  struct DelegateProperties{
  public:
    HelperHandle GetDelegateToSet() const { return _delegateToSet;}
    BehaviorStatusCallbackWithExternalInterface GetOnSuccessFunction() const {return _onSuccessFunction;}
    BehaviorStatusCallbackWithExternalInterface GetOnFailureFunction() const {return _onFailureFunction;}
    
    void SetDelegateToSet(HelperHandle delegate){ _delegateToSet = delegate;}
    void SetOnSuccessFunction(BehaviorStatusCallbackWithExternalInterface onSuccess){_onSuccessFunction = onSuccess;}
    void SetOnFailureFunction(BehaviorStatusCallbackWithExternalInterface onFailure){_onFailureFunction = onFailure;}

    // utility functions to automatically pass or fail when the delegate does
    void FailImmediatelyOnDelegateFailure();
    void SucceedImmediatelyOnDelegateFailure();
    
    void ClearDelegateProperties();
    
  private:
    HelperHandle _delegateToSet;
    BehaviorStatusCallbackWithExternalInterface _onSuccessFunction = nullptr;
    BehaviorStatusCallbackWithExternalInterface _onFailureFunction = nullptr;
  };
  
  // Initialize the helper with the behavior to start actions on, and a reference to the factory to delegate
  // out to new helpers
  IHelper(const std::string& name, BehaviorExternalInterface& behaviorExternalInterface, ICozmoBehavior& behavior, BehaviorHelperFactory& _helperFactory);

  void SetName(const std::string& name) { _name = name; }
  
  // Initialize variables when placed on stack
  virtual void OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface) override final;
  virtual void OnActivatedHelper(BehaviorExternalInterface& behaviorExternalInterface) {};
  
  BehaviorStatus OnDelegateSuccess(BehaviorExternalInterface& behaviorExternalInterface);
  BehaviorStatus OnDelegateFailure(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool IsControlDelegated();
  
  // Helper is being stopped externally
  void Stop(bool isActive);

  // override-able by helpers to handle cleanup when they are stopped
  virtual void StopInternal(bool isActive) {};
  
  // Called each tick when the helper is not active to determine if
  // the stack of delegates this helper has created should be canceled
  // and this helper should become active
  virtual bool ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const = 0;
  
  // Called each tick when the helper is at the top of the helper stack
  BehaviorStatus UpdateWhileActive(BehaviorExternalInterface& behaviorExternalInterface, HelperHandle& delegateToSet);
 
  // Called on the first time a helper is ticked while active
  // UpdateWhileActive will be called immediately after on the same tick if no
  // delegate is set
  virtual BehaviorStatus Init(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  
  // Allows sub classes to pass back a delegate, success and failure function for IHelper to manage. If a
  // delegate is set in the delegate properties, then it will be pushed onto the stack, and the callbacks from
  // the properties will be used
  virtual BehaviorStatus UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) = 0;
  

  
  ///////
  /// Convenience functions
  //////
  
  ActionResult IsAtPreActionPoseWithVisualVerification(BehaviorExternalInterface& behaviorExternalInterface,
                                                       const ObjectID& targetID,
                                                       PreActionPose::ActionType actionType,
                                                       const f32 offsetX_mm = 0,
                                                       const f32 offsetY_mm = 0);
  
  void DelegateAfterUpdate(const DelegateProperties& properties){ _delegateAfterUpdate = properties;}
  
  // Use the set behavior's DelegateIfInControl to delegate to another ICozmoBehavior
  template <typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult,BehaviorExternalInterface&));
  
  template <typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(const ExternalInterface::RobotCompletedAction&,
                                                             BehaviorExternalInterface&));
  
  // Pass in a function that maps action results to UserFacingActionResults so that the
  // BehaviorEventAnimationResponseDirector knows how to respond to the failure with the
  // appropriate animation
  template<typename T>
  bool DelegateWithResponseAnim(IActionRunner* action,
                                   void(T::*callback)(ActionResult, BehaviorExternalInterface&),
                                   UserFacingActionResultMapFunc mapFunc = nullptr);
  
  template<typename T>
  bool DelegateWithResponseAnim(IActionRunner* action,
                                   void(T::*callback)(const ExternalInterface::RobotCompletedAction&, BehaviorExternalInterface&),
                                   UserFacingActionResultMapFunc mapFunc = nullptr);

  bool DelegateIfInControl(IActionRunner* action, BehaviorActionResultWithExternalInterfaceCallback callback);
  bool DelegateIfInControl(IActionRunner* action, BehaviorRobotCompletedActionWithExternalInterfaceCallback callback);

  // Stop all delegates so that a new delegate can be set
  bool CancelDelegates(bool allowCallback);
  
  // Helpers to access the HelperFactory without needing access to the underlying behavior
  HelperHandle CreatePickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& targetID,
                                       const PickupBlockParamaters& parameters = {});
  HelperHandle CreatePlaceBlockHelper(BehaviorExternalInterface& behaviorExternalInterface);
  HelperHandle CreateRollBlockHelper(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& targetID,
                                     bool rollToUpright, const RollBlockParameters& parameters = {});
  HelperHandle CreateDriveToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                   const ObjectID& targetID,
                                   const DriveToParameters& parameters = {});
  HelperHandle CreatePlaceRelObjectHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                          const ObjectID& targetID,
                                          const bool placingOnGround = false,
                                          const PlaceRelObjectParameters& parameters = {});
  
  HelperHandle CreateSearchForBlockHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                          const SearchParameters& params = {});

  
  BehaviorStatus _status;
  
private:
  friend class DelegationComponent;
  void LogStopEvent(bool isActive);
  
  std::string _name;
  bool _hasStarted;
  BehaviorStatusCallbackWithExternalInterface _onSuccessFunction;
  BehaviorStatusCallbackWithExternalInterface _onFailureFunction;
  DelegateProperties _delegateAfterUpdate;
  float _timeStarted_s = 0.0f;


  ICozmoBehavior& _behaviorToCallActionsOn;
  BehaviorHelperFactory& _helperFactory;
  
  // Functions for responding to action results with DelegateIfInControlWithResponseAnim
  UserFacingActionResultMapFunc _actionResultMapFunc = nullptr;
  BehaviorActionResultWithExternalInterfaceCallback _callbackAfterResponseAnim = nullptr;
  BehaviorRobotCompletedActionWithExternalInterfaceCallback _callbackAfterResponseAnimUsingRCA = nullptr;
  
  AnimationTrigger AnimationResponseToActionResult(BehaviorExternalInterface& behaviorExternalInterface, UserFacingActionResult result);
  
  template<typename T>
  void RespondToActionWithAnim(const T& res,
                               ActionResult actionResult,
                               BehaviorExternalInterface& behaviorExternalInterface,
                               std::function<void(const T&,BehaviorExternalInterface&)>& callback);
  
  void RespondToRCAWithAnim(const ExternalInterface::RobotCompletedAction& rca, BehaviorExternalInterface& behaviorExternalInterface);
  void RespondToResultWithAnim(ActionResult result, BehaviorExternalInterface& behaviorExternalInterface);

};

  
template<typename T>
bool IHelper::DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult,BehaviorExternalInterface&))
{
  return DelegateIfInControl(action, 
          std::bind(callback, static_cast<T*>(this), std::placeholders::_1,
                                                     std::placeholders::_2));
}

template<typename T>
bool IHelper::DelegateIfInControl(IActionRunner* action,
                          void(T::*callback)(const ExternalInterface::RobotCompletedAction&,BehaviorExternalInterface&))
{
  return DelegateIfInControl(action,
                     std::bind(callback,
                               static_cast<T*>(this),
                               std::placeholders::_1,
                               std::placeholders::_2));
}
  
template<typename T>
bool IHelper::DelegateWithResponseAnim(IActionRunner* action,
                                          void(T::*callback)(ActionResult,BehaviorExternalInterface&),
                                          UserFacingActionResultMapFunc mapFunc)
{
  _callbackAfterResponseAnim = std::bind(callback,
                                         static_cast<T*>(this),
                                         std::placeholders::_1,
                                         std::placeholders::_2);
  _actionResultMapFunc = mapFunc;
  
  DEV_ASSERT(_callbackAfterResponseAnim != nullptr,
             "IHelper.DelegateWithResponseAnim.NullActionResultCallback");
  
  return DelegateIfInControl(action, &IHelper::RespondToResultWithAnim);
}

template<typename T>
bool IHelper::DelegateWithResponseAnim(IActionRunner* action,
                                          void(T::*callback)(const ExternalInterface::RobotCompletedAction&,BehaviorExternalInterface&),
                                          UserFacingActionResultMapFunc mapFunc)
{
  _callbackAfterResponseAnimUsingRCA = std::bind(callback,
                                                 static_cast<T*>(this),
                                                 std::placeholders::_1,
                                                 std::placeholders::_2);
  _actionResultMapFunc = mapFunc;
  
  DEV_ASSERT(_callbackAfterResponseAnimUsingRCA != nullptr,
             "IHelper.DelegateWithResponseAnim.NullRCACallback");
  
  return DelegateIfInControl(action, &IHelper::RespondToRCAWithAnim);
}

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_IHelper_H__
