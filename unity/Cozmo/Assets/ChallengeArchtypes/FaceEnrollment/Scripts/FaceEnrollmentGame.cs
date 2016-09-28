using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo.ExternalInterface;

// TODO: refactor into state machines...
namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    [SerializeField]
    private FaceEnrollmentListSlide _FaceListSlidePrefab;
    private FaceEnrollmentListSlide _FaceListSlideInstance;

    [SerializeField]
    private EnterNameSlide _EnterNameSlidePrefab;
    private EnterNameSlide _EnterNameSlideInstance;

    [SerializeField]
    private FaceEnrollmentInstructionsView _FaceEnrollmentInstructionsViewPrefab;
    private FaceEnrollmentInstructionsView _FaceEnrollmentInstructionsViewInstance;

    // used by press demo to skip saving to actual robot.
    private bool _SaveToRobot = true;

    private string _NameForFace;

    private int _FaceIDToEdit;
    private string _FaceOldNameEdit;
    private int _ReEnrollFaceID = 0;

    private int _FixedFaceID = -1;

    private bool _UseFixedFaceID = false;

    private long _UpdateThresholdLastSeenSeconds;
    private long _UpdateThresholdLastEnrolledSeconds;

    private bool _UserCancelledEnrollment = false;
    private bool _EnrollingFace = false;
    private bool _ShowDoneShelf = false;

    private Cozmo.UI.AlertView _ErrorAlertView = null;
    private Cozmo.UI.AlertView _DeleteConfirmationAlertView = null;

    // selects if we should save to the actual robot or only keep faces
    // for this session. used by press demo to not save to the actual robot.
    public void SetSaveToRobot(bool saveToRobot) {
      _SaveToRobot = saveToRobot;
    }

    public void SetFixedFaceID(int id) {
      _FixedFaceID = id;
      _UseFixedFaceID = true;
    }

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(HandleEnrolledFace);
      CurrentRobot.OnEnrolledFaceRemoved += HandleEraseEnrolledFace;

      FaceEnrollmentGameConfig faceEnrollmentConfig = (FaceEnrollmentGameConfig)minigameConfig;

      _UpdateThresholdLastSeenSeconds = faceEnrollmentConfig.UpdateThresholdLastSeenSeconds;
      _UpdateThresholdLastEnrolledSeconds = faceEnrollmentConfig.UpdateThresholdLastEnrolledSeconds;
    }

    protected override void AddDisabledReactionaryBehaviors() {
      // meet cozmo has special logic for this stuff
    }

    private void SetReactionaryBehaviors(bool enable) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.AcknowledgeFace, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.AcknowledgeObject, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToCubeMoved, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToCliff, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToPickup, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.ReactToFrustration, enable);
        RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionaryBehavior(_kReactionaryBehaviorOwnerId, Anki.Cozmo.BehaviorType.KnockOverCubes, enable);
      }
    }

    protected override void SetupViewAfterCozmoReady(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.SetupViewAfterCozmoReady(newView, data);
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        EnterNameForNewFace(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
      }
      else {
        ShowFaceListSlide(newView);
      }

    }

    private void HandleChangedObservedFaceID(Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID message) {
      if (_FixedFaceID != -1) {
        if (_FixedFaceID == message.oldID) {
          _FixedFaceID = message.newID;
        }
      }
    }

    private void ShowFaceListSlide(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _FaceListSlideInstance = newView.ShowWideGameStateSlide(_FaceListSlidePrefab.gameObject, "face_list_slide").GetComponent<FaceEnrollmentListSlide>();
      _FaceListSlideInstance.Initialize(CurrentRobot.EnrolledFaces, _UpdateThresholdLastSeenSeconds, _UpdateThresholdLastEnrolledSeconds);
      ShowShelf(newView);
      _FaceListSlideInstance.OnEnrollNewFaceRequested += EnterNameForNewFace;
      _FaceListSlideInstance.OnEditNameRequested += EditExistingName;
      _FaceListSlideInstance.OnDeleteEnrolledFace += RequestDeleteEnrolledFace;
      _FaceListSlideInstance.OnReEnrollFaceRequested += RequestReEnrollFace;
      newView.ShowQuitButton();
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.MeetCozmoFindFaces);
      SetReactionaryBehaviors(true);
    }

    private void CleanupFaceListSlide() {
      if (_FaceListSlideInstance != null) {
        _FaceListSlideInstance.OnEnrollNewFaceRequested -= EnterNameForNewFace;
        _FaceListSlideInstance.OnEditNameRequested -= EditExistingName;
        _FaceListSlideInstance.OnDeleteEnrolledFace -= RequestDeleteEnrolledFace;
        _FaceListSlideInstance.OnReEnrollFaceRequested -= RequestReEnrollFace;
      }

      SharedMinigameView.HideShelf();
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      SetReactionaryBehaviors(false);
    }

    private void EditExistingName(int faceID, string existingName) {
      _EnterNameSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "edit_name", EditNameInputSlideInDone).GetComponent<EnterNameSlide>();
      _EnterNameSlideInstance.SetNameInputField(existingName);
      _FaceIDToEdit = faceID;
      _FaceOldNameEdit = existingName;

      SharedMinigameView.ShowBackButton(() => {
        SharedMinigameView.ShowQuitButton();
        ShowFaceListSlide(SharedMinigameView);
      });
      CleanupFaceListSlide();
    }

    private void EnterNameForNewFace(string preFilledName) {
      _EnterNameSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "enter_new_name", NewNameInputSlideInDone).GetComponent<EnterNameSlide>();

      if (string.IsNullOrEmpty(preFilledName) == false) {
        _EnterNameSlideInstance.SetNameInputField(preFilledName);
      }

      SharedMinigameView.ShowBackButton(() => {
        ShowFaceListSlide(SharedMinigameView);
      });
      CleanupFaceListSlide();
    }

    private void NewNameInputSlideInDone() {
      _EnterNameSlideInstance.RegisterInputFocus();
      _EnterNameSlideInstance.OnNameEntered += HandleNewNameEntered;
    }

    private void EditNameInputSlideInDone() {
      _EnterNameSlideInstance.RegisterInputFocus();
      _EnterNameSlideInstance.OnNameEntered += HandleUpdatedNameEntered;
    }

    private void RequestReEnrollFace(int faceId, string faceName) {
      CleanupFaceListSlide();
      _ReEnrollFaceID = faceId;
      _NameForFace = faceName;

      ShowInstructions(() => {
        ShowFaceListSlide(SharedMinigameView);
      }, faceName);
    }

    private void HandleNewNameEntered(string faceName) {
      _NameForFace = faceName;
      _EnterNameSlideInstance.OnNameEntered -= HandleNewNameEntered;

      ShowInstructions(() => {
        EnterNameForNewFace(faceName);
      }, faceName);
    }

    private void ShowInstructions(System.Action handleInstructionsDone, string faceName) {
      SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentInstructionsViewInstance = UIManager.OpenView(_FaceEnrollmentInstructionsViewPrefab);
      _FaceEnrollmentInstructionsViewInstance.ViewClosedByUser += () => {
        if (_EnrollingFace) {
          CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.ENROLL_NAMED_FACE);
          _UserCancelledEnrollment = true;
          if (handleInstructionsDone != null) {
            handleInstructionsDone();
          }
        }
        else {
          // not enrolling face probably means we are playing say name animations, let's
          // clear the queue and don't listen to any callback responses
          CurrentRobot.CancelAllCallbacks();
          CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.UNKNOWN);
          ShowFaceListSlide(SharedMinigameView);
        }

      };
      if (string.IsNullOrEmpty(faceName) == false) {
        _FaceEnrollmentInstructionsViewInstance.SetFaceName(faceName);
      }
      HandleInstructionsSlideEntered();
    }

    private void HandleUpdatedNameEntered(string newName) {
      _EnterNameSlideInstance.OnNameEntered -= HandleUpdatedNameEntered;
      CurrentRobot.OnEnrolledFaceRenamed += HandleRobotRenamedEnrolledFace;
      CurrentRobot.UpdateEnrolledFaceByID(_FaceIDToEdit, _FaceOldNameEdit, newName);
    }

    private void HandleRobotRenamedEnrolledFace(int faceId, string faceName) {
      CurrentRobot.OnEnrolledFaceRenamed -= HandleRobotRenamedEnrolledFace;
      SharedMinigameView.HideBackButton();
      CurrentRobot.SayTextWithEvent(faceName, Anki.Cozmo.AnimationTrigger.MeetCozmoRenameFaceSayName, callback: (success) => {
        EditOrEnrollFaceComplete(true);
      });
    }

    private void HandleInstructionsSlideEntered() {
      CurrentRobot.SetEnableCliffSensor(false);
      if (_UseFixedFaceID) {
        CurrentRobot.EnrollNamedFace(_FixedFaceID, _ReEnrollFaceID, _NameForFace, saveToRobot: _SaveToRobot);
      }
      else {
        CurrentRobot.EnrollNamedFace(0, _ReEnrollFaceID, _NameForFace, saveToRobot: _SaveToRobot);
      }
      _EnrollingFace = true;
    }

    private void HandleEnrolledFace(Anki.Cozmo.ExternalInterface.RobotCompletedAction message) {
      // ignore all messages except ENROLL_NAMED_FACE
      if (message.actionType != Anki.Cozmo.RobotActionType.ENROLL_NAMED_FACE) {
        return;
      }

      _EnrollingFace = false;
      CurrentRobot.SetEnableCliffSensor(true);
      // dont show errors if we user cancels enrollment.
      if (_UserCancelledEnrollment) {
        // reset _ReEnrollFaceID
        _ReEnrollFaceID = 0;
        _UserCancelledEnrollment = false;
        return;
      }

      // cancelled for some other reason (eg. interrupt by picking up / place on back).
      // we also include FAILURE_NOT_STARTED because that can be triggered if the action was interrupted
      // before we even start the action (but it is queued).
      if (message.result == Anki.Cozmo.ActionResult.CANCELLED || message.result == Anki.Cozmo.ActionResult.FAILURE_NOT_STARTED) {
        // start listening for when the reactionary behavior is done so we can try again
        RobotEngineManager.Instance.AddCallback<ReactionaryBehaviorTransition>(RetryFaceEnrollmentOnReactionaryBehaviorEnd);
        if (message.completionInfo.faceEnrollmentCompleted.isFaceScanning) {
          HandleSetIdleAndGetOutAnim();
        }
        return;
      }

      if (message.result != Anki.Cozmo.ActionResult.SUCCESS) {

        // we didn't succeed so let's show an error to the user saying why
        Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab);

        if (message.completionInfo.faceEnrollmentCompleted.neverSawValidFace) {
          alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceTitle;
          alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceDescription;
          alertView.SetMessageArgs(new object[] { message.completionInfo.faceEnrollmentCompleted.name });
          alertView.SetPrimaryButton(LocalizationKeys.kButtonContinue);
        }
        else {
          alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentErrorsTimeOutTitle;
          alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentErrorsTimeOutDescription;

          bool reEnrolling = _ReEnrollFaceID != 0;
          int reEnrollingID = _ReEnrollFaceID;

          alertView.SetPrimaryButton(LocalizationKeys.kButtonRetry, () => {
            CleanupFaceListSlide();

            if (reEnrolling) {
              RequestReEnrollFace(reEnrollingID, _NameForFace);
            }
            else {
              HandleNewNameEntered(_NameForFace);
            }
          });
          alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);
          _ErrorAlertView = alertView;
        }

        if (message.completionInfo.faceEnrollmentCompleted.isFaceScanning) {
          HandleSetIdleAndGetOutAnim();
        }
      }
      else {
        // log success to das
        DAS.Event("robot.face_enrollment", message.completionInfo.faceEnrollmentCompleted.faceID.ToString());

        // since we succeeded we have to reset the idle
        CurrentRobot.PopIdleAnimation();

        if (CurrentRobot.EnrolledFaces.ContainsKey(message.completionInfo.faceEnrollmentCompleted.faceID)) {
          DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Re-enrolled existing face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(_NameForFace));
          CurrentRobot.EnrolledFaces[message.completionInfo.faceEnrollmentCompleted.faceID] = _NameForFace;
          CurrentRobot.EnrolledFacesLastEnrolledTime[message.completionInfo.faceEnrollmentCompleted.faceID] = Time.time;
          ReEnrolledExistingFaceAnimationSequence();
        }
        else {
          CurrentRobot.EnrolledFaces.Add(message.completionInfo.faceEnrollmentCompleted.faceID, _NameForFace);
          CurrentRobot.EnrolledFacesLastEnrolledTime.Add(message.completionInfo.faceEnrollmentCompleted.faceID, 0);
          CurrentRobot.EnrolledFacesLastSeenTime.Add(message.completionInfo.faceEnrollmentCompleted.faceID, 0);
          GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnMeetNewPerson);
          DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Enrolled new face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(_NameForFace));
          EnrolledNewFaceAnimationSequence();

          // log using up another face slot to das
          DAS.Event("robot.face_slots_used", CurrentRobot.EnrolledFaces.Count.ToString(),
            DASUtil.FormatExtraData("1"));
        }
      }

      // reset fixed face ID constraint after doing an enrollment
      // this handles the edge case of if a fixed ID was set at the start
      // of the activity but we want to enroll multiple faces in the same session
      _FixedFaceID = -1;

      // reset _ReEnrollFaceID
      _ReEnrollFaceID = 0;

      if (message.result != Anki.Cozmo.ActionResult.SUCCESS) {
        // we failed so lets go straight back to the face list screen
        EditOrEnrollFaceComplete(false);
        ContextManager.Instance.AppFlash(playChime: true);
        UIManager.CloseView(_FaceEnrollmentInstructionsViewInstance);
      }
    }

    private void HandleSetIdleAndGetOutAnim() {
      CurrentRobot.PopIdleAnimation();
      CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.MeetCozmoLookFaceGetOut);
    }

    private void EnrolledNewFaceAnimationSequence() {
      // play first time say name specific music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Meet_Cozmo_Say_Name);

      RobotActionUnion[] actions = {
        // 0. get out animation
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoLookFaceGetOut, true)),
        // 1. say name once
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentSayName,
          Anki.Cozmo.SayTextIntent.Name_FirstIntroduction_1)),
        // 2. repeat name                      
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentRepeatName,
          Anki.Cozmo.SayTextIntent.Name_FirstIntroduction_2)),
        // 3. final celebration (no name said)                
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration, true))
      };

      CurrentRobot.SendQueueCompoundAction(actions, HandleEnrollFaceAnimationSequenceComplete);

    }

    private void ReEnrolledExistingFaceAnimationSequence() {

      RobotActionUnion[] actions = {
        // 0. get out animation
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoLookFaceGetOut, true)),
        // 1. say name once
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoReEnrollmentSayName,
          Anki.Cozmo.SayTextIntent.Name_Normal))
      };

      CurrentRobot.SendQueueCompoundAction(actions, HandleEnrollFaceAnimationSequenceComplete);
    }

    private void HandleEnrollFaceAnimationSequenceComplete(bool success) {
      // resume minigame music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(GetDefaultMusicState());

      EditOrEnrollFaceComplete(true);
      ContextManager.Instance.AppFlash(playChime: true);
      UIManager.CloseView(_FaceEnrollmentInstructionsViewInstance);
    }

    private void EditOrEnrollFaceComplete(bool success) {
      SharedMinigameView.ShowQuitButton();
      if (success) {
        _ShowDoneShelf = true;
      }
      ShowFaceListSlide(SharedMinigameView);
    }

    private void RetryFaceEnrollmentOnReactionaryBehaviorEnd(Anki.Cozmo.ExternalInterface.ReactionaryBehaviorTransition message) {
      if (message.behaviorStarted == false) {
        // only try to re-enroll if we are still in the enrollment instructions view
        if (_FaceEnrollmentInstructionsViewInstance != null) {
          HandleInstructionsSlideEntered();
        }
        RobotEngineManager.Instance.RemoveCallback<ReactionaryBehaviorTransition>(RetryFaceEnrollmentOnReactionaryBehaviorEnd);
      }
    }

    // pop up a confirmation for deleting an enrolled face
    private void RequestDeleteEnrolledFace(int faceID) {
      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_NoText);

      alertView.SetDasEventName("delete_enrolled_face_confirm");
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmButton, () => HandleDeleteEnrolledFaceConfirmButton(faceID));
      alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);
      alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmTitle;
      alertView.SetTitleArgs(new object[] { CurrentRobot.EnrolledFaces[faceID] });
      _DeleteConfirmationAlertView = alertView;
    }

    private void HandleDeleteEnrolledFaceConfirmButton(int faceID) {
      CurrentRobot.EraseEnrolledFaceByID(faceID);
      // TODO: popup an in-progress thing that prevents users from doing things that would interfere with the deletion.
    }

    private void HandleEraseEnrolledFace(int faceId, string faceName) {
      _ShowDoneShelf = true;
      _FaceListSlideInstance.RefreshList(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces);
      // calling this explicitly to show the conditional shelf after erasing a face
      ShowShelf(SharedMinigameView);
      // log to das
      DAS.Event("robot.face_slots_used", CurrentRobot.EnrolledFaces.Count.ToString(),
        DASUtil.FormatExtraData("-1"));

    }

    private void ShowShelf(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      newView.ShowShelf();

      if (_ShowDoneShelf) {
        newView.ShelfWidget.SetWidgetText(LocalizationKeys.kFaceEnrollmentConditionAllChangesSaved);
        newView.ShelfWidget.ShowGameDoneButton(true);
        newView.ShelfWidget.GameDoneButtonPressed += () => {
          RaiseMiniGameQuit();
        };
      }
      else {
        newView.ShelfWidget.SetWidgetText(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDescription);
      }

    }

    protected override void CleanUpOnDestroy() {

      SetReactionaryBehaviors(true);

      // turn the default head and lift state off
      if (CurrentRobot != null) {
        CurrentRobot.SetDefaultHeadAndLiftState(false, 0.0f, 0.0f);
      }

      if (_ErrorAlertView != null) {
        _ErrorAlertView.CloseViewImmediately();
      }
      if (_DeleteConfirmationAlertView != null) {
        _DeleteConfirmationAlertView.CloseViewImmediately();
      }

      if (_FaceEnrollmentInstructionsViewInstance != null) {
        _FaceEnrollmentInstructionsViewInstance.CloseViewImmediately();
      }

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(HandleEnrolledFace);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
      if (CurrentRobot != null) {
        CurrentRobot.OnEnrolledFaceRemoved -= HandleEraseEnrolledFace;
      }
    }

  }

}
