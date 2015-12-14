using System;

namespace ScriptedSequences.Conditions {
  public class AnimationComplete : AbstractCozmoCondition {

    public bool AnyAnimation;
    public string AnimationName;

    protected override void EnableChangedAndRobotConnected(bool enabled) {
      if (enabled) {
        RobotEngineManager.Instance.RobotCompletedAnimation += HandleAnimationComplete;
      }
      else {
        RobotEngineManager.Instance.RobotCompletedAnimation -= HandleAnimationComplete;
      }
    }

    void HandleAnimationComplete (bool success, string animationName) {
      if (AnyAnimation || AnimationName == animationName) {
        IsMet = true;
      }
    }
  }
}

