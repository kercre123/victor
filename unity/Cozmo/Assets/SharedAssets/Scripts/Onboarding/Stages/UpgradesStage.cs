using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class UpgradesStage : OnboardingBaseStage {


    public override void Start() {
      base.Start();

      BaseView.BaseViewOpened += HandleViewOpened;
      OnboardingManager.Instance.GetHomeView().HandleCozmoTabButton();
    }

    public void OnDestroy() {
      BaseView.BaseViewClosed -= HandleViewOpened;
    }

    public void HandleViewOpened(BaseView view) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
