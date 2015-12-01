using UnityEngine;
using System.Collections;

public class SegueAnimationTransition_Push_Pop : Anki.UI.SegueAnimationTransition {

  private Vector2 _SourceInitialPivot;
  private Vector2 _DestinationInitialPivot;


  public SegueAnimationTransition_Push_Pop()
  {
    AnimationDuration = 0.3f;
  }

  protected override void PrepareForAnimation()
  {
  }
  
  protected override void AnimateTransition()
  {
    float width = ((RectTransform)DestinationViewController.transform).rect.width;
    
    RectTransform destinationRectTransfrom = (RectTransform)DestinationViewController.transform;
    destinationRectTransfrom.anchoredPosition = new Vector2(width, 0.0f);

    if ( Direction ==  Anki.UI.NavigationSegueDirection.Forward ) {
      StartCoroutine( MoveObjectAnchor(Vector2.zero, new Vector2(width * - 0.2f, 0.0f),
                                       new Vector2(width, 0.0f), Vector2.zero, AnimationDuration) );
    }
    else {
      StartCoroutine( MoveObjectAnchor(Vector2.zero,  new Vector2(width, 0.0f),
                                       new Vector2(width * - 0.2f, 0.0f), Vector2.zero, AnimationDuration) );
    }
  }
  
  // Animate Source & Destination Anchors over time
  IEnumerator MoveObjectAnchor(Vector2 sourceInitalAnchor, Vector3 sourceTargetAnchor,
                               Vector2 destinationInitalAnchor, Vector3 destinationTargetAnchor, float duration)
  {
    RectTransform sourceRectTransfrom = (RectTransform)SourceViewController.transform;
    RectTransform destinationRectTransfrom = (RectTransform)DestinationViewController.transform;
    
    float startTime = Time.time;
    while(Time.time < startTime + duration) {
      sourceRectTransfrom.anchoredPosition = Vector2.Lerp(sourceInitalAnchor, sourceTargetAnchor, (Time.time - startTime)/duration);
      destinationRectTransfrom.anchoredPosition = Vector2.Lerp(destinationInitalAnchor, destinationTargetAnchor, (Time.time - startTime)/duration);
      yield return null;
    }
    
    sourceRectTransfrom.anchoredPosition = sourceTargetAnchor;
    destinationRectTransfrom.anchoredPosition = destinationTargetAnchor;
    
    AnimationCompleted();
  }

}
