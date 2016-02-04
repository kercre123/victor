using System;

namespace SpeedTap {
  public class SpeedTapSelectDifficulty : State {

    private SpeedTapGame _SpeedTapGame;

    private int _CubesRequired = 0;

    public SpeedTapSelectDifficulty(int cubesRequired) {
      _CubesRequired = cubesRequired;
    }
      
    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.OpenDifficultySelectView(_SpeedTapGame.DifficultyOptions, 
        DataPersistence.DataPersistenceManager.Instance.Data.MinigameSaveData.SpeedTapHighestLevelCompleted + 1,
        HandleDifficultySelected);
    }

    private void HandleDifficultySelected(DifficultySelectOptionData option) {
      _SpeedTapGame.CloseDifficultySelectView();
      _SpeedTapGame.SetDifficulty(option.DifficultyId);
      InitialCubesState initCubeState = new InitialCubesState(new SpeedTapWaitForCubePlace(), _CubesRequired, _SpeedTapGame.InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
    }
  }
}

