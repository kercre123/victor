using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {

  // This screen is either initial
  public class SkipOrContinueStage : ShowContinueStage {

    [SerializeField]
    private CozmoButton _SkipButtonInstance;

    [SerializeField]
    private CozmoButton _OldRobotContinueButtonInstance;

    [SerializeField]
    private GameObject _OldRobotViewInstance;

    [SerializeField]
    private GameObject _NewRobotViewInstance;

    [SerializeField]
    private Transform[] _OldRobotSecondaryInfo;
    [SerializeField]
    private Transform _OldRobotSpinner;

    public override void Start() {
      base.Start();
      // More than just the default unlocks, therefore something has connected to this robot before.
      // This is the onboarding first unlock.
      bool isOldRobot = UnlockablesManager.Instance.IsUnlocked(Anki.Cozmo.UnlockId.StackTwoCubes);
      _OldRobotViewInstance.SetActive(isOldRobot);
      _NewRobotViewInstance.SetActive(!isOldRobot);
      if (isOldRobot) {
        _OldRobotContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
        _SkipButtonInstance.Initialize(HandleSkipClicked, "Onboarding." + name + ".skip", "Onboarding");
      }
    }

    protected override void HandleContinueClicked() {
      base.HandleContinueClicked();
      // is skip available, Did they skip.
      DAS.Event("onboarding.skip_status", _OldRobotViewInstance.activeInHierarchy ? "1" : "0", DASUtil.FormatExtraData("0"));
    }

    protected void HandleSkipClicked() {
      // No tutorials needed for the next few phases either
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.DailyGoals);
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Upgrades);
      DAS.Event("onboarding.skip_status", "1", DASUtil.FormatExtraData("1"));
      for (int i = 0; i < _OldRobotSecondaryInfo.Length; ++i) {
        _OldRobotSecondaryInfo[i].gameObject.SetActive(false);
      }
      _OldRobotSpinner.gameObject.SetActive(true);

      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.ConnectWakeUp, HandleWakeAnimationComplete);
    }

    private void HandleWakeAnimationComplete(bool success) {
      // Complete and shut down onboarding current phase.
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Home);
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe, Color.white);
    }

  }

}
