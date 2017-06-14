using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class ShowContinueStage : OnboardingBaseStage {

    [SerializeField]
    private CozmoButtonLegacy _ContinueButtonInstance;

    [SerializeField]
    private bool _FreeplayEnabledOnEnter = false;
    [SerializeField]
    private bool _FreeplayEnabledOnExit = false;

    protected virtual void Awake() {
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
    }

    public override void Start() {
      base.Start();
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayActivity(_FreeplayEnabledOnEnter);
    }

    public override void SkipPressed() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.SetEnableFreeplayActivity(_FreeplayEnabledOnExit);
      }
      OnboardingManager.Instance.GoToNextStage();
    }

    protected virtual void HandleContinueClicked() {
      SkipPressed();
    }
  }

}
