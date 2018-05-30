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

#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/animationTag.h"
#include "engine/encodedImage.h"
#include "util/entityComponent/entity.h"
#include "engine/events/ankiEvent.h"
#include "util/entityComponent/dependencyManagedEntity.h"
#include "engine/ramp.h"
#include "engine/fullRobotPose.h"
#include "engine/robotComponents_fwd.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/visionMarker.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/animationTypes.h"
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
class RandomGenerator;
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
class BehaviorSystemManager;
class BlockTapFilterComponent;
class BlockWorld;
class CozmoContext;
class CubeAccelComponent;
class CubeCommsComponent;
class DrivingAnimationHandler;
class FaceWorld;
class IExternalInterface;
class InventoryComponent;
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
class PublicStateBroadcaster;
class VisionComponent;
class VisionScheduleMediator;
class PathComponent;
class DockingComponent;
class CarryingComponent;
class CliffSensorComponent;
class ProxSensorComponent;
class TouchSensorComponent;
class AnimationComponent;
class MapComponent;
class MicComponent;
class BatteryComponent;
class BeatDetectorComponent;
class TextToSpeechCoordinator;

namespace Audio {
  class EngineRobotAudioClient;
}

namespace RobotInterface {
class MessageHandler;
class EngineToRobot;
class RobotToEngine;
enum class EngineToRobotTag : uint8_t;
enum class RobotToEngineTag : uint8_t;
} // end namespace RobotInterface


// CozmoContext is a coretech class - this wrapper allows the context to work
// with the dependency managed component interface
class ContextWrapper:  public IDependencyManagedComponent<RobotComponentID> {
public:
  ContextWrapper(const CozmoContext* context)
  : IDependencyManagedComponent(this, RobotComponentID::CozmoContextWrapper)
  , context(context){}
  const CozmoContext* context;

  virtual ~ContextWrapper(){}

  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override {};
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {};
};

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

  const bool GetSyncRobotAcked() const {return _syncRobotAcked;}
  void       SetSyncRobotAcked()       {_syncRobotAcked = true; _syncRobotSentTime_sec = 0.0f; }

  Result SyncRobot();  // TODO:(bn) only for robot event handler, move out of this header...

  TimeStamp_t GetLastMsgTimestamp() const { return _lastMsgTimestamp; }

  // This is just for unit tests to fake a syncRobotAck message from the robot
  // and force the head into calibrated state.
  void FakeSyncRobotAck() { _syncRobotAcked = true; _isHeadCalibrated = true; _isLiftCalibrated = true; }

  // =========== Components ===========

  bool HasComponent(RobotComponentID componentID) const {
    return (_components != nullptr) &&
           _components->HasComponent(componentID) &&
           _components->GetComponent(componentID).IsValueValid();
  }

  template<typename T>
  T& GetComponent() const {return _components->GetValue<T>();}

  template<typename T>
  T& GetComponent() {return _components->GetValue<T>();}


  template<typename T>
  T* GetComponentPtr() const {return _components->GetBasePtr<T>();}

  template<typename T>
  T* GetComponentPtr() {return _components->GetBasePtr<T>();}



  inline BlockWorld&       GetBlockWorld()       {return GetComponent<BlockWorld>();}
  inline const BlockWorld& GetBlockWorld() const {return GetComponent<BlockWorld>();}

  inline FaceWorld&       GetFaceWorld()       {return GetComponent<FaceWorld>();}
  inline const FaceWorld& GetFaceWorld() const {return GetComponent<FaceWorld>();}

  inline PetWorld&       GetPetWorld()       {return GetComponent<PetWorld>();}
  inline const PetWorld& GetPetWorld() const {return GetComponent<PetWorld>();}

  inline VisionComponent&       GetVisionComponent()       { return GetComponent<VisionComponent>(); }
  inline const VisionComponent& GetVisionComponent() const { return GetComponent<VisionComponent>(); }

  inline VisionScheduleMediator& GetVisionScheduleMediator() {return GetComponent<VisionScheduleMediator>(); }
  inline const VisionScheduleMediator& GetVisionScheduleMediator() const {return GetComponent<VisionScheduleMediator>(); }

  inline MapComponent&       GetMapComponent()       {return GetComponent<MapComponent>();}
  inline const MapComponent& GetMapComponent() const {return GetComponent<MapComponent>();}

  inline BlockTapFilterComponent& GetBlockTapFilter() {return GetComponent<BlockTapFilterComponent>();}
  inline const BlockTapFilterComponent& GetBlockTapFilter() const {return GetComponent<BlockTapFilterComponent>();}

  inline MovementComponent& GetMoveComponent() {return GetComponent<MovementComponent>();}
  inline const MovementComponent& GetMoveComponent() const {return GetComponent<MovementComponent>();}

  inline CubeLightComponent& GetCubeLightComponent() {return GetComponent<CubeLightComponent>();}
  inline const CubeLightComponent& GetCubeLightComponent() const {return GetComponent<CubeLightComponent>();}

  inline BodyLightComponent& GetBodyLightComponent() {return GetComponent<BodyLightComponent>();}
  inline const BodyLightComponent& GetBodyLightComponent() const {return GetComponent<BodyLightComponent>();}

  inline CubeAccelComponent& GetCubeAccelComponent() {return GetComponent<CubeAccelComponent>();}
  inline const CubeAccelComponent& GetCubeAccelComponent() const {return GetComponent<CubeAccelComponent>();}

  inline CubeCommsComponent& GetCubeCommsComponent() {return GetComponent<CubeCommsComponent>();}
  inline const CubeCommsComponent& GetCubeCommsComponent() const {return GetComponent<CubeCommsComponent>();}

  inline const MoodManager& GetMoodManager() const { return GetComponent<MoodManager>();}
  inline MoodManager&       GetMoodManager()       { return GetComponent<MoodManager>();}


  inline const ProgressionUnlockComponent& GetProgressionUnlockComponent() const {return GetComponent<ProgressionUnlockComponent>();}
  inline ProgressionUnlockComponent& GetProgressionUnlockComponent() {return GetComponent<ProgressionUnlockComponent>();}

  inline const InventoryComponent& GetInventoryComponent() const {return GetComponent<InventoryComponent>();}
  inline InventoryComponent& GetInventoryComponent() {return GetComponent<InventoryComponent>();}

  inline const NVStorageComponent& GetNVStorageComponent() const {return GetComponent<NVStorageComponent>();}
  inline NVStorageComponent& GetNVStorageComponent() {return GetComponent<NVStorageComponent>();}

  inline const AIComponent& GetAIComponent() const {return GetComponent<AIComponent>();}
  inline AIComponent& GetAIComponent() {return GetComponent<AIComponent>();}

  inline const PublicStateBroadcaster& GetPublicStateBroadcaster() const {return GetComponent<PublicStateBroadcaster>();}
  inline PublicStateBroadcaster& GetPublicStateBroadcaster(){return GetComponent<PublicStateBroadcaster>();}

  inline DockingComponent& GetDockingComponent() {return GetComponent<DockingComponent>();}
  inline const DockingComponent& GetDockingComponent() const {return GetComponent<DockingComponent>();}

  inline CarryingComponent& GetCarryingComponent() {return GetComponent<CarryingComponent>();}
  inline const CarryingComponent& GetCarryingComponent() const {return GetComponent<CarryingComponent>();}

  inline RobotIdleTimeoutComponent& GetIdleTimeoutComponent() {return GetComponent<RobotIdleTimeoutComponent>();}
  inline const RobotIdleTimeoutComponent& GetIdleTimeoutComponent() const {return GetComponent<RobotIdleTimeoutComponent>();}

  inline const PathComponent& GetPathComponent() const { return GetComponent<PathComponent>(); }
  inline       PathComponent& GetPathComponent()       { return GetComponent<PathComponent>(); }

  inline const CliffSensorComponent& GetCliffSensorComponent() const { return GetComponent<CliffSensorComponent>(); }
  inline       CliffSensorComponent& GetCliffSensorComponent()       { return GetComponent<CliffSensorComponent>(); }

  inline const ProxSensorComponent& GetProxSensorComponent() const { return GetComponent<ProxSensorComponent>(); }
  inline       ProxSensorComponent& GetProxSensorComponent()       { return GetComponent<ProxSensorComponent>(); }

  inline const AnimationComponent& GetAnimationComponent() const { return GetComponent<AnimationComponent>(); }
  inline       AnimationComponent& GetAnimationComponent()       { return GetComponent<AnimationComponent>(); }

  inline const TextToSpeechCoordinator& GetTextToSpeechCoordinator() const { return GetComponent<TextToSpeechCoordinator>();}
  inline       TextToSpeechCoordinator& GetTextToSpeechCoordinator()       { return GetComponent<TextToSpeechCoordinator>();}

  inline const TouchSensorComponent& GetTouchSensorComponent() const { return GetComponent<TouchSensorComponent>(); }
  inline       TouchSensorComponent& GetTouchSensorComponent()       { return GetComponent<TouchSensorComponent>(); }

  const DrivingAnimationHandler& GetDrivingAnimationHandler() const { return GetComponent<DrivingAnimationHandler>(); }
  DrivingAnimationHandler& GetDrivingAnimationHandler() { return GetComponent<DrivingAnimationHandler>(); }

  const MicComponent& GetMicComponent() const { return GetComponent<MicComponent>(); }
  MicComponent&       GetMicComponent()       { return GetComponent<MicComponent>(); }

  const BatteryComponent&    GetBatteryComponent()    const { return GetComponent<BatteryComponent>(); }
  BatteryComponent&          GetBatteryComponent()          { return GetComponent<BatteryComponent>(); }

  const BeatDetectorComponent&    GetBeatDetectorComponent()    const { return GetComponent<BeatDetectorComponent>(); }
  BeatDetectorComponent&          GetBeatDetectorComponent()          { return GetComponent<BeatDetectorComponent>(); }
  
  const PoseOriginList&  GetPoseOriginList() const { return *_poseOrigins.get(); }

  ObjectPoseConfirmer& GetObjectPoseConfirmer() { return GetComponent<ObjectPoseConfirmer>(); }
  const ObjectPoseConfirmer& GetObjectPoseConfirmer() const {return GetComponent<ObjectPoseConfirmer>();}

  ActionList& GetActionList() { return GetComponent<ActionList>(); }

  Audio::EngineRobotAudioClient* GetAudioClient() { return &GetComponent<Audio::EngineRobotAudioClient>(); }
  const Audio::EngineRobotAudioClient* GetAudioClient() const { return &GetComponent<Audio::EngineRobotAudioClient>(); }

  RobotStateHistory* GetStateHistory() { return GetComponentPtr<RobotStateHistory>(); }
  const RobotStateHistory* GetStateHistory() const { return GetComponentPtr<RobotStateHistory>(); }

  RobotToEngineImplMessaging& GetRobotToEngineImplMessaging() { return GetComponent<RobotToEngineImplMessaging>(); }

  const CozmoContext* GetContext() const { return _context; }

  const Util::RandomGenerator& GetRNG() const;
  Util::RandomGenerator& GetRNG();



  inline const std::string&     GetBehaviorDebugString() const { return _behaviorDebugStr; }

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
  Result LocalizeToMat(const MatPiece* matSeen, MatPiece* existingMatPiece);

  Result LocalizeToObject(const ObservableObject* seenObject, ObservableObject* existingObject);

  // True if we are on the sloped part of a ramp
  bool IsOnRamp() const { return _onRamp; }

  // Set whether or not the robot is on a ramp
  Result SetOnRamp(bool t);

  // Just sets the ramp to use and in which direction, not whether robot is on it yet
  void SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction);

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

  // Return the timestamp of the last _processed_ image
  TimeStamp_t GetLastImageTimeStamp() const;

  // =========== Pose (of the robot or its parts) ===========
  const Pose3d&       GetPose() const;
  const PoseFrameID_t GetPoseFrameID()  const { return _frameId; }
  const Pose3d&       GetWorldOrigin()  const;
  PoseOriginID_t      GetWorldOriginID()const;

  Pose3d              GetCameraPose(const f32 atAngle) const;
  Transform3d         GetLiftTransformWrtCamera(const f32 atLiftAngle, const f32 atHeadAngle) const;

  OffTreadsState GetOffTreadsState() const {return _offTreadsState;}
  TimeStamp_t GetOffTreadsStateLastChangedTime_ms() const { return _timeOffTreadStateChanged_ms; }

  // Return whether the given pose is in the same origin as the robot's current origin
  bool IsPoseInWorldOrigin(const Pose3d& pose) const;

  // Figure out the head angle to look at the given pose. Orientation of pose is
  // ignored. All that matters is its distance from the robot (in any direction)
  // and height. Note that returned head angle can be outside possible range.
  Result ComputeHeadAngleToSeePose(const Pose3d& pose, Radians& headAngle, f32 yTolFrac) const;

  // Figure out absolute body pan and head tilt angles to turn towards a point in an image.
  // Note that the head tilt is approximate because this function makes the simplifying
  // assumption that the head rotates around the camera center.
  Result ComputeTurnTowardsImagePointAngles(const Point2f& imgPoint, const TimeStamp_t timestamp,
                                            Radians& absPanAngle, Radians& absTiltAngle) const;

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
  static Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 paddingScale = 0.f); // at specific pose

  // Return current height of lift's gripper
  f32 GetLiftHeight() const;

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

  EncodedImage& GetEncodedImage() { return _encodedImage; }

  bool IsPickedUp() const { return _isPickedUp; }

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

  // IMU temperature sent from the robot
  void SetImuTemperature(const float temp) { _robotImuTemperature_degC = temp; }
  float GetImuTemperature() const {return _robotImuTemperature_degC; }

  // send the request down to the robot
  Result RequestIMU(const u32 length_ms) const;



  // =========== Animation Commands =============

  // Returns true if the robot is currently playing an animation, according
  // to most recent state message. NOTE: Will also be true if the animation
  // is the "idle" animation!
  bool IsAnimating() const;

  u8 GetEnabledAnimationTracks() const;

  // =========== Mood =============

  // Load in all data-driven emotion events // TODO: move to mood manager?
  void LoadEmotionEvents();

  // =========== Pose history =============

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

  bool WasObjectTappedRecently(const ObjectID& objectID) const;

  // ======== Power button ========

  bool IsPowerButtonPressed() const { return _powerButtonPressed; }

  // Abort everything the robot is doing, including path following, actions,
  // animations, and docking. This is like the big red E-stop button.
  // TODO: Probably need a more elegant way of doing this.
  Result AbortAll();

  // Abort things individually
  Result AbortAnimation();

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

  IExternalInterface* GetExternalInterface() const;

  RobotInterface::MessageHandler* GetRobotMessageHandler() const;
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

  // Populate a RobotState message with robot's current state information (suitable for sending to external listeners)
  ExternalInterface::RobotState GetRobotState() const;

  // Populate a RobotState message with default values (suitable for sending to the robot itself, e.g. in unit tests)
  static RobotState GetDefaultRobotState();

  const u32 GetHeadSerialNumber() const { return _serialNumberHead; }
  void SetHeadSerialNumber(const u32 num) { _serialNumberHead = num; }
  const u32 GetBodySerialNumber() const { return _serialNumberBody; }
  void SetBodySerialNumber(const u32 num) { _serialNumberBody = num; }

  void SetModelNumber(const u32 num) { _modelNumber = num; }

  void SetBodyHWVersion(const s32 num) { _bodyHWVersion = num; }
  const s32 GetBodyHWVersion() const   { return _bodyHWVersion;}

  void SetBodyColor(const s32 color);
  const BodyColor GetBodyColor() const { return _bodyColor; }
  
  bool HasReceivedFirstStateMessage() const { return _gotStateMsgAfterRobotSync; }

  void Shutdown() { _toldToShutdown = true; }
  bool ToldToShutdown() const { return _toldToShutdown; }
  
protected:  
  bool _toldToShutdown = false;

  const CozmoContext* _context;
  std::unique_ptr<PoseOriginList> _poseOrigins;

  using EntityType = DependencyManagedEntity<RobotComponentID>;
  using ComponentPtr = std::unique_ptr<EntityType>;

  ComponentPtr _components;

  RobotWorldOriginChangedSignal _robotWorldOriginChangedSignal;
  // The robot's identifier
  RobotID_t _ID;
  bool      _isPhysical       = false;
  u32       _serialNumberHead = 0;
  u32       _serialNumberBody = 0;
  u32       _modelNumber      = 0;
  s32       _bodyHWVersion    = -1;
  BodyColor _bodyColor        = BodyColor::UNKNOWN;

  // Whether or not sync was acknowledged by physical robot
  bool _syncRobotAcked = false;

  // Flag indicating whether a robotStateMessage was ever received
  TimeStamp_t _lastMsgTimestamp;
  bool        _newStateMsgAvailable = false;

  std::string                            _behaviorDebugStr;


  // Hash to not spam debug messages
  size_t _lastDebugStringHash;


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

  f32              _leftWheelSpeed_mmps;
  f32              _rightWheelSpeed_mmps;

  // We can assume the motors are calibrated by the time
  // engine connects to robot
  bool             _isHeadCalibrated = true;
  bool             _isLiftCalibrated = true;

  // Ramping
  bool             _onRamp = false;
  ObjectID         _rampID;
  Point2f          _rampStartPosition;
  f32              _rampStartHeight;
  Ramp::TraversalDirection _rampDirection;

  // Charge base ID that is being docked to
  ObjectID         _chargerID;

  // State
  ImageSendMode    _imageSendMode            = ImageSendMode::Off;
  u32              _lastSentImageID          = 0;
  bool             _powerButtonPressed       = false;
  bool             _isPickedUp               = false;
  bool             _isCliffReactionDisabled  = false;
  bool             _gotStateMsgAfterRobotSync = false;
  u32              _lastStatusFlags          = 0;

  OffTreadsState   _offTreadsState                 = OffTreadsState::OnTreads;
  OffTreadsState   _awaitingConfirmationTreadState = OffTreadsState::OnTreads;
  TimeStamp_t      _timeOffTreadStateChanged_ms    = 0;
  TimeStamp_t      _fallingStartedTime_ms          = 0;

  // IMU data
  AccelData        _robotAccel;
  GyroData         _robotGyro;
  float            _robotAccelMagnitude = 0.0f; // current magnitude of accelerometer data (norm of all three axes)
  float            _robotAccelMagnitudeFiltered = 0.0f; // low-pass filtered accelerometer magnitude
  AccelData        _robotAccelFiltered; // low-pass filtered robot accelerometer data (for each axis)
  float            _robotImuTemperature_degC = 0.f;

  // Sets robot pose but does not update the pose on the robot.
  // Unless you know what you're doing you probably want to use
  // the public function SetNewPose()
  void SetPose(const Pose3d &newPose);

  // Takes startPose and moves it forward as if it were a robot pose by distance mm and
  // puts result in movedPose.
  static void MoveRobotPoseForward(const Pose3d &startPose, const f32 distance, Pose3d &movedPose);

  EncodedImage _encodedImage; // TODO:(bn) store pointer?
  double       _timeSinceLastImage_s = 0.0;
  double       _lastImageLatencyTime_s = 0.0;
  Util::Stats::RecentStatsAccumulator _imageStats{50};

  // returns whether the tread state was updated or not
  bool CheckAndUpdateTreadsState(const RobotState& msg);

  Result SendAbsLocalizationUpdate(const Pose3d&        pose,
                                   const TimeStamp_t&   t,
                                   const PoseFrameID_t& frameId) const;

  // Sync with physical robot
  Result SendSyncRobot() const;

  float _syncRobotSentTime_sec = 0.0f;
  constexpr static float kMaxSyncRobotAckDelay_sec = 5.0f;

  // Used to calculate tick rate
  float _prevCurrentTime_sec = 0.0f;

  // Send robot's current pose
  Result SendAbsLocalizationUpdate() const;

  // Update the head angle on the robot
  Result SendHeadAngleUpdate() const;

  // Request imu log from robot
  Result SendIMURequest(const u32 length_ms) const;

  Result SendAbortAnimation();

  // As a temp dev step, try swapping out aiComponent after construction/initialization
  // for something like unit tests - theoretically this should be handled by having a non
  // fully enumerated constructor option, but for the time being use this for dev/testing purposes
  // only since caching etc could blow it all to shreds
  void DevReplaceAIComponent(AIComponent* aiComponent, bool shouldManage = false);

  // Performs various startup checks and displays fault codes as appropriate
  Result UpdateStartupChecks();
}; // class Robot


//
// Inline accessors:
//
inline const RobotID_t Robot::GetID(void) const
{ return _ID; }

inline const Pose3d& Robot::GetPose(void) const
{
  ANKI_VERIFY(GetComponent<FullRobotPose>().GetPose().GetRootID() == GetWorldOriginID(),
              "Robot.GetPose.BadPoseRootOrWorldOriginID",
              "WorldOriginID:%d(%s), RootID:%d",
              GetWorldOriginID(), GetWorldOrigin().GetName().c_str(),
              GetComponent<FullRobotPose>().GetPose().GetRootID());

  // TODO: COZMO-1637: Once we figure this out, switch this back to dev_assert for efficiency
  ANKI_VERIFY(GetComponent<FullRobotPose>().GetPose().HasSameRootAs(GetWorldOrigin()),
              "Robot.GetPose.PoseOriginNotWorldOrigin",
              "WorldOrigin: %s, Pose: %s",
              GetWorldOrigin().GetNamedPathToRoot(false).c_str(),
              GetComponent<FullRobotPose>().GetPose().GetNamedPathToRoot(false).c_str());

  return GetComponent<FullRobotPose>().GetPose();
}

inline const Pose3d& Robot::GetDriveCenterPose(void) const
{
  // TODO: COZMO-1637: Once we figure this out, switch this back to dev_assert for efficiency
  ANKI_VERIFY(_driveCenterPose.HasSameRootAs(GetWorldOrigin()),
              "Robot.GetDriveCenterPose.PoseOriginNotWorldOrigin",
              "WorldOrigin: %s, Pose: %s",
              GetWorldOrigin().GetNamedPathToRoot(false).c_str(),
              _driveCenterPose.GetNamedPathToRoot(false).c_str());

  return _driveCenterPose;
}

inline void Robot::SetRamp(const ObjectID& rampID, const Ramp::TraversalDirection direction) {
  _rampID = rampID;
  _rampDirection = direction;
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
