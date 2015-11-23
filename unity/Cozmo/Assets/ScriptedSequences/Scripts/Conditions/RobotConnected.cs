using System;

namespace ScriptedSequences.Conditions {
  public class RobotConnected : ScriptedSequenceCondition {

    #region implemented abstract members of ScriptedSequenceCondition
    protected override void EnableChanged(bool enabled) {
      if (enabled) {
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          IsMet = true;
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

    private void HandleRobotConnected(string str)
    {
      IsMet = true;
    }

    private void HandleRobotDisconnected(DisconnectionReason reason)
    {
      IsMet = false;
    }

  }
}

