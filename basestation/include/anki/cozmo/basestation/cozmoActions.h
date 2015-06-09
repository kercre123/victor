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
#include "anki/cozmo/basestation/signals/cozmoEngineSignals.h"

#include "anki/common/types.h"

#include "anki/common/basestation/objectTypesAndIDs.h"
#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/messageBuffers/shared/actionTypes.def"

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
      DriveToPoseAction(const std::vector<Pose3d>& poses, const bool useManualSpeed = false);
      
      // TODO: Add methods to adjust the goal thresholds from defaults
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_POSE; }
      
    protected:

      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      virtual void Cleanup(Robot &robot) override;
      virtual void Reset() override;
      
      Result SetGoal(const Pose3d& pose);
      Result SetGoal(const Pose3d& pose, const Point3f& distThreshold, const Radians& angleThreshold);
      
      // Set possible goal options
      Result SetGoals(const std::vector<Pose3d>& poses);
      Result SetGoals(const std::vector<Pose3d>& poses, const Point3f& distThreshold, const Radians& angleThreshold);
      
      bool IsUsingManualSpeed() {return _useManualSpeed;}
      
      // Don't lock wheels if we're using manual speed control (i.e. "assisted RC")
      virtual bool ShouldLockWheels() const override { return !_useManualSpeed; }
      
    private:
      bool     _isGoalSet;
      
      std::vector<Pose3d> _goalPoses;
      size_t              _selectedGoalIndex;
      
      Point3f  _goalDistanceThreshold;
      Radians  _goalAngleThreshold;
      bool     _useManualSpeed;
      bool     _startedTraversingPath;
      bool     _forceReplanOnNextWorldChange;
      
      Signal::SmartHandle _signalHandle;
      
    }; // class DriveToPoseAction
    
    
    // Uses the robot's planner to select the best pre-action pose for the
    // specified action type. Drives there using a DriveToPoseAction. Then
    // moves the robot's head to the angle indicated by the pre-action pose
    // (which may be different from the angle used for path following).
    class DriveToObjectAction : public IAction //: public DriveToPoseAction
    {
    public:
      DriveToObjectAction(const ObjectID& objectID, const PreActionPose::ActionType& actionType, const bool useManualSpeed = false);
      DriveToObjectAction(const ObjectID& objectID, const f32 distance_mm, const bool useManualSpeed = false);
      
      // TODO: Add version where marker code is specified instead of action?
      //DriveToObjectAction(Robot& robot, const ObjectID& objectID, Vision::Marker::Code code);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_OBJECT; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
      ActionResult InitHelper(Robot& robot, ActionableObject* object);
      ActionResult GetPossiblePoses(const Robot& robot, ActionableObject* object,
                                    std::vector<Pose3d>& possiblePoses,
                                    bool& alreadyInPosition);
      
      virtual void Cleanup(Robot &robot) override;
      virtual void Reset() override;
      
      // Not private b/c DriveToPlaceCarriedObject uses
      ObjectID                   _objectID;
      PreActionPose::ActionType  _actionType;
      f32                        _distance_mm;
      bool                       _useManualSpeed;
      CompoundActionSequential   _compoundAction;
      
    }; // DriveToObjectAction
    
    
    class DriveToPlaceCarriedObjectAction : public DriveToObjectAction
    {
    public:
      DriveToPlaceCarriedObjectAction(const Robot& robot, const Pose3d& placementPose, const bool useManualSpeed);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_PLACE_CARRIED_OBJECT; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override; // Simplified version from DriveToObjectAction
      Pose3d _placementPose;
      
    }; // DriveToPlaceCarriedObjectAction()
    
    
    // Turn in place by a given angle, wherever the robot is when the action
    // is executed.
    class TurnInPlaceAction : public DriveToPoseAction
    {
    public:
      TurnInPlaceAction(const Radians& angle);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::TURN_IN_PLACE; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
    private:
      Radians _turnAngle;
      bool    _startedTraversingPath;
      
    }; // class TurnInPlaceAction
    
    
    class MoveHeadToAngleAction : public IAction
    {
    public:
      MoveHeadToAngleAction(const Radians& headAngle, const f32 tolerance = DEG_TO_RAD(2.f));
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::MOVE_HEAD_TO_ANGLE; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
    private:
      
      bool IsHeadInPosition(const Robot& robot) const;
      
      Radians     _headAngle;
      Radians     _angleTolerance;
      std::string _name;
      bool        _inPosition;
      
    };  // class MoveHeadToAngleAction
    
    // Set the lift to specified height with a given tolerance. Note that settign
    // the tolerance too small will likely lead to an action timeout.
    class MoveLiftToHeightAction : public IAction
    {
    public:
      
      // Named presets:
      enum class Preset : u8 {
        LOW_DOCK,
        HIGH_DOCK,
        CARRY,
        OUT_OF_FOV // Moves to low or carry, depending on which is closer to current height
      };
      
      MoveLiftToHeightAction(const f32 height_mm, const f32 tolerance_mm = 5.f);
      MoveLiftToHeightAction(const Preset preset, const f32 tolerance_mm = 5.f);
      
      virtual const std::string& GetName() const override { return _name; };
      virtual RobotActionType GetType() const override { return RobotActionType::MOVE_LIFT_TO_HEIGHT; }
      
    protected:
      
      static f32 GetPresetHeight(Preset preset);
      static const std::string& GetPresetName(Preset preset);
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      
    private:
      
      bool IsLiftInPosition(const Robot& robot) const;
      
      f32          _height_mm;
      f32          _heightTolerance;
      std::string  _name;
      bool         _inPosition;
      
    }; // class MoveLiftToHeightAction
    
    
    // Tilt head and rotate body to face the specified (marker on an) object.
    // Use angles specified at construction to control the body rotation.
    class FaceObjectAction : public IAction
    {
    public:
      // If facing the object requires less than turnAngleTol turn, then no
      // turn is performed. If a turn greater than maxTurnAngle is required,
      // the action fails. For angles in between, the robot will first turn
      // to face the object, then tilt its head. To disallow turning, set
      // maxTurnAngle to zero.
      
      FaceObjectAction(ObjectID objectID, Radians turnAngleTol, Radians maxTurnAngle,
                       bool headTrackWhenDone = false);
      
      FaceObjectAction(ObjectID objectID, Vision::Marker::Code whichCode,
                       Radians turnAngleTol, Radians maxTurnAngle,
                       bool headTrackWhenDone = false);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::FACE_OBJECT; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      virtual void Reset() override;
      
      // Reduce delays from their defaults
      virtual f32 GetStartDelayInSeconds() const override { return 0.0f; }
      
      // Amount of time to wait before verifying after moving head that we are
      // indeed seeing the object/marker we expect.
      // TODO: Can this default be reduced?
      virtual f32 GetWaitToVerifyTime() const { return 0.25f; }
      
      // Override to allow wheel control while facing the object
      virtual bool ShouldLockWheels() const override { return false; }
      
      CompoundActionParallel _compoundAction;
      bool                   _compoundActionDone;
      
      ObjectID             _objectID;
      Vision::Marker::Code _whichCode;
      Radians              _turnAngleTol;
      Radians              _maxTurnAngle;
      f32                  _waitToVerifyTime;
      bool                 _headTrackWhenDone;
      
    }; // FaceObjectAction
    
    
    // Verify that an object exists by facing tilting the head to face its
    // last-known pose and verify that we can still see it. Optionally, you can
    // also require that a specific marker be seen as well.
    class VisuallyVerifyObjectAction : public FaceObjectAction
    {
    public:
      VisuallyVerifyObjectAction(ObjectID objectID,
                                 Vision::Marker::Code whichCode = Vision::Marker::ANY_CODE);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::VISUALLY_VERIFY_OBJECT; }
      
    protected:
      
      virtual bool ShouldLockWheels() const override { return true; }
      
    }; // class VisuallyVerifyObjectAction
    
    
    // Interface for actions that involve "docking" with an object
    class IDockAction : public IAction
    {
    public:
      IDockAction(ObjectID objectID, const bool useManualSpeed);
      
      virtual ~IDockAction();
      
      // Use a value <= 0 to ignore how far away the robot is from the closest
      // PreActionPose and proceed regardless.
      void SetPreActionPoseAngleTolerance(Radians angleTolerance);
      
    protected:
      
      // IDockAction implements these two required methods from IAction for its
      // derived classes
      virtual ActionResult Init(Robot& robot) override final;
      virtual ActionResult CheckIfDone(Robot& robot) override final;
      virtual void Cleanup(Robot& robot) override final;
      
      // Most docking actions don't use a second dock marker, but in case they
      // do, they can override this method to choose one from the available
      // preaction poses, given which one was closest.
      virtual const Vision::KnownMarker* GetDockMarker2(const std::vector<PreActionPose>& preActionPoses,
                                                        const size_t closestIndex) { return nullptr; }
      
      // Pure virtual methods that must be implemented by derived classes in
      // order to define the parameters of docking and how to verify success.
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) = 0;
      virtual PreActionPose::ActionType GetPreActionType() = 0;
      virtual ActionResult Verify(Robot& robot) = 0;
      
      virtual void Reset() override;
      
      // Optional additional delay before verification
      virtual f32 GetVerifyDelayInSeconds() const { return 0.f; }
      
      // Should only lock wheels if we are not using manual speed (i.e. "assisted RC")
      virtual bool ShouldLockWheels() const { return !_useManualSpeed; }
      
      ObjectID                    _dockObjectID;
      DockAction_t                _dockAction;
      const Vision::KnownMarker*  _dockMarker;
      const Vision::KnownMarker*  _dockMarker2;
      Radians                     _preActionPoseAngleTolerance;
      f32                         _waitToVerifyTime;
      bool                        _wasPickingOrPlacing;
      bool                        _useManualSpeed;
      VisuallyVerifyObjectAction* _visuallyVerifyAction;
    }; // class IDockAction

    
    // If not carrying anything, picks up the specified object.
    // If carrying an object, places it on the specified object.
    class PickAndPlaceObjectAction : public IDockAction
    {
    public:
      PickAndPlaceObjectAction(ObjectID objectID, const bool useManualSpeed);
      virtual ~PickAndPlaceObjectAction();
      
      virtual const std::string& GetName() const override;
      
      // Override to determine type (pick/place, low/high) dynamically depending
      // on what we were doing.
      virtual RobotActionType GetType() const override;
      
      // Override completion signal to fill in information about objects picked
      // or placed
      virtual void EmitCompletionSignal(Robot& robot, ActionResult result) const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual ActionResult Verify(Robot& robot) override;
      
      virtual void Reset() override;
      
      // For verifying if we successfully picked up the object
      Pose3d _dockObjectOrigPose;
      
      // If placing an object, we need a place to store what robot was
      // carrying, for verification.
      ObjectID                   _carryObjectID;
      const Vision::KnownMarker* _carryObjectMarker;
      
      IActionRunner*             _placementVerifyAction;
      bool                       _verifyComplete; // used in PLACE modes
      
    }; // class PickAndPlaceObjectAction

    
    // If not carrying anything, rolls the specified object.
    // If carrying an object, fails.
    class RollObjectAction : public IDockAction
    {
    public:
      RollObjectAction(ObjectID objectID, const bool useManualSpeed);
      virtual ~RollObjectAction();
      
      virtual const std::string& GetName() const override;
      
      // Override to determine type (low roll, or potentially other rolls) dynamically depending
      // on what we were doing.
      virtual RobotActionType GetType() const override;
      
      // Override completion signal to fill in information about rolled objects
      virtual void EmitCompletionSignal(Robot& robot, ActionResult result) const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::DOCKING; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual ActionResult Verify(Robot& robot) override;
      
      virtual void Reset() override;
      
      // For verifying if we successfully rolled the object
      Pose3d _dockObjectOrigPose;
      
      IActionRunner*             _rollVerifyAction;
      
    }; // class RollObjectAction
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then picking/placing it.
    class DriveToPickAndPlaceObjectAction : public CompoundActionSequential
    {
    public:
      DriveToPickAndPlaceObjectAction(const ObjectID& objectID, const bool useManualSpeed = false)
      : CompoundActionSequential({
        new DriveToObjectAction(objectID, PreActionPose::DOCKING, useManualSpeed),
        //new VisuallyVerifyObjectAction(objectID),
        new PickAndPlaceObjectAction(objectID, useManualSpeed)})
      {

      }
      
      // GetType returns the type from the PickAndPlaceObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use PickAndPlaceObjectAction's completion signal
      virtual void EmitCompletionSignal(Robot& robot, ActionResult result) const override {
        return _actions.back().second->EmitCompletionSignal(robot, result);
      }
      
    };
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then rolling it.
    class DriveToRollObjectAction : public CompoundActionSequential
    {
    public:
      DriveToRollObjectAction(const ObjectID& objectID, const bool useManualSpeed = false)
      : CompoundActionSequential({
        new DriveToObjectAction(objectID, PreActionPose::DOCKING, useManualSpeed),
        new RollObjectAction(objectID, useManualSpeed)})
      {
        
      }
      
      // GetType returns the type from the PickAndPlaceObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use RollObjectAction's completion signal
      virtual void EmitCompletionSignal(Robot& robot, ActionResult result) const override {
        return _actions.back().second->EmitCompletionSignal(robot, result);
      }
      
    };

    
    
    class PlaceObjectOnGroundAction : public IAction
    {
    public:
      
      PlaceObjectOnGroundAction();
      virtual ~PlaceObjectOnGroundAction();
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::PLACE_OBJECT_LOW; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      virtual void Reset() override;
      
      // Need longer than default for check if done:
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.5f; }
      
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryObjectMarker;
      VisuallyVerifyObjectAction* _verifyAction;
      
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
      
      virtual RobotActionType GetType() const override { return RobotActionType::PLACE_OBJECT_LOW; }
    };
    
    class CrossBridgeAction : public IDockAction
    {
    public:
      CrossBridgeAction(ObjectID bridgeID, const bool useManualSpeed);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::CROSS_BRIDGE; }
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual ActionResult Verify(Robot& robot) override;
      
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
      virtual RobotActionType GetType() const override { return RobotActionType::ASCEND_OR_DESCEND_RAMP; }
      
    protected:
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual ActionResult Verify(Robot& robot) override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      // Give the robot a little longer to start ascending/descending before
      // checking if it is done
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.f; }
      
    }; // class AscendOrDesceneRampAction
    
    
    class MountChargerAction : public IDockAction
    {
    public:
      MountChargerAction(ObjectID chargerID, const bool useManualSpeed);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::MOUNT_CHARGER; }
      
    protected:
      
      virtual Result SelectDockAction(Robot& robot, ActionableObject* object) override;
      
      virtual ActionResult Verify(Robot& robot) override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      // Give the robot a little longer to start ascending/descending before
      // checking if it is done
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.f; }
      
    }; // class MountChargerAction
    
    
    // This is just a selector for AscendOrDescendRampAction or
    // CrossBridgeAction, depending on the object's type.
    class TraverseObjectAction : public IActionRunner
    {
    public:
      TraverseObjectAction(ObjectID objectID, const bool useManualSpeed);
      virtual ~TraverseObjectAction();
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::TRAVERSE_OBJECT; }
      
      virtual void Cleanup(Robot& robot) override {
        if(_chosenAction != nullptr) {
          _chosenAction->Cleanup(robot);
        }
      }
      
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
      
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_AND_TRAVERSE_OBJECT; }
      
    };
    
    
    class PlayAnimationAction : public IAction
    {
    public:
      PlayAnimationAction(const std::string& animName);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::PLAY_ANIMATION; }
      
    protected:
      
      virtual ActionResult Init(Robot& robot) override;
      virtual ActionResult CheckIfDone(Robot& robot) override;
      virtual void Cleanup(Robot& robot) override;
      
      //AnimationID_t _animID;
      std::string   _animName;
      std::string   _name;
      
    }; // class PlayAnimationAction
    
    
    class PlaySoundAction : public IAction
    {
    public:
      PlaySoundAction(const std::string& soundName);
      
      virtual const std::string& GetName() const override { return _name; }
      virtual RobotActionType GetType() const override { return RobotActionType::PLAY_SOUND; }
      
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
      virtual RobotActionType GetType() const override { return RobotActionType::WAIT; }
      
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
