/**
 * File: cozmoActions.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_ACTIONS_H
#define ANKI_COZMO_ACTIONS_H

#include "anki/cozmo/basestation/actionableObject.h"
#include "anki/cozmo/basestation/actionInterface.h"
#include "anki/cozmo/basestation/compoundActions.h"

#include "anki/common/types.h"

#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/pose.h"

namespace Anki {
  
  namespace Vision {
    // Forward Declarations:
    class KnownMarker;
  }
  
  namespace Cozmo {

    // Forward Declarations:
    class Robot;

    
    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(const Pose3d& pose, const bool useManualSpeed = false);
      DriveToPoseAction(const bool useManualSpeed = false); // Note that SetGoal() must be called befure Update()!
      
      // TODO: Add methods to adjust the goal thresholds from defaults
      
      virtual const std::string& GetName() const override;
      
    protected:

      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;

      Result SetGoal(const Pose3d& pose);
      bool IsUsingManualSpeed() {return _useManualSpeed;}
      
    private:
      bool     _isGoalSet;
      Pose3d   _goalPose;
      f32      _goalDistanceThreshold;
      Radians  _goalAngleThreshold;
      bool     _useManualSpeed;
      bool     _startedTraversingPath;
      bool     _forceReplanOnNextWorldChange;
      
    }; // class DriveToPoseAction
    
    
    // Uses the robot's planner to select the best pre-action pose for the
    // specified action type. Drives there using a DriveToPoseAction. Then
    // moves the robot's head to the angle indicated by the pre-action pose
    // (which may be different from the angle used for path following).
    class DriveToObjectAction : public DriveToPoseAction
    {
    public:
      DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType, const bool useManualSpeed = false);
      
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
      DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose, const bool useManualSpeed);
      
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
      IDockAction(ObjectID objectID, const bool useManualSpeed);
      
      // Use a value <= 0 to ignore how far away the robot is from the closest
      // PreActionPose and proceed regardless.
      void SetMaxPreActionPoseDistance(f32 maxDistance);
      
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
      f32                         _maxPreActionPoseDistance;
      f32                         _waitToVerifyTime;
      bool                        _wasPickingOrPlacing;
      bool                        _useManualSpeed;

    }; // class IDockAction

    
    // If not carrying anything, picks up the specified object.
    // If carrying an object, places it on the specified object.
    class PickAndPlaceObjectAction : public IDockAction
    {
    public:
      PickAndPlaceObjectAction(ObjectID objectID, const bool useManualSpeed);
      
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
      DriveToPickAndPlaceObjectAction(const ObjectID& objectID, const bool useManualSpeed = false)
      : CompoundActionSequential({
        new DriveToObjectAction(objectID, PreActionPose::DOCKING, useManualSpeed),
        new PickAndPlaceObjectAction(objectID, useManualSpeed)})
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
      PlaceObjectOnGroundAtPoseAction(const Robot& robot, const Pose3d& placementPose, const bool useManualSpeed)
      : CompoundActionSequential({
        new DriveToPlaceCarriedObjectAction(robot, placementPose, useManualSpeed),
        new PlaceObjectOnGroundAction()})
      {
        
      }
    };
    
    class CrossBridgeAction : public IDockAction
    {
    public:
      CrossBridgeAction(ObjectID bridgeID, const bool useManualSpeed);
      
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
      AscendOrDescendRampAction(ObjectID rampID, const bool useManualSpeed);
      
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
      TraverseObjectAction(ObjectID objectID, const bool useManualSpeed);
      virtual ~TraverseObjectAction();
      
      virtual const std::string& GetName() const override;
      
    protected:
      
      // Update will just call the chosenAction's implementation
      virtual ActionResult UpdateInternal(Robot& robot) override;
      virtual void Reset() override;
      
      ObjectID       _objectID;
      IActionRunner* _chosenAction;
      bool           _useManualSpeed;
      
    }; // class TraverseObjectAction
    
    
    // Common compound action
    class DriveToAndTraverseObjectAction : public CompoundActionSequential
    {
    public:
      DriveToAndTraverseObjectAction(const ObjectID& objectID, const bool useManualSpeed = false)
      : CompoundActionSequential({
        new DriveToObjectAction(objectID, PreActionPose::ENTRY, useManualSpeed),
        new TraverseObjectAction(objectID, useManualSpeed)})
      {
        
      }
    };
    
    
    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(const std::string& animName);
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      //AnimationID_t _animID;
      std::string   _animName;
      std::string   _name;
      
    }; // class PlayAnimationAction
    
    
    class PlaySoundAction : public IAction
    {
    public:
      PlaySoundAction(const std::string& soundName);
      
      virtual const std::string& GetName() const override { return _name; }
      
    protected:
      
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      std::string _soundName;
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

#endif // ANKI_COZMO_ACTIONS_H
