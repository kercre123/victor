/**
 * File: movementComponent.h
 *
 * Author: Lee Crippen
 * Created: 10/21/2015
 *
 * Description: Robot component to handle logic and messages associated with the robot moving.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_MovementComponent_H__
#define __Anki_Cozmo_Basestation_Components_MovementComponent_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/objectIDs.h"
#include "coretech/vision/engine/trackedFace.h"
#include "engine/components/animationComponent.h"
#include "util/entityComponent/entity.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include <list>
#include <map>

namespace Anki {
namespace Cozmo {
  
// declarations
class Robot;
struct RobotState;
class IExternalInterface;
enum class AnimTrackFlag : uint8_t;
  
class MovementComponent : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  using MotorActionID = u8;

  MovementComponent();
  virtual ~MovementComponent() { }

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override{};
  //////
  // end IDependencyManagedComponent functions
  //////
  
  void NotifyOfRobotState(const RobotState& robotState);
  
  void OnRobotDelocalized();
  
  // Checks for unexpected movement specifically while turning such as
  // - Cozmo is turning one direction but you turn him the other way
  // - Cozmo is turning one direction and you turn him faster so he overshoots his turn angle
  // - Cozmo is stuck on an object and is unable to turn
  void CheckForUnexpectedMovement(const RobotState& robotState);
  
  bool IsUnexpectedMovementDetected() const { return _unexpectedMovement.IsDetected(); }
  
  UnexpectedMovementSide GetUnexpectedMovementSide() const { return IsUnexpectedMovementDetected() ?
                                                                    _unexpectedMovementSide        :
                                                                    UnexpectedMovementSide::UNKNOWN; }
  
  // True if any motor speed (head, left, or wheels) is non-zero in most recent RobotState message
  bool   IsMoving()        const { return _isMoving; }
  
  // True if head/lift/wheels are moving
  bool   IsHeadMoving()    const { return _isHeadMoving; }
  bool   IsLiftMoving()    const { return _isLiftMoving; }
  bool   AreWheelsMoving() const { return _areWheelsMoving; }
  
  // Same as above, but looks up moving state in history, by given time
  // - If we fail finding the state in history, all methods return TRUE (to be conservative)
  // - These are non-const because they can insert things into pose history due to interpolation
  bool   WasMoving(TimeStamp_t atTime); // Reminder: head, lift, or wheels!
  bool   WasHeadMoving(TimeStamp_t atTime);
  bool   WasLiftMoving(TimeStamp_t atTime);
  bool   WereWheelsMoving(TimeStamp_t atTime);
  
  // Convenience methods for checking head OR wheels, since either moves the camera
  bool   IsCameraMoving() const { return _isHeadMoving || _areWheelsMoving; }
  bool   WasCameraMoving(TimeStamp_t atTime); // Slightly more efficient than calling WasHeadMoving _and_ WereWheelsMoving

  uint8_t GetTracksLockedBy(const std::string& who) const;

  // Returns true if any of the tracks are locked
  bool AreAnyTracksLocked(u8 tracks) const;
  // Returns true if all of the specified tracks are locked
  bool AreAllTracksLocked(u8 tracks) const;
  // Returns true if all the specified tracks are locked by 'who'
  bool AreAllTracksLockedBy(u8 tracks, const std::string& who) const;
  
  // The string 'who' indicates who is locking the tracks
  // In order to unlock tracks, the unlock 'who' needs to match the 'who' that did the locking
  void LockTracks(u8 tracks, const std::string& who, const std::string& debugName);
  // Returns true if there are any locks on tracks after unlocking tracks locked by 'who'
  bool UnlockTracks(u8 tracks, const std::string& who);
  
  // Converts int who to a string (used to easily allow actions to lock tracks with their tag)
  void LockTracks(u8 tracks, const int who, const std::string& debugName) { LockTracks(tracks, std::to_string(who), debugName); }
  bool UnlockTracks(u8 tracks, const int who) { return UnlockTracks(tracks, std::to_string(who)); }
  
  // Completely unlocks all tracks to have an lock count of 0 as opposed to UnlockTracks(ALL_TRACKS)
  // which will only decrement each track lock count by 1
  void CompletelyUnlockAllTracks();
  
  // Sends calibrate command to robot
  Result CalibrateMotors(bool head, bool lift);
  
  // Enables lift power on the robot.
  // If disabled, lift goes limp.
  Result EnableLiftPower(bool enable);
  
  // Enables head power on the robot.
  // If disabled, head goes limp.
  Result EnableHeadPower(bool enable);
  
  // Below are low-level actions to tell the robot to do something "now"
  // without using the ActionList system:
  
  // Sends a message to the robot to move the lift to the specified height.
  // When the command is received by the robot, it returns a MotorActionAck
  // message with the ID that can optionally be written to actionID_out.
  Result MoveLiftToHeight(const f32 height_mm,
                          const f32 max_speed_rad_per_sec,
                          const f32 accel_rad_per_sec2,
                          const f32 duration_sec = 0.f,
                          MotorActionID* actionID_out = nullptr);
  
  // Sends a message to the robot to move the head to the specified angle.
  // When the command is received by the robot, it returns a MotorActionAck
  // message with the ID that can optionally be written to actionID_out.
  Result MoveHeadToAngle(const f32 angle_rad,
                         const f32 max_speed_rad_per_sec,
                         const f32 accel_rad_per_sec2,
                         const f32 duration_sec = 0.f,
                         MotorActionID* actionID_out = nullptr);

  // Sends raw wheel commands to robot
  // Generally, you should be using an action instead of calling this directly
  // unless you have a need to run the wheels for an indeterminate amount of time.
  Result DriveWheels(const f32 lWheel_speed_mmps,
                     const f32 rWheel_speed_mmps,
                     const f32 lWheel_accel_mmps2,
                     const f32 rWheel_accel_mmps2);
  
  // Sends a message to the robot to turn in place to the specified angle.
  // When the command is received by the robot, it returns a MotorActionAck
  // message with the ID that can optionally be written to actionID_out.
  Result TurnInPlace(const f32 angle_rad,              // Absolute angle the robot should achieve at the end of the turn.
                     const f32 max_speed_rad_per_sec,  // Maximum speed of the turn (sign determines the direction of the turn)
                     const f32 accel_rad_per_sec2,     // Accel/decel to use during the turn
                     const f32 angle_tolerance,
                     const u16 num_half_revolutions,   // Used for turns greater than 180 degrees (e.g. "turn 720 degrees")
                     bool use_shortest_direction,      // If true, robot should take the shortest path to angle_rad and disregard
                                                       // the sign of max_speed and num_half_revolutions. If false, consider sign
                                                       // of max_speed and num_half_revolutions to determine which direction to
                                                       // turn and how far to turn (e.g. for turns longer than 360 deg)
                     MotorActionID* actionID_out = nullptr);

  
  // Flag the eye shift layer with the given name for removal next time head moves.
  // You may optionally specify the duration of the layer removal (i.e. how
  // long it takes to return to not making any face adjustment)
  void RemoveEyeShiftWhenHeadMoves(const std::string& name, TimeStamp_t duration_ms=0);
  
  Result StopAllMotors();
  Result StopHead();
  Result StopLift();
  Result StopBody();
  
  // Tracking is handled by actions now, but we will continue to maintain the
  // state of what is being tracked in this class.
  const ObjectID& GetTrackToObject() const { return _trackToObjectID; }
  void  SetTrackToObject(ObjectID objectID) { _trackToObjectID = objectID; }
  void  UnSetTrackToObject() { _trackToObjectID.UnSet(); }
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  void PrintLockState() const;
  
  // Returns a string of who is locking each of the specified tracks
  std::string WhoIsLocking(u8 tracks) const;
  
  bool IsDirectDriving() const { return ((_drivingWheels || _drivingHead || _drivingLift)); }
  
  u8 GetMaxUnexpectedMovementCount() const { return UnexpectedMovement::kMaxUnexpectedMovementCount; }

  void SetAllowedToHandleActions(bool allowedToHandleActions);
  
  // Enable/disable detection of rotation without motors. This must be explicitly enabled since it
  // differs from the most common use case of this component.
  void EnableUnexpectedRotationWithoutMotors(bool enabled) { _enableRotatedWithoutMotors = enabled; }
  bool IsUnexpectedRotationWithoutMotorsEnabled() const { return _enableRotatedWithoutMotors; }
  
private:
  
  void InitEventHandlers(IExternalInterface& interface);
  int GetFlagIndex(uint8_t flag) const;
  AnimTrackFlag GetFlagFromIndex(int index);
  
  // Checks if the speed is near zero and if it is sets flag to false and unlocks tracks
  // otherwise it will set flag to true and lock the tracks if they are not locked
  void DirectDriveCheckSpeedAndLockTracks(f32 speed, bool& flag, u8 tracks, const std::string& who,
                                          const std::string& debugName);

  // Return an actionID for the next motor command to be sent to robot.
  // Also writes same actionID to actionID_out if it's non-null.
  MotorActionID GetNextMotorActionID(MotorActionID* actionID_out = nullptr);
  
  Robot* _robot = nullptr;
  
  MotorActionID _lastMotorActionID = 0;
  
  bool _isMoving = false;
  bool _isHeadMoving = false;
  bool _isLiftMoving = false;
  bool _areWheelsMoving = false;
  bool _enableRotatedWithoutMotors = false;

  // TODO Set to false so it is only enabled when BehaviorSDKInterface is activated.
  bool _isAllowedToHandleActions = true;//false;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  // Object being tracked
  ObjectID _trackToObjectID;
  
  //bool _trackWithHeadOnly = false;
  
  struct LockInfo
  {
    std::string who;
    std::string debugName;
    
    inline const bool operator<(const LockInfo& other) const
    {
      return who < other.who;
    }
    
    inline const bool operator==(const LockInfo& other) const
    {
      return who == other.who;
    }
  };
  
  std::array<std::multiset<LockInfo>, (size_t)AnimConstants::NUM_TRACKS> _trackLockCount;
  
  struct EyeShiftToRemove {
    TimeStamp_t duration_ms;
    bool        headWasMoving;
  };
  std::unordered_map<std::string, EyeShiftToRemove> _eyeShiftToRemove;
  
  // Helper class for detecting unexpected movement
  class UnexpectedMovement
  {
    TimeStamp_t  _startTime;
    f32          _sumWheelSpeedL_mmps;
    f32          _sumWheelSpeedR_mmps;
    u8           _count;
    
  public:
    
    UnexpectedMovement() { Reset(); }
    
    static const u8  kMaxUnexpectedMovementCount;
    
    u8          GetCount() const { return _count; }
    TimeStamp_t GetStartTime() const { return _startTime; }
    void        GetAvgWheelSpeeds(f32& left, f32& right) const;
    
    bool IsDetected() const;
    void Increment(u8 countInc, f32 leftSpeed_mmps, f32 rightSpeed_mmps, TimeStamp_t currentTime);
    void Decrement();
    void Reset();
  };
  
  UnexpectedMovement _unexpectedMovement;
  
  UnexpectedMovementSide _unexpectedMovementSide = UnexpectedMovementSide::UNKNOWN;
  
  const f32 kGyroTol_radps                 = DEG_TO_RAD(10);
  const f32 kWheelDifForTurning_mmps       = 30;
  const f32 kMinWheelSpeed_mmps            = 20;
  const f32 kExpectedVsActualGyroTol_radps = 0.2f;
  
  // Flags for whether or not we are currently directly driving the following motors

  bool _drivingWheels     = false;
  bool _drivingHead       = false;
  bool _drivingLift       = false;
  const char* kDrivingWheelsStr  = "DirectDriveWheels";
  const char* kDrivingHeadStr    = "DirectDriveHead";
  const char* kDrivingLiftStr    = "DirectDriveLift";
  const char* kDrivingArcStr     = "DirectDriveArc";
  const char* kDrivingTurnStr    = "DirectDriveTurnInPlace";
  const char* kOnChargerInSdkStr = "OnChargerInSDK";
  
  const u8 kAllMotorTracks = ((u8)AnimTrackFlag::HEAD_TRACK |
                              (u8)AnimTrackFlag::LIFT_TRACK |
                              (u8)AnimTrackFlag::BODY_TRACK);
  
}; // class MovementComponent
  
  
} // namespace Cozmo
} // namespace Anki

#endif //  __Anki_Cozmo_Basestation_Components_MovementComponent_H__
