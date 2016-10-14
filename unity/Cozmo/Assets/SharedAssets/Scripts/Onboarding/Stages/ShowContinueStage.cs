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

    [SerializeField]
    private bool _ShowOutline = false;

    protected virtual void Awake() {
      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
    }

    public override void Start() {
      base.Start();
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(_FreeplayEnabledOnEnter);

      if (_TransitionBGColorYellow) {
        UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.Yellow);
      }
      if (_ShowOutline) {
        // Trying to keep the Onboarding as isolated as possible, so rather than making several layers
        // of getters in homeview, just find it
        DailyGoalPanel panel = (DailyGoalPanel)GameObject.FindObjectOfType(typeof(DailyGoalPanel));
        if (panel != null) {
          OnboardingManager.Instance.SetOutlineRegion(panel.transform);
          OnboardingManager.Instance.ShowOutlineRegion(true);
        }
      }
    }

    public override void SkipPressed() {
      RobotEngineManager.Instance.CurrentRobot.SetEnableFreeplayBehaviorChooser(_FreeplayEnabledOnExit);
      OnboardingManager.Instance.GoToNextStage();
    }
    protected virtual void HandleContinueClicked() {
      SkipPressed();
    }
  }

}
