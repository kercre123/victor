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
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/pickupBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/placeBlockHelper.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/rollBlockHelper.h"
#include "util/helpers/boundedWhile.h"

#include <iterator>
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Cozmo {
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperComponent::BehaviorHelperComponent()
: IDependencyManagedComponent<BCComponentID>(BCComponentID::BehaviorHelperComponent)
, _helperFactory(new BehaviorHelperFactory(*this))
{
}

void BehaviorHelperComponent::InitDependent(Robot* robot, const BCCompMap& dependentComponents) 
{
  _beiWrapper = std::make_unique<BEIWrapper>(
    dependentComponents.find(BCComponentID::BehaviorExternalInterface)->second.GetValue<BehaviorExternalInterface>()
  );
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
  helperHandle->Init(_beiWrapper->_bei);
  helperHandle->OnEnteredActivatableScope();
  return helperHandle;
}
  
///////
// Functions and variables for maintaining helper stack
//////

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorHelperComponent::DelegateToHelper(HelperHandle handleToRun,
                                               BehaviorSimpleCallback successCallback,
                                               BehaviorSimpleCallback failureCallback)
{
  ClearStackMaintenanceVars();
  _behaviorSuccessCallback = successCallback;
  _behaviorFailureCallback = failureCallback;
  
  if(!_helperStack.empty()){
    DEV_ASSERT(false, "BehaviorHelperComponent.DelegateToHelper.HelperStackAlreadyExists");
    return false;
  }
  
  PushHelperOntoStackAndUpdate(handleToRun);
  _worldOriginIDAtStart = _beiWrapper->_bei.GetRobotInfo().GetWorldOriginID();
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
void BehaviorHelperComponent::Update()
{
  // Update the active helper stack
  CheckInactiveStackHelpers();
  UpdateActiveHelper();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::CheckInactiveStackHelpers()
{
  if(!_helperStack.empty()){
    // Grab the iterator to the active helper
    auto activeIter = _helperStack.end();
    activeIter--;
    
    // Iterate through all inactive helpers, and give them the opportunity
    // to clear the stack on top of them and become active again.
    auto inactiveIter = _helperStack.begin();
    while(inactiveIter != activeIter){
      const bool shouldCancelDelegates = (*inactiveIter)->ShouldCancelDelegates();

      inactiveIter++;
      if(shouldCancelDelegates){
        ClearStackFromTopToIter(inactiveIter);
        break;
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::UpdateActiveHelper()
{
  IHelper::HelperStatus helperStatus = IHelper::HelperStatus::Running;
  if(!_helperStack.empty()){
    auto activeIter = _helperStack.end();
    activeIter--;

    const auto& robotInfo = _beiWrapper->_bei.GetRobotInfo();
    // TODO: COZMO-10389 - return to base helper if origin changes
    bool blockWorldOriginChange = (robotInfo.GetWorldOriginID() != _worldOriginIDAtStart);
    if(blockWorldOriginChange){
      _worldOriginIDAtStart = robotInfo.GetWorldOriginID();
    }
    while(!_helperStack.empty() &&
          activeIter != _helperStack.end()){
      
      // First, check to see if we've just completed a helper
      if(helperStatus == IHelper::HelperStatus::Complete){
        helperStatus = (*activeIter)->OnDelegateSuccess();
      }else if(helperStatus == IHelper::HelperStatus::Failure){
        helperStatus = (*activeIter)->OnDelegateFailure();
      }
      
      
      HelperHandle newDelegate;
      
      // Override the helper status to fail out of all delegates and return
      // to the lowest level delegate
      if(blockWorldOriginChange && _helperStack.size() != 1){
        helperStatus = IHelper::HelperStatus::Failure;
      }else if(blockWorldOriginChange && _helperStack.size() == 1){
        blockWorldOriginChange = false;
      }
      
      // Allows Delegate to directly check on success/failure
      if(helperStatus == IHelper::HelperStatus::Running){
        // Give helper update tick
        helperStatus = (*activeIter)->UpdateWhileActive(newDelegate);
      }
      
      // If helper completed, start the next helper, or if the
      // helper specified a new delegate, make that the actiev helper
      if(helperStatus != IHelper::HelperStatus::Running){

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
          PushHelperOntoStackAndUpdate(newDelegate);
          activeIter = _helperStack.end();
          activeIter--;
        }
      }
    } // end activeIter while
  }

  // If the helpers have just completed, notify the behavior appropriately
  if(_helperStack.empty() && helperStatus != IHelper::HelperStatus::Running){
    // Copy the callback function so that it can be called after
    // this helper stack has been cleared in case it wants to set up a
    // new helper stack/callbacks
    BehaviorSimpleCallback behaviorCallbackToRun = nullptr;
    
    if(helperStatus == IHelper::HelperStatus::Complete &&
       _behaviorSuccessCallback != nullptr){
      behaviorCallbackToRun = _behaviorSuccessCallback;
    }else if(helperStatus == IHelper::HelperStatus::Failure &&
             _behaviorFailureCallback != nullptr){
      behaviorCallbackToRun = _behaviorFailureCallback;
    }
    
    // Clear variables tied to the current stack's lifetime
    ClearStackLifetimeVars();
    
    // Clear the current callbacks and then call the local callback copy
    // in case the callback wants to set up helpers/callbacks of its own
    ClearStackMaintenanceVars();
    if(behaviorCallbackToRun != nullptr){
      behaviorCallbackToRun();
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::PushHelperOntoStackAndUpdate(HelperHandle handle)
{
  PRINT_CH_INFO("Behaviors", "HelperComponent.StartNewHelper", "%s", handle->GetName().c_str());
  
  handle->OnActivatedInternal();
  _helperStack.push_back(handle);

  // Run the first update immediately so the helper can delegate control
  UpdateActiveHelper();
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

