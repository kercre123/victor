using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {

  public class FreeplayStage : ShowContinueStage {

    [SerializeField]
    protected GameObject _Container;
    [SerializeField]
    protected GameObject _ArrowContainer;

    private BehaviorDisplay _BehaviorDisplay;
    public override void Start() {
      base.Start();
      _BehaviorDisplay = (BehaviorDisplay)GameObject.FindObjectOfType(typeof(BehaviorDisplay));
      if (_BehaviorDisplay != null) {
        _BehaviorDisplay.SetOverrideString(Localization.Get(LocalizationKeys.kOnboardingFreeplayStringFakeOnboarding));
      }
    }

    // Hide but wait for animation to finish before really going into freeplay
    protected override void HandleContinueClicked() {
      _Container.SetActive(false);
      _ArrowContainer.SetActive(false);
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingGetOut, HandleReactionEndAnimationComplete);
      if (_BehaviorDisplay != null) {
        _BehaviorDisplay.SetOverrideString(null);
      }
    }

    private void HandleReactionEndAnimationComplete(bool success) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
