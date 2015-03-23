/**
 * File: actionInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements interfaces for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/actionInterface.h"

#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

namespace Anki {
  
  namespace Cozmo {
    
    IActionRunner::IActionRunner()
    : _numRetriesRemaining(0)
    {
      
    }
    
    bool IActionRunner::RetriesRemain()
    {
      if(_numRetriesRemaining > 0) {
        --_numRetriesRemaining;
        return true;
      } else {
        return false;
      }
    }
    
    const std::string& IActionRunner::GetName() const {
      static const std::string name("UnnamedAction");
      return name;
    }
    
#   if USE_ACTION_CALLBACKS
    void IActionRunner::AddCompletionCallback(ActionCompletionCallback callback)
    {
      _completionCallbacks.emplace_back(callback);
    }
    
    void IActionRunner::RunCallbacks(ActionResult result) const
    {
      for(auto callback : _completionCallbacks) {
        callback(result);
      }
    }
#   endif // USE_ACTION_CALLBACKS
    
    
#pragma mark ---- IAction ----
    
    IAction::IAction()
    {
      Reset();
    }
    
    void IAction::Reset()
    {
      _preconditionsMet = false;
      _waitUntilTime = -1.f;
      _timeoutTime = -1.f;
    }
    
    IAction::ActionResult IAction::Update(Robot& robot)
    {
      ActionResult result = RUNNING;
      SetStatus(GetName());
      
      // On first call to Update(), figure out the waitUntilTime
      const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_waitUntilTime < 0.f) {
        _waitUntilTime = currentTimeInSeconds + GetStartDelayInSeconds();
      }
      if(_timeoutTime < 0.f) {
        _timeoutTime = currentTimeInSeconds + GetTimeoutInSeconds();
      }
      
      // Fail if we have exceeded timeout time
      if(currentTimeInSeconds > _timeoutTime) {
        PRINT_NAMED_INFO("IAction.Update.TimedOut",
                         "%s timed out after %.1f seconds.\n",
                         GetName().c_str(), GetTimeoutInSeconds());
        
        result = FAILURE_TIMEOUT;
      }
      // Don't do anything until we have reached the waitUntilTime
      else if(currentTimeInSeconds > _waitUntilTime)
      {
        
        if(!_preconditionsMet) {
          //PRINT_NAMED_INFO("IAction.Update", "Updating %s: checking preconditions.\n", GetName().c_str());
          SetStatus(GetName() + ": check preconditions");
          
          // Note that derived classes will define what to do when pre-conditions
          // are not met: if they return RUNNING, then the action will effectively
          // just wait for the preconditions to be met. Otherwise, a failure
          // will get propagated out as the return value of the Update method.
          result = Init(robot);
          if(result == SUCCESS) {
            PRINT_NAMED_INFO("IAction.Update.PrecondtionsMet",
                             "Preconditions for %s successfully met.\n", GetName().c_str());
            
            // If preconditions were successfully met, switch result to RUNNING
            // so that we don't think the entire action is completed. (We still
            // need to do CheckIfDone() calls!)
            // TODO: there's probably a tidier way to do this.
            _preconditionsMet = true;
            result = RUNNING;
            
            // Don't check if done until a sufficient amount of time has passed
            // after preconditions are met
            _waitUntilTime = currentTimeInSeconds + GetCheckIfDoneDelayInSeconds();
          }
          
        } else {
          //PRINT_NAMED_INFO("IAction.Update", "Updating %s: checking if done.\n", GetName().c_str());
          SetStatus(GetName() + ": check if done");
          
          // Pre-conditions already met, just run until done
          result = CheckIfDone(robot);
        }
      } // if(currentTimeInSeconds > _waitUntilTime)
      
      if(result == FAILURE_RETRY && RetriesRemain()) {
        PRINT_NAMED_INFO("IAction.Update.CurrentActionFailedRetrying",
                         "Robot %d failed running action %s. Retrying.\n",
                         robot.GetID(), GetName().c_str());
        
        Reset();
        result = RUNNING;
      }
      
      if(result != RUNNING) {
#       if USE_ACTION_CALLBACKS
        RunCallbacks(result);
#       endif
        
        // Notify any listeners about this action's completion.
        // TODO: Populate the signal with any action-specific info?
        CozmoEngineSignals::RobotCompletedActionSignal().emit(robot.GetID(), result == SUCCESS);
      }
      
      return result;
    } // Update()
    
  } // namespace Cozmo
} // namespace Anki
