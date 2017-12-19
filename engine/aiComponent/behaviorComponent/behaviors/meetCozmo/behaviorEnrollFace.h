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

#include "engine/ankiEventUtil.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/events/animationTriggerHelpers.h"
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

// Forward declaration
class FaceWorld;

  
class BehaviorEnrollFace : public ICozmoBehavior
{
protected:
    
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorEnrollFace(const Json::Value& config);
    
public:

  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
  // Is activatable when FaceWorld has enrollment settings set
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)   override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)   override;

  virtual void AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileActivated(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileInScopeButNotActivated(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using Face = Vision::TrackedFace;
  using FaceID_t = Vision::FaceID_t;

  enum class State : uint8_t {
    Success,
    
    // All failure states:
    NotStarted,
    LookingForFace,
    Enrolling,
    SayingName,
    SavingToRobot,
    TimedOut,
    ScanningInterrupted,
    SaveFailed,
    Failed_WrongFace,
    Failed_UnknownReason,
    Cancelled,
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  Result InitEnrollmentSettings(BehaviorExternalInterface& behaviorExternalInterface);
  
  void TransitionToLookingForFace(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToEnrolling(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToScanningInterrupted(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToSayingName(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToSavingToRobot(BehaviorExternalInterface& behaviorExternalInterface);
  
  void UpdateFaceToEnroll(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateFaceIDandTime(const Face* newFace);
  
  IActionRunner* CreateTurnTowardsFaceAction(BehaviorExternalInterface& behaviorExternalInterface, FaceID_t faceID, FaceID_t saveID, bool playScanningGetOut);
  IActionRunner* CreateLookAroundAction(BehaviorExternalInterface& behaviorExternalInterface);

  bool HasTimedOut() const;
  bool IsSeeingTooManyFaces(FaceWorld& faceWorld, const TimeStamp_t lastImgTime);
  
  // Helper which returns false if the robot is not on its treads or a cliff is being detected
  bool CanMoveTreads(BehaviorExternalInterface& behaviorExternalInterface) const;
  
  bool IsEnrollmentRequested() const;
  void DisableEnrollment(BehaviorExternalInterface& behaviorExternalInterface);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Members
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  State                     _state = State::NotStarted;
  
  FaceID_t                  _faceID;
  FaceID_t                  _saveID;
  FaceID_t                  _observedUnusableID;
  
  TimeStamp_t               _lastFaceSeenTime_ms;
  
  s32                       _maxFacesVisible;
  f32                       _tooManyFacesRecentTime_sec;
  f32                       _tooManyFacesTimeout_sec;
  f32                       _startedSeeingMultipleFaces_sec;
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
