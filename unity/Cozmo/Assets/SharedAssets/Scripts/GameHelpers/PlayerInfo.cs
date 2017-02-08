using UnityEngine;
using System.Collections;

public enum PlayerType {
  None,
  Human,
  Cozmo,
  Automation,
}

// Future support for things like Memory Match solo mode.
public enum PlayerRole {
  Player,
  Spectator,
}

public class PlayerInfo {
  public PlayerType playerType;
  public PlayerRole playerRole;
  public string name;
  public int playerScoreRound;
  public int playerScoreTotal; // not reset between rounds...
  public int playerRoundsWon;
  public PlayerGoalBase _PlayerGoal = null;
  public PlayerInfo(PlayerType setPlayerType, string setName) {
    playerType = setPlayerType;
    name = setName;
  }

  public void UpdateGoal() {
    // because
    if (_PlayerGoal != null && _PlayerGoal.IsComplete) {
      SetGoal(null);
    }
    if (_PlayerGoal != null) {
      _PlayerGoal.Update();
    }
  }

  public void SetGoal(PlayerGoalBase goal) {
    if (_PlayerGoal != null) {
      _PlayerGoal.Exit();
    }
    _PlayerGoal = goal;
    if (_PlayerGoal != null) {
      _PlayerGoal.Init(this);
      _PlayerGoal.Enter();
    }
  }

}
