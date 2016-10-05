using UnityEngine;

public static class AnimatorUtil {
  public static void ListenForAnimationStart(this Animator animator,
                                             AnimationEventStateMachineBehavior.AnimationHandler callback) {
    AnimationEventStateMachineBehavior[] progressBarAnimatorSMBs = animator.GetBehaviours<AnimationEventStateMachineBehavior>();
    for (int i = 0; i < progressBarAnimatorSMBs.Length; i++) {
      progressBarAnimatorSMBs[i].OnAnyAnimationStarted += callback;
    }
  }

  public static void StopListenForAnimationStart(this Animator animator,
                                             AnimationEventStateMachineBehavior.AnimationHandler callback) {
    AnimationEventStateMachineBehavior[] progressBarAnimatorSMBs = animator.GetBehaviours<AnimationEventStateMachineBehavior>();
    for (int i = 0; i < progressBarAnimatorSMBs.Length; i++) {
      progressBarAnimatorSMBs[i].OnAnyAnimationStarted -= callback;
    }
  }

  public static void ListenForAnimationEnd(this Animator animator,
                                             AnimationEventStateMachineBehavior.AnimationHandler callback) {
    AnimationEventStateMachineBehavior[] progressBarAnimatorSMBs = animator.GetBehaviours<AnimationEventStateMachineBehavior>();
    for (int i = 0; i < progressBarAnimatorSMBs.Length; i++) {
      progressBarAnimatorSMBs[i].OnAnyAnimationEnded += callback;
    }
  }

  public static void StopListenForAnimationEnd(this Animator animator,
                                             AnimationEventStateMachineBehavior.AnimationHandler callback) {
    AnimationEventStateMachineBehavior[] progressBarAnimatorSMBs = animator.GetBehaviours<AnimationEventStateMachineBehavior>();
    for (int i = 0; i < progressBarAnimatorSMBs.Length; i++) {
      progressBarAnimatorSMBs[i].OnAnyAnimationEnded -= callback;
    }
  }
}
