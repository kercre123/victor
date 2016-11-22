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
      else if(Robot != null) {
        Robot.OnCarryingObjectSet -= HandleCarryingObjectSet;
      }
    }

    void HandleCarryingObjectSet (ActiveObject obj)
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

