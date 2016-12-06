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
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(HandleEnrolledFace);

      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentInstructionsViewInstance = UIManager.OpenModal(_FaceEnrollmentGame.FaceEnrollmentInstructionsViewPrefab);
      _FaceEnrollmentInstructionsViewInstance.ViewClosedByUser += HandleUserClosedView;

      _FaceEnrollmentInstructionsViewInstance.SetFaceName(_NameForFace);

      EnrollNamedFace();
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

      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotCompletedAction>(HandleEnrolledFace);
    }

    private void HandleUserClosedView() {

      _UserCancelledEnrollment = true;

      if (_EnrollingFace) {
        // cancel all actions, including enrolling named face or playing reaction animations.
        _CurrentRobot.CancelAction(Anki.Cozmo.RobotActionType.UNKNOWN);
      }
      else {
        // we are not actively enrolling a face, but still in this state, probably because we are playing
        // a reactionary behavior.
        HandleUserCancelledEnrollment(false);
      }
    }

    private void EnrollNamedFace() {
      _EnrollingFace = true;
      _CurrentRobot.SetEnableCliffSensor(false);
      _CurrentRobot.EnrollNamedFace(0, _ReEnrollFaceID, _NameForFace);
    }


    private void HandleInterruptSetIdleAndGetOutAnim(System.Action lookOutAnimDone = null) {
      _CurrentRobot.PopIdleAnimation();
      // plays the version of the getout animation without the chime
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.MeetCozmoLookFaceInterrupt, (bool success) => {
        if (lookOutAnimDone != null) {
          lookOutAnimDone();
        }
      });
    }

    private void HandleEnrolledFace(Anki.Cozmo.ExternalInterface.RobotCompletedAction message) {
      // ignore all messages except ENROLL_NAMED_FACE
      if (message.actionType != Anki.Cozmo.RobotActionType.ENROLL_NAMED_FACE) {
        return;
      }

      _EnrollingFace = false;

      _CurrentRobot.SetEnableCliffSensor(true);

      // cancelled for some reason other than UI back (eg. interrupt by picking up / place on back).
      // we also include FAILURE_NOT_STARTED because that can be triggered if the action was interrupted
      // before we even start the action (but it is queued).
      bool actionCancelledOrInterrupted = message.result == Anki.Cozmo.ActionResult.CANCELLED || message.result == Anki.Cozmo.ActionResult.FAILURE_NOT_STARTED;
      bool knownEnrollmentFailure = message.result != Anki.Cozmo.ActionResult.SUCCESS;

      if (_UserCancelledEnrollment) {
        HandleUserCancelledEnrollment(message.completionInfo.faceEnrollmentCompleted.isFaceScanning);
      }
      else if (actionCancelledOrInterrupted) {
        // start listening for when the reactionary behavior is done so we can try again
        RobotEngineManager.Instance.AddCallback<ReactionaryBehaviorTransition>(RetryFaceEnrollmentOnReactionaryBehaviorEnd);
        if (message.completionInfo.faceEnrollmentCompleted.isFaceScanning) {
          HandleInterruptSetIdleAndGetOutAnim();
        }
      }
      else if (knownEnrollmentFailure) {
        HandleKnownEnrollmentFailure(message.completionInfo.faceEnrollmentCompleted);
      }
      else {
        HandleSuccessfulEnrollment(message.completionInfo.faceEnrollmentCompleted);
      }
    }

    private void HandleUserCancelledEnrollment(bool isFaceScanning) {
      if (isFaceScanning) {
        HandleInterruptSetIdleAndGetOutAnim(_OnUserCancel);
      }
      else {
        if (_OnUserCancel != null) {
          _OnUserCancel();
        }
      }
    }

    private void HandleKnownEnrollmentFailure(Anki.Cozmo.FaceEnrollmentCompleted faceEnrollmentCompleted) {
      // we didn't succeed so let's show an error to the user saying why
      Cozmo.UI.AlertModal alertView = UIManager.OpenModal(Cozmo.UI.AlertModalLoader.Instance.AlertModalPrefab);

      if (faceEnrollmentCompleted.neverSawValidFace) {
        alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentErrorsNeverSawValidFaceDescription;
        alertView.SetMessageArgs(new object[] { faceEnrollmentCompleted.name });
        alertView.SetPrimaryButton(LocalizationKeys.kButtonContinue, ReturnToFaceSlide);
        alertView.ViewClosedByUser += ReturnToFaceSlide;
      }
      else {
        alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentErrorsTimeOutTitle;
        alertView.DescriptionLocKey = LocalizationKeys.kFaceEnrollmentErrorsTimeOutDescription;
        alertView.SetPrimaryButton(LocalizationKeys.kButtonRetry, EnrollNamedFace);
        alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel, ReturnToFaceSlide);
        alertView.ViewClosedByUser += ReturnToFaceSlide;
      }

      _ErrorAlertView = alertView;

      if (faceEnrollmentCompleted.isFaceScanning) {
        HandleInterruptSetIdleAndGetOutAnim();
      }
    }

    private void HandleSuccessfulEnrollment(Anki.Cozmo.FaceEnrollmentCompleted faceEnrollmentCompleted) {
      // log success to das
      DAS.Event("robot.face_enrollment", faceEnrollmentCompleted.faceID.ToString());

      // since we succeeded we have to reset the idle
      _CurrentRobot.PopIdleAnimation();
      _FaceEnrollmentGame.ShowDoneShelf = true;

      if (_CurrentRobot.EnrolledFaces.ContainsKey(faceEnrollmentCompleted.faceID)) {
        DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Re-enrolled existing face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(_NameForFace));
        _CurrentRobot.EnrolledFaces[faceEnrollmentCompleted.faceID] = _NameForFace;
        _CurrentRobot.EnrolledFacesLastEnrolledTime[faceEnrollmentCompleted.faceID] = Time.time;
        ReEnrolledExistingFaceAnimationSequence();
        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnReEnrollFace);
      }
      else {
        _CurrentRobot.EnrolledFaces.Add(faceEnrollmentCompleted.faceID, _NameForFace);
        _CurrentRobot.EnrolledFacesLastEnrolledTime.Add(faceEnrollmentCompleted.faceID, 0);
        _CurrentRobot.EnrolledFacesLastSeenTime.Add(faceEnrollmentCompleted.faceID, 0);
        GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnMeetNewPerson);
        DAS.Debug("FaceEnrollmentGame.HandleEnrolledFace", "Enrolled new face: " + PrivacyGuard.HidePersonallyIdentifiableInfo(_NameForFace));
        EnrolledNewFaceAnimationSequence();

        // log using up another face slot to das
        DAS.Event("robot.face_slots_used", _CurrentRobot.EnrolledFaces.Count.ToString(),
          DASUtil.FormatExtraData("1"));
      }
    }

    private void EnrolledNewFaceAnimationSequence() {
      // play first time say name specific music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Meet_Cozmo_Say_Name);

      RobotActionUnion[] actions = {
        // 0. get out animation
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(_CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoLookFaceGetOut, true, false, false, false)),
        // 1. say name once
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentSayName,
          Anki.Cozmo.SayTextIntent.Name_FirstIntroduction_1,
          false)),
        // 2. repeat name                      
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentRepeatName,
          Anki.Cozmo.SayTextIntent.Name_FirstIntroduction_2,
          false)),
        // 3. final celebration (no name said)                
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(_CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration, true, false, false, false))
      };

      _CurrentRobot.SendQueueCompoundAction(actions, HandleEnrollFaceAnimationSequenceComplete);

    }

    private void ReEnrolledExistingFaceAnimationSequence() {

      RobotActionUnion[] actions = {
        // 0. get out animation
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(_CurrentRobot.ID, 1, Anki.Cozmo.AnimationTrigger.MeetCozmoLookFaceGetOut, true, false, false, false)),
        // 1. say name once
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _NameForFace,
          Anki.Cozmo.AnimationTrigger.MeetCozmoReEnrollmentSayName,
          Anki.Cozmo.SayTextIntent.Name_Normal,
          false))
      };

      _CurrentRobot.SendQueueCompoundAction(actions, HandleEnrollFaceAnimationSequenceComplete);
    }

    private void HandleEnrollFaceAnimationSequenceComplete(bool success) {
      // don't want to show how to play again in re-enrollments
      if (_CurrentRobot.EnrolledFaces.Count == 1 && !IsReEnrollment) {
        DoneAlertView();
      }
      else {
        ReturnToFaceSlide();
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

    private void RetryFaceEnrollmentOnReactionaryBehaviorEnd(Anki.Cozmo.ExternalInterface.ReactionaryBehaviorTransition message) {
      if (message.behaviorStarted == false) {
        // only try to re-enroll if we are still in the enrollment instructions view
        if (_FaceEnrollmentInstructionsViewInstance != null) {
          EnrollNamedFace();
        }
        RobotEngineManager.Instance.RemoveCallback<ReactionaryBehaviorTransition>(RetryFaceEnrollmentOnReactionaryBehaviorEnd);
      }
    }
  }
}
