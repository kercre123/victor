using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class ShowContinueStage : OnboardingBaseStage {

    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    [SerializeField]
    private bool _FreeplayEnabledOnEnter = false;
    [SerializeField]
    private bool _FreeplayEnabledOnExit = false;

    public override void Start() {
      base.Start();
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(_FreeplayEnabledOnEnter);
    }

    public override void OnDestroy() {
      base.OnDestroy();
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(_FreeplayEnabledOnExit);
    }

    protected void HandleContinueClicked() {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
