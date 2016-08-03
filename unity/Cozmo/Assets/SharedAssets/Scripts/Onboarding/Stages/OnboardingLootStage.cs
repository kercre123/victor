using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class OnboardingLootStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();

      // Fills loot officially
      OnboardingManager.Instance.GiveEnergy(ChestRewardManager.Instance.GetNextRequirementPoints());
      BaseView.BaseViewCloseAnimationFinished += HandleViewClosed;
    }

    public void OnDestroy() {
      BaseView.BaseViewCloseAnimationFinished -= HandleViewClosed;
    }

    public void HandleViewClosed(BaseView view) {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
