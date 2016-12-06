using UnityEngine;
using System.Collections;

namespace FaceEnrollment {
  public class FaceSlideState : State {

    private FaceEnrollmentListSlide _FaceListSlideInstance;

    private FaceEnrollmentGame _FaceEnrollmentGame;

    private Cozmo.UI.AlertModal _DeleteConfirmationAlertView = null;

    public override void Pause(State.PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;
      _FaceEnrollmentGame.SharedMinigameView.ShowQuitButton();
      _FaceEnrollmentGame.ShowShelf();
      _FaceEnrollmentGame.SetReactionaryBehaviors(true);

      _FaceListSlideInstance = _FaceEnrollmentGame.SharedMinigameView.ShowWideGameStateSlide(_FaceEnrollmentGame.FaceListSlidePrefab.gameObject, "face_list_slide").GetComponent<FaceEnrollmentListSlide>();
      _FaceListSlideInstance.Initialize(_CurrentRobot.EnrolledFaces, _FaceEnrollmentGame.UpdateThresholdLastSeenSeconds, _FaceEnrollmentGame.UpdateThresholdLastEnrolledSeconds);

      _FaceListSlideInstance.OnEnrollNewFaceRequested += _FaceEnrollmentGame.EnterNameForNewFace;
      _FaceListSlideInstance.OnEditNameRequested += _FaceEnrollmentGame.EditExistingName;
      _FaceListSlideInstance.OnDeleteEnrolledFace += RequestDeleteEnrolledFace;
      _FaceListSlideInstance.OnReEnrollFaceRequested += _FaceEnrollmentGame.RequestReEnrollFace;

      _CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.MeetCozmoFindFaces);
      _CurrentRobot.OnEnrolledFaceRemoved += HandleEraseEnrolledFace;
    }

    public override void Exit() {
      base.Exit();
      UnRegisterCallbacks();

      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentGame.SharedMinigameView.HideShelf();
      _FaceEnrollmentGame.SetReactionaryBehaviors(false);

      if (_CurrentRobot != null) {
        _CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
        _CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
        _CurrentRobot.OnEnrolledFaceRemoved -= HandleEraseEnrolledFace;
      }

      if (_DeleteConfirmationAlertView != null) {
        _DeleteConfirmationAlertView.CloseViewImmediately();
      }
    }

    private void UnRegisterCallbacks() {
      if (_FaceListSlideInstance != null) {
        _FaceListSlideInstance.OnEnrollNewFaceRequested -= _FaceEnrollmentGame.EnterNameForNewFace;
        _FaceListSlideInstance.OnEditNameRequested -= _FaceEnrollmentGame.EditExistingName;
        _FaceListSlideInstance.OnDeleteEnrolledFace -= RequestDeleteEnrolledFace;
        _FaceListSlideInstance.OnReEnrollFaceRequested -= _FaceEnrollmentGame.RequestReEnrollFace;
      }

    }

    // pop up a confirmation for deleting an enrolled face
    private void RequestDeleteEnrolledFace(int faceID) {
      Cozmo.UI.AlertModal alertView = UIManager.OpenModal(Cozmo.UI.AlertModalLoader.Instance.NoTextAlertModalPrefab);

      alertView.SetDasEventName("delete_enrolled_face_confirm");
      alertView.SetCloseButtonEnabled(false);
      alertView.SetPrimaryButton(LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmButton, () => HandleDeleteEnrolledFaceConfirmButton(faceID));
      alertView.SetSecondaryButton(LocalizationKeys.kButtonCancel);
      alertView.TitleLocKey = LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmTitle;
      alertView.SetTitleArgs(new object[] { _CurrentRobot.EnrolledFaces[faceID] });
      _DeleteConfirmationAlertView = alertView;
    }

    private void HandleDeleteEnrolledFaceConfirmButton(int faceID) {
      _CurrentRobot.EraseEnrolledFaceByID(faceID);
      // TODO: popup an in-progress thing that prevents users from doing things that would interfere with the deletion.
    }

    private void HandleEraseEnrolledFace(int faceId, string faceName) {
      _FaceEnrollmentGame.ShowDoneShelf = true;
      _FaceListSlideInstance.RefreshList(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces);
      // calling this explicitly to show the conditional shelf after erasing a face
      _FaceEnrollmentGame.ShowShelf();
      // log to das
      DAS.Event("robot.face_slots_used", _CurrentRobot.EnrolledFaces.Count.ToString(),
        DASUtil.FormatExtraData("-1"));

    }

  }
}

