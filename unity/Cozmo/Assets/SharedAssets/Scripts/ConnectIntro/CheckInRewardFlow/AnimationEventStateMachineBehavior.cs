using UnityEngine;

public class AnimationEventStateMachineBehavior : StateMachineBehaviour {
  public delegate void AnimationHandler(AnimatorStateInfo animationStateInfo);
  public event AnimationHandler OnAnyAnimationStarted;
  public event AnimationHandler OnAnyAnimationEnded;

  private bool _FiredAnimationExitEvent = false;

  public override void OnStateEnter(Animator animator, AnimatorStateInfo stateInfo, int layerIndex) {
    if (OnAnyAnimationStarted != null) {
      OnAnyAnimationStarted(stateInfo);
    }
    _FiredAnimationExitEvent = false;
  }

  public override void OnStateMove(Animator animator, AnimatorStateInfo stateInfo, int layerIndex) {
    if (!_FiredAnimationExitEvent && (stateInfo.normalizedTime >= 1)) {
      if (OnAnyAnimationEnded != null) {
        OnAnyAnimationEnded(stateInfo);
      }
      _FiredAnimationExitEvent = true;
    }
  }

  public override void OnStateExit(Animator animator, AnimatorStateInfo stateInfo, int layerIndex) {
    if (!_FiredAnimationExitEvent) {
      if (OnAnyAnimationEnded != null) {
        OnAnyAnimationEnded(stateInfo);
      }
      _FiredAnimationExitEvent = true;
    }
  }
}
