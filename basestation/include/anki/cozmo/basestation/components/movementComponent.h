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

#include "anki/common/types.h"
#include "anki/common/basestation/objectIDs.h"
#include "anki/vision/basestation/trackedFace.h"
#include "anki/cozmo/basestation/animation/animationStreamer.h"
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
  
class MovementComponent : private Util::noncopyable
{
public:
  MovementComponent(Robot& robot);
  virtual ~MovementComponent() { }
  
  void Update(const RobotState& robotState);
  
  // Checks for unexpected movement specifically while turning such as
  // - Cozmo is turning one direction but you turn him the other way
  // - Cozmo is turning one direction and you turn him faster so he overshoots his turn angle
  // - Cozmo is stuck on an object and is unable to turn
  void CheckForUnexpectedMovement(const RobotState& robotState);
  
  // True if wheel speeds are non-zero in most recent RobotState message
  bool   IsMoving() const {return _isMoving;}
  
  // True if head/lift is on its way to a commanded angle/height
  bool   IsHeadMoving() const {return _isHeadMoving;}
  bool   IsLiftMoving() const {return _isLiftMoving;}
  bool   AreWheelsMoving() const {return _areWheelsMoving;}
  
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
  
  // Enables lift power on the robot.
  // If disabled, lift goes limp.
  Result EnableLiftPower(bool enable);
  
  // Below are low-level actions to tell the robot to do something "now"
  // without using the ActionList system:
  
  // Sends a message to the robot to move the lift to the specified height
  Result MoveLiftToHeight(const f32 height_mm,
                          const f32 max_speed_rad_per_sec,
                          const f32 accel_rad_per_sec2,
                          const f32 duration_sec = 0.f);
  
  // Sends a message to the robot to move the head to the specified angle
  Result MoveHeadToAngle(const f32 angle_rad,
                         const f32 max_speed_rad_per_sec,
                         const f32 accel_rad_per_sec2,
                         const f32 duration_sec = 0.f);
  
  // Register a persistent face layer tag for removal next time head moves
  // You may optionally specify the duration of the layer removal (i.e. how
  // long it takes to return to not making any face adjustment)
  void RemoveFaceLayerWhenHeadMoves(AnimationStreamer::Tag faceLayerTag, TimeStamp_t duration_ms=0);
  
  Result StopAllMotors();
  
  // Tracking is handled by actions now, but we will continue to maintain the
  // state of what is being tracked in this class.
  const ObjectID& GetTrackToObject() const { return _trackToObjectID; }
  const Vision::FaceID_t GetTrackToFace() const { return _trackToFaceID; }
  void  SetTrackToObject(ObjectID objectID) { _trackToObjectID = objectID; }
  void  SetTrackToFace(Vision::FaceID_t faceID) { _trackToFaceID = faceID; }
  void  UnSetTrackToObject() { _trackToObjectID.UnSet(); }
  void  UnSetTrackToFace()   { _trackToFaceID = Vision::UnknownFaceID; }
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  void PrintLockState() const;
  
  // Returns a string of who is locking each of the specified tracks
  std::string WhoIsLocking(u8 tracks) const;
  
  void IgnoreDirectDriveMessages(bool ignore) { _ignoreDirectDrive = ignore; }
  
  bool IsDirectDriving() const { return ((_drivingWheels || _drivingHead || _drivingLift) && !_ignoreDirectDrive); }
  
private:
  
  void InitEventHandlers(IExternalInterface& interface);
  int GetFlagIndex(uint8_t flag) const;
  AnimTrackFlag GetFlagFromIndex(int index);
  
  // Checks if the speed is near zero and if it is sets flag to false and unlocks tracks
  // otherwise it will set flag to true and lock the tracks if they are not locked
  void DirectDriveCheckSpeedAndLockTracks(f32 speed, bool& flag, u8 tracks, const std::string& who,
                                          const std::string& debugName);
  
  Robot& _robot;
  
  bool _isMoving;
  bool _isHeadMoving;
  bool _isLiftMoving;
  bool _areWheelsMoving;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  // Object/Face being tracked
  ObjectID _trackToObjectID;
  Vision::FaceID_t _trackToFaceID = Vision::UnknownFaceID;
  
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
  
  struct FaceLayerToRemove {
    TimeStamp_t duration_ms;
    bool        headWasMoving;
  };
  std::map<AnimationStreamer::Tag, FaceLayerToRemove> _faceLayerTagsToRemoveOnHeadMovement;
  
  // Helper class for detecting unexpected movement
  class UnexpectedMovement
  {
    TimeStamp_t  _startTime;
    f32          _sumWheelSpeedL_mmps;
    f32          _sumWheelSpeedR_mmps;
    u8           _count;
    
  public:
    
    UnexpectedMovement() { Reset(); }
    
    u8          GetCount() const { return _count; }
    TimeStamp_t GetStartTime() const { return _startTime; }
    void        GetAvgWheelSpeeds(f32& left, f32& right) const;
    
    void Increment(u8 countInc, f32 leftSpeed_mmps, f32 rightSpeed_mmps, TimeStamp_t currentTime);
    void Decrement();
    void Reset();
  };
  
  UnexpectedMovement _unexpectedMovement;
  
  const f32 kGyroTol_radps                 = DEG_TO_RAD(10);
  const f32 kWheelDifForTurning_mmps       = 30;
  const u8  kMaxUnexpectedMovementCount    = 10;
  const f32 kMinWheelSpeed_mmps            = 20;
  const f32 kExpectedVsActualGyroTol_radps = 0.2;
  
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
  bool _ignoreDirectDrive = false;
  
  const u8 kAllMotorTracks = ((u8)AnimTrackFlag::HEAD_TRACK |
                              (u8)AnimTrackFlag::LIFT_TRACK |
                              (u8)AnimTrackFlag::BODY_TRACK);
  
}; // class MovementComponent
  
  
} // namespace Cozmo
} // namespace Anki

#endif //  __Anki_Cozmo_Basestation_Components_MovementComponent_H__
