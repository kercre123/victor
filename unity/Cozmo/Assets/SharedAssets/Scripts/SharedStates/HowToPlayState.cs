using UnityEngine;
using System.Collections;

public class HowToPlayState : State {

  private State _NextState;
  private InitialCubesState.CubeStateDone _DoneCallback;

  public HowToPlayState(State nextState, InitialCubesState.CubeStateDone doneCallback = null) {
    _NextState = nextState;
    _DoneCallback = doneCallback;
  }

  public override void Enter() {
    GameBase game = _StateMachine.GetGame();
    game.ShowContinueButtonShelf();
    game.SetContinueButtonShelfText(Localization.Get(LocalizationKeys.kMinigameLabelReadyToPlay));
    game.SetContinueButtonText(Localization.Get(LocalizationKeys.kButtonStartChallenge));
    game.SetContinueButtonListener(HandleContinueButtonClicked);
    game.EnableContinueButton(true);
    game.OpenHowToPlayView();
  }

  private void HandleContinueButtonClicked() {
    // TODO: Check if the game has been run before; if so skip the HowToPlayState
    GameBase game = _StateMachine.GetGame();
    game.HideDefaultBackButton();
    game.CreateDefaultQuitButton();
    game.CloseHowToPlayView();
    game.HideContinueButtonShelf();
    _StateMachine.SetNextState(_NextState);

    if (_DoneCallback != null) {
      _DoneCallback();
    }
  }
}
