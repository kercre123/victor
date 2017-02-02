using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class OnboardingLootStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();

      BaseDialog.BaseDialogCloseAnimationFinished += HandleViewClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseDialog.BaseDialogCloseAnimationFinished -= HandleViewClosed;
    }

    public void HandleViewClosed(BaseDialog view) {
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
