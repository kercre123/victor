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
#include "clad/types/animationKeyFrames.h"

namespace Anki {
  
  namespace Cozmo {
    
    class DriveToPoseAction : public IAction
    {
    public:
      DriveToPoseAction(Robot& robot,
                        const Pose3d& pose,
                        const PathMotionProfile& motionProf = DEFAULT_PATH_MOTION_PROFILE,
                        const bool forceHeadDown  = true,
                        const bool useManualSpeed = false,
                        const Point3f& distThreshold = DEFAULT_POSE_EQUAL_DIST_THRESOLD_MM,
                        const Radians& angleThreshold = DEFAULT_POSE_EQUAL_ANGLE_THRESHOLD_RAD,
                        const float maxPlanningTime = DEFAULT_MAX_PLANNER_COMPUTATION_TIME_S,
                        const float maxReplanPlanningTime = DEFAULT_MAX_PLANNER_REPLAN_COMPUTATION_TIME_S);
      
      DriveToPoseAction(Robot& robot,
                        const PathMotionProfile& motionProf = DEFAULT_PATH_MOTION_PROFILE,
                        const bool forceHeadDown  = true,
                        const bool useManualSpeed = false); // Note that SetGoal(s) must be called befure Update()!
      
      DriveToPoseAction(Robot& robot,
                        const std::vector<Pose3d>& poses,
                        const PathMotionProfile& motionProf = DEFAULT_PATH_MOTION_PROFILE,
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
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_POSE; }

      // Don't lock wheels if we're using manual speed control (i.e. "assisted RC")
      virtual u8 GetTracksToLock() const override
      {
        u8 ignoredTracks = (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::LIFT_TRACK;
        if (!_useManualSpeed)
        {
          ignoredTracks |= ((u8)AnimTrackFlag::BODY_TRACK);
        }
        return ignoredTracks;
      }
      
      // Specify sounds to be played when movement starts, while driving, and once
      // the pose is reached. Use "" for any sound you don't want.
      // NOTE: these really specify the names of sound-only animations
      void SetSounds(const std::string& startSound,
                     const std::string& driveSound,
                     const std::string& stopSound);
      
      // Set the min/max time between end of last drive sound and beginning of next.
      // Actual gap will be randomly selected between these two values.
      void SetDriveSoundSpacing(f32 min_sec, f32 max_sec);
      
      // Defaults for playing sound
      static constexpr auto DefaultStartSound   = "";
      static constexpr auto DefaultDrivingSound = "";
      static constexpr auto DefaultStopSound    = "";
      static constexpr f32  DefaultDrivingSoundSpacingMin_sec = 0.5f;
      static constexpr f32  DefaultDrivingSoundSpacingMax_sec = 1.5f;

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
      
      Point3f  _goalDistanceThreshold;
      Radians  _goalAngleThreshold;
      bool     _useManualSpeed;
      
      float _maxPlanningTime;
      float _maxReplanPlanningTime;
      
      float _timeToAbortPlanning;
      
      // For playing sound
      std::string _startSound   = DriveToPoseAction::DefaultStartSound;
      std::string _drivingSound = DriveToPoseAction::DefaultDrivingSound;
      std::string _stopSound    = DriveToPoseAction::DefaultStopSound;
      f32         _drivingSoundSpacingMin_sec = DriveToPoseAction::DefaultDrivingSoundSpacingMin_sec;
      f32         _drivingSoundSpacingMax_sec = DriveToPoseAction::DefaultDrivingSoundSpacingMax_sec;
      f32         _nextDrivingSoundTime = 0.f;
      u32         _driveSoundActionTag  = (u32)ActionConstants::INVALID_TAG;
      
      Signal::SmartHandle _originChangedHandle;
      Signal::SmartHandle _soundCompletedHandle;
      
    }; // class DriveToPoseAction
    
    inline void DriveToPoseAction::SetSounds(const std::string& startSound,
                                             const std::string& driveSound,
                                             const std::string& stopSound)
    {
      _startSound   = startSound;
      _drivingSound = driveSound;
      _stopSound    = stopSound;
    }
    
    inline void DriveToPoseAction::SetDriveSoundSpacing(f32 min_sec, f32 max_sec)
    {
      if(max_sec <= min_sec) {
        PRINT_NAMED_WARNING("DriveToPoseAction.SetDriveSoundSpacing.InvalidMinMax",
                            "Min (%.3f) should be less than max (%.3f)",
                            min_sec, max_sec);
        return;
      }
      _drivingSoundSpacingMin_sec = min_sec;
      _drivingSoundSpacingMax_sec = max_sec;
    }

    
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
                          const PathMotionProfile& motionProf = DEFAULT_PATH_MOTION_PROFILE,
                          const f32 predockOffsetDistX_mm = 0,
                          const bool useApproachAngle = false,
                          const f32 approachAngle_rad = 0,
                          const bool useManualSpeed = false);
      
      DriveToObjectAction(Robot& robot,
                          const ObjectID& objectID,
                          const f32 distance_mm,
                          const PathMotionProfile& motionProf = DEFAULT_PATH_MOTION_PROFILE,
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
      
      // These just wrap the corresponding methods for the internally-used DriveToPose
      void SetSounds(const std::string& startSound,
                     const std::string& driveSound,
                     const std::string& stopSound);
      void SetDriveSoundSpacing(f32 min_sec, f32 max_sec);
      
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
      
      PathMotionProfile          _pathMotionProfile;
    
    private:
      // For playing sound
      std::string _startSound   = DriveToPoseAction::DefaultStartSound;
      std::string _drivingSound = DriveToPoseAction::DefaultDrivingSound;
      std::string _stopSound    = DriveToPoseAction::DefaultStopSound;
      f32         _drivingSoundSpacingMin_sec = DriveToPoseAction::DefaultDrivingSoundSpacingMin_sec;
      f32         _drivingSoundSpacingMax_sec = DriveToPoseAction::DefaultDrivingSoundSpacingMax_sec;

    }; // DriveToObjectAction
    
    inline void DriveToObjectAction::SetSounds(const std::string& startSound,
                                               const std::string& driveSound,
                                               const std::string& stopSound)
    {
      _startSound   = startSound;
      _drivingSound = driveSound;
      _stopSound    = stopSound;
    }
    
    inline void DriveToObjectAction::SetDriveSoundSpacing(f32 min_sec, f32 max_sec)
    {
      _drivingSoundSpacingMin_sec = min_sec;
      _drivingSoundSpacingMax_sec = max_sec;
    }
    
    
    class DriveToPlaceCarriedObjectAction : public DriveToObjectAction
    {
    public:
      DriveToPlaceCarriedObjectAction(Robot& robot,
                                      const Pose3d& placementPose,
                                      const bool placeOnGround,
                                      const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                      const bool useExactRotation = false,
                                      const bool useManualSpeed = false);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_PLACE_CARRIED_OBJECT; }
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override; // Simplified version from DriveToObjectAction
      Pose3d _placementPose;
      
      bool   _useExactRotation;
      
    }; // DriveToPlaceCarriedObjectAction()

    
    // Interface for all classes below which first drive to an object and then
    // do something with it.
    class IDriveToInteractWithObject : public CompoundActionSequential
    {
    public:
      
      // Wrappers for the sounds available in DriveToObjectAction, which is the
      // first action in the sequence for all derived classes
      void SetSounds(const std::string& startSound,
                     const std::string& driveSound,
                     const std::string& stopSound);
      
      void SetDriveSoundSpacing(f32 min_sec, f32 max_sec);
      
    protected:
      // Not directly instantiable
      IDriveToInteractWithObject(Robot& robot,
                                 const ObjectID& objectID,
                                 const PreActionPose::ActionType& actionType,
                                 const PathMotionProfile& motionProfile,
                                 const f32 distanceFromMarker_mm,
                                 const bool useApproachAngle,
                                 const f32 approachAngle_rad,
                                 const bool useManualSpeed);
      
    private:
      DriveToObjectAction* _driveToObjectAction;
      
    }; // class IDriveToInteractWithObject
    
    inline void IDriveToInteractWithObject::SetSounds(const std::string& startSound,
                                                      const std::string& driveSound,
                                                      const std::string& stopSound)
    {
      assert(nullptr != _driveToObjectAction);
      _driveToObjectAction->SetSounds(startSound, driveSound, stopSound);
    }
    
    inline void IDriveToInteractWithObject::SetDriveSoundSpacing(f32 min_sec, f32 max_sec)
    {
      assert(nullptr != _driveToObjectAction);
      _driveToObjectAction->SetDriveSoundSpacing(min_sec, max_sec);
    }
    
    
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
                                   const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                   const bool useApproachAngle = false,
                                   const f32 approachAngle_rad = 0,
                                   const bool useManualSpeed = false);
      
      // GetType returns the type from the AlignWithObjectAction
      virtual RobotActionType GetType() const override { return RobotActionType::ALIGN_WITH_OBJECT; }
      
      // Use AlignWithObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        _actions.back().second->GetCompletionUnion(completionUnion);
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
                                const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                const bool useApproachAngle = false,
                                const f32 approachAngle_rad = 0,
                                const bool useManualSpeed = false);
      
      // GetType returns the type from the PickupObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use PickupObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        _actions.back().second->GetCompletionUnion(completionUnion);
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
                                 const PathMotionProfile& motionProf = DEFAULT_PATH_MOTION_PROFILE,
                                 const bool useApproachAngle = false,
                                 const f32 approachAngle_rad = 0,
                                 const bool useManualSpeed = false);
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use PlaceRelObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        _actions.back().second->GetCompletionUnion(completionUnion);
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
                                  const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                  const f32 placementOffsetX_mm = 0,
                                  const bool useApproachAngle = false,
                                  const f32 approachAngle_rad = 0,
                                  const bool useManualSpeed = false);
      
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use PlaceRelObjectAction's completion info
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        _actions.back().second->GetCompletionUnion(completionUnion);
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
                              const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                              const bool useApproachAngle = false,
                              const f32 approachAngle_rad = 0,
                              const bool useManualSpeed = false);
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use RollObjectAction's completion signal
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        _actions.back().second->GetCompletionUnion(completionUnion);
      }
      
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
                               const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                               const bool useApproachAngle = false,
                               const f32 approachAngle_rad = 0,
                               const bool useManualSpeed = false);
      
      // GetType returns the type from the PlaceRelObjectAction, which is
      // determined dynamically
      virtual RobotActionType GetType() const override { return _actions.back().second->GetType(); }
      
      // Use RollObjectAction's completion signal
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        _actions.back().second->GetCompletionUnion(completionUnion);
      }
    };
  

    // Common compound action
    class DriveToAndTraverseObjectAction : public CompoundActionSequential
    {
    public:
      DriveToAndTraverseObjectAction(Robot& robot,
                                     const ObjectID& objectID,
                                     const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                     const bool useManualSpeed = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_AND_TRAVERSE_OBJECT; }
      
    };
    
    
    class DriveToAndMountChargerAction : public CompoundActionSequential
    {
    public:
      DriveToAndMountChargerAction(Robot& robot,
                                   const ObjectID& objectID,
                                   const PathMotionProfile& motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                   const bool useManualSpeed = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::DRIVE_TO_AND_MOUNT_CHARGER; }
      
    };
  }
}

#endif /* ANKI_COZMO_DRIVE_TO_ACTIONS_H */
