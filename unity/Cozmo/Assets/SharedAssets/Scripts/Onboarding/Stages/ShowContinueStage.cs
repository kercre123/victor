using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class ShowContinueStage : OnboardingBaseStage {

    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    public override void Start() {
      base.Start();
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
    }

    protected void HandleContinueClicked() {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
