namespace FaceEnrollment {
  public class FaceSlideState : State {

    private FaceEnrollmentListSlide _FaceListSlideInstance;
    private FaceEnrollmentGame _FaceEnrollmentGame;

    public override void Pause(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;
      _FaceEnrollmentGame.SharedMinigameView.ShowQuitButton();
      _FaceEnrollmentGame.ShowFaceListShelf();

      _FaceListSlideInstance = _FaceEnrollmentGame.SharedMinigameView.ShowWideGameStateSlide(_FaceEnrollmentGame.FaceListSlidePrefab.gameObject, "face_list_slide").GetComponent<FaceEnrollmentListSlide>();
      _FaceListSlideInstance.Initialize(_CurrentRobot.EnrolledFaces, _FaceEnrollmentGame);

    }

    public override void Exit() {
      base.Exit();
      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentGame.SharedMinigameView.HideShelf();
    }
  }
}
