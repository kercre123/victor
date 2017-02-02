using UnityEngine;
using System.Collections;

public class PlayerGoalBase {

  protected PlayerInfo _ParentPlayer;
  protected System.Action<PlayerInfo, int> OnPlayerGoalCompleted;
  public const int SUCCESS = 1;
  protected IRobot _CurrentRobot { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }
  public bool IsComplete { get; set; }

  public virtual void Enter() {
  }
  public virtual void Exit() {
  }
  public virtual void Update() {
  }

  public void Init(PlayerInfo parentPlayer) {
    _ParentPlayer = parentPlayer;
  }

  protected void CompletePlayerGoal(int completeCode) {
    IsComplete = true;
    if (OnPlayerGoalCompleted != null) {
      OnPlayerGoalCompleted(_ParentPlayer, completeCode);
    }
  }
}
