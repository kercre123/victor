using UnityEngine;
using Anki.Cozmo.ExternalInterface;

namespace FaceEnrollment {
  public class FaceEnrollInstructionState : State {

    private FaceEnrollmentInstructionsView _FaceEnrollmentInstructionsViewInstance;

    private Cozmo.UI.AlertModal _ErrorAlertView = null;

    private FaceEnrollmentGame _FaceEnrollmentGame;
    private System.Action _OnUserCancel;
    private string _NameForFace;
    private int _ReEnrollFaceID;
    private bool _UserCancelledEnrollment = false;
    private bool _EnrollingFace = false;

    private Cozmo.UI.AlertModal _DoneAlertView = null;

    public bool IsReEnrollment {
      get { return _ReEnrollFaceID != 0; }
    }

    public override void Pause(State.PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    // reEnrollID set to 0 if we aren't doing a re-enrollment
    public void Initialize(string faceName, System.Action onUserCancel, int reEnrollID = 0) {
      _NameForFace = faceName;
      _OnUserCancel = onUserCancel;
      _ReEnrollFaceID = reEnrollID;
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted>(HandleEnrolledFace);

      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentInstructionsViewInstance = UIManager.OpenModal(_FaceEnrollmentGame.FaceEnrollmentInstructionsViewPrefab);
      _FaceEnrollmentInstructionsViewInstance.ViewClosedByUser += HandleUserClosedView;

      _FaceEnrollmentInstructionsViewInstance.SetFaceName(_NameForFace);

      SendEnrollFace();
    }

    public override void Exit() {
      base.Exit();

      // resume minigame music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_FaceEnrollmentGame.GetDefaultMusicState());

      if (_FaceEnrollmentInstructionsViewInstance != null) {
        _FaceEnrollmentInstructionsViewInstance.CloseViewImmediately();
      }

      if (_ErrorAlertView != null) {
        _ErrorAlertView.CloseViewImmediately();
      }

      if (_DoneAlertView != null) {
        _DoneAlertView.CloseViewImmediately();
      }

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted>(HandleEnrolledFace);
    }

    private void HandleUserClosedView() {

      _UserCancelledEnrollment = true;

      if (_EnrollingFace) {
        _CurrentRobot.CancelFaceEnrollment();
      }
      else {
        // we are not actively enrolling a face, but still in this state, probably because we are playing
        // a reactionary behavior.
        HandleUserCancelledEnrollment();
      }

      ReturnToFaceSlide();
    }

    private void SendEnrollFace() {
      _EnrollingFace = true;
      //_CurrentRobot.SetEnableCliffSensor(false); // TODO: Handle this in the behavior?

      // First send enrollment settings, then activate the behavior to use them
      _CurrentRobot.SetFaceToEnroll(_ReEnrollFaceID, _NameForFace);
        }

    private void HandleEnrolledFace(Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted message) {

      _EnrollingFace = false;

      //_CurrentRobot.SetEnableCliffSensor(true); // TODO: Handle this in the behavior?

      if (_UserCancelledEnrollment) {
        HandleUserCancelledEnrollment();
      }
      else if (message.result == Anki.Cozmo.FaceEnrollmentResult.Success) {
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

    private void HandleKnownEnrollmentFailure(Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted faceEnrollmentCompleted) {
      // we didn't succeed so let's show an error to the user saying why
      Cozmo.UI.AlertModal alertView = UIManager.OpenModal(Cozmo.UI.AlertModalLoader.Instance.AlertModalPrefab);

      switch (faceEnrollmentCompleted.result) {

      case Anki.Cozmo.FaceEnrollmentResult.SawWrongFace:
        alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceDescription;
        alertView.SetMessageArgs(new object[] { faceEnrollmentCompleted.name });
        alertView.SetPrimaryButton(LocalizationKeys.kButtonContinue, ReturnToFaceSlide);
        alertView.ViewClosedByUser += ReturnToFaceSlide;
        break;

      // TODO: Add other special-case messaging for other failures

      default:
        alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentErrorsTimeOutTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentErrorsTimeOutDescription;
        alertView.SetPrimaryButton(LocalizationKeys.kButtonRetry, SendEnrollFace);
        alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel, ReturnToFaceSlide);
        alertView.ViewClosedByUser += ReturnToFaceSlide;
        break;
      }

      _ErrorAlertView = alertView;

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

        // for very first enrollment (not re-enrollment of first face!), exit Meet Cozmo completely when done
        // otherwise just go back to face list
        if (_CurrentRobot.EnrolledFaces.Count == 1) {
        DoneAlertView();
      }
      else {
        ReturnToFaceSlide();
      }
    }
    }

    private void DoneAlertView() {
      Cozmo.UI.AlertModal alertView = UIManager.OpenModal(Cozmo.UI.AlertModalLoader.Instance.AlertModalPrefab, overrideCloseOnTouchOutside: false);

      // show alert
      alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentFirstTimeCompleteAlertTitle;
      alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentFirstTimeCompleteAlertDescription;
      alertView.SetPrimaryButton(LocalizationKeys.kButtonContinue, EndFirstEnrollment);

      _DoneAlertView = alertView;
    }

    private void EndFirstEnrollment() {
      _FaceEnrollmentGame.RaiseMiniGameQuit();
    }

    private void ReturnToFaceSlide() {
      ContextManager.Instance.AppFlash(playChime: true);
      _StateMachine.SetNextState(new FaceSlideState());
    }

        }
      }
