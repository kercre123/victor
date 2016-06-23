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
          RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.ExternalInterface.RobotConnected), HandleRobotConnected);
          RobotEngineManager.Instance.AddCallback(typeof(Anki.Cozmo.ExternalInterface.RobotDisconnected), HandleRobotDisconnected);
        }
      }
      else {
        EnableChangedAndRobotConnected(false);
        RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.ExternalInterface.RobotConnected), HandleRobotConnected);
        RobotEngineManager.Instance.RemoveCallback(typeof(Anki.Cozmo.ExternalInterface.RobotDisconnected), HandleRobotDisconnected);
      }
    }

    #endregion

    protected abstract void EnableChangedAndRobotConnected(bool enabled);

    private void HandleRobotConnected(object message) {
      EnableChangedAndRobotConnected(IsEnabled);
    }

    private void HandleRobotDisconnected(object message) {
      IsMet = false;
    }

  }
}

