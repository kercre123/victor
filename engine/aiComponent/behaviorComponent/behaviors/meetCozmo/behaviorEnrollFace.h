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
#include "util/cladHelpers/cladFromJSONHelpers.h"
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
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
  virtual void InitBehavior() override;
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
    
    NotStarted,
    
    // contains both states and failure cases
    DriveOffCharger,
    PutDownBlock,
    LookingForFace,
    Enrolling,
    SayingName,
    Success,
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
  
  void TransitionToPutDownBlock();
  void TransitionToDriveOffCharger();
  void TransitionToLookingForFace();
  void TransitionToEnrolling();
  void TransitionToScanningInterrupted();
  void TransitionToSayingName();
  void TransitionToSayingIKnowThatName();
  void TransitionToSavingToRobot();
  void TransitionToWrongFace( const std::string& faceName );
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
  
  // true if the robot was physically turned or picked up by the user recently (persistent)
  bool WasMovedRecently() const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Members
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct InstanceConfig {
    InstanceConfig();
    
    s32              maxFacesVisible;
    f32              tooManyFacesRecentTime_sec;
    f32              tooManyFacesTimeout_sec;
    f32              timeout_sec;
    
    ICozmoBehaviorPtr driveOffChargerBehavior;
    ICozmoBehaviorPtr putDownBlockBehavior;
  };
  
  struct DynamicVariables {
    DynamicVariables();
    
    struct Persistent {
      State          state = State::NotStarted;
      bool           didEverLeaveCharger = false;
      TimeStamp_t    lastTimeUserMovedRobot = 0;
      TimeStamp_t    lastDeactivationTime_ms = 0;
      
      using EnrollmentSettings = ExternalInterface::SetFaceToEnroll;
      std::unique_ptr<EnrollmentSettings> settings;
      
      int numInterruptions = 0;
    };
    Persistent       persistent;
    
    bool             sayName;
    bool             useMusic;
    bool             saveToRobot;
    bool             saveSucceeded;
    bool             enrollingSpecificID;
    FaceID_t         faceID;
    FaceID_t         saveID;
    FaceID_t         observedUnusableID;
    
    TimeStamp_t      lastFaceSeenTime_ms;
    
    TimeStamp_t      timeScanningStarted_ms;
    TimeStamp_t      timeStartedLookingForFace_ms;
    
    f32 timeout_sec;
    
    bool wasUnexpectedRotationWithoutMotorsEnabled;
    
    f32              startedSeeingMultipleFaces_sec;
    f32              startTime_sec;
    
    f32              totalBackup_mm;
    
    
    
    ActionResult     saveEnrollResult;
    ActionResult     saveAlbumResult;
    
    std::string      faceName;
    std::string      observedUnusableName;
    
    Radians          lastRelBodyAngle;
    
    std::vector<std::pair<std::string, unsigned int>> knownFaceCounts;
    
    std::set<Vision::FaceID_t> facesSeen;
    std::unordered_map<Vision::FaceID_t, bool> isFaceNamed;
    
    State            failedState;
  };
  
  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
}; // class BehaviorEnrollFace
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEnrollFace_H__
