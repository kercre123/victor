using System;

namespace ScriptedSequences.Conditions {
  public abstract class AbstractCozmoCondition : ScriptedSequenceCondition {

    protected IRobot Robot {
      get { return RobotEngineManager.Instance.CurrentRobot; }
    }

    #region implemented abstract members of ScriptedSequenceCondition

    protected override void EnableChanged(bool enabled) {
      if (enabled) {
        if (Robot != null) {
          EnableChangedAndRobotConnected(true);
        }
        else {
          RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
          RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleRobotDisconnected);
        }
      }
      else {
        EnableChangedAndRobotConnected(false);
        RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotConnectionResponse>(HandleRobotConnected);
        RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RobotDisconnected>(HandleRobotDisconnected);
      }
    }

    #endregion

    protected abstract void EnableChangedAndRobotConnected(bool enabled);

    private void HandleRobotConnected(Anki.Cozmo.ExternalInterface.RobotConnectionResponse message) {
      if (message.result == Anki.Cozmo.RobotConnectionResult.Success) {
        EnableChangedAndRobotConnected(IsEnabled);
      }
    }

    private void HandleRobotDisconnected(object message) {
      IsMet = false;
    }

  }
}

