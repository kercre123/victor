using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo.ExternalInterface;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    [SerializeField]
    private FaceEnrollmentListSlide _FaceListSlidePrefab;
    private FaceEnrollmentListSlide _FaceListSlideInstance;

    [SerializeField]
    private FaceEnrollmentEnterNameSlide _EnterNameSlidePrefab;
    private FaceEnrollmentEnterNameSlide _EnterNameSlideInstance;

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
      // make cozmo look up
      CurrentRobot.SetHeadAngle(CozmoUtil.kIdealFaceViewHeadValue);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotOffTreadsStateChanged>(HandleOffTredsStateChanged);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(HandleEnrolledFace);
      CurrentRobot.OnEnrolledFaceRemoved += HandleEraseEnrolledFace;

      FaceEnrollmentGameConfig faceEnrollmentConfig = (FaceEnrollmentGameConfig)minigameConfig;

      _UpdateThresholdLastSeenSeconds = faceEnrollmentConfig.UpdateThresholdLastSeenSeconds;
      _UpdateThresholdLastEnrolledSeconds = faceEnrollmentConfig.UpdateThresholdLastEnrolledSeconds;
    }

    protected override void AddDisabledReactionaryBehaviors() {
      _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToCubeMoved);
      _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToCliff);
      _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToPickup);
      _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToUnexpectedMovement);
      _DisabledReactionaryBehaviors.Add(Anki.Cozmo.BehaviorType.ReactToFrustration);
    }

    protected override void SetupViewAfterCozmoReady(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.SetupViewAfterCozmoReady(newView, data);
      ShowFaceListSlide(newView);
    }

    private void HandleChangedObservedFaceID(Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID message) {
      if (_FixedFaceID != -1) {
        if (_FixedFaceID == message.oldID) {
          _FixedFaceID = message.newID;
        }
      }
    }

    private void HandleOffTredsStateChanged(Anki.Cozmo.ExternalInterface.RobotOffTreadsStateChanged message) {
      if (message.treadsState == Anki.Cozmo.OffTreadsState.OnTreads) {
        // make cozmo look up after response to back
        CurrentRobot.SetHeadAngle(CozmoUtil.kIdealFaceViewHeadValue);
      }
    }

    private void ShowFaceListSlide(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _FaceListSlideInstance = newView.ShowWideGameStateSlide(_FaceListSlidePrefab.gameObject, "face_list_slide").GetComponent<FaceEnrollmentListSlide>();
      _FaceListSlideInstance.Initialize(CurrentRobot.EnrolledFaces, _UpdateThresholdLastSeenSeconds, _UpdateThresholdLastEnrolledSeconds);
      newView.ShowShelf();
      _FaceListSlideInstance.OnEnrollNewFaceRequested += EnterNameForNewFace;
      _FaceListSlideInstance.OnEditNameRequested += EditExistingName;
      _FaceListSlideInstance.OnDeleteEnrolledFace += RequestDeleteEnrolledFace;
      _FaceListSlideInstance.OnReEnrollFaceRequested += RequestReEnrollFace;
      newView.ShelfWidget.SetWidgetText(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDescription);
      newView.ShowQuitButton();
    }

    private void CleanupFaceListSlide() {
      _FaceListSlideInstance.OnEnrollNewFaceRequested -= EnterNameForNewFace;
      _FaceListSlideInstance.OnEditNameRequested -= EditExistingName;
      _FaceListSlideInstance.OnDeleteEnrolledFace -= RequestDeleteEnrolledFace;
      _FaceListSlideInstance.OnReEnrollFaceRequested -= RequestReEnrollFace;
      SharedMinigameView.HideShelf();
    }

    private void EditExistingName(int faceID, string exisitingName) {
      _EnterNameSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "edit_name", EditNameInputSlideInDone).GetComponent<FaceEnrollmentEnterNameSlide>();
      _EnterNameSlideInstance.SetNameInputField(exisitingName);
      _FaceIDToEdit = faceID;
      _FaceOldNameEdit = exisitingName;

      SharedMinigameView.ShowBackButton(() => {
        SharedMinigameView.ShowQuitButton();
        ShowFaceListSlide(SharedMinigameView);
      });
      CleanupFaceListSlide();
    }

    private void EnterNameForNewFace(string preFilledName) {
      _EnterNameSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "enter_new_name", NewNameInputSlideInDone).GetComponent<FaceEnrollmentEnterNameSlide>();

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
      _ReEnrollFaceID = faceId;
      _NameForFace = faceName;

      ShowInstructions(() => {
        ShowFaceListSlide(SharedMinigameView);
      }, faceName);
    }

    private void HandleNewNameEntered(string faceName) {
      _NameForFace = faceName;

      ShowInstructions(() => {
        EnterNameForNewFace(faceName);
      }, faceName);
    }

    private void ShowInstructions(System.Action handleInstructionsDone, string faceName) {
      SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentInstructionsViewInstance = UIManager.OpenView(_FaceEnrollmentInstructionsViewPrefab);
      _FaceEnrollmentInstructionsViewInstance.ViewClosedByUser += () => {
        CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.ENROLL_NAMED_FACE);
        _UserCancelledEnrollment = true;
        if (handleInstructionsDone != null) {
          handleInstructionsDone();
        }
      };
      if (string.IsNullOrEmpty(faceName) == false) {
        _FaceEnrollmentInstructionsViewInstance.SetFaceName(faceName);
      }
      HandleInstructionsSlideEntered();
    }

    private void HandleUpdatedNameEntered(string newName) {
      CurrentRobot.OnEnrolledFaceRenamed += HandleRobotRenamedEnrolledFace;
      CurrentRobot.UpdateEnrolledFaceByID(_FaceIDToEdit, _FaceOldNameEdit, newName);
    }

    private void HandleRobotRenamedEnrolledFace(int faceId, string faceName) {
      CurrentRobot.OnEnrolledFaceRenamed -= HandleRobotRenamedEnrolledFace;
      CurrentRobot.SayTextWithEvent(faceName, Anki.Cozmo.AnimationTrigger.MeetCozmoRenameFaceSayName, callback: EditOrEnrollFaceComplete);
    }

    private void HandleInstructionsSlideEntered() {
      if (_UseFixedFaceID) {
        CurrentRobot.EnrollNamedFace(_FixedFaceID, _ReEnrollFaceID, _NameForFace, saveToRobot: _SaveToRobot);
      }
      else {
        CurrentRobot.EnrollNamedFace(0, _ReEnrollFaceID, _NameForFace, saveToRobot: _SaveToRobot);
      }
    }

    private void HandleEnrolledFace(Anki.Cozmo.ExternalInterface.RobotCompletedAction message) {
      // ignore all messages except ENROLL_NAMED_FACE
      if (message.actionType != Anki.Cozmo.RobotActionType.ENROLL_NAMED_FACE) {
        return;
      }

      // dont show errors if we user cancels enrollment.
      if (_UserCancelledEnrollment) {
        // reset _ReEnrollFaceID
        _ReEnrollFaceID = 0;
        _UserCancelledEnrollment = false;
        return;
      }

      // cancelled for some other reason (eg. interrupt by picking up / place on back).
      if (message.result == Anki.Cozmo.ActionResult.CANCELLED) {
        _ReEnrollFaceID = 0;
        UIManager.CloseView(_FaceEnrollmentInstructionsViewInstance);
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
          alertView.SetPrimaryButton(LocalizationKeys.kButtonRetry, () => {
            CleanupFaceListSlide();
            bool reEnrolling = _ReEnrollFaceID != 0;
            if (reEnrolling) {
              RequestReEnrollFace(_ReEnrollFaceID, _NameForFace);
            }
            else {
              HandleNewNameEntered(_NameForFace);
            }
          });
          alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);
        }
      }
      else {
        if (CurrentRobot.EnrolledFaces.ContainsKey(message.completionInfo.faceEnrollmentCompleted.faceID)) {
          DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Re-enrolled existing face: " + _NameForFace);
          CurrentRobot.EnrolledFaces[message.completionInfo.faceEnrollmentCompleted.faceID] = _NameForFace;
          CurrentRobot.EnrolledFacesLastEnrolledTime[message.completionInfo.faceEnrollmentCompleted.faceID] = Time.time;
          ReEnrolledExisitingFaceAnimationSequence();
        }
        else {
          CurrentRobot.EnrolledFaces.Add(message.completionInfo.faceEnrollmentCompleted.faceID, _NameForFace);
          CurrentRobot.EnrolledFacesLastEnrolledTime.Add(message.completionInfo.faceEnrollmentCompleted.faceID, 0);
          CurrentRobot.EnrolledFacesLastSeenTime.Add(message.completionInfo.faceEnrollmentCompleted.faceID, 0);
          GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnMeetNewPerson);
          DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Enrolled new face: " + _NameForFace);
          EnrolledNewFaceAnimationSequence();
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

    private void EnrolledNewFaceAnimationSequence() {
      // play first time say name specific music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Meet_Cozmo_Say_Name);

      RobotActionUnion[] actions = {
        // 1. say name once
        new RobotActionUnion().Initialize(Singleton<SayTextWithIntent>.Instance.Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentSayName,
          Anki.Cozmo.SayTextIntent.Name_FirstIntroduction)),
        // 2. repeat name                      
        new RobotActionUnion().Initialize(Singleton<SayTextWithIntent>.Instance.Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentRepeatName,
          Anki.Cozmo.SayTextIntent.Name_FirstIntroduction)),
        // 3. final celebration (no name said)                
        new RobotActionUnion().Initialize(Singleton<PlayAnimationTrigger>.Instance.Initialize(CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration))
      };

      CurrentRobot.SendQueueCompoundAction(actions, HandleEnrollFaceAnimationSequenceComplete);

    }

    private void ReEnrolledExisitingFaceAnimationSequence() {
      CurrentRobot.SayTextWithEvent(_NameForFace, Anki.Cozmo.AnimationTrigger.MeetCozmoReEnrollmentSayName, Anki.Cozmo.SayTextIntent.Name_Normal, HandleEnrollFaceAnimationSequenceComplete);
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
      ShowFaceListSlide(SharedMinigameView);
    }

    // pop up a confirmation for deleting an enrolled face
    private void RequestDeleteEnrolledFace(int faceID) {
      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_NoText);

      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmButton, () => HandleDeleteEnrolledFaceConfirmButton(faceID));
      alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);
      alertView.TitleLocKey = Localization.GetWithArgs(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmTitle, CurrentRobot.EnrolledFaces[faceID]);
    }

    private void HandleDeleteEnrolledFaceConfirmButton(int faceID) {
      CurrentRobot.EraseEnrolledFaceByID(faceID);
      // TODO: popup an in-progress thing that prevents users from doing things that would interfere with the deletion.
    }

    private void HandleEraseEnrolledFace(int faceId, string faceName) {
      _FaceListSlideInstance.RefreshList(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces);
    }

    protected override void CleanUpOnDestroy() {
      SharedMinigameView.HideGameStateSlide();
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(HandleEnrolledFace);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotOffTreadsStateChanged>(HandleOffTredsStateChanged);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
      CurrentRobot.OnEnrolledFaceRemoved -= HandleEraseEnrolledFace;
    }

  }

}
