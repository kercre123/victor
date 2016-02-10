using System;
using System.Collections.Generic;

public class SelectDifficultyState : State {

  private GameBase _Game;

  private int _HighestLevelCompleted;
  private List<DifficultySelectOptionData> _DifficultyOptions;
  private State _NextState;

  public SelectDifficultyState(State nextState, List<DifficultySelectOptionData> difficultyOptions, int highestLevelCompleted) {
    _NextState = nextState;
    _DifficultyOptions = difficultyOptions;
    _HighestLevelCompleted = highestLevelCompleted;
  }

  public override void Enter() {
    base.Enter();
    _Game = _StateMachine.GetGame();
    _Game.SharedMinigameView.OpenDifficultySelectView(_DifficultyOptions, 
      _HighestLevelCompleted);
    _Game.ShowContinueButtonShelf();
    _Game.SetContinueButtonShelfText(string.Empty, false);
    _Game.SetContinueButtonText(Localization.Get(LocalizationKeys.kButtonContinue));
    _Game.SetContinueButtonListener(HandleContinueButtonClicked);
    _Game.EnableContinueButton(true);

  }

  public override void Exit() {
    _Game.SharedMinigameView.CloseDifficultySelectView();
    _Game.HideContinueButtonShelf();
  }

  private void HandleContinueButtonClicked() {
    var option = _Game.SharedMinigameView.GetSelectedDifficulty();
    _Game.CurrentDifficulty = option != null ? option.DifficultyId : 0;
    _StateMachine.SetNextState(_NextState);
  }
}

