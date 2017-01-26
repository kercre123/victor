using UnityEngine;
using Anki.Cozmo.ExternalInterface;
using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceEnrollInstructionState : State {

    private FaceEnrollmentInstructionsModal _FaceEnrollmentInstructionsModalInstance;

    private AlertModal _ErrorAlertModal = null;

    private FaceEnrollmentGame _FaceEnrollmentGame;
    private System.Action _OnUserCancel;
    private string _NameForFace;
    private int _ReEnrollFaceID;
    private bool _EnrollingFace = false;

    private AlertModal _DoneAlertModal = null;

    public bool IsReEnrollment {
      get { return _ReEnrollFaceID != 0; }
    }

    public override void Pause(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    // reEnrollID defaults to 0, meaning we are doing a first time enrollment.
    // otherwise reEnrollID is the faceID of the person we are re-enrolling.
    public void Initialize(string faceName, System.Action onUserCancel, int reEnrollID = 0) {
      _NameForFace = faceName;
      _OnUserCancel = onUserCancel;
      _ReEnrollFaceID = reEnrollID;
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted>(HandleEnrolledFace);

      UIManager.OpenModal(_FaceEnrollmentGame.FaceEnrollmentInstructionsModalPrefab,
                          new ModalPriorityData(), HandleInstructionsModalCreated);
    }

    private void HandleInstructionsModalCreated(BaseModal newInstructionsModal) {
      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentInstructionsModalInstance = (FaceEnrollmentInstructionsModal)newInstructionsModal;
      _FaceEnrollmentInstructionsModalInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished += HandleUserClosedInstructionsModal;
      _FaceEnrollmentInstructionsModalInstance.ModalForceClosedAnimationFinished += HandleInstructionsModalForceClosed;

      _FaceEnrollmentInstructionsModalInstance.SetNameForFace(_NameForFace);

      SendEnrollFace();
    }

    public override void Exit() {
      base.Exit();

      // resume minigame music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_FaceEnrollmentGame.GetDefaultMusicState());

      if (_FaceEnrollmentInstructionsModalInstance != null) {
        _FaceEnrollmentInstructionsModalInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished -= HandleUserClosedInstructionsModal;
        _FaceEnrollmentInstructionsModalInstance.ModalForceClosedAnimationFinished -= HandleInstructionsModalForceClosed;
        _FaceEnrollmentInstructionsModalInstance.CloseDialogImmediately();
      }

      if (_ErrorAlertModal != null) {
        _ErrorAlertModal.CloseDialogImmediately();
      }

      if (_DoneAlertModal != null) {
        _DoneAlertModal.CloseDialogImmediately();
      }

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted>(HandleEnrolledFace);
    }

    private void HandleUserClosedInstructionsModal() {
      if (_EnrollingFace) {
        _CurrentRobot.CancelFaceEnrollment();
      }
      HandleUserCancelledEnrollment();
    }

    private void HandleInstructionsModalForceClosed() {
      ReturnToFaceSlide();
    }

    private void SendEnrollFace() {
      _EnrollingFace = true;
      // First send enrollment settings, then activate the behavior to use them
      _CurrentRobot.SetFaceToEnroll(_ReEnrollFaceID, _NameForFace);
    }

    private void HandleEnrolledFace(FaceEnrollmentCompleted message) {
      _EnrollingFace = false;

      if (message.result == Anki.Cozmo.FaceEnrollmentResult.Success) {
        HandleSuccessfulEnrollment(message);
      }
      else {
        HandleKnownEnrollmentFailure(message);
      }

    }

    private void HandleUserCancelledEnrollment() {
      if (_OnUserCancel != null) {
        _OnUserCancel();
      }
    }

    private void HandleKnownEnrollmentFailure(FaceEnrollmentCompleted faceEnrollmentCompleted) {
      AlertModalData errorAlertData = null;

      switch (faceEnrollmentCompleted.result) {

      case Anki.Cozmo.FaceEnrollmentResult.SawWrongFace:
        errorAlertData = new AlertModalData("enroll_never_saw_valid_face_alert",
                                            LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceTitle,
                                            LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceDescription,
                                            new AlertModalButtonData("continue_button", LocalizationKeys.kButtonContinue, ReturnToFaceSlide),
                                            descLocArgs: new object[] { faceEnrollmentCompleted.name });
        break;

      // TODO: Add other special-case messaging for other failures

      default:
        errorAlertData = new AlertModalData("enroll_time_out_alert",
                                            LocalizationKeys.kFaceEnrollmentErrorsTimeOutTitle,
                                            LocalizationKeys.kFaceEnrollmentErrorsTimeOutDescription,
                                            new AlertModalButtonData("retry_button", LocalizationKeys.kButtonRetry, SendEnrollFace),
                                            new AlertModalButtonData("cancel_button", LocalizationKeys.kButtonCancel, ReturnToFaceSlide));
        break;
      }

      var errorAlertPriorityData = ModalPriorityData.CreateSlightlyHigherData(_FaceEnrollmentInstructionsModalInstance.PriorityData);

      System.Action<AlertModal> errorAlertCreated = (alertModal) => {
        alertModal.ModalClosedWithCloseButtonOrOutsideAnimationFinished += ReturnToFaceSlide;
        _ErrorAlertModal = alertModal;
      };

      // we didn't succeed so let's show an error to the user saying why
      UIManager.OpenAlert(errorAlertData, errorAlertPriorityData, errorAlertCreated);
    }

    private void HandleSuccessfulEnrollment(Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted faceEnrollmentCompleted) {
      _FaceEnrollmentGame.ShowDoneShelf = true;

      if (_CurrentRobot.EnrolledFaces.ContainsKey(faceEnrollmentCompleted.faceID)) {
        DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Re-enrolled existing face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(_NameForFace));
        _CurrentRobot.EnrolledFaces[faceEnrollmentCompleted.faceID] = _NameForFace;
        _CurrentRobot.EnrolledFacesLastEnrolledTime[faceEnrollmentCompleted.faceID] = Time.time;
        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnReEnrollFace);
        // don't want to show how to play again in re-enrollments
        ReturnToFaceSlide();
      }
      else {
        _CurrentRobot.EnrolledFaces.Add(faceEnrollmentCompleted.faceID, _NameForFace);
        _CurrentRobot.EnrolledFacesLastEnrolledTime.Add(faceEnrollmentCompleted.faceID, 0);
        _CurrentRobot.EnrolledFacesLastSeenTime.Add(faceEnrollmentCompleted.faceID, 0);
        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnMeetNewPerson);
        DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Enrolled new face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(_NameForFace));

        // log using up another face slot to das
        DAS.Event("robot.face_slots_used", _CurrentRobot.EnrolledFaces.Count.ToString(),
          DASUtil.FormatExtraData("1"));

        if (_CurrentRobot.EnrolledFaces.Count == 1) {
          ShowDoneAlertModal();
        }
        else {
          ReturnToFaceSlide();
        }
      }
    }

    private void ShowDoneAlertModal() {
      var doneAlertData = new AlertModalData("enroll_first_time_finished_alert",
                                             LocalizationKeys.kFaceEnrollmentFirstTimeCompleteAlertTitle,
                                             LocalizationKeys.kFaceEnrollmentFirstTimeCompleteAlertDescription,
                                             new AlertModalButtonData("continue_button", LocalizationKeys.kButtonContinue, ReturnToFaceSlide));

      var doneAlertPriorityData = ModalPriorityData.CreateSlightlyHigherData(_FaceEnrollmentInstructionsModalInstance.PriorityData);

      UIManager.OpenAlert(doneAlertData, doneAlertPriorityData, HandleDoneAlertModalCreated,
                        overrideCloseOnTouchOutside: false);
    }

    private void HandleDoneAlertModalCreated(AlertModal alertModal) {
      _DoneAlertModal = alertModal;
    }

    private void ReturnToFaceSlide() {
      ContextManager.Instance.AppFlash(playChime: true);
      _StateMachine.SetNextState(new FaceSlideState());
    }
  }
}
