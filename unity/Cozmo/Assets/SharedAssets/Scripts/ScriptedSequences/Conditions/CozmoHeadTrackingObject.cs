using System;

namespace ScriptedSequences.Conditions {
  public class CozmoHeadTrackingObject : AbstractCozmoCondition {

    protected override void EnableChangedAndRobotConnected(bool enabled) {
      if (enabled) {
        if (Robot.HeadTrackingObject != null) {
          IsMet = true;
        }
        else {
          Robot.OnHeadTrackingObjectSet += HandleHeadTrackingObjectSet;
        }
      }
      else {
        Robot.OnHeadTrackingObjectSet -= HandleHeadTrackingObjectSet;
      }
    }

    void HandleHeadTrackingObjectSet (ObservedObject obj)
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

