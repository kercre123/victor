using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class OnboardingLootStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();

      BaseModal.BaseViewCloseAnimationFinished += HandleViewClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseModal.BaseViewCloseAnimationFinished -= HandleViewClosed;
    }

    public void HandleViewClosed(BaseModal view) {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
