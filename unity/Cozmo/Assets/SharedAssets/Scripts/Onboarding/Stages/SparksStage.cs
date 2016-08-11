using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class SparksStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();

      UnlockablesManager.Instance.OnSparkStarted += HandleSparkStarted;
      UnlockablesManager.Instance.OnSparkComplete += HandleSparkComplete;
      // You have somehow managed to escape the sparks screen
      // Probably because we don't force a robot erase and have lots on unlocks.
      // so just move on from this part too
      BaseView.BaseViewClosed += HandleViewClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      UnlockablesManager.Instance.OnSparkComplete -= HandleSparkComplete;
      UnlockablesManager.Instance.OnSparkStarted -= HandleSparkStarted;
      BaseView.BaseViewClosed -= HandleViewClosed;
      // turn off regardless sine we're going back to freeplay
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
    }
    // Sparks are a part of freeplay so we need to turn them on
    private void HandleSparkStarted(Anki.Cozmo.UnlockId unlock) {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(true);
    }
    private void HandleSparkComplete(CoreUpgradeDetailsDialog dialog) {
      dialog.CloseView();
    }

#if ENABLE_DEBUG_PANEL
    public override void SkipPressed() {
      if (!UnlockablesManager.Instance.IsUnlocked(Anki.Cozmo.UnlockId.PickupCube)) {
        UnlockablesManager.Instance.TrySetUnlocked(Anki.Cozmo.UnlockId.PickupCube, true);
      }
      // Gross but only done once we've hit this stage and only for debugging.
      // Basically waiting a minute sucks for testing and the the opened event is in the previous state.
      // So just find the window in scene in case "debug skip" is pressed.
      BaseView sparksView = FindObjectOfType<CoreUpgradeDetailsDialog>();
      if (sparksView != null) {
        sparksView.CloseView();
      }
      else {
        // Handle case if hit before window is even open for spamming
        base.SkipPressed();
      }
    }
#endif

    private void HandleViewClosed(BaseView view) {
      if (view is CoreUpgradeDetailsDialog) {
        OnboardingManager.Instance.GoToNextStage();
      }
    }
  }

}
