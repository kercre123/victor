using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  // This doesn't use a standard state machine so that data can
  // Be added in editor
  public class OnboardingBaseStage : MonoBehaviour {

    public bool ActiveTopBar { get { return _ActiveTopBar; } }
    public bool ActiveTabButtons { get { return _ActiveTabButtons; } }
    public bool ActiveMenuContent { get { return _ActiveMenuContent; } }
    public bool ReactionsEnabled { get { return _ReactionsEnabled; } }

    [SerializeField]
    protected bool _ActiveTopBar = false;

    [SerializeField]
    protected bool _ActiveTabButtons = false;

    [SerializeField]
    protected bool _ActiveMenuContent = false;

    [SerializeField]
    protected bool _ReactionsEnabled = true;

    [SerializeField]
    protected bool _PlayIdle = false;


    public virtual void Start() {
      DAS.Info("onboarding.stage.started", name);

      // Early idle states need to loop the loading animation.
      if (_PlayIdle) {
        HandleLoopedAnimationComplete();
      }
    }

    public virtual void OnDestroy() {
      if (_PlayIdle) {
        RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleLoopedAnimationComplete);
      }
      DAS.Info("onboarding.stage.ended", name);
    }

    public virtual void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }

    private void HandleLoopedAnimationComplete(bool success = true) {
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingPreBirth,
                                                                            HandleLoopedAnimationComplete);
    }
  }

}
