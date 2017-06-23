using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class ShowContinueStage : OnboardingBaseStage {

    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    protected virtual void Awake() {
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
    }

    protected virtual void HandleContinueClicked() {
      SkipPressed();
    }
  }

}
