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
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationKeyFrames.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
  
  namespace Cozmo {

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
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_POSE; }

      // Don't lock wheels if we're using manual speed control (i.e. "assisted RC")
      virtual u8 GetTracksToLock() const override
      {
        return (_useManualSpeed ? 0 : (u8)AnimTrackFlag::BODY_TRACK);
      }

    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      bool IsUsingManualSpeed() {return _useManualSpeed;}
      
      bool     _startedTraversingPath = false;
      
    private:
      bool     _isGoalSet;
      bool     _driveWithHeadDown;
      
      std::vector<Pose3d> _goalPoses;
      size_t              _selectedGoalIndex;
      
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
      virtual ~DriveToObjectAction() { };
      
      // TODO: Add version where marker code is specified instead of action?
      //DriveToObjectAction(Robot& robot, const ObjectID& objectID, Vision::Marker::Code code);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_OBJECT; }
      
      virtual u8 GetTracksToLock() const override { return (u8)AnimTrackFlag::BODY_TRACK; }
      
      // If set, instead of driving to the nearest preActionPose, only the preActionPose
      // that is most closely aligned with the approach angle is considered.
      void SetApproachAngle(const f32 angle_rad);
      
      // Whether or not to verify the final pose, once the path is complete,
      // according to the latest know preAction pose for the specified object.
      void DoPositionCheckOnPathCompletion(bool doCheck) { _doPositionCheckOnPathCompletion = doCheck; }
      
      void SetMotionProfile(const PathMotionProfile& motionProfile);
            
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      ActionResult InitHelper(ActionableObject* object);
      ActionResult GetPossiblePoses(ActionableObject* object,
                                    std::vector<Pose3d>& possiblePoses,
                                    bool& alreadyInPosition);
      
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
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_PLACE_CARRIED_OBJECT; }
      
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
    class IDriveToInteractWithObject : public CompoundActionSequential
    {
    protected:
      // Not directly instantiable
      IDriveToInteractWithObject(Robot& robot,
                                 const ObjectID& objectID,
                                 const PreActionPose::ActionType& actionType,
                                 const f32 distanceFromMarker_mm,
                                 const bool useApproachAngle,
                                 const f32 approachAngle_rad,
                                 const bool useManualSpeed);

    public:
      
      void SetMotionProfile(const PathMotionProfile& motionProfile);

    protected:

      // If set, instead of driving to the nearest preActionPose, only the preActionPose
      // that is most closely aligned with the approach angle is considered.
      void SetApproachAngle(const f32 angle_rad);
      
    private:

      DriveToObjectAction* _driveToObjectAction = nullptr;
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
                                   const bool useManualSpeed = false);
      
      // GetType returns the type from the AlignWithObjectAction
      virtual RobotActionType GetType() const override { return RobotActionType::ALIGN_WITH_OBJECT; }
      
      // Use AlignWithObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.back().first;
        }
      }
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
                                const bool useManualSpeed = false);
      
      // GetType returns the type from the PickupObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override {
        if(_actions.size() > 0) {
          return _actions.back()->GetType();
        } else {
          return _completedActionInfoStack.back().second;
        }
      }
      
      // Use PickupObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.back().first;
        }
      }
      
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
                                 const bool useManualSpeed = false);
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override {
        if(_actions.size() > 0) {
          return _actions.back()->GetType();
        } else {
          return _completedActionInfoStack.back().second;
        }
      }
      
      // Use PlaceRelObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.back().first;
        }
      }
      
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
                                  const f32 placementOffsetX_mm = 0,
                                  const bool useApproachAngle = false,
                                  const f32 approachAngle_rad = 0,
                                  const bool useManualSpeed = false);
      
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override {
        if(_actions.size() > 0) {
          return _actions.back()->GetType();
        } else {
          return _completedActionInfoStack.back().second;
        }
      }
      
      // Use PlaceRelObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.back().first;
        }
      }
      
    };
    
    
    // Common compound action for driving to an object, visually verifying we
    // can still see it, and then rolling it.
    // @param useApproachAngle  - If true, then only the preAction pose that results in a robot
    //                            approach angle closest to approachAngle_rad is considered.
    // @param approachAngle_rad - The desired docking approach angle of the robot in world coordinates.
    class DriveToRollObjectAction : public IDriveToInteractWithObject
    {
    public:
      DriveToRollObjectAction(Robot& robot,
                              const ObjectID& objectID,
                              const bool useApproachAngle = false,
                              const f32 approachAngle_rad = 0,
                              const bool useManualSpeed = false);

      // Sets the approach angle so that, if possible, the roll action will roll the block to land upright. If
      // the block is upside down or already upright, and roll action will be allowed
      void RollToUpright();
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override {
        if(_actions.size() > 0) {
          return _actions.back()->GetType();
        } else {
          return _completedActionInfoStack.back().second;
        }
      }
      
      // Use RollObjectAction's completion signal
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.back().first;
        }
      }

    private:
      ObjectID _objectID;

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
                               const bool useManualSpeed = false);
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override {
        if(_actions.size() > 0) {
          return _actions.back()->GetType();
        } else {
          return _completedActionInfoStack.back().second;
        }
      }
      
      // Use RollObjectAction's completion signal
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        if(_actions.size() > 0) {
          _actions.back()->GetCompletionUnion(completionUnion);
        } else {
          completionUnion = _completedActionInfoStack.back().first;
        }
      }
    };
  

    // Common compound action
    class DriveToAndTraverseObjectAction : public IDriveToInteractWithObject
    {
    public:
      DriveToAndTraverseObjectAction(Robot& robot,
                                     const ObjectID& objectID,
                                     const bool useManualSpeed = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_AND_TRAVERSE_OBJECT; }
      
    };
    
    
    class DriveToAndMountChargerAction : public IDriveToInteractWithObject
    {
    public:
      DriveToAndMountChargerAction(Robot& robot,
                                   const ObjectID& objectID,
                                   const bool useManualSpeed = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_AND_MOUNT_CHARGER; }
      
    };
  }
}

#endif /* ANKI_COZMO_DRIVE_TO_ACTIONS_H */
