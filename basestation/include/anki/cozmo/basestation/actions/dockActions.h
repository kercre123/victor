/**
 * File: dockActions.h
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements docking related cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_DOCKACTIONS_H
#define ANKI_COZMO_DOCKACTIONS_H

#include "clad/types/actionTypes.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "clad/types/animationKeyFrames.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"

namespace Anki {
  
  namespace Vision {
    // Forward Declarations:
    class KnownMarker;
  }
  
  namespace Cozmo {
    
    // Forward Declarations:
    class Robot;
    class Animation;
    
    f32 ComputePreActionPoseDistThreshold(const Pose3d& preActionPose,
                                          const ActionableObject* actionObject,
                                          const Radians& preActionPoseAngleTolerance);

    // Interface for actions that involve "docking" with an object
    class IDockAction : public IAction
    {
    public:
      IDockAction(ObjectID objectID, const bool useManualSpeed = false);
      
      virtual ~IDockAction();
      
      // Set whether or not to check if robot is near predock pose before docking.
      void SetDoNearPredockPoseCheck(bool doCheck) { _doNearPredockPoseCheck = doCheck; }
      
      // Use a value <= 0 to ignore how far away the robot is from the closest
      // PreActionPose and proceed regardless.
      void SetPreActionPoseAngleTolerance(Radians angleTolerance);
      
      // Set docking speed and acceleration
      void SetSpeedAndAccel(f32 speed_mmps, f32 accel_mmps2);
      void SetSpeed(f32 speed_mmps);
      void SetAccel(f32 accel_mmps2);
      
      // Set placement offset relative to marker
      void SetPlacementOffset(f32 offsetX_mm, f32 offsetY_mm, f32 offsetAngle_rad);
      
      // Set whether or not to place carried object on ground
      void SetPlaceOnGround(bool placeOnGround);
      
      // Sets the animation to play when lift moves after docking.
      // The animation should only contain a sound track!
      void SetPostDockLiftMovingAnimation(const std::string& animName);
      
      virtual u8 GetAnimTracksToDisable() const override {
        return (uint8_t)AnimTrackFlag::HEAD_TRACK | (uint8_t)AnimTrackFlag::LIFT_TRACK | (uint8_t)AnimTrackFlag::BODY_TRACK;
      }
      
      // Should only lock wheels if we are not using manual speed (i.e. "assisted RC")
      virtual u8 GetMovementTracksToIgnore() const override
      {
        u8 ignoredTracks = (uint8_t)AnimTrackFlag::HEAD_TRACK | (uint8_t)AnimTrackFlag::LIFT_TRACK;
        if (!_useManualSpeed)
        {
          ignoredTracks |= ((uint8_t)AnimTrackFlag::BODY_TRACK);
        }
        return ignoredTracks;
      }
      
    protected:
      
      // IDockAction implements these two required methods from IAction for its
      // derived classes
      virtual ActionResult Init() override final;
      virtual ActionResult CheckIfDone() override final;
      
      // Most docking actions don't use a second dock marker, but in case they
      // do, they can override this method to choose one from the available
      // preaction poses, given which one was closest.
      virtual const Vision::KnownMarker* GetDockMarker2(const std::vector<PreActionPose>& preActionPoses,
                                                        const size_t closestIndex) { return nullptr; }
      
      // Pure virtual methods that must be implemented by derived classes in
      // order to define the parameters of docking and how to verify success.
      virtual Result SelectDockAction(ActionableObject* object) = 0;
      virtual PreActionPose::ActionType GetPreActionType() = 0;
      virtual ActionResult Verify() = 0;
      
      // Optional additional delay before verification
      virtual f32 GetVerifyDelayInSeconds() const { return 0.f; }
      
      // Subclasses should call this because it sets the interaction result
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override {
        // TODO: Annoying we have to copy this out, bet the Get_() method is const...
        ObjectInteractionCompleted interactionCompleted = completionUnion.Get_objectInteractionCompleted();
        interactionCompleted.result = _interactionResult;
        completionUnion.Set_objectInteractionCompleted(interactionCompleted);
      }
      
      ObjectID                   _dockObjectID;
      DockAction                 _dockAction;
      const Vision::KnownMarker* _dockMarker                     = nullptr;
      const Vision::KnownMarker* _dockMarker2                    = nullptr;
      Radians                    _preActionPoseAngleTolerance    = DEFAULT_PREDOCK_POSE_ANGLE_TOLERANCE;
      f32                        _waitToVerifyTime               = -1;
      bool                       _wasPickingOrPlacing            = false;
      bool                       _useManualSpeed                 = false;
      IActionRunner*             _faceAndVerifyAction            = nullptr;
      f32                        _placementOffsetX_mm            = 0;
      f32                        _placementOffsetY_mm            = 0;
      f32                        _placementOffsetAngle_rad       = 0;
      bool                       _placeObjectOnGroundIfCarrying  = false;
      f32                        _dockSpeed_mmps                 = DEFAULT_DOCK_SPEED_MMPS;
      f32                        _dockAccel_mmps2                = DEFAULT_DOCK_ACCEL_MMPS2;
      ObjectInteractionResult    _interactionResult              = ObjectInteractionResult::INCOMPLETE;
      bool                       _doNearPredockPoseCheck         = true;
      
    private:
      
      // Handler for when lift begins to move so that we can play an accompanying sound
      Signal::SmartHandle        _liftMovingSignalHandle;
      
      // Name of animation to play when moving lift post-dock
      std::string                _liftMovingAnimation = "";
      
      AnimationStreamer::Tag     _squintLayerTag = AnimationStreamer::NotAnimatingTag;
      
    }; // class IDockAction
    
    
    // If not carrying anything, pops a wheelie off of the specified object
    class PopAWheelieAction : public IDockAction
    {
    public:
      PopAWheelieAction(ObjectID objectID, const bool useManualSpeed = false);
      
      virtual const std::string& GetName() const override;
      
      // Override to determine type (low roll, or potentially other rolls) dynamically depending
      // on what we were doing.
      virtual RobotActionType GetType() const override;
      
      // Override completion signal to fill in information about rolled objects
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ROLLING; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
    }; // class PopAWheelieAction
    
    
    // "Docks" to the specified object at the distance specified
    class AlignWithObjectAction : public IDockAction
    {
    public:
      AlignWithObjectAction(ObjectID objectID,
                            const f32 distanceFromMarker_mm,
                            const bool useManualSpeed = false);
      virtual ~AlignWithObjectAction();
      
      virtual const std::string& GetName() const override;
      
      virtual RobotActionType GetType() const override {return RobotActionType::ALIGN_WITH_OBJECT;};
      
    protected:
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ActionType::DOCKING; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
    }; // class AlignWithObjectAction
    
    
    // Picks up the specified object.
    class PickupObjectAction : public IDockAction
    {
    public:
      PickupObjectAction(ObjectID objectID,
                         const bool useManualSpeed = false);
      
      virtual const std::string& GetName() const override;
      
      // Override to determine type (pick/place, low/high) dynamically depending
      // on what we were doing.
      virtual RobotActionType GetType() const override;
      
    protected:
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ActionType::DOCKING; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
      // For verifying if we successfully picked up the object
      Pose3d _dockObjectOrigPose;
      
    }; // class PickupObjectAction
    
    
    class PlaceObjectOnGroundAction : public IAction
    {
    public:
      
      PlaceObjectOnGroundAction();
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::PLACE_OBJECT_LOW; }
      
      virtual u8 GetAnimTracksToDisable() const override { return (uint8_t)AnimTrackFlag::LIFT_TRACK; }
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual ActionResult Init() override;
      virtual ActionResult CheckIfDone() override;
      
      // Need longer than default for check if done:
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.5f; }
      
      ObjectID                    _carryingObjectID;
      const Vision::KnownMarker*  _carryObjectMarker = nullptr;
      IActionRunner*              _faceAndVerifyAction = nullptr;
      ObjectInteractionResult     _interactionResult = ObjectInteractionResult::INCOMPLETE;
      
    }; // class PlaceObjectOnGroundAction
    
    
    // Common compound action
    // @param placementPose    - The pose in which the carried object should be placed.
    // @param useExactRotation - If true, then the carried object is placed in the exact
    //                           6D pose represented by placement pose. Otherwise,
    //                           x,y and general axis alignment with placementPose rotation
    //                           are the only constraints.
    class PlaceObjectOnGroundAtPoseAction : public CompoundActionSequential
    {
    public:
      PlaceObjectOnGroundAtPoseAction(const Pose3d& placementPose,
                                      const PathMotionProfile motionProfile = DEFAULT_PATH_MOTION_PROFILE,
                                      const bool useExactRotation = false,
                                      const bool useManualSpeed = false);
      
      virtual RobotActionType GetType() const override { return RobotActionType::PLACE_OBJECT_LOW; }
    };
    
    
    // If carrying an object, places it on or relative to the specified object.
    class PlaceRelObjectAction : public IDockAction
    {
    public:
      PlaceRelObjectAction(ObjectID objectID,
                           const bool placeOnGround = false,
                           const f32 placementOffsetX_mm = 0,
                           const bool useManualSpeed = false);
      
      virtual const std::string& GetName() const override;
      
      // Override to determine type (pick/place, low/high) dynamically depending
      // on what we were doing.
      virtual RobotActionType GetType() const override;
      
    protected:
      
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ActionType::PLACE_RELATIVE; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
      // For verifying if we successfully picked up the object
      //Pose3d _dockObjectOrigPose;
      
      // If placing an object, we need a place to store what robot was
      // carrying, for verification.
      ObjectID                   _carryObjectID;
      const Vision::KnownMarker* _carryObjectMarker;
      
      IActionRunner*             _placementVerifyAction = nullptr;
      bool                       _verifyComplete; // used in PLACE modes
      
    }; // class PlaceRelObjectAction
    
    
    // If not carrying anything, rolls the specified object.
    // If carrying an object, fails.
    class RollObjectAction : public IDockAction
    {
    public:
      RollObjectAction(ObjectID objectID, const bool useManualSpeed = false);
      
      virtual const std::string& GetName() const override;
      
      // Override to determine type (low roll, or potentially other rolls) dynamically depending
      // on what we were doing.
      virtual RobotActionType GetType() const override;
      
      // Override completion signal to fill in information about rolled objects
      virtual void GetCompletionUnion(ActionCompletedUnion& completionUnion) const override;
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ROLLING; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
      // For verifying if we successfully rolled the object
      Pose3d _dockObjectOrigPose;
      
      const Vision::KnownMarker* _expectedMarkerPostRoll = nullptr;
      
      IActionRunner*             _rollVerifyAction = nullptr;
      
    }; // class RollObjectAction

    
    class CrossBridgeAction : public IDockAction
    {
    public:
      CrossBridgeAction(ObjectID bridgeID, const bool useManualSpeed);
      
      virtual const std::string& GetName() const override;
      virtual RobotActionType GetType() const override { return RobotActionType::CROSS_BRIDGE; }
      
    protected:
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
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
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
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
      
      virtual Result SelectDockAction(ActionableObject* object) override;
      
      virtual ActionResult Verify() override;
      
      virtual PreActionPose::ActionType GetPreActionType() override { return PreActionPose::ENTRY; }
      
      // Give the robot a little longer to start ascending/descending before
      // checking if it is done
      virtual f32 GetCheckIfDoneDelayInSeconds() const override { return 1.f; }
      
    }; // class MountChargerAction
  }
}

#endif /* ANKI_COZMO_DOCKACTIONS_H */