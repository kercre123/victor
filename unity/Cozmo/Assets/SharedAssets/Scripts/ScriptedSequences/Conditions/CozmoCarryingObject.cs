using System;

namespace ScriptedSequences.Conditions {
  public class CozmoEmotionObject : AbstractCozmoCondition {

    protected override void EnableChangedAndRobotConnected(bool enabled) {
      if (enabled) {
        if (Robot.CarryingObject != null) {
          IsMet = true;
        }
        else {
          Robot.OnCarryingObjectSet += HandleCarryingObjectSet;
        }
      }
      else {
        Robot.OnCarryingObjectSet -= HandleCarryingObjectSet;
      }
    }

    void HandleCarryingObjectSet (ObservedObject obj)
    {
      if (obj != null) {
        IsMet = true;
      }
      else {
        IsMet = false;
      }
    }
  }
}

