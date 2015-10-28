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
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal.hpp"
#include <list>

namespace Anki {
namespace Cozmo {
  
// declarations
class Robot;
class IExternalInterface;
  
class MovementComponent : private Util::noncopyable
{
public:
  MovementComponent(Robot& robot);
  virtual ~MovementComponent() { }
  
  // These are methods to lock/unlock subsystems of the robot to prevent
  // MoveHead/MoveLift/DriveWheels/etc commands from having any effect.
  void LockHead(bool tf) { _headLocked = tf; }
  void LockLift(bool tf) { _liftLocked = tf; }
  void LockWheels(bool tf) { _wheelsLocked = tf; }
  
  bool IsHeadLocked() const { return _headLocked; }
  bool IsLiftLocked() const { return _liftLocked; }
  bool AreWheelsLocked() const { return _wheelsLocked; }
  
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
  
  Result StopAllMotors();
  
  // If robot observes the given object ID, it will tilt its head and rotate its
  // body to keep looking at the last-observed marker. Fails if objectID doesn't exist.
  // If "headOnly" is true, then body rotation is not performed.
  Result EnableTrackToObject(const u32 objectID, bool headOnly);
  Result DisableTrackToObject();
  
  
  Result EnableTrackToFace(const Vision::TrackedFace::ID_t, bool headOnly);
  Result DisableTrackToFace();
  const ObjectID& GetTrackToObject() const { return _trackToObjectID; }
  const Vision::TrackedFace::ID_t GetTrackToFace() const { return _trackToFaceID; }
  bool  IsTrackingWithHeadOnly() const { return _trackWithHeadOnly; }
  
protected:
  Robot& _robot;
  std::list<Signal::SmartHandle> _eventHandles;

  bool _wheelsLocked = false;
  bool _headLocked = false;
  bool _liftLocked = false;
  
  // Object/Face to track head to whenever it is observed
  ObjectID _trackToObjectID;
  Vision::TrackedFace::ID_t   _trackToFaceID = Vision::TrackedFace::UnknownFace;
  bool _trackWithHeadOnly = false;
  bool _headLockedBeforeTracking = false;
  bool _wheelsLockedBeforeTracking = false;
  
private:
  void InitEventHandlers(IExternalInterface& interface);
  
}; // class MovementComponent

} // namespace Cozmo
} // namespace Anki

#endif //  __Anki_Cozmo_Basestation_Components_MovementComponent_H__