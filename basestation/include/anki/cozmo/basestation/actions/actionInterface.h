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
#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/basestation/actionableObject.h"
#include "anki/cozmo/basestation/actions/actionContainers.h"

#include "clad/types/actionTypes.h"
#include "clad/types/animationKeyFrames.h"

#include "util/random/randomGenerator.h"

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
      
      IActionRunner(Robot& robot);
      virtual ~IActionRunner();
      
      ActionResult Update();
      
      Robot& GetRobot() { return _robot; }
      
      // Tags can be used to identify specific actions. A unique tag is assigned
      // at construction, or it can be overridden with SetTag(). The Tag is
      // returned in the ActionCompletion signal as well.
      // Returns true if the tag has been set, false if it is invalid or already exists
      bool SetTag(u32 tag);
      // If a custom tag has been set return will be that otherwise it is the same as the
      // auto-generated tag
      u32  GetTag() const { return _customTag; }
      
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
      
      // If this method returns true, then it means the derived class is interruptible,
      // can safely be re-queued using "NOW_AND_RESUME", and will pick back up safely
      // after the newly-queued action completes. Otherwise, this action will just
      // be cancelled when NOW_AND_RESUME is used. Note that this relies on
      // sub-classes implementing InterruptInternal() and Reset().
      bool Interrupt();
      
      // Override this to take care of anything that needs to be done on Retry/Interrupt.
      virtual void Reset() = 0;
      
      // Get last status message
      const std::string& GetStatus() const { return _statusMsg; }
      
      // Override to have the action disable any animation tracks that may have
      // already been streamed and are in the robot's buffer, so they don't
      // interfere with the action. Note: uses the bits defined by AnimTrackFlag.
      virtual u8 GetAnimTracksToDisable() const { return 0; }
      
      // Override these to have the action allow the robot to move certain
      // subsystems while the action executes. I.e., by default actions
      // will lockout all control of the robot, and extra movement commands are ignored.
      // Note: uses the bits defined by AnimTrackFlag.
      virtual u8 GetMovementTracksToIgnore() const;
      
      // Used (e.g. in initialization of CompoundActions) to specify that a
      // consituent action should not try to lock or unlock tracks it uses
      void SetSuppressTrackLocking(bool tf) { _suppressTrackLocking = tf; }

      // Override this to fill in the ActionCompletedStruct emitted as part of the
      // completion signal with an action finishes. Note that this public because
      // subclasses that are composed of other actions may want to make use of
      // the completion info of their constituent actions.
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const { }

      // Enable/disable message display (Default is true)
      void EnableMessageDisplay(bool tf) { _displayMessages = tf; }
      bool IsMessageDisplayEnabled() const { return _displayMessages; }
      
      void SetEmitCompletionSignal(bool shouldEmit) { _emitCompletionSignal = shouldEmit; }
      bool GetEmitCompletionSignal() const { return _emitCompletionSignal; }

      // Called when the action stops running and sets varibles needed for completion.
      // This calls the overload-able GetCompletionUnion() method above.
      void PrepForCompletion();

    protected:
      
      Robot& _robot;
      
      virtual ActionResult UpdateInternal() = 0;
      
      // By default, actions are not interruptable
      virtual bool InterruptInternal() { return false; }
      
      bool RetriesRemain();
      
      // Derived actions can use this to set custom status messages here.
      void SetStatus(const std::string& msg);
      
    private:

      u8            _numRetriesRemaining = 0;
      
      std::string   _statusMsg;
      
      ActionResult         _result;
      ActionCompletedUnion _completionUnion;
      RobotActionType      _type;
      std::string          _name;
      uint8_t              _animTracks      = (uint8_t)AnimTrackFlag::ENABLE_ALL_TRACKS;
      uint8_t              _movementTracks  = (uint8_t)AnimTrackFlag::ENABLE_ALL_TRACKS;
      
      bool          _suppressTrackLocking   = false;
      bool          _isRunning              = false;
      bool          _isCancelled            = false;
      bool          _isInterrupted          = false;
      bool          _isFinished             = false;
      bool          _displayMessages        = true;
      bool          _emitCompletionSignal   = true;
      
      // Auto-generated tag
      u32           _idTag;
      u32           _customTag;
      
      static u32                sTagCounter;
      // Set of tags that are in use
      static std::set<u32> sInUseTagSet;
      
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
      
      IAction(Robot& robot);
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
      virtual ActionResult UpdateInternal() override final;

      // Derived Actions should implement these.
      virtual ActionResult  Init() { return ActionResult::SUCCESS; } // Optional: default is no preconditions to meet
      virtual ActionResult  CheckIfDone() = 0;
      
      //
      // Timing delays:
      //  (e.g. for allowing for communications to physical robot to have an effect)
      //
      
      // Before checking preconditions. Optional: default is no delay
      virtual f32 GetStartDelayInSeconds()       const { return 0.0f; }
      
      // Before first CheckIfDone() call, after preconditions are met. Optional: default is no delay
      virtual f32 GetCheckIfDoneDelayInSeconds() const { return 0.0f; }
      
      // Before giving up on entire action. Optional: default is 30 seconds
      virtual f32 GetTimeoutInSeconds()          const { return 30.f; }
      
      virtual void Reset() override final;
      
      // A random number generator all subclasses can share
      Util::RandomGenerator& GetRNG() const;
      
    private:
      
      bool          _preconditionsMet;
      f32           _waitUntilTime;
      f32           _timeoutTime;
      
    }; // class IAction
    
    inline Util::RandomGenerator& IAction::GetRNG() const {
      static Util::RandomGenerator sRNG;
      return sRNG;
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTION_INTERFACE_H
