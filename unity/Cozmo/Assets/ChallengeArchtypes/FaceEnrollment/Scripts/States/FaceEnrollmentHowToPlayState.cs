using UnityEngine;
using System.Collections;

namespace FaceEnrollment {
  public class FaceEnrollmentHowToPlayState : State {

    FaceEnrollmentGame _FaceEnrollmentGame;

    public override void Pause(State.PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // don't quit from reactionary behaviors.
    }

    public override void Enter() {
      base.Enter();
      _FaceEnrollmentGame = _StateMachine.GetGame() as FaceEnrollmentGame;
      _FaceEnrollmentGame.SharedMinigameView.ShowShelf();
      _FaceEnrollmentGame.SharedMinigameView.ShowContinueButtonOffset(HandleContinueButton,
                                                                      Localization.Get(LocalizationKeys.kButtonContinue),
                                                                      "", Color.black, "face_enrollment_how_to_play_continue");

      _FaceEnrollmentGame.SharedMinigameView.ShowWideGameStateSlide(_FaceEnrollmentGame.FaceEnrollmentHowToPlaySlidePrefab.gameObject, "face_enrollment_how_to_play_slide");
    }

    public override void Exit() {
      base.Exit();
      _FaceEnrollmentGame.SharedMinigameView.HideGameStateSlide();
      _FaceEnrollmentGame.SharedMinigameView.HideShelf();
      _FaceEnrollmentGame.SharedMinigameView.HideContinueButton();
    }

    private void HandleContinueButton() {
      _StateMachine.SetNextState(new FaceSlideState());
    }
  }
}
