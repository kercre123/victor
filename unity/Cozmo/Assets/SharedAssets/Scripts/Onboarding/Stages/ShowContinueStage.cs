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

    [SerializeField]
    private bool _TransitionBGColorYellow = false;

    public override void Start() {
      base.Start();
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(_FreeplayEnabledOnEnter);

      if (_TransitionBGColorYellow) {
        UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.Yellow);
      }
    }

    public override void SkipPressed() {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(_FreeplayEnabledOnExit);
      OnboardingManager.Instance.GoToNextStage();
    }
    protected void HandleContinueClicked() {
      SkipPressed();
    }
  }

}
