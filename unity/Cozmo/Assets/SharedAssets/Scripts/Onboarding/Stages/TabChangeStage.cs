using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class TabChangeStage : OnboardingBaseStage {

    public override void Start() {
      base.Start();
      OnboardingManager.Instance.GetHomeView().OnTabChanged += HandleTabChanged;
    }

    public void OnDestroy() {
      OnboardingManager.Instance.GetHomeView().OnTabChanged -= HandleTabChanged;
    }

    public void HandleTabChanged(string currTab) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
