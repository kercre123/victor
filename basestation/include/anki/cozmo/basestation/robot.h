/**
 * File: robot.h
 *
 * Author: Andrew Stein
 * Date:   8/23/13
 *
 * Description: Defines a Robot representation on the Basestation, which is 
 *              in charge of communicating with (and mirroring the state of)
 *              a physical (hardware) robot.  
 *
 *              Convention: Set*() methods do not actually command the physical
 *              robot to do anything; they simply update some aspect of the 
 *              state or internal representation of the Basestation Robot. 
 *              To command the robot to "do" something, methods beginning with
 *              other action words, or add IAction objects to its ActionList.
 *
 * Copyright: Anki, Inc. 2013
 **/

#ifndef ANKI_COZMO_BASESTATION_ROBOT_H
#define ANKI_COZMO_BASESTATION_ROBOT_H

#include "anki/common/basestation/math/pose.h"
#include "anki/common/types.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/animations/engineAnimationController.h"
#include "anki/cozmo/basestation/encodedImage.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/ramp.h"
#include "anki/vision/basestation/camera.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/visionMarker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/types/dockingSignals.h"
#include "clad/types/imageTypes.h"
#include "clad/types/ledTypes.h"
#include "clad/types/robotStatusAndActions.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal.hpp"
#include "util/stats/recentStatsAccumulator.h"
#include <queue>
#include <time.h>
#include <unordered_map>
#include <utility>


namespace Anki {
  
// Forward declaration:

class PoseOriginList;

namespace Util {
namespace Data {
class DataPlatform;
}
}
  
namespace Cozmo {
  
// Forward declarations:
class AIComponent;
class ActionList;
class BehaviorFactory;
class BehaviorManager;
class BlockFilter;
class BlockTapFilterComponent;
class BlockWorld;
class CozmoContext;
class CubeAccelComponent;
class DrivingAnimationHandler;
class FaceWorld;
class IExternalInterface;
class MatPiece;
class MoodManager;
class MovementComponent;
class NVStorageComponent;
class ObjectPoseConfirmer;
class PetWorld;
class ProgressionUnlockComponent;
class RobotGyroDriftDetector;
class RobotIdleTimeoutComponent;
class RobotStateHistory;
class HistRobotState;
class IExternalInterface;
struct RobotState;
class ActiveCube;
class CubeLightComponent;
class BodyLightComponent;
class RobotToEngineImplMessaging;
class TextToSpeechComponent;
class PublicStateBroadcaster;
class VisionComponent;
class NeedsManager;
struct RobotState;
class PathComponent;

namespace Audio {
class RobotAudioClient;
}
  
namespace RobotInterface {
class MessageHandler;
class EngineToRobot;
class RobotToEngine;
enum class EngineToRobotTag : uint8_t;
enum class RobotToEngineTag : uint8_t;
} // end namespace RobotInterface

//
// Compile-time switch for Animation Streamer 2.0
//
#define BUILD_NEW_ANIMATION_CODE 0

// indent 2 spaces << that way !!!! coding standards !!!!
class Robot : private Util::noncopyable
{
public:
    
  Robot(const RobotID_t robotID, const CozmoContext* context);
  ~Robot();

  // =========== Robot properties ===========
  
  const RobotID_t GetID() const;

  // Specify whether this robot is a physical robot or not.
  // Currently, adjusts headCamPose by slop factor if it's physical.
  void SetPhysicalRobot(bool isPhysical);
  bool IsPhysical() const {return _isPhysical;}

  // Whether or not to ignore all incoming external messages that create/queue actions
  // Use with care: Make sure a call to ignore is eventually followed by a call to unignore
  void SetIgnoreExternalActions(bool ignore) { _ignoreExternalActions = ignore; }
  bool GetIgnoreExternalActions() { return _ignoreExternalActions;}

  // =========== Robot Update ===========
  
  Result Update();
    
  Result UpdateFullRobotState(const RobotState& msg);
    
  bool HasReceivedRobotState() const;

  const bool GetTimeSynced() const {return _timeSynced;}
  void       SetTimeSynced()       {_timeSynced = true; _syncTimeSentTime_sec = 0.0f; }

  Result SyncTime();  // TODO:(bn) only for robot event handler, move out of this header...

  TimeStamp_t GetLastMsgTimestamp() const { return _lastMsgTimestamp; }

  // This is just for unit tests to fake a syncTimeAck message from the robot
  // and force the head into calibrated state.
  void FakeSyncTimeAck() { _timeSynced = true; _isHeadCalibrated = true; _isLiftCalibrated = true; }

  // =========== Components ===========
  
  inline BlockWorld&       GetBlockWorld()       {assert(_blockWorld); return *_blockWorld;}
  inline const BlockWorld& GetBlockWorld() const {assert(_blockWorld); return *_blockWorld;}
  
  inline FaceWorld&       GetFaceWorld()       {assert(_faceWorld); return *_faceWorld;}
  inline const FaceWorld& GetFaceWorld() const {assert(_faceWorld); return *_faceWorld;}

  inline PetWorld&       GetPetWorld()       {assert(_petWorld); return *_petWorld;}
  inline const PetWorld& GetPetWorld() const {assert(_petWorld); return *_petWorld;}

  inline VisionComponent&       GetVisionComponent()       { assert(_visionComponent); return *_visionComponent; }
  inline const VisionComponent& GetVisionComponent() const { assert(_visionComponent); return *_visionComponent; }
  
  inline BlockTapFilterComponent& GetBlockTapFilter() {
    assert(_tapFilterComponent);
    return *_tapFilterComponent;
  }
  inline const BlockTapFilterComponent& GetBlockTapFilter() const {
    assert(_tapFilterComponent);
    return *_tapFilterComponent;
  }

  inline TextToSpeechComponent& GetTextToSpeechComponent() {
    assert(_textToSpeechComponent);
    return *_textToSpeechComponent;
  }
  inline const TextToSpeechComponent& GetTextToSpeechComponent() const {
    assert(_textToSpeechComponent);
    return *_textToSpeechComponent;
  }

  inline MovementComponent& GetMoveComponent() {
    assert(_movementComponent);
    return *_movementComponent;
  }
  inline const MovementComponent& GetMoveComponent() const {
    assert(_movementComponent);
    return *_movementComponent;
  }
 
  inline CubeLightComponent& GetCubeLightComponent() {
    assert(_cubeLightComponent);
    return *_cubeLightComponent;
  }
  inline const CubeLightComponent& GetCubeLightComponent() const {
    assert(_cubeLightComponent);
    return *_cubeLightComponent;
  }
  
  inline BodyLightComponent& GetBodyLightComponent() {
    assert(_bodyLightComponent);
    return *_bodyLightComponent;
  }
  inline const BodyLightComponent& GetBodyLightComponent() const {
    assert(_bodyLightComponent);
    return *_bodyLightComponent;
  }
  
  inline CubeAccelComponent& GetCubeAccelComponent() {
    assert(_cubeAccelComponent);
    return *_cubeAccelComponent;
  }
  inline const CubeAccelComponent& GetCubeAccelComponent() const {
    assert(_cubeAccelComponent);
    return *_cubeAccelComponent;
  }

  inline const MoodManager& GetMoodManager() const { assert(_moodManager); return *_moodManager; }
  inline MoodManager&       GetMoodManager()       { assert(_moodManager); return *_moodManager; }

  inline const NeedsManager& GetNeedsManager() const { assert(_needsManager); return *_needsManager; }
  inline NeedsManager&       GetNeedsManager()       { assert(_needsManager); return *_needsManager; }
  
  inline const BehaviorManager& GetBehaviorManager() const { return *_behaviorMgr; }
  inline BehaviorManager&       GetBehaviorManager()       { return *_behaviorMgr; }

  const BehaviorFactory& GetBehaviorFactory() const;
  BehaviorFactory&       GetBehaviorFactory();
  
  
  inline const ProgressionUnlockComponent& GetProgressionUnlockComponent() const {
    assert(_progressionUnlockComponent);
    return *_progressionUnlockComponent;
  }  
  inline ProgressionUnlockComponent& GetProgressionUnlockComponent() {
    assert(_progressionUnlockComponent);
    return *_progressionUnlockComponent;
  }

  inline const NVStorageComponent& GetNVStorageComponent() const {
    assert(_nvStorageComponent);
    return *_nvStorageComponent;
  }
  inline NVStorageComponent& GetNVStorageComponent() {
    assert(_nvStorageComponent);
    return *_nvStorageComponent;
  }

  inline const AIComponent& GetAIComponent() const {
    assert(_aiComponent);
    return *_aiComponent;
  }
  inline AIComponent& GetAIComponent() {
    assert(_aiComponent);
    return *_aiComponent;
  }
  
  inline const PublicStateBroadcaster& GetPublicStateBroadcaster() const {
    assert(_publicStateBroadcaster);
    return *_publicStateBroadcaster;
  }
  
  inline PublicStateBroadcaster& GetPublicStateBroadcaster(){
    assert(_publicStateBroadcaster);
    return *_publicStateBroadcaster;
  }

  inline const PathComponent& GetPathComponent() const { return *_pathComponent; }
  inline       PathComponent& GetPathComponent()       { return *_pathComponent; }
  
  const DrivingAnimationHandler& GetDrivingAnimationHandler() const { return *_drivingAnimationHandler; }
  DrivingAnimationHandler& GetDrivingAnimationHandler() { return *_drivingAnimationHandler; }
  
      
  const Util::RandomGenerator& GetRNG() const;
  Util::RandomGenerator& GetRNG();
  
  // =========== Localization ===========
  
  bool IsLocalized() const;
  void Delocalize(bool isCarryingObject);

  // Updates the pose of the robot.
  // Sends new pose down to robot (on next tick).
  Result SetNewPose(const Pose3d& newPose);
      
  // Get the ID of the object we are localized to
  const ObjectID& GetLocalizedTo() const {return _localizedToID;}
    
  // Set the object we are localized to.
  // Use nullptr to UnSet the localizedTo object but still mark the robot
  // as localized (i.e. to "odometry").
  Result SetLocalizedTo(const ObservableObject* object);
  
  // Has the robot moved since it was last localized
  bool HasMovedSinceBeingLocalized() const;
  
  // Get the squared distance to the closest, most recently observed marker
  // on the object we are localized to
  f32 GetLocalizedToDistanceSq() const;
  
  // TODO: Can this be removed in favor of the more general LocalizeToObject() below?
  Result LocalizeToMat(const MatPiece* matSeen, MatPiece* existinMatPiece);
      
  Result LocalizeToObject(const ObservableObject* seenObject, ObservableObject* existingObject);
    
  // True if we are on the sloped part of a ramp
  bool IsOnRamp() const { return _onRamp; }
    
  // Set whether or not the robot is on a ramp
  Result SetOnRamp(bool t);
    
  // Just sets the ramp to use and in which direction, not whether robot is on it yet
  void SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction);

  // True if robot is on charger
  bool   IsOnCharger()         const { return _isOnCharger; }

  // True if we think the robot is on a charger. This becomes true only when the robot touches the charger
  // contacts, and remains true until we think the robot has driven off the charger. It will not become true
  // based on localization or observing the charger marker, only based on feeling the charger. A robot on the
  // charger contacts is always on the platform (NOTE: even if it thinks it's in the air or on it's side)
  bool   IsOnChargerPlatform() const { return _isOnChargerPlatform; }
  
  // True if robot is charging
  bool   IsCharging()          const { return _isCharging; }
  // True if charger is out of spec
  bool   IsChargerOOS()        const { return _chargerOOS; }
  // Updates pose to be on charger
  Result SetPoseOnCharger();
  
  // Sets the charger that it's docking to
  void           SetCharger(const ObjectID& chargerID) { _chargerID = chargerID; }
  const ObjectID GetCharger() const                    { return _chargerID; }

  // =========== Cliff reactions ===========

  // whether or not the robot should react (the sensor may still be enabled)
  const bool GetIsCliffReactionDisabled() { return _isCliffReactionDisabled; }
  
  // =========== Face Display ============
  u32 GetDisplayWidthInPixels() const;
  u32 GetDisplayHeightInPixels() const;
   
  // =========== Camera / Vision ===========
  Vision::Camera GetHistoricalCamera(const HistRobotState& histState, TimeStamp_t t) const;
  Result         GetHistoricalCamera(TimeStamp_t t_request, Vision::Camera& camera) const;
  Pose3d         GetHistoricalCameraPose(const HistRobotState& histState, TimeStamp_t t) const;
  
  // Set the calibrated rotation of the camera
  void SetCameraRotation(f32 roll, f32 pitch, f32 yaw);

  // Return the timestamp of the last _processed_ image
  TimeStamp_t GetLastImageTimeStamp() const;  

  // =========== Pose (of the robot or its parts) ===========
  const Pose3d&       GetPose() const;
  const f32           GetHeadAngle() const;
  const f32           GetLiftAngle() const;
  const Pose3d&       GetLiftPose()     const { return _liftPose; } // At current lift position!
  const Pose3d&       GetLiftBasePose() const { return _liftBasePose; }
  const PoseFrameID_t GetPoseFrameID()  const { return _frameId; }
  const Pose3d*       GetWorldOrigin()  const { return _worldOrigin; }
  Pose3d              GetCameraPose(f32 atAngle) const;
  Pose3d              GetLiftPoseWrtCamera(f32 atLiftAngle, f32 atHeadAngle) const;

  OffTreadsState GetOffTreadsState() const {return _offTreadsState;}
  
  // Figure out the head angle to look at the given pose. Orientation of pose is
  // ignored. All that matters is its distance from the robot (in any direction)
  // and height. Note that returned head angle can be outside possible range.
  Result ComputeHeadAngleToSeePose(const Pose3d& pose, Radians& headAngle, f32 yTolFrac) const;
  
  // Figure out absolute body pan and head tilt angles to turn towards a point in an image.
  // Note that the head tilt is approximate because this function makes the simplifying
  // assumption that the head rotates around the camera center.
  Result ComputeTurnTowardsImagePointAngles(const Point2f& imgPoint, const TimeStamp_t timestamp,
                                            Radians& absPanAngle, Radians& absTiltAngle) const;
  
  const PoseOriginList&  GetPoseOriginList() const { assert(_poseOriginList); return *_poseOriginList; }
  
  ObjectPoseConfirmer& GetObjectPoseConfirmer() { assert(_objectPoseConfirmerPtr); return *_objectPoseConfirmerPtr; }
  
  // These change the robot's internal (basestation) representation of its
  // head angle, and lift angle, but do NOT actually command the
  // physical robot to do anything!
  void SetHeadAngle(const f32& angle);
  void SetLiftAngle(const f32& angle);

  void SetHeadCalibrated(bool isCalibrated);
  void SetLiftCalibrated(bool isCalibrated);

  bool IsHeadCalibrated() const;
  bool IsLiftCalibrated() const;
  
  // #notImplemented
  //    // Get 3D bounding box of the robot at its current pose or a given pose
  //    void GetBoundingBox(std::array<Point3f, 8>& bbox3d, const Point3f& padding_mm) const;
  //    void GetBoundingBox(const Pose3d& atPose, std::array<Point3f, 8>& bbox3d, const Point3f& padding_mm) const;

  // Get the bounding quad of the robot at its current or a given pose
  Quad2f GetBoundingQuadXY(const f32 padding_mm = 0.f) const; // at current pose
  Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 paddingScale = 0.f) const; // at specific pose
    
  // Return current height of lift's gripper
  f32 GetLiftHeight() const;
  
  // Conversion functions between lift height and angle
  static f32 ConvertLiftHeightToLiftAngleRad(f32 height_mm);
  static f32 ConvertLiftAngleToLiftHeightMM(f32 angle_rad);
  
  // Leaves input liftPose's parent alone and computes its position w.r.t.
  // liftBasePose, given the angle
  static void ComputeLiftPose(const f32 atAngle, Pose3d& liftPose);
  
  // Get pitch angle of robot
  Radians GetPitchAngle() const;
  
  // Return current bounding height of the robot, taking into account whether lift
  // is raised
  f32 GetHeight() const;
    
  // Wheel speeds, mm/sec
  f32 GetLeftWheelSpeed() const { return _leftWheelSpeed_mmps; }
  f32 GetRightWheelSpeed() const { return _rightWheelSpeed_mmps; }
    
  // Return pose of robot's drive center based on what it's currently carrying
  const Pose3d& GetDriveCenterPose() const;
    
  // Computes the drive center offset from origin based on current carrying state
  f32 GetDriveCenterOffset() const;
    
  // Computes pose of drive center for the given robot pose
  void ComputeDriveCenterPose(const Pose3d &robotPose, Pose3d &driveCenterPose) const;
    
  // Computes robot origin pose for the given drive center pose
  void ComputeOriginPose(const Pose3d &driveCenterPose, Pose3d &robotPose) const;
      
  // =========== Object Docking / Carrying ===========

  const ObjectID&            GetDockObject()          const {return _dockObjectID;}
  const ObjectID&            GetCarryingObject()      const {return _carryingObjectID;}
  const ObjectID&            GetCarryingObjectOnTop() const {return _carryingObjectOnTopID;}
  const std::set<ObjectID>   GetCarryingObjects()     const;
  const Vision::Marker::Code GetCarryingMarkerCode()  const {return _carryingMarkerCode;}

  bool IsCarryingObject()   const {return _carryingObjectID.IsSet(); }
  bool IsCarryingObject(const ObjectID& objectID) const;
  
  bool IsPickingOrPlacing() const {return _isPickingOrPlacing;}
  
  EncodedImage& GetEncodedImage() { return _encodedImage; }
  
  // TODO Define better API for (un)setting carried objects: they are confused easily with SetCarriedObjectAsUnattached
  void SetCarryingObject(ObjectID carryObjectID, Vision::Marker::Code atMarkerCode);
  void UnSetCarryingObjects(bool topOnly = false);
  
  // If objID == carryingObjectOnTopID, only that object's carry state is unset.
  // If objID == carryingObjectID, all carried objects' carry states are unset.
  void UnSetCarryObject(ObjectID objID);
  
  // Tell the physical robot to dock with the specified marker
  // of the specified object that it should currently be seeing.
  // If pixel_radius == std::numeric_limits<u8>::max(), the marker can be seen anywhere in the image,
  // otherwise the marker's center must be seen within pixel_radius of the
  // specified image coordinates.
  // marker2 needs to be specified when dockAction == DA_CROSS_BRIDGE to indicate
  // the expected marker on the end of the bridge. Otherwise, it is ignored.
  Result DockWithObject(const ObjectID objectID,
                        const f32 speed_mmps,
                        const f32 accel_mmps2,
                        const f32 decel_mmps2,
                        const Vision::KnownMarker::Code marker,
                        const Vision::KnownMarker::Code marker2,
                        const DockAction dockAction,
                        const u16 image_pixel_x,
                        const u16 image_pixel_y,
                        const u8 pixel_radius,
                        const f32 placementOffsetX_mm = 0,
                        const f32 placementOffsetY_mm = 0,
                        const f32 placementOffsetAngle_rad = 0,
                        const bool useManualSpeed = false,
                        const u8 numRetries = 2,
                        const DockingMethod dockingMethod = DockingMethod::BLIND_DOCKING,
                        const bool doLiftLoadCheck = false);
  
  // Same as above but without specifying image location for marker
  Result DockWithObject(const ObjectID objectID,
                        const f32 speed_mmps,
                        const f32 accel_mmps2,
                        const f32 decel_mmps2,
                        const Vision::KnownMarker::Code marker,
                        const Vision::KnownMarker::Code marker2,
                        const DockAction dockAction,
                        const f32 placementOffsetX_mm = 0,
                        const f32 placementOffsetY_mm = 0,
                        const f32 placementOffsetAngle_rad = 0,
                        const bool useManualSpeed = false,
                        const u8 numRetries = 2,
                        const DockingMethod dockingMethod = DockingMethod::BLIND_DOCKING,
                        const bool doLiftLoadCheck = false);
    
  // Transitions the object that robot was docking with to the one that it
  // is carrying, and puts it in the robot's pose chain, attached to the
  // lift. Returns RESULT_FAIL if the robot wasn't already docking with
  // an object.
  Result SetDockObjectAsAttachedToLift();
    
  // Same as above, but with specified object
  Result SetObjectAsAttachedToLift(const ObjectID& objectID,
                                   const Vision::KnownMarker::Code atMarkerCode);
  
  void UnsetDockObjectID() { _dockObjectID.UnSet(); _dockMarkerCode = Vision::MARKER_INVALID; }
  void SetLastPickOrPlaceSucceeded(bool tf) { _lastPickOrPlaceSucceeded = tf;  }
  bool GetLastPickOrPlaceSucceeded() const { return _lastPickOrPlaceSucceeded; }
    
  // Places the object that the robot was carrying in its current position w.r.t. the world, and removes
  // it from the lift pose chain so it is no "attached" to the robot. Set deleteLocatedObject=true to delete
  // the object instead of leaving it at the pose
  Result SetCarriedObjectAsUnattached(bool deleteLocatedObjects = false);

  // Send a message to the robot to place whatever it is carrying on the
  // ground right where it is. Returns RESULT_FAIL if robot is not carrying
  // anything.
  Result PlaceObjectOnGround(const bool useManualSpeed = false);
  
  bool WasObjectTappedRecently(const ObjectID& objectID) const;

  // =========== Object Stacking ===========
  
  // lets the robot decide if we should try to stack on top of the given object, so that we have a central place
  // to make the appropriate checks.
  // returns true if we should try to stack on top of the given object, false if something would prevent it,
  // for example if we think that block has something on top or it's too high to reach
  bool CanStackOnTopOfObject(const ObservableObject& object) const;

  // lets the robot decide if we should try to pick up the given object (assuming it is flat, not picking up
  // out of someone's hand). Checks that object is flat, not moving, no unknown pose, etc.
  bool CanPickUpObject(const ObservableObject& object) const;

  // same as above, but check that the block is on the ground (as opposed to stacked, on top of a notebook or
  // something, or in someone's hand
  bool CanPickUpObjectFromGround(const ObservableObject& object) const;
    
  /*
  // =========== Proximity Sensors ===========
  u8   GetProxSensorVal(ProxSensor_t sensor)    const {return _proxVals[sensor];}
  bool IsProxSensorBlocked(ProxSensor_t sensor) const {return _proxBlocked[sensor];}

  // Pose of where objects are assumed to be with respect to robot pose when
  // obstacles are detected by proximity sensors
  static const Pose3d ProxDetectTransform[NUM_PROX];
  */

  void SetEnableCliffSensor(bool val) { _enableCliffSensor = val; }
  bool IsCliffSensorEnabled() const { return _enableCliffSensor; }
  
  // Returns true if a cliff event was detected
  void SetCliffDetected(const bool isCliffDetected) { _isCliffDetected = isCliffDetected; }
  bool IsCliffDetected() const { return _isCliffDetected; }
  bool IsCliffSensorOn() const { return _isCliffSensorOn; }
  
  bool IsPickedUp() const { return _isPickedUp; }
  
  u16  GetCliffDataRaw() const { return _cliffDataRaw; }
  
  // sets distance detected by forward proximity sensor
  void SetForwardSensorValue(u16 value_mm)       { _forwardSensorValue_mm = value_mm; }
  u16  GetForwardSensorValue()             const { return _forwardSensorValue_mm; }

  // Evaluate how suspicious the cliff that caused robot to stop is based on how much the reading
  // actually rises while it's stopping in reaction to the supposed cliff.
  void EvaluateCliffSuspiciousnessWhenStopped();
  
  // Returns true if floor is suspiciously cliff-y looking based on variance of cliff readings
  bool IsFloorSuspiciouslyCliffy() const;
  
  // Returns current threshold for cliff detection
  u16 GetCliffDetectThreshold() const { return _cliffDetectThreshold; }
  
  // Get the mean/variance of cliff readings over the past few seconds. (See kCliffSensorRunningStatsWindowSize)
  f32  GetCliffRunningMean() const { return _cliffRunningMean; }
  f32  GetCliffRunningVar()  const { return _cliffRunningVar; }
  void ClearCliffRunningStats();
  
  // =========== IMU Data =============
  
  // Returns pointer to robot accelerometer readings in mm/s^2 with respect to head frame.
  // x-axis: points out face
  // y-axis: points out left ear
  // z-axis: points out top of head
  const AccelData& GetHeadAccelData() const {return _robotAccel; }
  
  // Returns pointer to robot gyro readings in rad/s with respect to head frame.
  // x-axis: points out face
  // y-axis: points out left ear
  // z-axis: points out top of head
  const GyroData& GetHeadGyroData() const {return _robotGyro; }
  
  // Returns the current accelerometer magnitude (norm of all 3 axes).
  float GetHeadAccelMagnitude() const {return _robotAccelMagnitude; }
  
  // Returns the current accelerometer magnitude, after being low-pass filtered.
  float GetHeadAccelMagnitudeFiltered() const {return _robotAccelMagnitudeFiltered; }
  
  // send the request down to the robot
  Result RequestIMU(const u32 length_ms) const;

  // =========== Actions Commands =============
    
  // Return a reference to the robot's action list for directly adding things
  // to do, either "now" or in queues.
  // TODO: This seems simpler than writing/maintaining wrappers, but maybe that would be better?
  ActionList& GetActionList() { assert(_actionList); return *_actionList; }

  bool HasActionList() const { return _actionList != nullptr; }
    
  // =========== Animation Commands =============
  
#if BUILD_NEW_ANIMATION_CODE
  inline IAnimationStreamer & GetAnimationStreamer() {
    return (*_animationController.get());
  }
#else
  inline IAnimationStreamer & GetAnimationStreamer() {
    return _animationStreamer;
  }
#endif

  // Returns the number of animation bytes or audio frames played on the robot since
  // it was initialized with SyncTime.
  s32 GetNumAnimationBytesPlayed() const;
  s32 GetNumAnimationAudioFramesPlayed() const;

  // Returns a count of the total number of bytes or audio frames streamed to the robot.
  s32 GetNumAnimationBytesStreamed() const;
  s32 GetNumAnimationAudioFramesStreamed() const;
  
  void IncrementNumAnimationBytesStreamed(s32 num);
  void IncrementNumAnimationAudioFramesStreamed(s32 num);
  
  // Tell the animation streamer to move the eyes by this x,y amount over the
  // specified duration (layered on top of any other animation that's playing).
  // Use tag = AnimationStreamer::NotAnimatingTag to start a new layer (in which
  // case tag will be set to the new layer's tag), or use an existing tag
  // to add the shift to that layer.
  void ShiftEyes(AnimationStreamer::Tag& tag, f32 xPix, f32 yPix,
                 TimeStamp_t duration_ms, const std::string& name = "ShiftEyes");
  
  void SetNumAnimationBytesPlayed(s32 numAnimationsBytesPlayed) {
    _numAnimationBytesPlayed = numAnimationsBytesPlayed;
  }
  
  void SetNumAnimationAudioFramesPlayed(s32 numAnimationAudioFramesPlayed) {
    _numAnimationAudioFramesPlayed = numAnimationAudioFramesPlayed;
  }
  
  void SetEnabledAnimTracks(u8 enabledAnimTracks) {
    _enabledAnimTracks = enabledAnimTracks;
  }
  
  void SetAnimationTag(u8 animationTag) {
    _animationTag = animationTag;
  }

  // Returns true if the robot is currently playing an animation, according
  // to most recent state message. NOTE: Will also be true if the animation
  // is the "idle" animation!
  bool IsAnimating() const;
    
  // Returns true iff the robot is currently playing the idle animation.
  bool IsIdleAnimating() const;
    
  // Returns the "tag" of the animation currently playing on the robot
  u8 GetCurrentAnimationTag() const;

  u8 GetEnabledAnimationTracks() const { return _enabledAnimTracks; }
  
  // =========== Audio =============
  Audio::RobotAudioClient* GetRobotAudioClient() { return _audioClient.get(); }
  
  // =========== Mood =============

  // Load in all data-driven emotion events // TODO: move to mood manager?
  void LoadEmotionEvents();      

  // =========== Pose history =============
  
  RobotStateHistory* GetStateHistory() { return _stateHistory.get(); }
  const RobotStateHistory* GetStateHistory() const { return _stateHistory.get(); }
  
  // Adds robot state information to history at t = state.timestamp
  // Only state updates should be calling this, however, it is exposed for unit tests
  Result AddRobotStateToHistory(const Pose3d& pose, const RobotState& state);
  
  // Increments frameID and adds a vision-only pose to history
  // Sets a flag to send a localization update on the next tick
  Result AddVisionOnlyStateToHistory(const TimeStamp_t t,
                                    const Pose3d& pose, 
                                    const f32 head_angle,
                                    const f32 lift_angle);

  // Updates the current pose to the best estimate based on
  // historical poses including vision-based poses. 
  // Returns true if the pose is successfully updated, false otherwise.
  bool UpdateCurrPoseFromHistory();

  Result GetComputedStateAt(const TimeStamp_t t_request, Pose3d& pose) const;

  // =========  Block messages  ============

  // Assign which objects the robot should connect to.
  // Max size of set is ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS.
  Result ConnectToObjects(const FactoryIDArray& factory_ids);
  
  // Returns true if the robot has succesfully connected to the object with the given factory ID
  bool IsConnectedToObject(FactoryID factoryID) const;

  // Called when messages related to the connection with the objects are received from the robot
  void HandleConnectedToObject(uint32_t activeID, FactoryID factoryID, ObjectType objectType);
  void HandleDisconnectedFromObject(uint32_t activeID, FactoryID factoryID, ObjectType objectType);

  // Set whether or not to broadcast to game which objects are available for connection
  void BroadcastAvailableObjects(bool enable);
    
  // =========  Other State  ============
  f32 GetBatteryVoltage() const { return _battVoltage; }
      
  // Abort everything the robot is doing, including path following, actions,
  // animations, and docking. This is like the big red E-stop button.
  // TODO: Probably need a more elegant way of doing this.
  Result AbortAll();
    
  // Abort things individually
  Result AbortAnimation();
  Result AbortDocking(); // a.k.a. PickAndPlace
  
  // Helper template for sending Robot messages with clean syntax
  template<typename T, typename... Args>
  Result SendRobotMessage(Args&&... args) const
  {
    return SendMessage(RobotInterface::EngineToRobot(T(std::forward<Args>(args)...)));
  }
  
  // Send a message to the physical robot
  Result SendMessage(const RobotInterface::EngineToRobot& message,
                     bool reliable = true, bool hot = false) const;
  
  
  // Sends debug string out to game and viz
  Result SendDebugString(const char *format, ...);
  
  // =========  Events  ============
  using RobotWorldOriginChangedSignal = Signal::Signal<void (RobotID_t)>;
  RobotWorldOriginChangedSignal& OnRobotWorldOriginChanged() { return _robotWorldOriginChangedSignal; }
  bool HasExternalInterface() const;
  
  IExternalInterface* GetExternalInterface();
  
  RobotInterface::MessageHandler* GetRobotMessageHandler();
  void SetImageSendMode(ImageSendMode newMode) { _imageSendMode = newMode; }
  const ImageSendMode GetImageSendMode() const { return _imageSendMode; }
  
  void SetLastSentImageID(u32 lastSentImageID) { _lastSentImageID = lastSentImageID; }
  const u32 GetLastSentImageID() const { return _lastSentImageID; }

  void SetCurrentImageDelay(double lastImageLatencyTime) { _lastImageLatencyTime_s = lastImageLatencyTime; }
  const Util::Stats::StatsAccumulator& GetImageStats() const { return _imageStats.GetPrimaryAccumulator(); }
  Util::Stats::RecentStatsAccumulator& GetRecentImageStats() { return _imageStats; }
  void SetTimeSinceLastImage(double timeSinceLastImage) { _timeSinceLastImage_s = 0.0; }
  double GetCurrentImageDelay() const { return std::max(_lastImageLatencyTime_s, _timeSinceLastImage_s); }

  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

  // Convenience wrapper for broadcasting an event if the robot has an ExternalInterface.
  // Does nothing if not. Returns true if event was broadcast, false if not (i.e.
  // if there was no external interface).
  bool Broadcast(ExternalInterface::MessageEngineToGame&& event);
  
  bool Broadcast(VizInterface::MessageViz&& event);
  
  void BroadcastEngineErrorCode(EngineErrorCode error);
  
  Util::Data::DataPlatform* GetContextDataPlatform();
  const CozmoContext* GetContext() const { return _context; }
  
  // Populate a RobotState message with robot's current state information (suitable for sending to external listeners)
  ExternalInterface::RobotState GetRobotState() const;
  
  // Populate a RobotState message with default values (suitable for sending to the robot itself, e.g. in unit tests)
  static RobotState GetDefaultRobotState();
  
  void SetDiscoveredObjects(FactoryID factoryId, ObjectType objectType, int8_t rssi, TimeStamp_t lastDiscoveredTimetamp);
  ObjectType GetDiscoveredObjectType(FactoryID id);
  void RemoveDiscoveredObjects(FactoryID factoryId) { _discoveredObjects.erase(factoryId); }
  const bool GetEnableDiscoveredObjectsBroadcasting() const { return _enableDiscoveredObjectsBroadcasting; }
  FactoryID GetClosestDiscoveredObjectsOfType(ObjectType type, uint8_t maxRSSI = std::numeric_limits<uint8_t>::max()) const;
  
  RobotToEngineImplMessaging& GetRobotToEngineImplMessaging() { return *_robotToEngineImplMessaging; }
  
  const u32 GetHeadSerialNumber() const { return _serialNumberHead; }
  void SetHeadSerialNumber(const u32 num) { _serialNumberHead = num; }
  const u32 GetBodySerialNumber() const { return _serialNumberBody; }
  void SetBodySerialNumber(const u32 num) { _serialNumberBody = num; }
  
  void SetModelNumber(const u32 num) { _modelNumber = num; }
  
  void SetBodyHWVersion(const s32 num) { _bodyHWVersion = num; }
  const s32 GetBodyHWVersion() const   { return _bodyHWVersion;}
  
  void SetBodyColor(const s32 color);
  const BodyColor GetBodyColor() const { return _bodyColor; }
  
  bool HasReceivedFirstStateMessage() const { return _gotStateMsgAfterTimeSync; }
  
protected:
  
  const CozmoContext* _context;
  
  RobotWorldOriginChangedSignal _robotWorldOriginChangedSignal;
  // The robot's identifier
  RobotID_t _ID;
  bool      _isPhysical       = false;
  u32       _serialNumberHead = 0;
  u32       _serialNumberBody = 0;
  u32       _modelNumber      = 0;
  s32       _bodyHWVersion    = -1;
  BodyColor _bodyColor        = BodyColor::UNKNOWN;
  
  // Whether or not sync time was acknowledged by physical robot
  bool _timeSynced = false;

  // Flag indicating whether a robotStateMessage was ever received
  TimeStamp_t _lastMsgTimestamp;
  bool        _newStateMsgAvailable = false;
    
  // A reference to the BlockWorld the robot lives in
  std::unique_ptr<BlockWorld>            _blockWorld;
 
  // A container for faces/people the robot knows about
  std::unique_ptr<FaceWorld>             _faceWorld;
 
  // A container for all pet faces the robot knows about
  std::unique_ptr<PetWorld>              _petWorld;
 
  std::unique_ptr<PublicStateBroadcaster> _publicStateBroadcaster;

  std::unique_ptr<BehaviorManager>       _behaviorMgr;
  
  ///////// Audio /////////
  std::unique_ptr<Audio::RobotAudioClient> _audioClient;

  // handles planning and path following
  std::unique_ptr<PathComponent> _pathComponent;
  
  ///////// Animation /////////
  // TODO:(bn) make animation streamer a pointer, pull Tag out into a constants / definitions file
  AnimationStreamer _animationStreamer;
  s32               _numAnimationBytesPlayed         = 0;
  s32               _numAnimationBytesStreamed       = 0;
  s32               _numAnimationAudioFramesPlayed   = 0;
  s32               _numAnimationAudioFramesStreamed = 0;
  u8                _animationTag                    = 0;
  
  std::unique_ptr<DrivingAnimationHandler> _drivingAnimationHandler;
  
  ///////// NEW Animation /////////
  std::unique_ptr<RobotAnimation::EngineAnimationController>  _animationController;
  
  std::unique_ptr<ActionList>             _actionList;
  std::unique_ptr<MovementComponent>      _movementComponent;
  std::unique_ptr<VisionComponent>        _visionComponent;
  std::unique_ptr<NVStorageComponent>     _nvStorageComponent;
  std::unique_ptr<AIComponent>            _aiComponent;
  std::unique_ptr<TextToSpeechComponent>  _textToSpeechComponent;
  std::unique_ptr<ObjectPoseConfirmer>    _objectPoseConfirmerPtr;
  std::unique_ptr<CubeLightComponent>     _cubeLightComponent;
  std::unique_ptr<BodyLightComponent>     _bodyLightComponent;
  std::unique_ptr<CubeAccelComponent>     _cubeAccelComponent;
  std::unique_ptr<RobotGyroDriftDetector> _gyroDriftDetector;

  // Hash to not spam debug messages
  size_t _lastDebugStringHash;

  /*
  // Proximity sensors
  std::array<u8,   NUM_PROX>  _proxVals;
  std::array<bool, NUM_PROX>  _proxBlocked;
  */
  
  // Geometry / Pose
  std::unique_ptr<PoseOriginList> _poseOriginList;
 
  Pose3d*        _worldOrigin;
  Pose3d         _pose;
  Pose3d         _driveCenterPose;
  PoseFrameID_t  _frameId                   = 0;
  ObjectID       _localizedToID; // ID of mat object robot is localized to
  bool           _hasMovedSinceLocalization = false;
  u32            _numMismatchedFrameIDs     = 0;
  
  // May be true even if not localized to an object, if robot has not been picked up
  bool _isLocalized                  = true;
  bool _localizedToFixedObject; // false until robot sees a _fixed_ mat
  bool _needToSendLocalizationUpdate = false;
  
  // Whether or not to ignore all external action messages
  bool _ignoreExternalActions = false;
  
  // Stores (squared) distance to the closest observed marker of the object we're localized to
  f32 _localizedMarkerDistToCameraSq = -1.0f;
  
  Result UpdateWorldOrigin(Pose3d& newPoseWrtNewOrigin);
    
  const Pose3d     _neckPose;     // joint around which head rotates
  Pose3d           _headCamPose;  // in canonical (untilted) position w.r.t. neck joint
  static const RotationMatrix3d _kDefaultHeadCamRotation;
  const Pose3d     _liftBasePose; // around which the base rotates/lifts
  Pose3d           _liftPose;     // current, w.r.t. liftBasePose

  f32              _currentHeadAngle;
  
  f32              _currentLiftAngle = 0;
  Radians          _pitchAngle;
  
  f32              _leftWheelSpeed_mmps;
  f32              _rightWheelSpeed_mmps;
  
  bool             _isHeadCalibrated = false;
  bool             _isLiftCalibrated = false;
    
  // Ramping
  bool             _onRamp = false;
  ObjectID         _rampID;
  Point2f          _rampStartPosition;
  f32              _rampStartHeight;
  Ramp::TraversalDirection _rampDirection;
  
  // Charge base ID that is being docked to
  ObjectID         _chargerID;
  
  // State
  bool             _isPickingOrPlacing    = false;
  bool             _isOnCharger           = false;
  bool             _isCharging            = false;
  bool             _chargerOOS            = false;
  f32              _battVoltage           = 5;
  ImageSendMode    _imageSendMode         = ImageSendMode::Off;
  bool             _enableCliffSensor     = true;
  u32              _lastSentImageID       = 0;
  u8               _enabledAnimTracks     = (u8)AnimTrackFlag::ALL_TRACKS;
  bool             _isPickedUp            = false;
  bool             _isCliffDetected       = false;
  bool             _isCliffSensorOn       = false;
  u16              _cliffDataRaw          = std::numeric_limits<u16>::max();
  u16              _forwardSensorValue_mm = 0;
  bool             _isOnChargerPlatform   = false;
  bool             _isCliffReactionDisabled = false;
  bool             _isBodyInAccessoryMode = true;
  u8               _setBodyModeTicDelay   = 0;
  bool             _gotStateMsgAfterTimeSync = false;
  
  u32              _suspiciousCliffCnt = 0;
  u16              _cliffDetectThreshold;
  u32              _cliffStartTimestamp = 0;
  
  u32              _lastStatusFlags       = 0;
  
  // Increments count of suspicious cliff. (i.e. Cliff was detected but data looks like maybe it's not real.)
  void IncrementSuspiciousCliffCount();
  
  // Assesses suspiciousness of current cliff and adjusts cliff threshold if necessary
  void UpdateCliffDetectThreshold();
  
  // Cliff-yness tracking
  std::deque<u16> _cliffDataQueue;
  f32             _cliffRunningMean;
  f32             _cliffRunningVar;
  f32             _cliffRunningVar_acc;
  void UpdateCliffRunningStats(const RobotState& msg);
  

  OffTreadsState    _offTreadsState  = OffTreadsState::OnTreads;
  OffTreadsState    _awaitingConfirmationTreadState = OffTreadsState::OnTreads;
  TimeStamp_t      _timeOffTreadStateChanged_ms = 0;
  TimeStamp_t      _fallingStartedTime_ms = 0;
  
  // IMU data
  AccelData        _robotAccel;
  GyroData         _robotGyro;
  float            _robotAccelMagnitude = 0.0f; // current magnitude of accelerometer data (norm of all three axes)
  float            _robotAccelMagnitudeFiltered = 0.0f; // low-pass filtered accelerometer magnitude
  
  // Sets robot pose but does not update the pose on the robot.
  // Unless you know what you're doing you probably want to use
  // the public function SetNewPose()
  void SetPose(const Pose3d &newPose);

  // helper for CanStackOnTopOfObject and CanPickUpObjectFromGround
  bool CanInteractWithObjectHelper(const ObservableObject& object, Pose3d& relPose) const;
  
  // State history
  std::unique_ptr<RobotStateHistory> _stateHistory;
    
  // Takes startPose and moves it forward as if it were a robot pose by distance mm and
  // puts result in movedPose.
  static void MoveRobotPoseForward(const Pose3d &startPose, const f32 distance, Pose3d &movedPose);
  
  // Docking / Carrying
  // We can't store pointers to makers eithers because the object may be unobserved and reoverserved, which
  // could cause a located instance to be destroyed and recreated
  ObjectID                  _dockObjectID;
  Vision::KnownMarker::Code _dockMarkerCode = Vision::MARKER_INVALID;
  ObjectID                  _carryingObjectID;
  Vision::KnownMarker::Code _carryingMarkerCode = Vision::MARKER_INVALID;
  ObjectID                  _carryingObjectOnTopID;
  bool                      _lastPickOrPlaceSucceeded = false;
    
  EncodedImage _encodedImage; // TODO:(bn) store pointer?
  double       _timeSinceLastImage_s = 0.0;
  double       _lastImageLatencyTime_s = 0.0;
  Util::Stats::RecentStatsAccumulator _imageStats{50};

  ///////// Modifiers ////////
  
  void SetPickingOrPlacing(bool t)        {_isPickingOrPlacing = t;}
  void SetOnCharger(bool onCharger);
  void SetOnChargerPlatform(bool onPlatform);
  void SetIsCharging(bool isCharging)     {_isCharging = isCharging;}
  
  // returns whether the tread state was updated or not
  bool CheckAndUpdateTreadsState(const RobotState& msg);
  
  ///////// Mood/Emotions ////////
  MoodManager*         _moodManager;

  ///////// Needs ////////
  NeedsManager*        _needsManager;
  
  ///////// Progression/Skills ////////
  ProgressionUnlockComponent* _progressionUnlockComponent;
    
  //////// Block pool ////////
  BlockFilter* _blockFilter;
  
  //////// Block Taps Filter ////////
  BlockTapFilterComponent* _tapFilterComponent;
  
  // Set of desired objects to connect to. Set by BlockFilter.
  struct ObjectToConnectToInfo {
    FactoryID factoryID;
    bool      pending;
      
    ObjectToConnectToInfo() {
      Reset();
    }
      
    void Reset();
  };
  std::array<ObjectToConnectToInfo, (size_t)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS> _objectsToConnectTo;

  // Map of discovered objects and the last time that they were heard from
  struct ActiveObjectInfo {
    enum class ConnectionState {
      Invalid,
      PendingConnection,
      Connected,
      PendingDisconnection,
      Disconnected
    };
    
    FactoryID       factoryID;
    ObjectType      objectType;
    ConnectionState connectionState;
    uint8_t         rssi;
    TimeStamp_t     lastDiscoveredTimeStamp;
    float           lastDisconnectionTime;
      
    ActiveObjectInfo() {
      Reset();
    }
      
    void Reset();
  };
  std::unordered_map<FactoryID, ActiveObjectInfo> _discoveredObjects;
  bool _enableDiscoveredObjectsBroadcasting = false;

  // Vector of currently connected objects by active slot index
  std::array<ActiveObjectInfo, (size_t)ActiveObjectConstants::MAX_NUM_ACTIVE_OBJECTS> _connectedObjects;
  
  double _lastDisconnectedCheckTime;
  constexpr static double kDisconnectedCheckDelay = 2.0f; // How often do we check for disconnected objects
  constexpr static double kDisconnectedDelay = 2.0f;      // How long must be the object disconnected before we really remove it from the list of connected objects

  // Called in Update(), checks if there are objectsToConnectTo that
  // have been discovered and should be connected to
  void ConnectToRequestedObjects();
  
  // Called during Update(), it checks if objects we have received disconnected messages from should really
  // be considered disconnected.
  void CheckDisconnectedObjects();

  std::unique_ptr<RobotToEngineImplMessaging> _robotToEngineImplMessaging;
  std::unique_ptr<RobotIdleTimeoutComponent>  _robotIdleTimeoutComponent;

  Result SendAbsLocalizationUpdate(const Pose3d&        pose,
                                   const TimeStamp_t&   t,
                                   const PoseFrameID_t& frameId) const;
    
  // Sync time with physical robot and trigger it to send back camera calibration
  Result SendSyncTime() const;
  
  float _syncTimeSentTime_sec = 0.0f;
  constexpr static float kMaxSyncTimeAckDelay_sec = 5.0f;
  
  // Send robot's current pose
  Result SendAbsLocalizationUpdate() const;
    
  // Update the head angle on the robot
  Result SendHeadAngleUpdate() const;

  // Request imu log from robot
  Result SendIMURequest(const u32 length_ms) const;

  Result SendAbortDocking();
  Result SendAbortAnimation();
    
  Result SendSetCarryState(CarryState state);

    
  // =========  Active Object messages  ============
  Result SendFlashObjectIDs();
  void ActiveObjectLightTest(const ObjectID& objectID);  // For testing
  
}; // class Robot

//
// Inline Mutators
//
  
inline void Robot::SetDiscoveredObjects(FactoryID factoryId, ObjectType objectType, int8_t rssi, TimeStamp_t lastDiscoveredTimetamp)
{
  ActiveObjectInfo& discoveredObject = _discoveredObjects[factoryId];
  
  discoveredObject.factoryID = factoryId;
  discoveredObject.objectType = objectType;
  discoveredObject.rssi = rssi;
  discoveredObject.lastDiscoveredTimeStamp = lastDiscoveredTimetamp;
}
  
//
// Inline accessors:
//
inline const RobotID_t Robot::GetID(void) const
{ return _ID; }

inline const Pose3d& Robot::GetPose(void) const
{ return _pose; }

inline const Pose3d& Robot::GetDriveCenterPose(void) const
{return _driveCenterPose; }

inline const f32 Robot::GetHeadAngle() const
{ return _currentHeadAngle; }

inline const f32 Robot::GetLiftAngle() const
{ return _currentLiftAngle; }

inline void Robot::SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction) {
  _rampID = rampID;
  _rampDirection = direction;
}
  
inline Result Robot::SetDockObjectAsAttachedToLift(){
  return SetObjectAsAttachedToLift(_dockObjectID, _dockMarkerCode);
}
  
inline u8 Robot::GetCurrentAnimationTag() const {
  return _animationTag;
}

inline bool Robot::IsAnimating() const {
  return _animationTag != 0;
}

inline bool Robot::IsIdleAnimating() const {
  return _animationTag == 255;
}

inline s32 Robot::GetNumAnimationBytesPlayed() const {
  return _numAnimationBytesPlayed;
}

inline s32 Robot::GetNumAnimationBytesStreamed() const {
  return _numAnimationBytesStreamed;
}

inline s32 Robot::GetNumAnimationAudioFramesPlayed() const {
  return _numAnimationAudioFramesPlayed;
}
  
inline s32 Robot::GetNumAnimationAudioFramesStreamed() const {
  return _numAnimationAudioFramesStreamed;
}
  
inline void Robot::IncrementNumAnimationBytesStreamed(s32 num) {
  _numAnimationBytesStreamed += num;
}
  
inline void Robot::IncrementNumAnimationAudioFramesStreamed(s32 num) {
  _numAnimationAudioFramesStreamed += num;
}

inline f32 Robot::GetLocalizedToDistanceSq() const {
  return _localizedMarkerDistToCameraSq;
}
  
inline bool Robot::HasMovedSinceBeingLocalized() const {
  return _hasMovedSinceLocalization;
}
  
inline bool Robot::IsLocalized() const {
  
  DEV_ASSERT(_isLocalized || (!_isLocalized && !_localizedToID.IsSet()),
             "Robot can't think it is localized and have localizedToID set!");
  
  return _isLocalized;
}

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_BASESTATION_ROBOT_H
