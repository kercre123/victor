/**
 * File: actionQueue.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Defines IAction interface for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ACTIONQUEUE_H
#define ANKI_COZMO_ACTIONQUEUE_H

#include "anki/common/types.h"
#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/shared/cozmoTypes.h"

#include "actionableObject.h"

#include <list>

namespace Anki {
  
  // TODO: Is this Cozmo-specific or can it be moved to coretech?
  // (Note it does require a Robot, which is currently only present in Cozmo)
  
  
  namespace Vision {
    // Forward Declarations:
    class KnownMarker;
  }
  
  namespace Cozmo {

    // Forward Declarations:
    class Robot;
    
#define USE_ACTION_CALLBACKS 1
    
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

    
#pragma mark ---- IAction ---
    
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
    
    // This is an ordered list of actions to be run. It is similar to an
    // CompoundActionSequential, but actions can be added to it dynamically,
    // either "next" or at the end of the queue. As actions are completed,
    // they are popped off the queue. Thus, when it is empty, it is "done".
    class ActionQueue
    {
    public:
      ActionQueue();
      
      ~ActionQueue();
      
      Result   Update(Robot& robot);
      
      Result   QueueNext(IActionRunner *,  u8 numRetries = 0);
      Result   QueueAtEnd(IActionRunner *, u8 numRetires = 0);
      
      void     Clear();
      
      bool     IsEmpty() const { return _queue.empty(); }
      
      IActionRunner* GetCurrentAction();
      void           PopCurrentAction();
      
      void Print() const;
      
    protected:
      std::list<IActionRunner*> _queue;
      
    }; // class ActionQueue
    
    
    // This is a list of concurrent actions to be run, addressable by ID handle.
    // Each slot in the list is really a queue, to which new actions can be added
    // using that slot's ID handle. When a slot finishes, it is popped.
    class ActionList
    {
    public:
      using SlotHandle = u32;

      ActionList();
      ~ActionList();
      
      // Updates the current action of each queue in each slot
      Result     Update(Robot& robot);
      
      // Add a new action to be run concurrently, generating a new slot, whose
      // handle is returned. If there is no desire to queue anything to run after
      // this action, the SlotHandle can be ignored.
      SlotHandle AddAction(IActionRunner* action, u8 numRetries = 0);
      
      // Queue an action into a specific slot. If that slot does not exist
      // (perhaps because it completed before this call) it will be created.
      Result     QueueActionNext(SlotHandle  atSlot, IActionRunner* action, u8 numRetries = 0);
      Result     QueueActionAtEnd(SlotHandle atSlot, IActionRunner* action, u8 numRetries = 0);
      
      bool       IsEmpty() const;
      
      void       Clear();
      
      void       Print() const;
      
    protected:
      
      SlotHandle _slotCounter;
      std::map<SlotHandle, ActionQueue> _queues;
      
    }; // class ActionList
    
    
    inline Result ActionList::QueueActionNext(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      return _queues[atSlot].QueueNext(action, numRetries);
    }
    
    inline Result ActionList::QueueActionAtEnd(SlotHandle atSlot, IActionRunner* action, u8 numRetries)
    {
      return _queues[atSlot].QueueAtEnd(action, numRetries);
    }

    
#pragma mark ---- ICompoundAction ----
    
    // Interface for compound actions, which are fixed sets of actions to be
    // run together or in order (determined by derived type)
    class ICompoundAction : public IActionRunner
    {
    public:
      ICompoundAction(std::initializer_list<IActionRunner*> actions);
      
      // Constituent actions will be deleted upon destruction of the group
      virtual ~ICompoundAction();
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      // Call the constituent actions' Reset() methods and mark them each not done.
      virtual void Reset() override;
      
      std::list<std::pair<bool, IActionRunner*> > _actions;
      std::string _name;
    };
    
    
    // Executes a fixed set of actions sequentially
    class CompoundActionSequential : public ICompoundAction
    {
    public:
      
      CompoundActionSequential(std::initializer_list<IActionRunner*> actions);
      
      virtual ActionResult Update(Robot& robot) override final;
      
      // Add a delay, in seconds, between running each action in the group.
      // Default is 0 (no delay).
      void SetDelayBetweenActions(f32 seconds);
      
    private:
      virtual void Reset() override;
      
      f32 _delayBetweenActionsInSeconds;
      f32 _waitUntilTime;
      std::list<std::pair<bool,IActionRunner*> >::iterator _currentActionPair;
    }; // class CompoundActionSequential
    
    inline void CompoundActionSequential::SetDelayBetweenActions(f32 seconds) {
      _delayBetweenActionsInSeconds = seconds;
    }
    
    // Executes a fixed set of actions in parallel
    class CompoundActionParallel : public ICompoundAction
    {
    public:
      
      CompoundActionParallel(std::initializer_list<IActionRunner*> actions);
      
      virtual ActionResult Update(Robot& robot) override final;
      
    protected:
      CompoundActionParallel();
      
    }; // class CompoundActionParallel
    
    
#pragma mark ---- Individual Action Defintions ----
    // TODO: Move these to cozmo-specific actions file
    
    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(const Pose3d& pose);
      DriveToPoseAction(); // Note that SetGoal() must be called befure Update()!
      
      // TODO: Add methods to adjust the goal thresholds from defaults
      
      virtual const std::string& GetName() const override;
      
    protected:

      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;

      Result SetGoal(const Pose3d& pose);
      
    private:
      bool     _isGoalSet;
      Pose3d   _goalPose;
      f32      _goalDistanceThreshold;
      Radians  _goalAngleThreshold;
      
      bool     _forceReplanOnNextWorldChange;
      
    }; // class DriveToPoseAction
    
    
    // Uses the robot's planner to select the best pre-action pose for the
    // specified action type. Drives there using a DriveToPoseAction. Then
    // moves the robot's head to the angle indicated by the pre-action pose
    // (which may be different from the angle used for path following).
    class DriveToObjectAction : public DriveToPoseAction
    {
    public:
      DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType);
      
      // TODO: Add version where marker code is specified instead of action?
      //DriveToObjectAction(Robot& robot, const ObjectID& objectID, Vision::Marker::Code code);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      ActionResult InitHelper(Robot& robot, ActionableObject* object);
      
      ObjectID                   _objectID;
      PreActionPose::ActionType  _actionType;
      Radians                    _finalHeadAngle;
      
    }; // DriveToObjectAction
    
    
    class DriveToPlaceCarriedObjectAction : public DriveToObjectAction
    {
    public:
      DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      
      Pose3d _placementPose;
      
    }; // DriveToPlaceCarriedObjectAction()
    
    
    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public DriveToPoseAction
    {
    public:
      TurnInPlaceAction(const Radians& angle);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      
      Radians _turnAngle;
      
    }; // class TurnInPlaceAction
    
    
    class MoveHeadToAngleAction : public IAction
    {
    public:
      MoveHeadToAngleAction(const Radians& headAngle, const f32 tolerance = DEG_TO_RAD(2.f));
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      Radians     _headAngle;
      Radians     _angleTolerance;
      std::string _name;
    };  // class MoveHeadToAngleAction
    
    
    // Interface for actions that involve "docking" with an object
    class IDockAction : public IAction
    {
    public:
      IDockAction(ObjectID objectID);
      
    protected:
      
      // IDockAction implements these two required methods from IAction for its
      // derived classes
      virtual ActionResult Init(Robot& robot) override final;
      virtual ActionResult CheckIfDone(Robot& robot) override final;
      
      // Most docking actions don't use a second dock marker, but in case they
      // do, they can override this method to choose one from the available
      // preaction poses, given which one was closest.
      virtual const Vision::KnownMarker* GetDockMarker2(const std::vector<PreActionPose>& preActionPoses,
                                                        const size_t closestIndex) { return nullptr; }
      
      // Pure virtual methods that must be implemented by derived classes in
      // order to define the parameters of docking and how to verify success.
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) = 0;
      virtual PreActionPose::ActionType GetPreActionType() = 0;
      virtual IAction::ActionResult Verify(Robot& robot) const = 0;
      
      // Optional additional delay before verification
      virtual f32 GetVerifyDelayInSeconds() const { return 0.f; }
      
      ObjectID                    _dockObjectID;
      DockAction_t                _dockAction;
      const Vision::KnownMarker*  _dockMarker;
      f32                         _waitToVerifyTime;

    }; // class IDockAction

    
    // If not carrying anything, picks up the specified object.
    // If carrying an object, places it on the specified object.
    class PickAndPlaceObjectAction : public IDockAction
    {
    public:
      PickAndPlaceObjectAction(ObjectID objectID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify(Robot& robot) const override;
      
      // For verifying if we successfully picked up the object
      Pose3d _dockObjectOrigPose;
      
      // If placing an object, we need a place to store what robot was
      // carrying, for verification.
      ObjectID                   _carryObjectID;
      const Vision::KnownMarker* _carryObjectMarker;
      
    }; // class DockWithObjectAction

    
    // Common compound action
    class DriveToPickAndPlaceObjectAction : public CompoundActionSequential
    {
    public:
      DriveToPickAndPlaceObjectAction(const ObjectID& objectID)
      : CompoundActionSequential({
        new DriveToObjectAction(objectID, PreActionPose::DOCKING),
        new PickAndPlaceObjectAction(objectID)})
      {
        
      }
    };
    
    
    class PlaceObjectOnGroundAction : public IAction
    {
    public:
      
      PlaceObjectOnGroundAction();
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      // Need longer than default for check if done:
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.5f; }
      
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryObjectMarker;
      
    }; // class PlaceObjectOnGroundAction
    
    
    // Common compound action
    class PlaceObjectOnGroundAtPoseAction : public CompoundActionSequential
    {
    public:
      PlaceObjectOnGroundAtPoseAction(const Robot& robot, const Pose3d& placementPose)
      : CompoundActionSequential({
        new DriveToPlaceCarriedObjectAction(robot, placementPose),
        new PlaceObjectOnGroundAction()})
      {
        
      }
    };
    
    class CrossBridgeAction : public IDockAction
    {
    public:
      CrossBridgeAction(ObjectID bridgeID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify(Robot& robot) const override;
      
      // Crossing a bridge _does_ require the second dockMarker,
      // so override the virtual method for setting it
      virtual const Vision::KnownMarker* GetDockMarker2(const std::vector<PreActionPose>& preActionPoses,
                                                        const size_t closestIndex) override;
      
    }; // class CrossBridgeAction
    
    
    class AscendOrDescendRampAction : public IDockAction
    {
    public:
      AscendOrDescendRampAction(ObjectID rampID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify(Robot& robot) const override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      // Give the robot a little longer to start ascending/descending before
      // checking if it is done
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.f; }
      
    }; // class AscendOrDesceneRampAction
    
    
    // This is just a selector for AscendOrDescendRampAction or
    // CrossBridgeAction, depending on the object's type.
    class TraverseObjectAction : public IActionRunner
    {
    public:
      TraverseObjectAction(ObjectID objectID);
      virtual ~TraverseObjectAction();
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      // Update will just call the chosenAction's implementation
      virtual ActionResult Update(Robot& robot) override;
      virtual void Reset() override;
      
      ObjectID       _objectID;
      IActionRunner* _chosenAction;
      
    }; // class TraverseObjectAction
    
    
    // Common compound action
    class DriveToAndTraverseObjectAction : public CompoundActionSequential
    {
    public:
      DriveToAndTraverseObjectAction(const ObjectID& objectID)
      : CompoundActionSequential({
        new DriveToObjectAction(objectID, PreActionPose::ENTRY),
        new TraverseObjectAction(objectID)})
      {
        
      }
    };
    
    
    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(AnimationID_t animID);
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      AnimationID_t _animID;
      std::string   _name;
      
    }; // class PlayAnimationAction
    
    
    class PlaySoundAction : public IAction
    {
    public:
      PlaySoundAction(SoundID_t soundID);
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      SoundID_t   _soundID;
      std::string _name;
      
    }; // class PlaySoundAction
    
    // Waits for a specified amount of time in seconds, from the time the action
    // is begun. Returns RUNNING while waiting and SUCCESS when the time has
    // elapsed.
    class WaitAction : public IAction
    {
    public:
      WaitAction(f32 waitTimeInSeconds);
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      f32         _waitTimeInSeconds;
      f32         _doneTimeInSeconds;
      std::string _name;
      
    };
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTIONQUEUE_H
