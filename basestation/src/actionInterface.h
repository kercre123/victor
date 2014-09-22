/**
 * File: actionInterface.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Defines interfaces for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ACTION_INTERFACE_H
#define ANKI_COZMO_ACTION_INTERFACE_H

#include "anki/common/types.h"
#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/shared/cozmoTypes.h"

#include "actionableObject.h"

#include <list>

// Not sure if we want to support callbacks yet, but this switch enables some
// preliminary callback code for functions to be run when an action completes.
#define USE_ACTION_CALLBACKS 1

// TODO: Is this Cozmo-specific or can it be moved to coretech?
// (Note it does require a Robot, which is currently only present in Cozmo)

namespace Anki {
 
  namespace Cozmo {
    
    // Forward Declarations:
    class Robot;
    
    // Parent container for running actions, which can hold simple actions as
    // well as "compound" ones, defined elsewhere.
    class IActionRunner
    {
    public:
      
      typedef enum  {
        RUNNING,
        SUCCESS,
        FAILURE_TIMEOUT,
        FAILURE_PROCEED,
        FAILURE_RETRY,
        FAILURE_ABORT
      } ActionResult;
      
      IActionRunner();
      virtual ~IActionRunner() { }
      
      virtual ActionResult Update(Robot& robot) = 0;
      
      // If a FAILURE_RETRY is encountered, how many times will the action
      // be retried before return FAILURE_ABORT.
      void SetNumRetries(const u8 numRetries) {_numRetriesRemaining = numRetries;}
      
      // Override this in derived classes to get more helpful messages for
      // debugging. Otherwise, defaults to "UnnamedAction".
      virtual const std::string& GetName() const;
      
      virtual void Reset() = 0;
      
      // Get last status message
      const std::string& GetStatus() const { return _statusMsg; }
      
    protected:
      
      bool RetriesRemain();
      
      // Derived actions can use this to set custom status messages here.
      void SetStatus(const std::string& msg);
      
    private:
      u8 _numRetriesRemaining;
      
      std::string   _statusMsg;
      
      
#     if USE_ACTION_CALLBACKS
    public:
      using ActionCompletionCallback = std::function<void(ActionResult)>;
      void  AddCompletionCallback(ActionCompletionCallback callback);
      
    protected:
      void RunCallbacks(ActionResult result) const;
      
    private:
      std::list<ActionCompletionCallback> _completionCallbacks;
#     endif
      
    }; // class IActionRunner
    
    inline void IActionRunner::SetStatus(const std::string& msg) {
      _statusMsg = msg;
    }
    
    
    // Action Interface
    class IAction : public IActionRunner
    {
    public:
      
      IAction();
      virtual ~IAction() { }
      
      // This Update() is what gets called from the outside.  It in turn
      // handles timing delays and runs (protected) Init() and CheckIfDone()
      // methods. Those are the virtual methods that specific classes should
      // implement to get desired action behaviors. Note that this method
      // is final and cannot be overridden by specific individual actions.
      virtual ActionResult Update(Robot& robot) override final;
      
      // Provide a retry function that will be called by Update() if
      // FAILURE_RETRY is returned by the derived CheckIfDone() method.
      //void SetRetryFunction(std::function<Result(Robot&)> retryFcn);
      
      // Runs the retry function if one was specified.
      //Result Retry();
      
    protected:
      
      // Derived Actions should implement these.
      virtual ActionResult  Init(Robot& robot) { return SUCCESS; } // Optional: default is no preconditions to meet
      virtual ActionResult  CheckIfDone(Robot& robot) = 0;
      
      //
      // Timing delays:
      //  (e.g. for allowing for communications to physical robot to have an effect)
      //
      
      // Before checking preconditions. Optional: default is 0.5s delay
      virtual f32 GetStartDelayInSeconds()       const { return 0.5f; }
      
      // Before first CheckIfDone() call, after preconditions are met. Optional: default is 0.5s delay
      virtual f32 GetCheckIfDoneDelayInSeconds() const { return 0.5f; }
      
      // Before giving up on entire action. Optional: default is one minute
      virtual f32 GetTimeoutInSeconds()          const { return 60.f; }
      
      virtual void Reset() override;
      
    private:
      
      bool          _preconditionsMet;
      f32           _waitUntilTime;
      f32           _timeoutTime;
      
    }; // class IAction
       
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTION_INTERFACE_H
