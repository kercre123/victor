using System;

namespace ScriptedSequences.Conditions {
  public abstract class AbstractCozmoCondition : ScriptedSequenceCondition {

    protected Robot Robot {
      get { return RobotEngineManager.Instance.CurrentRobot; }
    }

    #region implemented abstract members of ScriptedSequenceCondition

    protected override void EnableChanged(bool enabled) {
      if (enabled) {
        if (Robot != null) {
          EnableChangedAndRobotConnected(true);
        }
        else {
          RobotEngineManager.Instance.RobotConnected += HandleRobotConnected;
          RobotEngineManager.Instance.DisconnectedFromClient += HandleRobotDisconnected;
        }
      }
      else {
        EnableChangedAndRobotConnected(false);
        RobotEngineManager.Instance.RobotConnected -= HandleRobotConnected;
        RobotEngineManager.Instance.DisconnectedFromClient -= HandleRobotDisconnected;
      }
    }

    #endregion

    protected abstract void EnableChangedAndRobotConnected(bool enabled);

    private void HandleRobotConnected(int id) {
      EnableChangedAndRobotConnected(IsEnabled);
    }

    private void HandleRobotDisconnected(DisconnectionReason reason) {
      IsMet = false;
    }

  }
}

