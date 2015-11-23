using System;

namespace ScriptedSequences.Conditions {
  public abstract class AbstractCozmoCondition : ScriptedSequenceCondition {

    protected Robot Robot
    {
      get { return RobotEngineManager.Instance.CurrentRobot; }
    }
    #region implemented abstract members of ScriptedSequenceCondition
    protected override void EnableChanged(bool enabled) {
      if (enabled) {
        if (Robot != null) {
          EnableChangedAndRobotConnected(enabled);
        }
        else {
          RobotEngineManager.Instance.ConnectedToClient += HandleRobotConnected;
          RobotEngineManager.Instance.DisconnectedFromClient += HandleRobotDisconnected;
        }
      }
      else {
        RobotEngineManager.Instance.ConnectedToClient -= HandleRobotConnected;
        RobotEngineManager.Instance.DisconnectedFromClient -= HandleRobotDisconnected;
      }
    }
    #endregion

    protected abstract void EnableChangedAndRobotConnected(bool enabled);

    private void HandleRobotConnected(string str)
    {
      EnableChangedAndRobotConnected(IsEnabled);
    }

    private void HandleRobotDisconnected(DisconnectionReason reason)
    {
      IsMet = false;
    }

  }
}

