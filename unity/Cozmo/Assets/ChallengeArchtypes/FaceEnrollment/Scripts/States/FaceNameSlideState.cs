namespace FaceEnrollment {
  public class FaceNameSlideState : State {

    private EnterNameSlide _EnterNameSlideInstance;
    private FaceEnrollmentGame _FaceEnrollmentGame;
    private string _PreFilledName = "";
    private System.Action<string> _OnNameEntered;

    public void Initialize(string preFilledName, System.Action<string> onNameEntered) {
      _PreFilledName = preFilledName;
      _OnNameEntered = onNameEntered;
    }

    public override void Pause(State.PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;

      _EnterNameSlideInstance = _FaceEnrollmentGame.SharedMinigameView.ShowWideGameStateSlide(
        _FaceEnrollmentGame.EnterNameSlidePrefab.gameObject,
        "enter_new_name",
        NewNameInputSlideInDone).GetComponent<EnterNameSlide>();

      if (string.IsNullOrEmpty(_PreFilledName) == false) {
        _EnterNameSlideInstance.SetNameInputField(_PreFilledName);
      }

      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        _FaceEnrollmentGame.SharedMinigameView.ShowQuitButton();
      }
      else {
        _FaceEnrollmentGame.SharedMinigameView.ShowBackButton(() => {
          _StateMachine.SetNextState(new FaceSlideState());
        });
      }


    }

    public override void Exit() {
      base.Exit();
    }

    private void NewNameInputSlideInDone() {
      _EnterNameSlideInstance.RegisterInputFocus();
      _EnterNameSlideInstance.OnNameEntered += HandleNewNameEntered;
    }

    private void HandleNewNameEntered(string faceName) {
      _EnterNameSlideInstance.OnNameEntered -= HandleNewNameEntered;
      _FaceEnrollmentGame.SharedMinigameView.HideBackButton();
      if (_OnNameEntered != null) {
        _OnNameEntered(faceName);
      }
    }

  }
}
