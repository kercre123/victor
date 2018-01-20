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

#include "coretech/common/engine/objectIDs.h"
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
class IActionRunner;
class BehaviorHelperFactory;

class IHelper : public IBehavior{
  // Enforce creation/interaction through HelperComponent
  friend class BehaviorHelperComponent;
  friend class BehaviorHelperFactory;

public:
  enum class HelperStatus {
    Failure,
    Running,
    Complete
  };

  using HelperStatusCallback = std::function<HelperStatus()>;

  virtual ~IHelper(){};

  const std::string& GetName() const { return _name; }
  
protected:
  // VIC-322: For the time being the behavior helper component ticks helpers instead of
  // the BSM. While this is true it will be impossible to delegate to helpers in the way
  // other Behaviors are delegated to - so don't tie this UpdateInternal in just yet
  
  // Currently unused overrides of IBehavior since no equivalence in old BM system
  virtual void InitInternal() override final;
  virtual bool WantsToBeActivatedInternal() const override { return false;};
  virtual void OnDeactivatedInternal() override {};
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override {}
  virtual void OnEnteredActivatableScopeInternal() override {};
  virtual void OnLeftActivatableScopeInternal() override {};
  virtual void UpdateInternal() override {
  DEV_ASSERT("IHelper.UpdateInternal.TickedImproperly",
             "Helpers are still managed by the BehaviorHelperComponent and this function doesn't work");
  };
  
  using UserFacingActionResultMapFunc = std::function<UserFacingActionResult(ActionResult)>;
  
  struct DelegateProperties{
  public:
    HelperHandle GetDelegateToSet() const { return _delegateToSet;}
    HelperStatusCallback GetOnSuccessFunction() const {return _onSuccessFunction;}
    HelperStatusCallback GetOnFailureFunction() const {return _onFailureFunction;}
    
    void SetDelegateToSet(HelperHandle delegate){ _delegateToSet = delegate;}
    void SetOnSuccessFunction(HelperStatusCallback onSuccess){_onSuccessFunction = onSuccess;}
    void SetOnFailureFunction(HelperStatusCallback onFailure){_onFailureFunction = onFailure;}

    // utility functions to automatically pass or fail when the delegate does
    void FailImmediatelyOnDelegateFailure();
    void SucceedImmediatelyOnDelegateFailure();
    
    void ClearDelegateProperties();
    
  private:
    HelperHandle _delegateToSet;
    HelperStatusCallback _onSuccessFunction = nullptr;
    HelperStatusCallback _onFailureFunction = nullptr;
  };
  
  // Initialize the helper with the behavior to start actions on, and a reference to the factory to delegate
  // out to new helpers
  IHelper(const std::string& name, ICozmoBehavior& behavior, BehaviorHelperFactory& _helperFactory);

  void SetName(const std::string& name) { _name = name; }
  
  // Initialize variables when placed on stack
  virtual void OnActivatedInternal() override final;
  virtual void OnActivatedHelper() {};
  
  IHelper::HelperStatus OnDelegateSuccess();
  IHelper::HelperStatus OnDelegateFailure();
  
  bool IsControlDelegated();
  bool IsActing() const;

  // Helper is being stopped externally
  void Stop(bool isActive);

  // override-able by helpers to handle cleanup when they are stopped
  virtual void StopInternal(bool isActive) {};
  
  // Called each tick when the helper is not active to determine if
  // the stack of delegates this helper has created should be canceled
  // and this helper should become active
  virtual bool ShouldCancelDelegates() const = 0;
  
  // Called each tick when the helper is at the top of the helper stack
  IHelper::HelperStatus UpdateWhileActive(HelperHandle& delegateToSet);
 
  // Called on the first time a helper is ticked while active
  // UpdateWhileActive will be called immediately after on the same tick if no
  // delegate is set
  virtual IHelper::HelperStatus InitBehaviorHelper() = 0;
  
  // Allows sub classes to pass back a delegate, success and failure function for IHelper to manage. If a
  // delegate is set in the delegate properties, then it will be pushed onto the stack, and the callbacks from
  // the properties will be used
  virtual IHelper::HelperStatus UpdateWhileActiveInternal() = 0;
  

  
  ///////
  /// Convenience functions
  //////
  
  ActionResult IsAtPreActionPoseWithVisualVerification(const ObjectID& targetID,
                                                       PreActionPose::ActionType actionType,
                                                       const f32 offsetX_mm = 0,
                                                       const f32 offsetY_mm = 0);
  
  void DelegateAfterUpdate(const DelegateProperties& properties){ _delegateAfterUpdate = properties;}
  
  // Use the set behavior's DelegateIfInControl to delegate to another ICozmoBehavior
  template <typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult));
  
  template <typename T>
  bool DelegateIfInControl(IActionRunner* action, void(T::*callback)(const ExternalInterface::RobotCompletedAction&));
  
  // Pass in a function that maps action results to UserFacingActionResults so that the
  // BehaviorEventAnimationResponseDirector knows how to respond to the failure with the
  // appropriate animation
  template<typename T>
  bool DelegateWithResponseAnim(IActionRunner* action,
                                   void(T::*callback)(ActionResult),
                                   UserFacingActionResultMapFunc mapFunc = nullptr);
  
  template<typename T>
  bool DelegateWithResponseAnim(IActionRunner* action,
                                   void(T::*callback)(const ExternalInterface::RobotCompletedAction&),
                                   UserFacingActionResultMapFunc mapFunc = nullptr);

  bool DelegateIfInControl(IActionRunner* action, BehaviorActionResultCallback callback);
  bool DelegateIfInControl(IActionRunner* action, BehaviorRobotCompletedActionCallback callback);

  // Stop all delegates so that a new delegate can be set
  bool CancelDelegates(bool allowCallback);
  
  // Helpers to access the HelperFactory without needing access to the underlying behavior
  HelperHandle CreatePickupBlockHelper(const ObjectID& targetID,
                                       const PickupBlockParamaters& parameters = {});
  HelperHandle CreatePlaceBlockHelper();
  HelperHandle CreateRollBlockHelper(const ObjectID& targetID,
                                     bool rollToUpright, const RollBlockParameters& parameters = {});
  HelperHandle CreateDriveToHelper(const ObjectID& targetID,
                                   const DriveToParameters& parameters = {});
  HelperHandle CreatePlaceRelObjectHelper(const ObjectID& targetID,
                                          const bool placingOnGround = false,
                                          const PlaceRelObjectParameters& parameters = {});
  
  HelperHandle CreateSearchForBlockHelper(const SearchParameters& params = {});

  
  IHelper::HelperStatus _status;
  
private:
  friend class DelegationComponent;
  void LogStopEvent(bool isActive);
  
  std::string _name;
  bool _hasStarted;
  HelperStatusCallback _onSuccessFunction;
  HelperStatusCallback _onFailureFunction;
  DelegateProperties _delegateAfterUpdate;
  float _timeStarted_s = 0.0f;


  ICozmoBehavior& _behaviorToCallActionsOn;
  BehaviorHelperFactory& _helperFactory;
  
  // Functions for responding to action results with DelegateIfInControlWithResponseAnim
  UserFacingActionResultMapFunc _actionResultMapFunc = nullptr;
  BehaviorActionResultCallback _callbackAfterResponseAnim = nullptr;
  BehaviorRobotCompletedActionCallback _callbackAfterResponseAnimUsingRCA = nullptr;
  
  AnimationTrigger AnimationResponseToActionResult(UserFacingActionResult result);
  
  template<typename T>
  void RespondToActionWithAnim(const T& res,
                               ActionResult actionResult,
                               std::function<void(const T&)>& callback);
  
  void RespondToRCAWithAnim(const ExternalInterface::RobotCompletedAction& rca);
  void RespondToResultWithAnim(ActionResult result);

};

  
template<typename T>
bool IHelper::DelegateIfInControl(IActionRunner* action, void(T::*callback)(ActionResult))
{
  return DelegateIfInControl(action, 
          std::bind(callback, static_cast<T*>(this), std::placeholders::_1));
}

template<typename T>
bool IHelper::DelegateIfInControl(IActionRunner* action,
                                  void(T::*callback)(const ExternalInterface::RobotCompletedAction&))
{
  return DelegateIfInControl(action,
                     std::bind(callback,
                               static_cast<T*>(this),
                               std::placeholders::_1));
}
  
template<typename T>
bool IHelper::DelegateWithResponseAnim(IActionRunner* action,
                                       void(T::*callback)(ActionResult),
                                       UserFacingActionResultMapFunc mapFunc)
{
  _callbackAfterResponseAnim = std::bind(callback,
                                         static_cast<T*>(this),
                                         std::placeholders::_1);
  _actionResultMapFunc = mapFunc;
  
  DEV_ASSERT(_callbackAfterResponseAnim != nullptr,
             "IHelper.DelegateWithResponseAnim.NullActionResultCallback");
  
  return DelegateIfInControl(action, &IHelper::RespondToResultWithAnim);
}

template<typename T>
bool IHelper::DelegateWithResponseAnim(IActionRunner* action,
                                       void(T::*callback)(const ExternalInterface::RobotCompletedAction&),
                                       UserFacingActionResultMapFunc mapFunc)
{
  _callbackAfterResponseAnimUsingRCA = std::bind(callback,
                                                 static_cast<T*>(this),
                                                 std::placeholders::_1);
  _actionResultMapFunc = mapFunc;
  
  DEV_ASSERT(_callbackAfterResponseAnimUsingRCA != nullptr,
             "IHelper.DelegateWithResponseAnim.NullRCACallback");
  
  return DelegateIfInControl(action, &IHelper::RespondToRCAWithAnim);
}

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_IHelper_H__
