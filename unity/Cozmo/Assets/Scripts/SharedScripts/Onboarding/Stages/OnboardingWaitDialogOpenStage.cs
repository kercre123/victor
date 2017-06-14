using Cozmo.UI;

namespace Onboarding {

  public class OnboardingWaitDialogOpenStage : OnboardingBaseStage {

    public override void Start() {
      base.Start();
      BaseModal.BaseModalOpened += HandleModalOpened;
      BaseView.BaseViewOpened += HandleViewOpened;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseModal.BaseModalOpened -= HandleModalOpened;
      BaseView.BaseViewOpened -= HandleViewOpened;
    }

    private void HandleViewOpened(BaseView view) {
      if (view is Cozmo.Needs.Sparks.UI.SparksView) {
        OnboardingManager.Instance.GoToNextStage();
      }
    }

    private void HandleModalOpened(BaseModal modal) {
      if (modal is Cozmo.Repair.UI.NeedsRepairModal) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else if (modal is Cozmo.Energy.UI.NeedsEnergyModal) {
        OnboardingManager.Instance.GoToNextStage();
      }
    }

  }

}
