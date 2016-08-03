using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class FinalStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();
      BaseView.BaseViewOpened += HandleViewOpened;
      OnboardingManager.Instance.GetHomeView().OnTabChanged += HandleTabChanged;
    }

    public void OnDestroy() {
      BaseView.BaseViewClosed -= HandleViewOpened;
      OnboardingManager.Instance.GetHomeView().OnTabChanged -= HandleTabChanged;
    }

    public void HandleViewOpened(BaseView view) {
      OnboardingManager.Instance.GoToNextStage();
    }

    public void HandleTabChanged(string currTab) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
