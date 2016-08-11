using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class OnboardingLootStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();

      BaseView.BaseViewCloseAnimationFinished += HandleViewClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseView.BaseViewCloseAnimationFinished -= HandleViewClosed;
    }

    public void HandleViewClosed(BaseView view) {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
