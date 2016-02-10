using UnityEngine;
using System.Collections;

public class HowToPlayState : State {

  private State _NextState;

  public HowToPlayState(State nextState) {
    _NextState = nextState;
  }

  private GameBase _Game;

  public override void Enter() {
    _Game = _StateMachine.GetGame();
    _Game.ShowContinueButtonShelf();
    _Game.SetContinueButtonShelfText(Localization.Get(LocalizationKeys.kMinigameLabelReadyToPlay), false);
    _Game.SetContinueButtonText(Localization.Get(LocalizationKeys.kButtonStartChallenge));
    _Game.SetContinueButtonListener(HandleContinueButtonClicked);
    _Game.EnableContinueButton(true);
    _Game.SharedMinigameView.CreateHowToPlayButton();
    _Game.SharedMinigameView.OpenHowToPlayView();
  }

  private void HandleContinueButtonClicked() {
    // TODO: Check if the game has been run before; if so skip the HowToPlayState
    _Game.SharedMinigameView.HideQuickQuitButton();
    _Game.SharedMinigameView.CreateQuitButton();
    _Game.SharedMinigameView.CloseHowToPlayView();
    _Game.HideContinueButtonShelf();

    if (_NextState == null) {
      _StateMachine.PopState();
    }
    else {
      _StateMachine.SetNextState(_NextState);
    }
  }
}
