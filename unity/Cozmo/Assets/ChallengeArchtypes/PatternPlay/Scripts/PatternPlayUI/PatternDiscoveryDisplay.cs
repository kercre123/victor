using UnityEngine;
using System.Collections;
using DG.Tweening;

namespace PatternPlay {

  public class PatternDiscoveryDisplay : MonoBehaviour {

    [SerializeField]
    private PatternDisplay _StackPatternDisplay;

    [SerializeField]
    private PatternDisplay _HorizontalPatternDisplay;

    [SerializeField]
    private GameObject _AnimationAnchorPrefab;
    private GameObject _AnimationAnchorInstance;

    private void OnDestroy() {
      GameObject.Destroy(_AnimationAnchorInstance);
    }

    public void Initialize(BlockPattern discoveredPattern) {
      if (discoveredPattern.VerticalStack) {
        _StackPatternDisplay.Pattern = discoveredPattern;
        _HorizontalPatternDisplay.Pattern = null;
      }
      else {
        _StackPatternDisplay.Pattern = null;
        _HorizontalPatternDisplay.Pattern = discoveredPattern;
      } 
      _AnimationAnchorInstance = UIManager.CreateUIElement(_AnimationAnchorPrefab);
    }

    public void AddCloseAnimationSequence(Sequence sequence) {
      if (_StackPatternDisplay.Pattern != null) {
        AddCascadeAnimationSequence(sequence, _StackPatternDisplay.cubes);
      }
      else {
        AddCascadeAnimationSequence(sequence, _HorizontalPatternDisplay.cubes);
      }
    }

    private void AddCascadeAnimationSequence(Sequence sequence, CozmoCube[] cubesToAnimate) {
      float duration = 0.4f;
      float stagger = 0.1f;
      for (int i = 0; i < cubesToAnimate.Length; i++) {
        Transform target = cubesToAnimate[i].gameObject.transform;
        Tweener moveTween = target.DOMove(_AnimationAnchorInstance.transform.position, duration);
        moveTween.SetEase(Ease.InBack);
        Tweener scaleTween = target.DOScale(0.01f, duration);
        scaleTween.SetEase(Ease.InBack);
        if (i <= 0) {
          sequence.Append(moveTween);
          sequence.Join(scaleTween);
        }
        else {
          moveTween.SetDelay(stagger);
          sequence.Join(moveTween);
          sequence.Join(scaleTween);
        }
      }
    }
  }

}
