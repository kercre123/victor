using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentDetailsSlideState : State {

    private FaceEnrollmentDetailsSlide _FaceEnrollmentDetailsSlideInstance;
    private FaceEnrollmentGame _FaceEnrollmentGame;
    private int _FaceID;
    private string _NameForFace;

    private Cozmo.UI.AlertModal _DeleteConfirmationAlertView = null;

    public void Initialize(int faceID, string nameForFace) {
      _FaceID = faceID;
      _NameForFace = nameForFace;
    }

    public override void Pause(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;
      _FaceEnrollmentGame.SharedMinigameView.ShowBackButton(ReturnToFaceSlide);

      _FaceEnrollmentGame.ShowDetailsShelf(() => {
        RequestDeleteEnrolledFace(_FaceID);
      });

      _CurrentRobot.OnEnrolledFaceRemoved += HandleEraseEnrolledFace;

      _FaceEnrollmentDetailsSlideInstance = _FaceEnrollmentGame.SharedMinigameView.ShowWideGameStateSlide(_FaceEnrollmentGame.FaceEnrollmentDetailsSlidePrefab.gameObject,
                                                                                                          "face_details_slide").GetComponent<FaceEnrollmentDetailsSlide>();
      _FaceEnrollmentDetailsSlideInstance.Initialize(_FaceID, _NameForFace);
      _FaceEnrollmentDetailsSlideInstance.OnRequestReEnrollFace += _FaceEnrollmentGame.RequestReEnrollFace;

    }

    public override void Exit() {
      base.Exit();

      if (_CurrentRobot != null) {
        _CurrentRobot.OnEnrolledFaceRemoved -= HandleEraseEnrolledFace;
      }

      if (_DeleteConfirmationAlertView != null) {
        _DeleteConfirmationAlertView.CloseDialogImmediately();
      }

      _FaceEnrollmentDetailsSlideInstance.OnRequestReEnrollFace -= _FaceEnrollmentGame.RequestReEnrollFace;

      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentGame.SharedMinigameView.HideShelf();
    }

    private void ReturnToFaceSlide() {
      _StateMachine.SetNextState(new FaceSlideState());
    }

    private void HandleEraseEnrolledFace(int faceId, string faceName) {
      _FaceEnrollmentGame.ShowDoneShelf = true;
      _StateMachine.SetNextState(new FaceSlideState());
      // log to das
      DAS.Event("robot.face_slots_used", _CurrentRobot.EnrolledFaces.Count.ToString(),
        DASUtil.FormatExtraData("-1"));
    }

    // pop up a confirmation for deleting an enrolled face
    private void RequestDeleteEnrolledFace(int faceID) {

      AlertModalButtonData primaryButtonData = new AlertModalButtonData("confirm_delete_button",
                                                                        LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmButton,
                                                                        () => HandleDeleteEnrolledFaceConfirmButton(faceID));

      AlertModalData requestDeleteEnrolledData = new AlertModalData("delete_enrolled_face_alert",
                                                                    LocalizationKeys.kFaceEnrollmentFaceEnrollmentListDeleteConfirmTitle,
                                                                    primaryButtonData: primaryButtonData,
                                                                    secondaryButtonData: new AlertModalButtonData("cancel_button", LocalizationKeys.kButtonCancel),
                                                                    titleLocArgs: new object[] { _CurrentRobot.EnrolledFaces[faceID] });

      UIManager.OpenAlert(requestDeleteEnrolledData, new ModalPriorityData(), (alertModal) => {
        _DeleteConfirmationAlertView = alertModal;
      });
    }

    private void HandleDeleteEnrolledFaceConfirmButton(int faceID) {
      _CurrentRobot.EraseEnrolledFaceByID(faceID);
    }


  }

}