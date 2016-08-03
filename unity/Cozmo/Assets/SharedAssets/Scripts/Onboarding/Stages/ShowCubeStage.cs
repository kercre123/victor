using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections.Generic;
using Cozmo;
using Cozmo.UI;
using DG.Tweening;
using DataPersistence;

namespace Onboarding {
  public class ShowCubeStage : OnboardingBaseStage {

    [SerializeField]
    private AnkiTextLabel _ShowCozmoCubesLabel;

    [SerializeField]
    private RectTransform _CozmoImageTransform;

    [SerializeField]
    private CozmoButton _ContinueButtonInstance;

    enum SubState {
      WaitForShowCube,
      WaitForEnergy,
      WaitForInspectCube,
      WaitForFinalContinue,
    };
    private SubState _SubState;

    public override void Start() {
      base.Start();

      _ContinueButtonInstance.Initialize(HandleContinueClicked, "Onboarding." + name, "Onboarding");
      RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
      UpdateSubstate(SubState.WaitForShowCube);
    }

    protected void HandleContinueClicked() {
      if (_SubState == SubState.WaitForEnergy) {
        UpdateSubstate(SubState.WaitForInspectCube);
      }
      else {
        // From SubState.WaitForFinalContinue
        OnboardingManager.Instance.GoToNextStage();
      }
    }

    private void HandleEndAnimationComplete(bool success) {
      UpdateSubstate(SubState.WaitForFinalContinue);
    }

    public override void SkipPressed() {
      if (_SubState == SubState.WaitForFinalContinue) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else {
        UpdateSubstate((SubState)((int)_SubState + 1));
      }
    }

    public void Update() {
      // Not using RobotObservedObject because we probably want to constantly track this and go to error if it changes back.
      bool isAnyInView = false;
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        foreach (KeyValuePair<int, LightCube> kvp in RobotEngineManager.Instance.CurrentRobot.LightCubes) {
          isAnyInView |= kvp.Value.IsInFieldOfView;
        }
      }

      // TODO: error state
      if (_SubState == SubState.WaitForShowCube && isAnyInView) {
        UpdateSubstate(SubState.WaitForEnergy);
      }
    }

    private void UpdateSubstate(SubState nextState) {
      if (nextState == SubState.WaitForShowCube) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body1);
        _ContinueButtonInstance.gameObject.SetActive(false);
      }
      else if (nextState == SubState.WaitForEnergy) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body2);
        _ContinueButtonInstance.gameObject.SetActive(true);
        OnboardingManager.Instance.GiveEnergy(ChestRewardManager.Instance.GetNextRequirementPoints() / 4);
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingReactToCube);
      }
      else if (nextState == SubState.WaitForInspectCube) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body3);
        _ContinueButtonInstance.gameObject.SetActive(false);

        // TODO: behavior to align with cube?
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingInteractWithCube, HandleEndAnimationComplete);
      }
      else if (nextState == SubState.WaitForFinalContinue) {
        _ShowCozmoCubesLabel.text = Localization.Get(LocalizationKeys.kOnboardingPhase3Body4);
        _ContinueButtonInstance.gameObject.SetActive(true);
      }
      _SubState = nextState;
    }
  }
}