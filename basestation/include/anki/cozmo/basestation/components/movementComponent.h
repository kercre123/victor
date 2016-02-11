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
#include "util/signals/simpleSignal.hpp"
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
  
  // True if wheel speeds are non-zero in most recent RobotState message
  bool   IsMoving() const {return _isMoving;}
  
  // True if head/lift is on its way to a commanded angle/height
  bool   IsHeadMoving() const {return _isHeadMoving;}
  bool   IsLiftMoving() const {return _isLiftMoving;}
  
  // Returns true if any of the tracks are locked
  bool AreAnyTracksLocked(u8 tracks) const;
  // Returns true if all of the specified tracks are locked
  bool AreAllTracksLocked(u8 tracks) const;
  
  void LockTracks(u8 tracks);
  void UnlockTracks(u8 tracks);
  
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
  const Vision::TrackedFace::ID_t GetTrackToFace() const { return _trackToFaceID; }
  void  SetTrackToObject(ObjectID objectID) { _trackToObjectID = objectID; }
  void  SetTrackToFace(Vision::TrackedFace::ID_t faceID) { _trackToFaceID = faceID; }
  void  UnSetTrackToObject() { _trackToObjectID.UnSet(); }
  void  UnSetTrackToFace()   { _trackToFaceID = Vision::TrackedFace::UnknownFace; }
  
  template<typename T>
  void HandleMessage(const T& msg);
  
  void PrintLockState() const;
  
private:
  
  void InitEventHandlers(IExternalInterface& interface);
  int GetFlagIndex(uint8_t flag) const;
  
  Robot& _robot;
  
  bool _isMoving;
  bool _isHeadMoving;
  bool _isLiftMoving;
  
  std::list<Signal::SmartHandle> _eventHandles;
  
  // Object/Face being tracked
  ObjectID _trackToObjectID;
  Vision::TrackedFace::ID_t _trackToFaceID = Vision::TrackedFace::UnknownFace;
  
  //bool _trackWithHeadOnly = false;

  
  
  std::array<int, (size_t)AnimConstants::NUM_TRACKS> _trackLockCount;
  
  struct FaceLayerToRemove {
    TimeStamp_t duration_ms;
    bool        headWasMoving;
  };
  std::map<AnimationStreamer::Tag, FaceLayerToRemove> _faceLayerTagsToRemoveOnHeadMovement;
  
}; // class MovementComponent
  
  
} // namespace Cozmo
} // namespace Anki

#endif //  __Anki_Cozmo_Basestation_Components_MovementComponent_H__
