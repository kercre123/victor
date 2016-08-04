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

    public void OnDestroy() {
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

    private void HandleViewClosed(BaseView view) {
      if (view is CoreUpgradeDetailsDialog) {
        OnboardingManager.Instance.GoToNextStage();
      }
    }
  }

}
