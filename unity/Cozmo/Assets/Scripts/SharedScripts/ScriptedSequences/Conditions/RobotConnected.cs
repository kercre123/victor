using System;

namespace ScriptedSequences.Conditions {
  public class RobotConnected : AbstractCozmoCondition {

    #region implemented abstract members of AbstractCozmoCondition
    protected override void EnableChangedAndRobotConnected(bool enabled) {
      if (enabled) {
        IsMet = true;
      }
    }
    #endregion

  }
}

