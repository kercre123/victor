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
    
    class ActionList
    {
    public:
      using ActionID = u32;
      using ActionCompletionCallback = std::function<void(IActionRunner::ActionResult)>;
      
      ActionList();
      ~ActionList();
      
      Result   Update(Robot& robot);
      
      ActionID AddAction(IActionRunner* action, u8 numRetries = 0);

      // TODO: Should action removal be allowed? What if it's in progress?
      //Result   RemoveAction(ActionID ID);
      
      // TODO: Do we want to support callbacks on completion? Just use sequential grouping instead?
      // TODO: If we use completion callbacks, maybe they should be part of IActionRunner
      Result   RegisterCompletionCallback(ActionID actionID,
                                          ActionCompletionCallback callback);
      
      bool     IsEmpty() const;
      
      void     Clear();
      
      void     Print() const;
      
    protected:
      struct ActionListMember {
        IActionRunner *action;
        std::list<ActionCompletionCallback> completionCallbacks;
        
        ~ActionListMember() {
          if(action != nullptr) {
            delete action;
          }
        }
      };
      
      ActionID _IDcounter;
      
      std::map<u32, ActionListMember> _actionList;
      
    }; // class ActionList
    
    class ActionQueue
    {
    public:
      ActionQueue();
      
      ~ActionQueue();
      
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
    
#pragma mark ---- IActionGroup ----
    
    // Interface for groups of actions
    class IActionGroup : public IActionRunner
    {
    public:
      IActionGroup(std::initializer_list<IActionRunner*> actions);
      
      // Constituent actions will be deleted upon destruction of the group
      virtual ~IActionGroup();
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      // Call the constituent actions' Reset() methods and mark them each not done.
      virtual void Reset() override;
      
      std::list<std::pair<bool, IActionRunner*> > _actions;
      std::string _name;
    };
    
    
    // Executes a group of actions sequentially
    class ActionGroupSequential : public IActionGroup
    {
    public:
      
      ActionGroupSequential(std::initializer_list<IActionRunner*> actions);
      
      virtual ActionResult Update(Robot& robot) override final;
      
    private:
      std::list<std::pair<bool,IActionRunner*> >::iterator _currentActionPair;
    }; // class ActionGroupSequential
    
    
    // Executes a group of actions in parallel
    class ActionGroupParallel : public IActionGroup
    {
    public:
      
      ActionGroupParallel(std::initializer_list<IActionRunner*> actions);
      
      virtual ActionResult Update(Robot& robot) override final;
      
    }; // class ActionGroupParallel
    
    
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
      
    }; // class DriveToPoseAction
    
    
    class DriveToObjectAction : public DriveToPoseAction
    {
    public:
      DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType);
      
      // TODO: Add version where marker code is specified instead of action?
      //DriveToObjectAction(Robot& robot, const ObjectID& objectID, Vision::Marker::Code code);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      
      ActionResult CheckPreconditionsHelper(Robot& robot, ActionableObject* object);
      
      ObjectID                   _objectID;
      PreActionPose::ActionType  _actionType;
      
    }; // DriveToObjectAction
    
    class DriveToPlaceCarriedObjectAction : public DriveToObjectAction
    {
    public:
      DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose);
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      
      Pose3d _placementPose;
      
    }; // DriveToPlaceCarriedObjectAction()
    
    
    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public IAction
    {
    public:
      TurnInPlaceAction(const Radians& angle);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckIfDone(Robot& robot) override;
      virtual ActionResult Init(Robot& robot) override;
      
      Radians _turnAngle;
      Pose3d  _goalPose;
      
    }; // class TurnInPlaceAction
    
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
      
      // This helper can be optionally overridden by a derived by class to do
      // special things, but for simple usage, derived classes can just rely
      // on this one.
      virtual Result DockWithObjectHelper(Robot& robot,
                                          const std::vector<PreActionPose>& preActionPoses,
                                          const size_t closestIndex);
      
      // This helper can be optionally overridden by a derived class to check
      // if the object to be docked with is available. This is called by
      // CheckPreconditions(). If it is false, FAILURE_RETRY is returned.
      virtual bool IsObjectAvailable(ActionableObject* object) const { return true; }
      
      // Pure virtual methods that must be implemented by derived classes
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) = 0;
      virtual PreActionPose::ActionType GetPreActionType() = 0;
      virtual IAction::ActionResult Verify(Robot& robot) const = 0;
      
      const ObjectID& GetDockObjectID() const { return _dockObjectID; }
      
      ObjectID                    _dockObjectID;
      const Vision::KnownMarker*  _dockMarker;
      DockAction_t                _dockAction;

    }; // class IDockAction

    
    class PickUpObjectAction : public IDockAction
    {
    public:
      PickUpObjectAction(ObjectID objectID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify(Robot& robot) const override;
      
      // For verifying if we successfully picked up the object
      Pose3d _dockObjectOrigPose;
      
    }; // class DockWithObjectAction

    
    class PutDownObjectAction : public IAction
    {
    public:
      
      PutDownObjectAction();
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      // Need longer than default for check if done:
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.5f; }
      
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryObjectMarker;
      
    }; // class PutDownObjectAction
    
    class CrossBridgeAction : public IDockAction
    {
    public:
      CrossBridgeAction(ObjectID bridgeID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify(Robot& robot) const override;
      
      virtual Result DockWithObjectHelper(Robot& robot,
                                          const std::vector<PreActionPose>& preActionPoses,
                                          const size_t closestIndex) override;
      
      const Vision::KnownMarker*  _dockMarker2;
      
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
      
      // Returns false if ramp is in use by another robot
      virtual bool IsObjectAvailable(ActionableObject* object) const;
      
    }; // class AscendOrDesceneRampAction
    
    
    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(AnimationID_t animID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      AnimationID_t _animID;
      
    }; // class PlayAnimationAction
    
    
    class PlaySoundAction : public IAction
    {
    public:
      PlaySoundAction(SoundID_t soundID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      SoundID_t _soundID;

    }; // class PlaySoundAction
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTIONQUEUE_H
