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
    
    // Action Interface
    class IAction
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
      
      IAction(Robot& robot);
      
      virtual ~IAction() { }
      
      // This Update() is what gets called from the outside.  It in turn
      // handles timing delays and runs (protected) CheckPreconditions() and
      // CheckIfDone() methods. Those are the virtual methods that derived
      // classes should implement to get desired action behaviors.
      // Note that this method is final and cannot be overridden in derived
      // classes.
      virtual ActionResult Update() final;
    
      // Override this in derived classes to get more helpful messages for
      // debugging. Otherwise, defaults to "UnnamedAction".
      virtual const std::string& GetName() const;
    
      // Provide a retry function that will be called by Update() if
      // FAILURE_RETRY is returned by the derived CheckIfDone() method.
      void SetRetryFunction(std::function<Result(Robot&)> retryFcn);
      
      // Runs the retry function if one was specified.
      Result Retry();
      
      // Get last status message
      const std::string& GetStatus() const { return _statusMsg; }
      
    protected:
      
      Robot&        _robot;
      
      // Derived Actions should implement these:
      virtual ActionResult  CheckPreconditions() { return SUCCESS; } // Optional: default is no preconditions to meet
      virtual ActionResult  CheckIfDone() = 0;
      
      // Derived actions can use this to set custom status messages here.
      void SetStatus(const std::string& msg);
      
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
      
    private:
      
      void Reset();
      
      bool          _preconditionsMet;
      f32           _waitUntilTime;
      f32           _timeoutTime;
      
      std::string   _statusMsg;
      
      std::function<Result(Robot&)> _retryFcn;
      
    }; // class IAction
    
    
    class ActionQueue
    {
    public:
      ActionQueue();
      
      ~ActionQueue();
      
      Result   QueueNext(IAction *);
      Result   QueueAtEnd(IAction *);
      
      void     Clear();
      
      bool     IsEmpty() const { return _queue.empty(); }
      
      IAction* GetCurrentAction();
      void     PopCurrentAction();
      
      void Print() const;
      
    protected:
      std::list<IAction*> _queue;
    }; // class ActionQueue
    
    
    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(Robot& robot, const Pose3d& pose);
      DriveToPoseAction(Robot& robot); // Note that SetGoal() must be called befure Update()!
      
      // Let robot's planner select "best" possible pose as the goal
      //DriveToPoseAction(Robot& robot, const std::vector<Pose3d>& possiblePoses, const Radians& headAngle);
      //DriveToPoseAction(Robot& robot, const std::vector<Pose3d>& possiblePoses); // with current head angle
      
      // TODO: Add methods to adjust the goal thresholds from defaults
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckIfDone() override;
      virtual ActionResult CheckPreconditions() override;
      
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
      DriveToObjectAction(Robot& robot, const ObjectID& objectID, const PreActionPose::ActionType& actionType);
      
      // TODO: Add version where marker code is specified instead of action?
      //DriveToObjectAction(Robot& robot, const ObjectID& objectID, Vision::Marker::Code code);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckPreconditions() override;
      
      ActionResult CheckPreconditionsHelper(ActionableObject* object);
      
      ObjectID                   _objectID;
      PreActionPose::ActionType  _actionType;
      
      //std::vector<Pose3d> _possibleGoalPoses;
      
    }; // DriveToObjectAction
    
    class DriveToPlaceCarriedObjectAction : public DriveToObjectAction
    {
    public:
      DriveToPlaceCarriedObjectAction(Robot& robot, const Pose3d& placementPose);
      
    protected:
      
      virtual ActionResult CheckPreconditions() override;
      
      Pose3d _placementPose;
      
    }; // DriveToPlaceCarriedObjectAction()
    
    
    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public IAction
    {
    public:
      TurnInPlaceAction(Robot& robot, const Radians& angle);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckIfDone() override;
      virtual ActionResult CheckPreconditions() override;
      
      Radians _turnAngle;
      Pose3d  _goalPose;
      
    }; // class TurnInPlaceAction
    
    
    class IDockAction : public IAction
    {
    public:
      IDockAction(Robot& robot, ObjectID objectID);
      
    protected:
      
      // IDockAction implements these two required methods from IAction for its
      // derived classes
      virtual ActionResult CheckPreconditions() override;
      virtual ActionResult CheckIfDone() override;
      
      // This helper can be optionally overridden by a derived by class to do
      // special things, but for simple usage, derived classes can just rely
      // on this one.
      virtual Result DockWithObjectHelper(const std::vector<PreActionPose>& preActionPoses,
                                          const size_t closestIndex);
      
      // Pure virtual methods that must be implemented by derived classes
      virtual Result SelectDockAction(ActionableObject* object) = 0;
      virtual PreActionPose::ActionType GetPreActionType() = 0;
      virtual IAction::ActionResult Verify() const = 0;
      
      const ObjectID& GetDockObjectID() const { return _dockObjectID; }
      
      ObjectID                    _dockObjectID;
      const Vision::KnownMarker*  _dockMarker;
      const Vision::KnownMarker*  _dockMarker2; // for bridges
      DockAction_t                _dockAction;

    }; // class IDockAction

    
    class PickUpObjectAction : public IDockAction
    {
    public:
      PickUpObjectAction(Robot& robot, ObjectID objectID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify() const override;
      
      // For verifying if we successfully picked up the object
      Pose3d _dockObjectOrigPose;
      
    }; // class DockWithObjectAction

    
    class PutDownObjectAction : public IAction
    {
    public:
      
      PutDownObjectAction(Robot& robot);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual ActionResult CheckPreconditions() override;
      virtual ActionResult CheckIfDone() override;
      
      // Need longer than default for check if done:
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.5f; }
      
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryObjectMarker;
    };
    
    class CrossBridgeAction : public IDockAction
    {
    public:
      CrossBridgeAction(Robot& robot, ObjectID bridgeID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify() const override;
      
      virtual Result DockWithObjectHelper(const std::vector<PreActionPose>& preActionPoses,
                                          const size_t closestIndex) override;
      
    }; // class TraverseObjectAction
    
    
    class AscendOrDescendRampAction : public IDockAction
    {
    public:
      AscendOrDescendRampAction(Robot& robot, ObjectID rampID);
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual IAction::ActionResult Verify() const override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      
      
    }; // class AscendOrDesceneRampAction
    
  } // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ACTIONQUEUE_H
