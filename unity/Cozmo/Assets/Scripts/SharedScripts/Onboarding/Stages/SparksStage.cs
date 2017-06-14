using Cozmo.UI;
using Cozmo.Upgrades;

namespace Onboarding {

  public class SparksStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();

      UnlockablesManager.Instance.OnSparkStarted += HandleSparkStarted;
      UnlockablesManager.Instance.OnSparkComplete += HandleSparkComplete;
      // You have somehow managed to escape the sparks screen
      // Probably because we don't force a robot erase and have lots on unlocks.
      // so just move on from this part too
      BaseModal.BaseModalClosed += HandleModalClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      UnlockablesManager.Instance.OnSparkComplete -= HandleSparkComplete;
      UnlockablesManager.Instance.OnSparkStarted -= HandleSparkStarted;
      BaseModal.BaseModalClosed -= HandleModalClosed;
      // turn off regardless sine we're going back to freeplay
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayActivity(false);
    }
    // Sparks are a part of freeplay so we need to turn them on
    private void HandleSparkStarted(Anki.Cozmo.UnlockId unlock) {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayActivity(true);
    }
    private void HandleSparkComplete() {
      // Do nothing
    }

#if ENABLE_DEBUG_PANEL
    public override void SkipPressed() {
      if (!UnlockablesManager.Instance.IsUnlocked(Anki.Cozmo.UnlockId.PickupCube)) {
        UnlockablesManager.Instance.TrySetUnlocked(Anki.Cozmo.UnlockId.PickupCube, true);
      }
      base.SkipPressed();
    }
#endif

    private void HandleModalClosed(BaseModal modal) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
