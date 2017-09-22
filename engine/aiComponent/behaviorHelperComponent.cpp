/**
 * File: behaviorHelperComponent.cpp
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

#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/pickupBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/placeBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/rollBlockHelper.h"
#include "engine/robot.h"
#include "util/helpers/boundedWhile.h"

#include <iterator>
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperComponent::BehaviorHelperComponent()
: _helperFactory(new BehaviorHelperFactory(*this))
{
  
  
}

///////
// Functions for creating new handles
//////
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HelperHandle BehaviorHelperComponent::AddHelperToComponent(IHelper*& helper)
{
  // We actually don't want to track for right now
  auto helperHandle = std::shared_ptr<IHelper>(helper);
  helper = nullptr;
  return helperHandle;
}
  
///////
// Functions and variables for maintaining helper stack
//////

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorHelperComponent::DelegateToHelper(BehaviorExternalInterface& behaviorExternalInterface,
                                               HelperHandle handleToRun,
                                               BehaviorSimpleCallbackWithExternalInterface successCallback,
                                               BehaviorSimpleCallbackWithExternalInterface failureCallback)
{
  ClearStackMaintenanceVars();
  _behaviorSuccessCallback = successCallback;
  _behaviorFailureCallback = failureCallback;
  
  if(!_helperStack.empty()){
    DEV_ASSERT(false, "BehaviorHelperComponent.DelegateToHelper.HelperStackAlreadyExists");
    return false;
  }
  
  PushHelperOntoStackAndUpdate(behaviorExternalInterface, handleToRun);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  _worldOriginIDAtStart = robot.GetWorldOriginID();
  return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorHelperComponent::StopHelperWithoutCallback(const HelperHandle& helperToStop)
{
  ClearStackLifetimeVars();
  auto stackBegin = _helperStack.begin();
  if(helperToStop == *stackBegin){
    ClearStackFromTopToIter(stackBegin);
    return true;
  }
  
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::Update(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Update the active helper stack
  CheckInactiveStackHelpers(behaviorExternalInterface);
  UpdateActiveHelper(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::CheckInactiveStackHelpers(BehaviorExternalInterface& behaviorExternalInterface)
{
  if(!_helperStack.empty()){
    // Grab the iterator to the active helper
    auto activeIter = _helperStack.end();
    activeIter--;
    
    // Iterate through all inactive helpers, and give them the opportunity
    // to clear the stack on top of them and become active again.
    auto inactiveIter = _helperStack.begin();
    while(inactiveIter != activeIter){
      const bool shouldCancelDelegates = (*inactiveIter)->ShouldCancelDelegates(behaviorExternalInterface);

      inactiveIter++;
      if(shouldCancelDelegates){
        ClearStackFromTopToIter(inactiveIter);
        break;
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::UpdateActiveHelper(BehaviorExternalInterface& behaviorExternalInterface)
{
  IBehavior::Status helperStatus = IBehavior::Status::Running;
  if(!_helperStack.empty()){
    auto activeIter = _helperStack.end();
    activeIter--;
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    // TODO: COZMO-10389 - return to base helper if origin changes
    bool blockWorldOriginChange = (robot.GetWorldOriginID() != _worldOriginIDAtStart);
    if(blockWorldOriginChange){
      _worldOriginIDAtStart = robot.GetWorldOriginID();
    }
    while(!_helperStack.empty() &&
          activeIter != _helperStack.end()){
      
      // First, check to see if we've just completed a helper
      if(helperStatus == IBehavior::Status::Complete){
        helperStatus = (*activeIter)->OnDelegateSuccess(behaviorExternalInterface);
      }else if(helperStatus == IBehavior::Status::Failure){
        helperStatus = (*activeIter)->OnDelegateFailure(behaviorExternalInterface);
      }
      
      
      HelperHandle newDelegate;
      
      // Override the helper status to fail out of all delegates and return
      // to the lowest level delegate
      if(blockWorldOriginChange && _helperStack.size() != 1){
        helperStatus = IBehavior::Status::Failure;
      }else if(blockWorldOriginChange && _helperStack.size() == 1){
        blockWorldOriginChange = false;
      }
      
      // Allows Delegate to directly check on success/failure
      if(helperStatus == IBehavior::Status::Running){
        // Give helper update tick
        helperStatus = (*activeIter)->UpdateWhileActive(behaviorExternalInterface, newDelegate);
      }
      
      // If helper completed, start the next helper, or if the
      // helper specified a new delegate, make that the actiev helper
      if(helperStatus != IBehavior::Status::Running){

        PRINT_CH_INFO("Behaviors", "HelperComponent.UpdateActive.Complete", "%s no longer running",
                      (*activeIter)->GetName().c_str());
        
        // Call Stop on the helper that just finished
        const bool isActive = true;
        (*activeIter)->Stop(isActive);
        
        activeIter = _helperStack.erase(activeIter);
        activeIter--;

        if(!_helperStack.empty()) {
          PRINT_CH_INFO("Behaviors", "HelperComponent.UpdateActive.ReturningToPrevious", "returning to helper: %s",
                      (*activeIter)->GetName().c_str());
        }        
      }else{
        if(newDelegate){
          PRINT_CH_INFO("Behaviors", "HelperComponent.UpdateActive.SubHelper", "Helper %s delegated to helper %s",
                        (*activeIter)->GetName().c_str(),
                        newDelegate->GetName().c_str());
        }
        
        activeIter = _helperStack.end();

        if(newDelegate){          
          PushHelperOntoStackAndUpdate(behaviorExternalInterface, newDelegate);
          activeIter = _helperStack.end();
          activeIter--;
        }
      }
    } // end activeIter while
  }

  // If the helpers have just completed, notify the behavior appropriately
  if(_helperStack.empty() && helperStatus != IBehavior::Status::Running){
    // Copy the callback function so that it can be called after
    // this helper stack has been cleared in case it wants to set up a
    // new helper stack/callbacks
    BehaviorSimpleCallbackWithExternalInterface behaviorCallbackToRun = nullptr;
    
    if(helperStatus == IBehavior::Status::Complete &&
       _behaviorSuccessCallback != nullptr){
      behaviorCallbackToRun = _behaviorSuccessCallback;
    }else if(helperStatus == IBehavior::Status::Failure &&
             _behaviorFailureCallback != nullptr){
      behaviorCallbackToRun = _behaviorFailureCallback;
    }
    
    // Clear variables tied to the current stack's lifetime
    ClearStackLifetimeVars();
    
    // Clear the current callbacks and then call the local callback copy
    // in case the callback wants to set up helpers/callbacks of its own
    ClearStackMaintenanceVars();
    if(behaviorCallbackToRun != nullptr){
      behaviorCallbackToRun(behaviorExternalInterface);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::PushHelperOntoStackAndUpdate(BehaviorExternalInterface& behaviorExternalInterface, HelperHandle handle)
{
  PRINT_CH_INFO("Behaviors", "HelperComponent.StartNewHelper", "%s", handle->GetName().c_str());
  
  handle->OnActivatedInternal(behaviorExternalInterface);
  _helperStack.push_back(handle);

  // Run the first update immediately so the helper can start acting
  UpdateActiveHelper(behaviorExternalInterface);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::ClearStackMaintenanceVars()
{
  _behaviorSuccessCallback = nullptr;
  _behaviorFailureCallback = nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::ClearStackLifetimeVars()
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::ClearStackFromTopToIter(HelperIter& iter_in)
{
  // Remove all helpers above the helper on the stack, starting at the end
  const auto& lowestHelperToStop = *iter_in;

  bool last = true;
  BOUNDED_WHILE( 1000, !_helperStack.empty() ) {
    const bool lastIter = lowestHelperToStop == _helperStack.back();

    _helperStack.back()->Stop(last);
    last = false;
    _helperStack.pop_back();

    // break after clearing the "lowest" item to clear
    if( lastIter ) {
      break;
    }
  }
}
  
} // namespace Cozmo
} // namespace Anki

