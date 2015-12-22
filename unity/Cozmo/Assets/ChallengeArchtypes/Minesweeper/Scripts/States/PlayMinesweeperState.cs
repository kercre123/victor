using System;

namespace Minesweeper {
  public class PlayMinesweeperState : State {

    private MinesweeperGame _Game;

    public PlayMinesweeperState() {
    }

    public override void Enter() {
      base.Enter();
      _Game = (MinesweeperGame)_StateMachine.GetGame();
      _Game.SetupPanel();
    }




    public override void Exit() {
      base.Exit();
    }
  }
}

