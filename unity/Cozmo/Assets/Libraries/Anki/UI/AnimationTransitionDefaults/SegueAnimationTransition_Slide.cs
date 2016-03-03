using UnityEngine;
using System.Collections;
using DG.Tweening;

public class SegueAnimationTransition_Slide : Anki.UI.SegueAnimationTransition {
	
  private Vector2 _SourceInitialPivot;
  private Vector2 _DestinationInitialPivot;
	
  public SegueAnimationTransition_Slide() {
    AnimationDuration = 0.3f;
  }

  protected override void PrepareForAnimation() {
    base.PrepareForAnimation();
  }
	
  protected override void AnimateTransition() {
    float width = ((RectTransform)DestinationViewController.transform).rect.width;
		
    RectTransform destinationRectTransfrom = (RectTransform)DestinationViewController.transform;
    destinationRectTransfrom.anchoredPosition = new Vector2(width, 0.0f);
		
    if (Direction == Anki.UI.NavigationSegueDirection.Forward) {
      MoveObjectAnchor(Vector2.zero, new Vector2(-width, 0.0f),
			                 new Vector2(width, 0.0f), Vector2.zero);
    }
    else {
      MoveObjectAnchor(Vector2.zero, new Vector2(width, 0.0f),
			                 new Vector2(-width, 0.0f), Vector2.zero);
    }
  }
	
  // Animate Source & Destination Anchors over time
  void MoveObjectAnchor(Vector2 sourceInitalAnchor, Vector3 sourceTargetAnchor,
	                      Vector2 destinationInitalAnchor, Vector3 destinationTargetAnchor) {
    RectTransform sourceRectTransfrom = (RectTransform)SourceViewController.transform;
    RectTransform destinationRectTransfrom = (RectTransform)DestinationViewController.transform;
		
    sourceRectTransfrom.anchoredPosition = sourceInitalAnchor;
    destinationRectTransfrom.anchoredPosition = destinationInitalAnchor;
		
    Sequence slideSequence = DOTween.Sequence();
    slideSequence.Append(sourceRectTransfrom.DOAnchorPos(sourceTargetAnchor, AnimationDuration));
    slideSequence.Join(destinationRectTransfrom.DOAnchorPos(destinationTargetAnchor, AnimationDuration));
    slideSequence.AppendCallback(AnimationCompleted);
  }
	
}
