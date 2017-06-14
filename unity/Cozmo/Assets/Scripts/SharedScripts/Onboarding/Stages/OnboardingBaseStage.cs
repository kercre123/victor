using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  // This doesn't use a standard state machine so that data can
  // Be added in editor
  public class OnboardingBaseStage : MonoBehaviour {

    public bool ActiveMenuContent { get { return _ActiveMenuContent; } }
    public bool ReactionsEnabled { get { return _ReactionsEnabled; } }
    public int DASPhaseID { get { return _DASPhaseID; } }

    public OnboardingButtonStates ButtonStateDiscover { get { return _ButtonStateDiscover; } }
    public OnboardingButtonStates ButtonStateRepair { get { return _ButtonStateRepair; } }
    public OnboardingButtonStates ButtonStateFeed { get { return _ButtonStateFeed; } }
    public OnboardingButtonStates ButtonStatePlay { get { return _ButtonStatePlay; } }

    [SerializeField]
    protected bool _ActiveMenuContent = false;

    [SerializeField]
    protected bool _ReactionsEnabled = true;

    [SerializeField]
    protected bool _PlayIdle = false;

    [SerializeField]
    protected int _DASPhaseID = 0;

    [SerializeField]
    protected bool _IsLastPhase = false;

    // Serialized as an int, do not reorder/delete
    public enum OnboardingButtonStates {
      Hidden,
      Disabled,
      Sparkle,
      Active
    };

    [SerializeField]
    protected OnboardingButtonStates _ButtonStateDiscover = OnboardingButtonStates.Active;
    [SerializeField]
    protected OnboardingButtonStates _ButtonStateRepair = OnboardingButtonStates.Active;
    [SerializeField]
    protected OnboardingButtonStates _ButtonStateFeed = OnboardingButtonStates.Active;
    [SerializeField]
    protected OnboardingButtonStates _ButtonStatePlay = OnboardingButtonStates.Active;

    public virtual void Start() {
      DAS.Info("DEV onboarding stage.started", name);

      // Early idle states need to loop the loading animation.
      if (_PlayIdle && RobotEngineManager.Instance.CurrentRobot != null) {
        // Really doesn't show a one frame pop to default idle between states
        RobotEngineManager.Instance.CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.OnboardingPreBirth);
        HandleLoopedAnimationComplete();
      }
    }

    public virtual void OnDestroy() {
      if (_PlayIdle && RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.PopIdleAnimation();
        RobotEngineManager.Instance.CurrentRobot.CancelCallback(HandleLoopedAnimationComplete);
      }
      DAS.Info("DEV onboarding stage.ended", name);
    }

    public virtual void SkipPressed() {
      OnboardingManager.Instance.GoToNextStage();
    }

    protected virtual void HandleLoopedAnimationComplete(bool success = true) {
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnboardingPreBirth,
                                                                              HandleLoopedAnimationComplete);
      }
    }
  }

}
