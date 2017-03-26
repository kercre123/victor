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

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/pickupBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/placeBlockHelper.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/rollBlockHelper.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/helpers/boundedWhile.h"

#include <iterator>
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

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
bool BehaviorHelperComponent::DelegateToHelper(Robot& robot,
                                               HelperHandle handleToRun,
                                               BehaviorSimpleCallbackWithRobot successCallback,
                                               BehaviorSimpleCallbackWithRobot failureCallback)
{
  ClearStackVars();
  _behaviorSuccessCallback = successCallback;
  _behaviorFailureCallback = failureCallback;
  
  if(!_helperStack.empty()){
    DEV_ASSERT(false, "BehaviorHelperComponent.DelegateToHelper.HelperStackAlreadyExists");
    return false;
  }
  
  PushHelperOntoStackAndUpdate(robot, handleToRun);
  _worldOriginAtStart = robot.GetWorldOrigin();
  return true;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorHelperComponent::StopHelperWithoutCallback(const HelperHandle& helperToStop)
{
  auto stackBegin = _helperStack.begin();
  if(helperToStop == *stackBegin){
    ClearStackFromTopToIter(stackBegin);
    return true;
  }
  
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::Update(Robot& robot)
{
  // Update the active helper stack
  CheckInactiveStackHelpers(robot);
  UpdateActiveHelper(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::CheckInactiveStackHelpers(const Robot& robot)
{
  if(!_helperStack.empty()){
    // Grab the iterator to the active helper
    auto activeIter = _helperStack.end();
    activeIter--;
    
    // Iterate through all inactive helpers, and give them the opportunity
    // to clear the stack on top of them and become active again.
    auto inactiveIter = _helperStack.begin();
    while(inactiveIter != activeIter){
      const bool shouldCancelDelegates = (*inactiveIter)->ShouldCancelDelegates(robot);

      inactiveIter++;
      if(shouldCancelDelegates){
        ClearStackFromTopToIter(inactiveIter);
        break;
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::UpdateActiveHelper(Robot& robot)
{
  IBehavior::Status helperStatus = IBehavior::Status::Running;
  if(!_helperStack.empty()){
    auto activeIter = _helperStack.end();
    activeIter--;
    // TODO: COZMO-10389 - return to base helper if origin changes
    bool blockWorldOriginChange = robot.GetWorldOrigin() != _worldOriginAtStart;
    if(blockWorldOriginChange){
      _worldOriginAtStart = robot.GetWorldOrigin();
    }
    while(!_helperStack.empty() &&
          activeIter != _helperStack.end()){
      
      // First, check to see if we've just completed a helper
      if(helperStatus == IBehavior::Status::Complete){
        helperStatus = (*activeIter)->OnDelegateSuccess(robot);
      }else if(helperStatus == IBehavior::Status::Failure){
        helperStatus = (*activeIter)->OnDelegateFailure(robot);
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
        helperStatus = (*activeIter)->UpdateWhileActive(robot, newDelegate);
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
          PushHelperOntoStackAndUpdate(robot, newDelegate);
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
    BehaviorSimpleCallbackWithRobot behaviorCallbackToRun = nullptr;
    
    if(helperStatus == IBehavior::Status::Complete &&
       _behaviorSuccessCallback != nullptr){
      behaviorCallbackToRun = _behaviorSuccessCallback;
    }else if(helperStatus == IBehavior::Status::Failure &&
             _behaviorFailureCallback != nullptr){
      behaviorCallbackToRun = _behaviorFailureCallback;
    }
    
    // Clear the current callbacks and then call the local callback copy
    // in case the callback wants to set up helpers/callbacks of its own
    ClearStackVars();
    if(behaviorCallbackToRun != nullptr){
      behaviorCallbackToRun(robot);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::PushHelperOntoStackAndUpdate(Robot& robot, HelperHandle handle)
{
  PRINT_CH_INFO("Behaviors", "HelperComponent.StartNewHelper", "%s", handle->GetName().c_str());
  
  handle->InitializeOnStack();
  _helperStack.push_back(handle);

  // Run the first update immediately so the helper can start acting
  UpdateActiveHelper(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorHelperComponent::ClearStackVars()
{
  _behaviorSuccessCallback = nullptr;
  _behaviorFailureCallback = nullptr;
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

