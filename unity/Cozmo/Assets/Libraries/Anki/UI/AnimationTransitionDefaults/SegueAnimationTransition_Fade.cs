using UnityEngine;
using System.Collections;
using DG.Tweening;

public class SegueAnimationTransition_Fade : Anki.UI.SegueAnimationTransition {
	
  const float VISIBLE = 1.0f;
  const float INVISIBLE = 0.0f;
	
  public SegueAnimationTransition_Fade() {
    AnimationDuration = 0.3f;
  }

  protected override void PrepareForAnimation() {
    base.PrepareForAnimation();
  }

  protected override void AnimateTransition() {
    PerformFade(VISIBLE, INVISIBLE, INVISIBLE, VISIBLE);
  }
	
  void PerformFade(float sourceInitialValue, float sourceTargetValue, float destinationInitialValue, float destinationTargetValue) {
    DAS.Debug("SegueAnimationTransition_Fade.PerformFade", SourceViewController.gameObject.name + " -> " + DestinationViewController.gameObject.name);
    CanvasGroup destinationCG = DestinationViewController.gameObject.GetComponent<CanvasGroup>();
    CanvasGroup sourceCG = SourceViewController.gameObject.GetComponent<CanvasGroup>();

    destinationCG.alpha = destinationInitialValue;
    sourceCG.alpha = sourceInitialValue;

    Sequence fadeSequence = DOTween.Sequence();
    fadeSequence.Append(sourceCG.DOFade(sourceTargetValue, AnimationDuration / 2.0f));
    fadeSequence.Append(destinationCG.DOFade(destinationTargetValue, AnimationDuration / 2.0f));
    fadeSequence.AppendCallback(AnimationCompleted);
  }
	
}
