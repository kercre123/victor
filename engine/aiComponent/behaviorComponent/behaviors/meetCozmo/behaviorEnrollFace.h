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

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/faceEnrollmentResult.h"

#include <string>

namespace Anki {

namespace Vision {
class TrackedFace;
}

namespace Vector {

// Forward declaration
class BehaviorTextToSpeechLoop;
class FaceWorld;
  
namespace ExternalInterface {
  struct SetFaceToEnroll;
}

  
class BehaviorEnrollFace : public ICozmoBehavior
{
protected:
    
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorEnrollFace(const Json::Value& config);
    
public:  
  // Is activatable when FaceWorld has enrollment settings set
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual ~BehaviorEnrollFace();
  
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
    WaitingInPlaceForFace,
    LookingForFace,
    AlreadyKnowYouPrompt,
    AlreadyKnowYouHandle,
    Enrolling,
    SayingName,
    Success,
    SayingIKnowThatName,
    SayingWrongName,
    EmotingConfusion,
    SavingToRobot,
    TimedOut,
    ScanningInterrupted,
    SaveFailed,
    Failed_WrongFace,
    Failed_UnknownReason,
    Failed_NameInUse,
    Failed_NamedStorageFull,
    Cancelled,
  };
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Methods
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  Result InitEnrollmentSettings();
  
  void TransitionToPutDownBlock();
  void TransitionToDriveOffCharger();
  void TransitionToWaitInPlaceForFace();
  void TransitionToLookingForFace();
  void TransitionToAlreadyKnowYouPrompt();
  void TransitionToAlreadyKnowYouHandler();
  void TransitionToStartEnrollment();
  void TransitionToEnrolling();
  void TransitionToScanningInterrupted();
  void TransitionToSayingName();
  void TransitionToSayingIKnowThatName();
  void TransitionToSavingToRobot();
  // depending on settings, either fail or animate/speak to indicate recognition of
  // a face with a different name than the one being enrolled
  void TransitionToWrongFace(FaceID_t faceID, const std::string& faceName );
  // catch all for "confusion" animations that should play before transitioning to the given state
  void TransitionToFailedState( State state, const std::string& stateName);
  
  void UpdateFaceToEnroll();
  void UpdateFaceTime(const Face* newFace);
  void UpdateFaceIDandTime(const Face* newFace);
  
  IActionRunner* CreateTurnTowardsFaceAction(FaceID_t faceID, FaceID_t saveID, bool playScanningGetOut);
  IActionRunner* CreateLookAroundAction();

  bool HasTimedOut() const;
  bool IsSeeingTooManyFaces(FaceWorld& faceWorld, const RobotTimeStamp_t lastImgTime);
  bool IsSeeingWrongFace(FaceID_t& wrongFaceID, std::string& wrongName) const;
  
  // Helper which returns false if the robot is not on its treads or a cliff is being detected
  bool CanMoveTreads() const;
  
  bool IsEnrollmentRequested() const;
  void DisableEnrollment(); // Completely disable, before stopping the behavior
  void ResetEnrollment();   // Reset to try enrollment again, e.g. before returning to LookingForFace
  
  // helper to see if a user intent was left in the user intent component for us by a parent behavior
  void CheckForIntentData() const;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Members
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  struct InstanceConfig;
  struct DynamicVariables;
  
  std::unique_ptr<InstanceConfig>   _iConfig;
  std::unique_ptr<DynamicVariables> _dVars;
  
}; // class BehaviorEnrollFace
  
} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorEnrollFace_H__
