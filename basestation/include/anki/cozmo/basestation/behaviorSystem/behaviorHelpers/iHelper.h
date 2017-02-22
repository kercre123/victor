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
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/helperHandle.h"
#include "anki/cozmo/basestation/behaviors/iBehavior_fwd.h"
#include "clad/types/actionResults.h"
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
  
  using SimpleCallbackWithRobot = std::function<void(Robot& robot)>;
  using StatusCallbackWithRobot = std::function<BehaviorStatus(Robot& robot)>;
  
  struct DelegateProperties{
  public:
    HelperHandle GetDelegateToSet() const { return _delegateToSet;}
    StatusCallbackWithRobot GetOnSuccessFunction() const {return _onSuccessFunction;}
    StatusCallbackWithRobot GetOnFailureFunction() const {return _onFailureFunction;}
    
    void SetDelegateToSet(HelperHandle delegate){ _delegateToSet = delegate;}
    void SetOnSuccessFunction(StatusCallbackWithRobot onSuccess){_onSuccessFunction = onSuccess;}
    void SetOnFailureFunction(StatusCallbackWithRobot onFailure){_onFailureFunction = onFailure;}

    
  private:
    HelperHandle _delegateToSet;
    StatusCallbackWithRobot _onSuccessFunction;
    StatusCallbackWithRobot _onFailureFunction;
  };
  
  // Initialize the helper with the behavior to start actions on, and a reference to the factory to delegate
  // out to new helpers
  IHelper(const std::string& name, Robot& robot, IBehavior* behavior, BehaviorHelperFactory& _helperFactory);

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
  virtual BehaviorStatus Init(Robot& robot,
                              DelegateProperties& delegateProperties) = 0;
  
  // Allows sub classes to pass back a delegate, success and failure function for IHelper to manage. If a
  // delegate is set in the delegate properties, then it will be pushed onto the stack, and the callbacks from
  // the properties will be used
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot,
                                                   DelegateProperties& delegateProperties) = 0;

  
  // Use the set behavior's start acting to perform an action
  template <typename T>
  bool StartActing(IActionRunner* action, void(T::*callback)(ActionResult,Robot&));

  bool StartActing(IActionRunner* action, BehaviorActionResultWithRobotCallback callback);

  // Helpers to access the HelperFactory without needing access to the underlying behavior
  HelperHandle CreatePickupBlockHelper(Robot& robot, const ObjectID& targetID);
  HelperHandle CreatePlaceBlockHelper(Robot& robot);
  HelperHandle CreateRollBlockHelper(Robot& robot, const ObjectID& targetID, bool rollToUpright);

  
  BehaviorStatus _status;
  
private:
  bool _hasStarted;
  StatusCallbackWithRobot _onSuccessFunction;
  StatusCallbackWithRobot _onFailureFunction;
  std::string _name;

  IBehavior* _behaviorToCallActionsOn;
  BehaviorHelperFactory& _helperFactory;
};

  
template<typename T>
bool IHelper::StartActing(IActionRunner* action, void(T::*callback)(ActionResult,Robot&))
{
  return StartActing(action, 
          std::bind(callback, static_cast<T*>(this), std::placeholders::_1,
                                                     std::placeholders::_2));
}
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_IHelper_H__
