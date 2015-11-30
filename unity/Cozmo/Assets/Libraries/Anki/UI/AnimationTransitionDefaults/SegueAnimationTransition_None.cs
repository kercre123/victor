using UnityEngine;
using System.Collections;

public class SegueAnimationTransition_None : Anki.UI.SegueAnimationTransition {

  public SegueAnimationTransition_None() {
    AnimationDuration = 0.0f;
  }

  protected override void AnimateTransition() {
    AnimationCompleted();
  }
}
