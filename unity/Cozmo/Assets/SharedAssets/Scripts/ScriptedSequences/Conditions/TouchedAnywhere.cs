using System;

namespace ScriptedSequences.Conditions {
  public class TouchedAnywhere : ScriptedSequenceCondition {

    protected override void EnableChanged(bool enabled) {
      var touchCatcher = UIManager.Instance.TouchCatcher;

      if (touchCatcher != null) {
        touchCatcher.DASEventButtonName = "touch_catcher";
        touchCatcher.DASEventViewController = "touched_anywhere";

        if (enabled) {
          touchCatcher.OnTouch += HandleTouched;
        }
        else {
          touchCatcher.OnTouch -= HandleTouched;
        }
      }
      else if (enabled) {
        DAS.Warn(this, "Touch Catcher not active when TouchedAnywhere Condition Enabled");
      }
    }

    protected void HandleTouched() {
      if (UIManager.Instance.TouchCatcher != null) {
        UIManager.Instance.TouchCatcher.OnTouch -= HandleTouched;
      }
      IsMet = true;
    }
  }
}

