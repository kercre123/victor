using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

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

    private bool _DidSkip = false;

    protected override void Awake() {
      base.Awake();

      bool oldRobot = OnboardingManager.Instance.IsOldRobot();
      _OldRobotViewInstance.SetActive(oldRobot);
      _NewRobotViewInstance.SetActive(!oldRobot);
      if (oldRobot) {
        _OldRobotContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
        _SkipButtonInstance.Initialize(HandleSkipClicked, "Onboarding." + name + ".skip", "Onboarding");
      }
    }

    public override void OnDestroy() {
      base.OnDestroy();
      // if we skipped fast forward past the intro
      if (_DidSkip) {
        Cozmo.Needs.UI.NeedsHubView needsHubView = OnboardingManager.Instance.NeedsView;
        if (needsHubView != null) {
          needsHubView.OnboardingSkipped();
        }
      }
    }

    protected override void HandleContinueClicked() {
      base.HandleContinueClicked();
      // is skip available, Did they skip.
      DAS.Event("onboarding.skip_status", _OldRobotViewInstance.activeInHierarchy ? "1" : "0", DASUtil.FormatExtraData("0"));
      // Make sure the robot is wiped if they've actively decided they want to go through onboarding again
      if (_OldRobotViewInstance.activeInHierarchy) {
        IRobot robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.WipeRobotNeedsData();
          robot.WipeDeviceNeedsData(false);
        }
      }
    }

    protected void HandleSkipClicked() {
      // No tutorials needed for the next few phases either
      for (int i = 0; i < OnboardingManager.kRequiredPhases.Length; ++i) {
        if (OnboardingManager.kRequiredPhases[i] != OnboardingManager.OnboardingPhases.InitialSetup) {
          OnboardingManager.Instance.CompletePhase(OnboardingManager.kRequiredPhases[i]);
        }
      }

      DAS.Event("onboarding.skip_status", "1", DASUtil.FormatExtraData("1"));
      for (int i = 0; i < _OldRobotSecondaryInfo.Length; ++i) {
        _OldRobotSecondaryInfo[i].gameObject.SetActive(false);
      }
      _OldRobotSpinner.gameObject.SetActive(true);

      _DidSkip = true;
      // Using action chaining to try to prevent one frame pop of idle between these
      IRobot CurrentRobot = RobotEngineManager.Instance.CurrentRobot;
      if (CurrentRobot != null) {
        CurrentRobot.SendAnimationTrigger(ConnectionFlowController.GetAnimationForWakeUp(), HandleWakeAnimationComplete);
        RobotEngineManager.Instance.Message.NotifyCozmoWakeup = Singleton<NotifyCozmoWakeup>.Instance;
        RobotEngineManager.Instance.SendMessage();
      }
    }

    private void HandleWakeAnimationComplete(bool success) {
      // Complete and shut down onboarding current phase.
      OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.InitialSetup);
      HubWorldBase.Instance.StartFreeplay();
    }

  }

}
