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

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      // make cozmo look up
      CurrentRobot.SetHeadAngle(CozmoUtil.kIdealFaceViewHeadValue);
    }

    protected override void InitializeView(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.InitializeView(newView, data);
      ShowFaceListSlide(newView);
    }

    private void ShowFaceListSlide(Cozmo.MinigameWidgets.SharedMinigameView newView) {
      newView.ShowQuitButton();
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

      SharedMinigameView.ShowBackButton(() => ShowFaceListSlide(SharedMinigameView));
      CleanupFaceListSlide();
    }

    private void EnterNameForNewFace() {
      _EnterNameSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "enter_new_name", NewNameInputSlideInDone).GetComponent<FaceEnrollmentEnterNameSlide>();
      SharedMinigameView.ShowBackButton(() => ShowFaceListSlide(SharedMinigameView));
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
        EnterNameForNewFace();
        SharedMinigameView.HideSpinnerWidget();
      });
    }

    private void HandleUpdatedNameEntered(string newName) {

      // TODO: Check for confirmation by engine
      CurrentRobot.EnrolledFaces[_FaceIDToEdit] = newName;

      RobotEngineManager.Instance.CurrentRobot.UpdateEnrolledFaceByID(_FaceIDToEdit, _FaceOldNameEdit, newName);

      // TODO: manually trigger say new name?

      EditOrEnrollFaceComplete(true);
    }

    private void HandleInstructionsSlideEntered() {
      if (_UseFixedFaceID) {
        CurrentRobot.EnrollNamedFace(_FixedFaceID, _NameForFace, saveToRobot: _SaveToRobot, callback: HandleEnrolledFace);
        _AttemptedEnrollFace = true;
      }
      else {
        RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
      }
    }

    private void HandleObservedNewFace(int id, Vector3 pos, Quaternion rot) {

      if (_AttemptedEnrollFace) {
        return;
      }

      CurrentRobot.EnrollNamedFace(id, _NameForFace, saveToRobot: _SaveToRobot, callback: HandleEnrolledFace);
      _FaceIDToEnroll = id;
      _AttemptedEnrollFace = true;
    }

    private void HandleEnrolledFace(bool success) {

      if (!success) {
        // TODO: Retry or notify failure or something?
      }
      else {

        CurrentRobot.EnrolledFaces.Add(_FaceIDToEnroll, _NameForFace);
      }
      EditOrEnrollFaceComplete(success);
      _AttemptedEnrollFace = false;
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

    private void EditOrEnrollFaceComplete(bool success) {
      if (success) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedWin);
      }
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
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

  }

}
