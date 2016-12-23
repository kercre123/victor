/**
 * File: behaviorInteractWithFaces.h
 *
 * Author: Andrew Stein
 * Created: 2016-11-22
 *
 * Description: Enroll a new face with a name or re-enroll an existing face.
 *              
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorEnrollFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorEnrollFace_H__

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/vision/basestation/faceIdTypes.h"

#include "clad/types/faceEnrollmentResult.h"

#include <string>

namespace Anki {

namespace Vision {
class TrackedFace;
}
  
namespace ExternalInterface {
  struct SetFaceToEnroll;
}

namespace Cozmo {

class BehaviorEnrollFace : public IBehavior
{
protected:
    
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorEnrollFace(Robot& robot, const Json::Value& config);
    
public:

  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
  // Is runnable when FaceWorld has enrollment settings set
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // IBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  virtual Result InitInternal(Robot& robot)   override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot)   override;

  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void AlwaysHandle(const GameToEngineEvent& event, const Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  
private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using Face = Vision::TrackedFace;
  using FaceID_t = Vision::FaceID_t;

  enum class State : uint8_t {
    NotStarted,
    LookingForFace,
    Enrolling,
    SayingName,
    SavingToRobot,
    TimedOut,
    ScanningInterrupted,
    SaveFailed,
    SuccessNoSave,
    SuccessWithSave,
    Failed_WrongFace,
    Failed_UnknownReason
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  Result InitEnrollmentSettings(Robot& robot);
  
  void TransitionToLookingForFace(Robot& robot);
  void TransitionToEnrolling(Robot& robot);
  void TransitionToScanningInterrupted(Robot& robot);
  void TransitionToSayingName(Robot& robot);
  void TransitionToSavingToRobot(Robot& robot);
  
  void UpdateFaceToEnroll(const Robot& robot);
  void UpdateFaceIDandTime(const Face* newFace);
  
  IActionRunner* CreateTurnTowardsFaceAction(Robot& robot, FaceID_t faceID, FaceID_t saveID, bool playScanningGetOut);
  IActionRunner* CreateLookAroundAction(Robot& robot);

  bool HasTimedOut() const;
  
  bool IsEnrollmentRequested() const;
  void DisableEnrollment();
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Members
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  State                     _state = State::NotStarted;
  
  FaceID_t                  _faceID;
  FaceID_t                  _saveID;
  FaceID_t                  _observedUnusableID;
  
  TimeStamp_t               _lastFaceSeenTime_ms;
  
  f32                       _startTime_sec;
  f32                       _timeout_sec;
  f32                       _totalBackup_mm;
  
  bool                      _sayName;
  bool                      _useMusic;
  bool                      _saveToRobot;
  bool                      _saveSucceeded;
  bool                      _enrollingSpecificID;
  
  ActionResult              _saveEnrollResult;
  ActionResult              _saveAlbumResult;
  
  std::string               _faceName;
  std::string               _observedUnusableName;
  
  Radians                   _lastRelBodyAngle;
  
  using EnrollmentSettings = ExternalInterface::SetFaceToEnroll;
  std::unique_ptr<EnrollmentSettings> _settings;
  
}; // class BehaviorEnrollFace
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEnrollFace_H__
