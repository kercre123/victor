/**
 * File: driveToActions.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements drive-to cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_DRIVE_TO_ACTIONS_H
#define ANKI_COZMO_DRIVE_TO_ACTIONS_H

#include "anki/common/types.h"
#include "anki/cozmo/basestation/actionableObject.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/planning/shared/goalDefs.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/actionTypes.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/dockingSignals.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
  
  namespace Cozmo {
    
    class PickupObjectAction;
    class IDockAction;
    class TurnTowardsLastFacePoseAction;
    class TurnTowardsObjectAction;

    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(Robot& robot,
                        const Pose3d& pose,
                        const bool forceHeadDown  = true,
                        const bool useManualSpeed = false,
                        const Point3f& distThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                        const Radians& angleThreshold = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD,
                        const float maxPlanningTime = DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S,
                        const float maxReplanPlanningTime = DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S);
      
      DriveToPoseAction(Robot& robot,
                        const bool forceHeadDown  = true,
                        const bool useManualSpeed = false); // Note that SetGoal(s) must be called befure Update()!
      
      DriveToPoseAction(Robot& robot,
                        const std::vector<Pose3d>& poses,
                        const bool forceHeadDown  = true,
                        const bool useManualSpeed = false,
                        const Point3f& distThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                        const Radians& angleThreshold = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD,
                        const float maxPlanningTime = DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S,
                        const float maxReplanPlanningTime = DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S);
      virtual ~DriveToPoseAction();
      
      // TODO: Add methods to adjust the goal thresholds from defaults
      
      // Set single goal
      Result SetGoal(const Pose3d& pose,
                     const Point3f& distThreshold  = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                     const Radians& angleThreshold = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD);
      
      // Set possible goal options
      Result SetGoals(const std::vector<Pose3d>& poses,
                      const Point3f& distThreshold  = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                      const Radians& angleThreshold = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD);
      
      void SetMotionProfile(const PathMotionProfile& motionProfile);

    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      bool IsUsingManualSpeed() {return _useManualSpeed;}
      
    private:
      bool     _isGoalSet;
      bool     _driveWithHeadDown;
      
      std::vector<Pose3d> _goalPoses;
      Planning::GoalID    _selectedGoalIndex;
      
      PathMotionProfile _pathMotionProfile;
      bool _hasMotionProfile = false;
      
      Point3f  _goalDistanceThreshold;
      Radians  _goalAngleThreshold;
      bool     _useManualSpeed;
      
      float _maxPlanningTime;
      float _maxReplanPlanningTime;
      
      float _timeToAbortPlanning;
            
      Signal::SmartHandle _originChangedHandle;
      
    }; // class DriveToPoseAction

    
    // Uses the robot's planner to select the best pre-action pose for the
    // specified action type. Drives there using a DriveToPoseAction. Then
    // moves the robot's head to the angle indicated by the pre-action pose
    // (which may be different from the angle used for path following).
    class DriveToObjectAction : public IAction
    {
    public:
      DriveToObjectAction(Robot& robot,
                          const ObjectID& objectID,
                          const PreActionPose::ActionType& actionType,
                          const f32 predockOffsetDistX_mm = 0,
                          const bool useApproachAngle = false,
                          const f32 approachAngle_rad = 0,
                          const bool useManualSpeed = false);
      
      DriveToObjectAction(Robot& robot,
                          const ObjectID& objectID,
                          const f32 distance_mm,
                          const bool useManualSpeed = false);
      virtual ~DriveToObjectAction();
      
      // TODO: Add version where marker code is specified instead of action?
      //DriveToObjectAction(Robot& robot, const ObjectID& objectID, Vision::Marker::Code code);
      
      // If set, instead of driving to the nearest preActionPose, only the preActionPose
      // that is most closely aligned with the approach angle is considered.
      void SetApproachAngle(const f32 angle_rad);
      const bool GetUseApproachAngle() const;
      
      // Whether or not to verify the final pose, once the path is complete,
      // according to the latest know preAction pose for the specified object.
      void DoPositionCheckOnPathCompletion(bool doCheck) { _doPositionCheckOnPathCompletion = doCheck; }
      
      void SetMotionProfile(const PathMotionProfile& motionProfile);
      
      using GetPossiblePosesFunc = std::function<ActionResult(ActionableObject* object,
                                                              std::vector<Pose3d>& possiblePoses,
                                                              bool& alreadyInPosition)>;
      
      void SetGetPossiblePosesFunc(GetPossiblePosesFunc func)
      {
        _getPossiblePosesFunc = func;
      }
      
      // Default GetPossiblePoses function is public in case others want to just
      // use it as the baseline and modify it's results slightly
      ActionResult GetPossiblePoses(ActionableObject* object,
                                    std::vector<Pose3d>& possiblePoses,
                                    bool& alreadyInPosition);
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      ActionResult InitHelper(ActionableObject* object);
      
      
      // Not private b/c DriveToPlaceCarriedObject uses
      ObjectID                   _objectID;
      PreActionPose::ActionType  _actionType;
      f32                        _distance_mm;
      f32                        _predockOffsetDistX_mm;
      bool                       _useManualSpeed;
      CompoundActionSequential   _compoundAction;
      
      bool                       _useApproachAngle;
      Radians                    _approachAngle_rad;

      bool                       _doPositionCheckOnPathCompletion;
      
      PathMotionProfile          _pathMotionProfile;
      bool _hasMotionProfile = false;
      
    private:
      GetPossiblePosesFunc _getPossiblePosesFunc;
      bool _lightsSet = false;
    }; // DriveToObjectAction

  
    class DriveToPlaceCarriedObjectAction : public DriveToObjectAction
    {
    public:
      // destinationObjectPadding_mm: padding around the object size at destination used if checkDestinationFree is true
      DriveToPlaceCarriedObjectAction(Robot& robot,
                                      const Pose3d& placementPose,
                                      const bool placeOnGround,
                                      const bool useExactRotation = false,
                                      const bool useManualSpeed = false,
                                      const bool checkDestinationFree = false,
                                      const float destinationObjectPadding_mm = 0.0f);
      
    protected:
    
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override; // Simplified version from DriveToObjectAction
      
      // checks if the placement destination is free (alternatively we could provide an std::function callback)
      bool IsPlacementGoalFree() const;
      
      Pose3d _placementPose;
      
      bool   _useExactRotation;
      bool   _checkDestinationFree; // if true the action will often check that the destination is still free to place the object
      float  _destinationObjectPadding_mm; // padding around the object size at destination if _checkDestinationFree is true
      
    }; // DriveToPlaceCarriedObjectAction()

    
    // Interface for all classes below which first drive to an object and then
    // do something with it.
    // If maxTurnTowardsFaceAngle > 0, robot will turn a maximum of that angle towards
    // last face after driving to the object (and say name if that is specified).
    class IDriveToInteractWithObject : public CompoundActionSequential
    {
    protected:
      // Not directly instantiable
      IDriveToInteractWithObject(Robot& robot,
                                 const ObjectID& objectID,
                                 const PreActionPose::ActionType& actionType,
                                 const f32 predockOffsetDistX_mm,
                                 const bool useApproachAngle,
                                 const f32 approachAngle_rad,
                                 const bool useManualSpeed,
                                 Radians maxTurnTowardsFaceAngle_rad,
                                 const bool sayName);
      
      IDriveToInteractWithObject(Robot& robot,
                                 const ObjectID& objectID,
                                 const f32 distance,
                                 const bool useManualSpeed);

    public:
      virtual ~IDriveToInteractWithObject();
      
      void SetMotionProfile(const PathMotionProfile& motionProfile);
      
      void SetMaxTurnTowardsFaceAngle(const Radians angle);
      void SetTiltTolerance(const Radians tol);
      
      DriveToObjectAction* GetDriveToObjectAction() {
        return _driveToObjectAction;
      }
      
      // Subclasses that are a drive-to action followed by a dock action should be calling
      // this function instead of the base classes AddAction() in order to set the approriate
      // preDock pose offset for the dock action
      void AddDockAction(IDockAction* dockAction, bool ignoreFailure = false);

      // Sets the animation trigger to use to say the name. Only valid if sayName was true
      void SetSayNameAnimationTrigger(AnimationTrigger trigger);
      
      // Sets the backup animation to play if the name is not known, but there is a confirmed face. Only valid
      // if sayName is true (this is because we are trying to use an animation to say the name, but if we
      // don't have a name, we want to use this animation instead)
      void SetNoNameAnimationTrigger(AnimationTrigger trigger);
      
      const bool GetUseApproachAngle() const;
      
    protected:

      virtual Result UpdateDerived() override;
      
      // If set, instead of driving to the nearest preActionPose, only the preActionPose
      // that is most closely aligned with the approach angle is considered.
      void SetApproachAngle(const f32 angle_rad);
      
    private:
      DriveToObjectAction*           _driveToObjectAction           = nullptr;
      TurnTowardsLastFacePoseAction* _turnTowardsLastFacePoseAction = nullptr;
      TurnTowardsObjectAction*       _turnTowardsObjectAction       = nullptr;
      ObjectID _objectID;
      bool     _lightsSet = false;
      f32      _preDockPoseDistOffsetX_mm = 0;
      
    }; // class IDriveToInteractWithObject
        
    
    // Compound action for driving to an object, visually verifying it can still be seen,
    // and then driving to it until it is at the specified distance (i.e. distanceFromMarker_mm)
    // from the marker.
    // @param distanceFromMarker_mm - The distance from the marker along it's normal axis that the robot should stop at.
    // @param useApproachAngle  - If true, then only the preAction pose that results in a robot
    //                            approach angle closest to approachAngle_rad is considered.
    // @param approachAngle_rad - The desired docking approach angle of the robot in world coordinates.
    class DriveToAlignWithObjectAction : public IDriveToInteractWithObject
    {
    public:
      DriveToAlignWithObjectAction(Robot& robot,
                                   const ObjectID& objectID,
                                   const f32 distanceFromMarker_mm,
                                   const bool useApproachAngle = false,
                                   const f32 approachAngle_rad = 0,
                                   const AlignmentType alignmentType = AlignmentType::CUSTOM,
                                   const bool useManualSpeed = false,
                                   Radians maxTurnTowardsFaceAngle_rad = 0.f,
                                   const bool sayName = false);
      
      virtual ~DriveToAlignWithObjectAction() { }
    };
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then picking it up.
    // @param useApproachAngle  - If true, then only the preAction pose that results in a robot
    //                            approach angle closest to approachAngle_rad is considered.
    // @param approachAngle_rad - The desired docking approach angle of the robot in world coordinates.
    class DriveToPickupObjectAction : public IDriveToInteractWithObject
    {
    public:
      DriveToPickupObjectAction(Robot& robot,
                                const ObjectID& objectID,
                                const bool useApproachAngle = false,
                                const f32 approachAngle_rad = 0,
                                const bool useManualSpeed = false,
                                Radians maxTurnTowardsFaceAngle_rad = 0.f,
                                const bool sayName = false);
      
      virtual ~DriveToPickupObjectAction() { }
      
      void SetDockingMethod(DockingMethod dockingMethod);
      
    private:
      PickupObjectAction* _pickupAction = nullptr;
    };
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then placing an object on it.
    // @param objectID         - object to place carried object on
    class DriveToPlaceOnObjectAction : public IDriveToInteractWithObject
    {
    public:
      
      // Places carried object on top of objectID
      DriveToPlaceOnObjectAction(Robot& robot,
                                 const ObjectID& objectID,
                                 const bool useApproachAngle = false,
                                 const f32 approachAngle_rad = 0,
                                 const bool useManualSpeed = false,
                                 Radians maxTurnTowardsFaceAngle_rad = 0.f,
                                 const bool sayName = false);
      
      virtual ~DriveToPlaceOnObjectAction() { }
    };
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then placing an object relative to it.
    // @param placementOffsetX_mm - The desired distance between the center of the docking marker
    //                              and the center of the object that is being placed, along the
    //                              direction of the docking marker's normal.
    // @param useApproachAngle  - If true, then only the preAction pose that results in a robot
    //                            approach angle closest to approachAngle_rad is considered.
    // @param approachAngle_rad - The desired docking approach angle of the robot in world coordinates.
    class DriveToPlaceRelObjectAction : public IDriveToInteractWithObject
    {
    public:
      // Place carried object on ground at specified placementOffset from objectID,
      // chooses preAction pose closest to approachAngle_rad if useApproachAngle == true.
      DriveToPlaceRelObjectAction(Robot& robot,
                                  const ObjectID& objectID,
                                  const bool placingOnGround = true,
                                  const f32 placementOffsetX_mm = 0,
                                  const f32 placementOffsetY_mm = 0,
                                  const bool useApproachAngle = false,
                                  const f32 approachAngle_rad = 0,
                                  const bool useManualSpeed = false,
                                  Radians maxTurnTowardsFaceAngle_rad = 0.f,
                                  const bool sayName = false);
      
      virtual ~DriveToPlaceRelObjectAction() { }
    };
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then rolling it.
    // @param useApproachAngle  - If true, then only the preAction pose that results in a robot
    //                            approach angle closest to approachAngle_rad is considered.
    // @param approachAngle_rad - The desired docking approach angle of the robot in world coordinates.
    class RollObjectAction;
    class DriveToRollObjectAction : public IDriveToInteractWithObject
    {
    public:
      DriveToRollObjectAction(Robot& robot,
                              const ObjectID& objectID,
                              const bool useApproachAngle = false,
                              const f32 approachAngle_rad = 0,
                              const bool useManualSpeed = false,
                              Radians maxTurnTowardsFaceAngle_rad = 0.f,
                              const bool sayName = false);

      virtual ~DriveToRollObjectAction() { }
      
      // Sets the approach angle so that, if possible, the roll action will roll the block to land upright. If
      // the block is upside down or already upright, and roll action will be allowed
      void RollToUpright();

      Result EnableDeepRoll(bool enable);
      
    private:
      ObjectID _objectID;
      RollObjectAction* _rollAction = nullptr;
    };
    
    
    // Common compound action for driving to an object and popping a wheelie off of it
    // @param useApproachAngle  - If true, then only the preAction pose that results in a robot
    //                            approach angle closest to approachAngle_rad is considered.
    // @param approachAngle_rad - The desired docking approach angle of the robot in world coordinates.
    class DriveToPopAWheelieAction : public IDriveToInteractWithObject
    {
    public:
      DriveToPopAWheelieAction(Robot& robot,
                               const ObjectID& objectID,
                               const bool useApproachAngle = false,
                               const f32 approachAngle_rad = 0,
                               const bool useManualSpeed = false,
                               Radians maxTurnTowardsFaceAngle_rad = 0.f,
                               const bool sayName = false);
      
      virtual ~DriveToPopAWheelieAction() { }
    };
  

    // Common compound action
    class DriveToAndTraverseObjectAction : public IDriveToInteractWithObject
    {
    public:
      DriveToAndTraverseObjectAction(Robot& robot,
                                     const ObjectID& objectID,
                                     const bool useManualSpeed = false,
                                     Radians maxTurnTowardsFaceAngle_rad = 0.f,
                                     const bool sayName = false);
      
      virtual ~DriveToAndTraverseObjectAction() { }
      
    };
    
    
    class DriveToAndMountChargerAction : public IDriveToInteractWithObject
    {
    public:
      DriveToAndMountChargerAction(Robot& robot,
                                   const ObjectID& objectID,
                                   const bool useManualSpeed = false,
                                   Radians maxTurnTowardsFaceAngle_rad = 0.f,
                                   const bool sayName = false);
      
      virtual ~DriveToAndMountChargerAction() { }
      
    };
    
    class DriveToRealignWithObjectAction : public CompoundActionSequential
    {
    public:
      DriveToRealignWithObjectAction(Robot& robot, ObjectID objectID, float dist_mm);
    };
  }
}

#endif /* ANKI_COZMO_DRIVE_TO_ACTIONS_H */
