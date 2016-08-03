using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class PlayAnimStage : OnboardingBaseStage {

    [SerializeField]
    private SerializableAnimationTrigger _Animation;

    public override void Start() {
      base.Start();
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(_Animation.Value, HandleEndAnimationComplete);
    }

    private void HandleEndAnimationComplete(bool success) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
