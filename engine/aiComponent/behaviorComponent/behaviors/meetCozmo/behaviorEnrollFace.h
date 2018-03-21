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
#include "coretech/vision/engine/faceIdTypes.h"

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
    
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorEnrollFace(const Json::Value& config);
    
public:  
  // Is activatable when FaceWorld has enrollment settings set
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ICozmoBehavior API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated()   override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated()   override;

  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  virtual void HandleWhileActivated(const GameToEngineEvent& event) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void HandleWhileInScopeButNotActivated(const GameToEngineEvent& event) override;
  
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
    SayingIKnowThatName,
    EmotingConfusion,
    SavingToRobot,
    TimedOut,
    ScanningInterrupted,
    SaveFailed,
    Failed_WrongFace,
    Failed_UnknownReason,
    Failed_NameInUse,
    Cancelled,
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  Result InitEnrollmentSettings();
  
  void TransitionToLookingForFace();
  void TransitionToEnrolling();
  void TransitionToScanningInterrupted();
  void TransitionToSayingName();
  void TransitionToSayingIKnowThatName();
  void TransitionToSavingToRobot();
  // catch all for "confusion" animations that should play before transitioning to the given state
  void TransitionToFailedState( State state, const std::string& stateName);
  
  void UpdateFaceToEnroll();
  void UpdateFaceIDandTime(const Face* newFace);
  
  IActionRunner* CreateTurnTowardsFaceAction(FaceID_t faceID, FaceID_t saveID, bool playScanningGetOut);
  IActionRunner* CreateLookAroundAction();

  bool HasTimedOut() const;
  bool IsSeeingTooManyFaces(FaceWorld& faceWorld, const TimeStamp_t lastImgTime);
  
  // Helper which returns false if the robot is not on its treads or a cliff is being detected
  bool CanMoveTreads() const;
  
  bool IsEnrollmentRequested() const;
  void DisableEnrollment();
  
  // helper to see if a user intent was left in the user intent component for us by a parent behavior
  void CheckForIntentData() const;
  
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
  
  State _failedState;
  
}; // class BehaviorEnrollFace
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEnrollFace_H__
