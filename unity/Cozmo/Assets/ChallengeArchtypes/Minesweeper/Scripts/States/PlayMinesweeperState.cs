using System;
using UnityEngine;

namespace Minesweeper {
  public class PlayMinesweeperState : State {

    private MinesweeperGame _Game;

    public PlayMinesweeperState() {
    }

    public override void Enter() {
      base.Enter();
      _Game = (MinesweeperGame)_StateMachine.GetGame();
      _Game.HideHowToPlaySlide();
      _Game.SetupPanel();
    }

    public override void Update() {
      base.Update();

      if (_Game.IsComplete()) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kMajorWin, WinAnimationComplete);
        _StateMachine.SetNextState(animState);
      }

      var closestCell = _CurrentRobot.WorldPosition / MinesweeperGame.kCellWidth + new Vector3(_Game.Columns * 0.5f, _Game.Rows * 0.5f, 0);

      int row = Mathf.RoundToInt(closestCell.x);
      int col = Mathf.RoundToInt(closestCell.y);

      int newRow, newCol;
      if (FindClosestFreeSpace(row, col, out newRow, out newCol)) {
        _StateMachine.PushSubState(new GoToNextCellState(newRow, newCol));
      }
    }

    private bool FindClosestFreeSpace(int row, int col, out int newRow, out int newCol) {
      int maxRadius = Mathf.Max(_Game.Rows, _Game.Columns);

      int rowCount = _Game.Rows;
      int colCount = _Game.Columns;

      for (int i = 0; i < maxRadius; i++) {
        for (int k = -1; k < 2; k += 2) {
          int tr = i * k + row;
          if (tr >= 0 && tr < rowCount) {
            for (int j = 0; j < i; j++) {
              for (int l = -1; l < 2; l += 2) {
                int tc = j * l + col;

                if (tc >= 0 && tc < colCount) {
                  if (_Game.GridStatus[tr, tc] == MinesweeperGame.CellStatus.Hidden) {
                    newRow = tr;
                    newCol = tc;
                    return true;
                  }
                }
              }
            }
          }
        }
      }
      newRow = -1;
      newCol = -1;
      return false;
    }

    private void WinAnimationComplete(bool success) {
      _Game.RaiseMiniGameWin();
    }


    public override void Exit() {
      base.Exit();
    }
  }

  public class GoToNextCellState : State {

    private int _Row;
    private int _Col;

    public GoToNextCellState(int row, int col) {
      _Row = row;
      _Col = col;
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.SetLiftHeight(1f);
      var game = (MinesweeperGame)_StateMachine.GetGame();

      var pos = new Vector3(_Col - game.Columns * 0.5f, _Row - game.Rows * 0.5f) * MinesweeperGame.kCellWidth;
      var delta = pos - _CurrentRobot.WorldPosition;

      var angle = Mathf.Atan2(delta.y, delta.x) * Mathf.Rad2Deg;

      // round to the nearest 90 degrees
      var rotation = Quaternion.Euler(0, 0, Mathf.Round(angle / 90f) * 90f);

      _CurrentRobot.GotoPose(_CurrentRobot.WorldPosition + delta, rotation, HandleGoToPoseComplete);
    }

    private void HandleGoToPoseComplete(bool success) {
      _StateMachine.SetNextState(new DigHoleState(_Row, _Col));
    }      
  }

  public class DigHoleState : State {
    private int _Row;
    private int _Col;

    private float _StartTime;
    private int _LastMovedTime;

    private MinesweeperGame _Game;
    public DigHoleState(int row, int col) {
      _Row = row;
      _Col = col;
    }

    public override void Enter() {
      base.Enter();

      _Game = (MinesweeperGame)_StateMachine.GetGame();

      if (_Game.GridStatus[_Row, _Col] != MinesweeperGame.CellStatus.Hidden) {
        // pop state, move to next cell
        _StateMachine.PopState();
        return;
      }
      _StartTime = Time.time;
      _CurrentRobot.SetLiftHeight(1f);
    }

    public override void Update() {
      base.Update();

      int time = (int)(Time.time - _StartTime);

      if (time != _LastMovedTime) {
        _CurrentRobot.SetLiftHeight(time % 2);
        _LastMovedTime = time;

        if (time >= 4) {
          // last chance to cancel the dig
          if (_Game.GridStatus[_Row, _Col] != MinesweeperGame.CellStatus.Hidden) {
            // pop state, move to next cell
            _StateMachine.PopState();
          }
          else {
            Dig();
          }
        }
      }
    }

    private void Dig() {
      if (!_Game.TryRevealSpace(_Row, _Col)) {
        AnimationState animState = new AnimationState();
        animState.Initialize(AnimationName.kShocked, LoseAnimationComplete);
        _StateMachine.SetNextState(animState);
      }
      else {
        _StateMachine.PopState();
      }
    }

    private void LoseAnimationComplete(bool success) {
      _Game.RaiseMiniGameLose();
    }
  }
}

