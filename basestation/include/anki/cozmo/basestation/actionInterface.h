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

#include "anki/cozmo/basestation/actionableObject.h"
#include "clad/types/actionTypes.h"

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
      
      IActionRunner();
      virtual ~IActionRunner() { }
      
      ActionResult Update(Robot& robot);
      
      void Cancel() { _isCancelled = true; }
      
      // Tags can be used to identify specific actions. A unique tag is assigned
      // at construction, or it can be overridden with SetTag(). The Tag is
      // returned in the ActionCompletion signal as well.
      void SetTag(u32 tag) { _idTag = tag; }
      u32  GetTag() const  { return _idTag; }
      
      // Derived classes can implement any required cleanup by overriding this
      // method. It is called when Update() is about return anything other than
      // RUNNING (including cancellation). Note that it cannot change the ActionResult
      // (so if Update is about to failure or success, nothing that Cleanup does
      // can change that.)
      // Note that this is also called on cancellation, so this should also cleanup
      // after the action when stopped mid-way through (and, for example, abort
      // [physical] robot functions).
      virtual void Cleanup(Robot& robot) { }
      
      // If a FAILURE_RETRY is encountered, how many times will the action
      // be retried before return FAILURE_ABORT.
      void SetNumRetries(const u8 numRetries) {_numRetriesRemaining = numRetries;}
      
      // Override this in derived classes to get more helpful messages for
      // debugging. Otherwise, defaults to "UnnamedAction".
      virtual const std::string& GetName() const;
      
      // Override this in derived classes to return a (probably unique) integer
      // representing this action's type. This will be reported in any
      // completion signal that is emitted.
      virtual RobotActionType GetType() const = 0;
      
      // Override this to take care of anything that needs to be done on Retry.
      // This should take care of anything that might otherwise be done by the
      // constructor (i.e. before Init() is ever called.) For example, if your
      // action allocates a new IAction[Runner] as a member as part of Init()
      // but only deletes it upon destruction, you probably want to delete it
      // in Reset() as well.
      virtual void Reset() = 0;
      
      // Get last status message
      const std::string& GetStatus() const { return _statusMsg; }
      
      // Override these to have the action allow the robot to move certain
      // subsystems while the action executes. I.e., by default actions
      // will lockout all control of the robot.
      virtual bool ShouldLockHead() const   { return true; }
      virtual bool ShouldLockLift() const   { return true; }
      virtual bool ShouldLockWheels() const { return true; }
      
      // Used (e.g. in initialization of CompoundActions) to specify that a
      // consituent action is part of a compound action
      void SetIsPartOfCompoundAction(bool tf) { _isPartOfCompoundAction = tf; }

      // Override this to fill in the ActionCompletedStruct emitted as part of the
      // completion signal with an action finishes. Note that this public because
      // subclasses that are composed of other actions may want to make use of
      // the completion info of their constituent actions.
      virtual void GetCompletionStruct(Robot& robot, ActionCompletedStruct& completionInfo) const;

    protected:
      
      virtual ActionResult UpdateInternal(Robot& robot) = 0;
      
      bool RetriesRemain();
      
      // Derived actions can use this to set custom status messages here.
      void SetStatus(const std::string& msg);
      
    private:
      
      // This is called when the action stops running, as long as it is not
      // marked as being part of a compound action. This calls the overload-able
      // GetCompletionStruct() method above.
      void EmitCompletionSignal(Robot& robot, ActionResult result) const;

      u8            _numRetriesRemaining;
      
      std::string   _statusMsg;
      
      bool          _isPartOfCompoundAction;
      bool          _isRunning;
      bool          _isCancelled;
      
      u32           _idTag;
      
      static u32    sTagCounter;
      
#   if USE_ACTION_CALLBACKS
    public:
      using ActionCompletionCallback = std::function<void(ActionResult)>;
      void  AddCompletionCallback(ActionCompletionCallback callback);
      
    protected:
      void RunCallbacks(ActionResult result) const;
      
    private:
      std::list<ActionCompletionCallback> _completionCallbacks;
#   endif
      
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
      
      
      // Provide a retry function that will be called by Update() if
      // FAILURE_RETRY is returned by the derived CheckIfDone() method.
      //void SetRetryFunction(std::function<Result(Robot&)> retryFcn);
      
      // Runs the retry function if one was specified.
      //Result Retry();
      
    protected:
      
      // This UpdateInternal() is what gets called by IActionRunner's Update().  It in turn
      // handles timing delays and runs (protected) Init() and CheckIfDone()
      // methods. Those are the virtual methods that specific classes should
      // implement to get desired action behaviors. Note that this method
      // is final and cannot be overridden by specific individual actions.
      virtual ActionResult UpdateInternal(Robot& robot) override final;

      // Derived Actions should implement these.
      virtual ActionResult  Init(Robot& robot) { return ActionResult::SUCCESS; } // Optional: default is no preconditions to meet
      virtual ActionResult  CheckIfDone(Robot& robot) = 0;
      
      //
      // Timing delays:
      //  (e.g. for allowing for communications to physical robot to have an effect)
      //
      
      // Before checking preconditions. Optional: default is 0.05s delay
      virtual f32 GetStartDelayInSeconds()       const { return 0.05f; }
      
      // Before first CheckIfDone() call, after preconditions are met. Optional: default is 0.05s delay
      virtual f32 GetCheckIfDoneDelayInSeconds() const { return 0.05f; }
      
      // Before giving up on entire action. Optional: default is 30 seconds
      virtual f32 GetTimeoutInSeconds()          const { return 30.f; }
      
      virtual void Reset() override;
      
    private:
      
      bool          _preconditionsMet;
      f32           _waitUntilTime;
      f32           _timeoutTime;
      
    }; // class IAction
       
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTION_INTERFACE_H
