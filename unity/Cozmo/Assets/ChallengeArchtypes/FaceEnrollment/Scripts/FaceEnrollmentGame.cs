using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    [SerializeField]
    private FaceEnrollmentListSlide _FaceListSlidePrefab;
    private FaceEnrollmentListSlide _FaceListSlideInstance;

    [SerializeField]
    private FaceEnrollmentEnterNameSlide _EnterNameSlidePrefab;
    private FaceEnrollmentEnterNameSlide _EnterNameSlideInstance;

    [SerializeField]
    private GameObject _FaceEnrollmentDiagramPrefab;

    private bool _AttemptedEnrollFace = false;

    // used by press demo to skip saving to actual robot.
    private bool _SaveToRobot = true;

    private string _NameForFace;

    private int _FaceIDToEdit;
    private string _FaceOldNameEdit;

    private int _FaceIDToEnroll;

    private int _FixedFaceID = -1;

    private bool _UseFixedFaceID = false;

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
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotOnBackFinished>(HandleOnBackFinished);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
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

    private void HandleOnBackFinished(Anki.Cozmo.ExternalInterface.RobotOnBackFinished message) {
      // make cozmo look up after response to back
      CurrentRobot.SetHeadAngle(CozmoUtil.kIdealFaceViewHeadValue);
    }

    private void ShowFaceListSlide(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      _FaceListSlideInstance = newView.ShowWideGameStateSlide(_FaceListSlidePrefab.gameObject, "face_list_slide").GetComponent<FaceEnrollmentListSlide>();
      _FaceListSlideInstance.Initialize(CurrentRobot.EnrolledFaces);
      newView.ShowShelf();
      _FaceListSlideInstance.OnEnrollNewFaceRequested += EnterNameForNewFace;
      _FaceListSlideInstance.OnEditNameRequested += EditExistingName;
      _FaceListSlideInstance.OnDeleteEnrolledFace += RequestDeleteEnrolledFace;
      newView.ShelfWidget.SetWidgetText(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDescription);
    }

    private void CleanupFaceListSlide() {
      _FaceListSlideInstance.OnEnrollNewFaceRequested -= EnterNameForNewFace;
      _FaceListSlideInstance.OnEditNameRequested -= EditExistingName;
      _FaceListSlideInstance.OnDeleteEnrolledFace -= RequestDeleteEnrolledFace;
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

    private void EnterNameForNewFace() {
      _EnterNameSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "enter_new_name", NewNameInputSlideInDone).GetComponent<FaceEnrollmentEnterNameSlide>();

      // if this the first name then we should pre-populate the name with the profile name.
      if (CurrentRobot.EnrolledFaces.Count == 0) {
        _EnterNameSlidePrefab.SetNameInputField(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
      }

      SharedMinigameView.ShowBackButton(() => {
        SharedMinigameView.ShowQuitButton();
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

    private void HandleNewNameEntered(string name) {
      _NameForFace = name;
      SharedMinigameView.ShowWideAnimationSlide("faceEnrollment.instructions", "face_enrollment_wait_instructions", _FaceEnrollmentDiagramPrefab, HandleInstructionsSlideEntered);
      SharedMinigameView.ShowShelf();
      SharedMinigameView.ShowSpinnerWidget();

      SharedMinigameView.ShowBackButton(() => {
        SharedMinigameView.ShowQuitButton();
        EnterNameForNewFace();
        SharedMinigameView.HideSpinnerWidget();
      });
    }

    private void HandleUpdatedNameEntered(string newName) {

      // TODO: Check for confirmation by engine
      CurrentRobot.EnrolledFaces[_FaceIDToEdit] = newName;

      RobotEngineManager.Instance.CurrentRobot.UpdateEnrolledFaceByID(_FaceIDToEdit, _FaceOldNameEdit, newName);

      // TODO: manually trigger say new name?
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Meet_Cozmo_Say_Name);

      EditOrEnrollFaceComplete(true);
    }

    private void HandleInstructionsSlideEntered() {
      if (_UseFixedFaceID) {
        // TODO: Make sure the mergeIntoID makes sense! (COZMO-2926)
        const int tempMergeIntoID = 0;
        CurrentRobot.EnrollNamedFace(_FixedFaceID, tempMergeIntoID, _NameForFace, saveToRobot: _SaveToRobot, callback: HandleEnrolledFace);
        _AttemptedEnrollFace = true;
      }
      else {
        RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotObservedFace>(HandleObservedFace);
      }
    }

    private void HandleObservedFace(Anki.Cozmo.ExternalInterface.RobotObservedFace message) {
      bool newFace = message.faceID > 0 && message.name == "";

      if (_AttemptedEnrollFace || !newFace) {
        return;
      }

      // TODO: Get mergeIntoID from UI if we are re-enrolling! (COZMO-2926)
      const int tempMergeIntoID = 0;
      CurrentRobot.EnrollNamedFace(message.faceID, tempMergeIntoID, _NameForFace, saveToRobot: _SaveToRobot, callback: HandleEnrolledFace);
      _FaceIDToEnroll = message.faceID;
      _AttemptedEnrollFace = true;
    }

    private void HandleEnrolledFace(bool success) {

      if (!success) {
        // TODO: Retry or notify failure or something?
        // Currently only does app flash.
        ContextManager.Instance.AppFlash(playChime: true);
      }
      else {

        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnMeetNewPerson);
        if (CurrentRobot.EnrolledFaces.ContainsKey(_FaceIDToEnroll)) {
          DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Re-enrolled existing face: " + _NameForFace);
          CurrentRobot.EnrolledFaces[_FaceIDToEnroll] = _NameForFace;
        }
        else {
          CurrentRobot.EnrolledFaces.Add(_FaceIDToEnroll, _NameForFace);
          DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Enrolled new face: " + _NameForFace);
        }
      }
      EditOrEnrollFaceComplete(success);
      _AttemptedEnrollFace = false;
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotObservedFace>(HandleObservedFace);
      // reset fixed face ID constraint after doing an enrollment
      // this handles the edge case of if a fixed ID was set at the start
      // of the activity but we want to enroll multiple faces in the same session
      _FixedFaceID = -1;
    }

    private void EditOrEnrollFaceComplete(bool success) {
      if (success) {
        ContextManager.Instance.AppFlash(playChime: true);
      }
      SharedMinigameView.ShowQuitButton();
      ShowFaceListSlide(SharedMinigameView);
      SharedMinigameView.HideSpinnerWidget();
    }

    // pop up a confirmation for deleting an enrolled face
    private void RequestDeleteEnrolledFace(int faceID) {

      Cozmo.UI.AlertView alertView = UIManager.OpenView(Cozmo.UI.AlertViewLoader.Instance.AlertViewPrefab_NoText);
      // Hook up callbacks
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmButton, () => HandleDeleteEnrolledFace(faceID));
      alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);
      alertView.TitleLocKey = Localization.GetWithArgs(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmTitle, CurrentRobot.EnrolledFaces[faceID]);
      // Listen for dialog close
    }

    private void HandleDeleteEnrolledFace(int faceID) {
      RobotEngineManager.Instance.CurrentRobot.EraseEnrolledFaceByID(faceID);
      // TODO: confirm deletion from engine
      RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Remove(faceID);
      _FaceListSlideInstance.RefreshList(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces);
    }

    protected override void CleanUpOnDestroy() {
      SharedMinigameView.HideGameStateSlide();
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotObservedFace>(HandleObservedFace);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotOnBackFinished>(HandleOnBackFinished);
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotChangedObservedFaceID>(HandleChangedObservedFaceID);
    }

  }

}
