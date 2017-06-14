using Cozmo.UI;

namespace Onboarding {

  public class OnboardingWaitDialogCloseStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();
      BaseModal.BaseModalClosed += HandleModalClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseModal.BaseModalClosed -= HandleModalClosed;
    }

    private void HandleModalClosed(BaseModal modal) {
      if (modal is Cozmo.Repair.UI.NeedsRepairModal) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else if (modal is Cozmo.Energy.UI.NeedsEnergyModal) {
        OnboardingManager.Instance.GoToNextStage();
      }
    }

  }

}
