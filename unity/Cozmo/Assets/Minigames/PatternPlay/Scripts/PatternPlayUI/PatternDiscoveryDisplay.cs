using UnityEngine;
using System.Collections;
using DG.Tweening;

public class PatternDiscoveryDisplay : MonoBehaviour {

  [SerializeField]
  private PatternDisplay _stackPatternDisplay;
  
  [SerializeField]
  private PatternDisplay _horizontalPatternDisplay;

  [SerializeField]
  private GameObject _animationAnchorPrefab;
  private GameObject _animationAnchorInstance;

  private void OnDestroy() {
    GameObject.Destroy (_animationAnchorInstance);
  }

  public void Initialize(BlockPattern discoveredPattern) {
    if (discoveredPattern.verticalStack_) {
      _stackPatternDisplay.pattern = discoveredPattern;
      _horizontalPatternDisplay.pattern = null;
    } else {
      _stackPatternDisplay.pattern = null;
      _horizontalPatternDisplay.pattern = discoveredPattern;
    } 
    _animationAnchorInstance = UIManager.CreatePerspectiveUI (_animationAnchorPrefab);
  }

  public void AddCloseAnimationSequence(Sequence sequence) {
    if (_stackPatternDisplay.pattern != null) {
      AddCascadeAnimationSequence(sequence, _stackPatternDisplay.cubes);
    } else {
      AddCascadeAnimationSequence(sequence, _horizontalPatternDisplay.cubes);
    }
  }

  private void AddCascadeAnimationSequence(Sequence sequence, CozmoCube[] cubesToAnimate) {
    float duration = 0.4f;
    float stagger = 0.1f;
    for (int i = 0; i < cubesToAnimate.Length; i++) {
      Transform target = cubesToAnimate[i].gameObject.transform;
      Tweener moveTween = target.DOMove(_animationAnchorInstance.transform.position, duration);
      moveTween.SetEase(Ease.InBack);
      Tweener scaleTween = target.DOScale(0.01f, duration);
      scaleTween.SetEase(Ease.InBack);
      if (i <= 0) {
        sequence.Append(moveTween);
        sequence.Join(scaleTween);
      } else {
        moveTween.SetDelay(stagger);
        sequence.Join(moveTween);
        sequence.Join(scaleTween);
      }
    }
  }
}
